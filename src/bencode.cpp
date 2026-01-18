#include "bencode.h"

#include <format>
#include <algorithm>

Bencode::Bencode() {}
Bencode::Bencode(std::string value) : data_(value) {}
Bencode::Bencode(const char* raw_string) : data_(std::string(raw_string)) {}
Bencode::Bencode(long value) : data_(value) {}
Bencode::Bencode(List value) : data_(value) {}
Bencode::Bencode(Dict value) : data_(value) {}
Bencode::Bencode(std::initializer_list<Bencode> init) {
    bool construct_dict = true;
    if (init.size() % 2 == 1)
        construct_dict = false;
    else {
        for (auto it = init.begin(); it != init.end(); it+=2) {
            if (it->Type() != ValueType::kString)
                construct_dict = false;
        }
    }

    if (construct_dict) {
        Dict data {};
        for (auto it = init.begin(); it != init.end(); it+=2)
            data[std::get<std::string>(it->data_)] = *(it+1);
        data_ = data;
    }
    else
        data_ = List(init.begin(), init.end());
}


Bencode Bencode::Parse(std::istream& input) {
    Bencode root_elem {};
    if (input.peek() == EOF)
        return root_elem;

    root_elem = ParseRecursive(input);

    if (input.peek() != EOF)
        throw ParseError(ParseError::ExceptionID::kTooMuchData);

    return root_elem;
}

Bencode Bencode::ParseRecursive(std::istream& input) {
    Bencode elem {};
    switch (input.peek()) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
        elem = ParseString(input);
        break;
    case 'i':
        elem = ParseInteger(input);
        break;
    case 'l':
        elem = ParseList(input);
        break;
    case 'd':
        elem = ParseDictionary(input);
        break;
    case EOF:
        throw ParseError(ParseError::ExceptionID::kUnexpectedEOF);
    default:
        throw ParseError(ParseError::ExceptionID::kBadPrefix);
    }

    return elem;
}

Bencode Bencode::ParseString(std::istream& input) {
    std::size_t string_length;
    if (input.peek() == '-')
        throw ParseError(ParseError::ExceptionID::kNegativeStringLength);
    else if (input.peek() == '0') {
        input.ignore();
        if (input.peek() >= '0' && input.peek() <= '9')
            throw ParseError(ParseError::ExceptionID::kLeading0);
        string_length = 0;
    } else
        input >> string_length;

    if (input.get() != ':')
        throw ParseError(ParseError::ExceptionID::kStringMissingColon);

    std::string string(string_length, '\0');
    input.read(string.data(), string_length);
    if (input.gcount() != string_length)
        throw ParseError(ParseError::ExceptionID::kUnexpectedEOF);

    return Bencode(string);
}

Bencode Bencode::ParseInteger(std::istream& input) {
    input.ignore();  // Ignore i

    switch (input.peek()) {
    case EOF:
        throw ParseError(ParseError::ExceptionID::kUnexpectedEOF);
    case 'e':
        throw ParseError(ParseError::ExceptionID::kIntegerEmpty);
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        break;
    default:
        throw ParseError(ParseError::ExceptionID::kIntegerNonDecimal);
    }

    long number;
    input >> number;

    if (input.peek() == EOF)
        throw ParseError(ParseError::ExceptionID::kUnexpectedEOF);
    else if (input.peek() != 'e')
        throw ParseError(ParseError::ExceptionID::kMissingPostfix);
    input.ignore();

    return Bencode(number);
}

Bencode Bencode::ParseList(std::istream& input) {
    input.ignore();  // Ignore l

    Bencode list_elem = List {};
    while (input.peek() != 'e')
        list_elem.push_back(ParseRecursive(input));

    input.ignore();  // Ignore e
    return list_elem;
}

Bencode Bencode::ParseDictionary(std::istream& input) {
    input.ignore();  // Ignore d

    Bencode dict_elem = Dict {};
    std::vector<std::string> key_list {};
    while (input.peek() != 'e') {
        Bencode key = ParseRecursive(input);
        if (key.Type() != ValueType::kString)
            throw ParseError(ParseError::ExceptionID::kDictKeyNotString);
        key_list.push_back(std::get<std::string>(key.data_));

        if (input.peek() == 'e')
            throw ParseError(ParseError::ExceptionID::kDictIncompletePair);
        dict_elem[std::get<std::string>(key.data_)] = ParseRecursive(input);
    }

    if (!std::is_sorted(key_list.begin(), key_list.end()))
        throw ParseError(ParseError::ExceptionID::kDictBadOrder);
    if (std::adjacent_find(key_list.begin(), key_list.end()) != key_list.end())
        throw ParseError(ParseError::ExceptionID::kDictDuplicateKeys);

    input.ignore();  // Ignore e
    return dict_elem;
}

std::string Bencode::Dump() const {
    std::string output;

    switch (Type()) {
    case ValueType::kNull:
        throw DumpError(DumpError::ExceptionID::kNull);
    case ValueType::kString:
        output = std::format("{}:{}", get_string().length(), get_string());
        break;
    case ValueType::kInteger:
        output = std::format("i{}e", get_int());
        break;
    case ValueType::kList:
        output = "l";
        for (const Bencode& elem : *this)
            output += elem.Dump();
        output.push_back('e');
        break;
    case ValueType::kDictionary:
        output = "d";
        for (const auto &[key, value] : this->items()) {
            output += Bencode(key).Dump();
            output += value.Dump();
        }
        output.push_back('e');
        break;
    }

    return output;
}


Bencode::ValueType Bencode::Type() const {
    switch (data_.index()) {
    case 0:
        return ValueType::kNull;
    case 1:
        return ValueType::kString;
    case 2:
        return ValueType::kInteger;
    case 3:
        return ValueType::kList;
    case 4:
        return ValueType::kDictionary;
    default:
        throw std::logic_error("Unreachable state");
    }
}


std::string_view Bencode::get_string() const {
    return std::get<std::string>(data_);
}

long Bencode::get_int() const {
    return std::get<long>(data_);
}


const Bencode& Bencode::at(std::size_t idx) const {
    return std::get<List>(data_).at(idx);
}

const Bencode& Bencode::at(const std::string& key) const {
    return std::get<Dict>(data_).at(key);
}

Bencode& Bencode::operator[](std::size_t idx) {
    List &data = std::get<List>(data_);
    if (idx >= size())
        throw std::out_of_range(
            std::format("Bad index. Size: {} Got: {}", size(), idx));
    return data[idx];
}

Bencode& Bencode::operator[](const std::string& key) {
    if (Type() == ValueType::kNull)
        data_ = Dict {};
    return std::get<Dict>(data_)[key];
}


Bencode::const_iterator::const_iterator(List::const_iterator it)
    : it_(it), type_(IteratorType::List) {}

Bencode::const_iterator::const_iterator(Dict::const_iterator it)
    : it_(it), type_(IteratorType::Dictionary) {}

void Bencode::const_iterator::operator++() {
    if (type_ == IteratorType::List)
        ++(std::get<List::const_iterator>(it_));
    else
        ++(std::get<Dict::const_iterator>(it_));
}

bool Bencode::const_iterator::operator!=(const const_iterator& other) const {
    return it_ != other.it_;
}

Bencode Bencode::const_iterator::operator*() const {
    if (type_ == IteratorType::List)
        return *(std::get<List::const_iterator>(it_));
    else
        return std::get<Dict::const_iterator>(it_)->first;
}

Bencode::const_iterator Bencode::begin() const {
    switch (Type()) {
    case ValueType::kList:
        return const_iterator(std::get<List>(data_).cbegin());
    case ValueType::kDictionary:
        return const_iterator(std::get<Dict>(data_).cbegin());
    default:
        throw std::bad_variant_access();
    }
}

Bencode::const_iterator Bencode::end() const {
    switch (Type()) {
    case ValueType::kList:
        return const_iterator(std::get<List>(data_).cend());
    case ValueType::kDictionary:
        return const_iterator(std::get<Dict>(data_).cend());
    default:
        throw std::bad_variant_access();
    }
}


Bencode::ConstIterationProxy::ConstIterationProxy(const Dict& data)
    : data_(data) {}

Bencode::ConstIterationProxy::const_iterator Bencode::ConstIterationProxy
        ::begin() const {
    return ConstIterationProxy::const_iterator(data_.cbegin());
}

Bencode::ConstIterationProxy::const_iterator Bencode::ConstIterationProxy
        ::end() const {
    return ConstIterationProxy::const_iterator(data_.cend());
}

Bencode::ConstIterationProxy::const_iterator
    ::const_iterator(Dict::const_iterator it) : it_(it) {}

void Bencode::ConstIterationProxy::const_iterator::operator++() {
    ++it_;
}

bool Bencode::ConstIterationProxy::const_iterator
    ::operator!=(const const_iterator& other) const {
    return it_ != other.it_;
}

std::pair<std::string, Bencode> Bencode::ConstIterationProxy::const_iterator
    ::operator*() const {
    return *it_;
}

Bencode::ConstIterationProxy Bencode::items() const {
    return ConstIterationProxy(std::get<Dict>(data_));
}


bool Bencode::contains(const std::string& key) const {
    if (Type() != ValueType::kDictionary)
        return false;
    return std::get<Dict>(data_).contains(key);
}


std::size_t Bencode::size() const {
    switch (Type()) {
    case ValueType::kNull:
        return 0;
    case ValueType::kString:
        return 1;
    case ValueType::kInteger:
        return 1;
    case ValueType::kList:
        return std::get<List>(data_).size();
    case ValueType::kDictionary:
        return std::get<Dict>(data_).size();
    default:
        throw std::logic_error("Unreachable state");
    }
}

bool Bencode::empty() const {
    switch (Type()) {
    case ValueType::kNull:
        return true;
    case ValueType::kString:
        return false;
    case ValueType::kInteger:
        return false;
    case ValueType::kList:
        return std::get<List>(data_).empty();
    case ValueType::kDictionary:
        return std::get<Dict>(data_).empty();
    default:
        throw std::logic_error("Unreachable state");
    }
}


void Bencode::clear() {
    switch (Type()) {
    case ValueType::kNull:
        return;
    case ValueType::kString:
        std::get<std::string>(data_).clear();
        return;
    case ValueType::kInteger:
        std::get<long>(data_) = 0;
        return;
    case ValueType::kList:
        std::get<List>(data_).clear();
        return;
    case ValueType::kDictionary:
        std::get<Dict>(data_).clear();
        return;
    default:
        throw std::logic_error("Not yet implemented");
    }
}

std::size_t Bencode::erase(const std::string& key) {
    return std::get<Dict>(data_).erase(key);
}

void Bencode::erase(std::size_t idx) {
    List &data = std::get<List>(data_);
    auto it = data.begin() + idx;
    if (it == data.end())
        throw std::out_of_range("Bad index for erase");
    data.erase(it);
}

void Bencode::push_back(Bencode elem) {
    if (Type() == ValueType::kNull)
        data_ = List {};
    std::get<List>(data_).push_back(elem);
}


bool Bencode::operator==(const Bencode& rhs) const {
    return data_ == rhs.data_;
}


Bencode::ParseError::ParseError(ExceptionID id) : id_(id) {}

const char* Bencode::ParseError::what() const noexcept {
    switch (id_) {
    case ExceptionID::kUnexpectedEOF:
        return "syntax error - unexpected EOF";
    case ExceptionID::kLeading0:
        return "syntax error - encountered leading 0";
    case ExceptionID::kTooMuchData:
        return "syntax error - encountered data after root entry";
    case ExceptionID::kMissingPostfix:
        return "syntax error - missing postfix 'e'";
    case ExceptionID::kBadPrefix:
        return "syntax error - encountered invalid character";
    case ExceptionID::kNegativeStringLength:
        return "syntax error - negative string length; encountered '-'";
    case ExceptionID::kStringMissingColon:
        return "syntax error - missing colon in string";
    case ExceptionID::kIntegerEmpty:
        return "syntax error - empty integer value";
    case ExceptionID::kIntegerNonDecimal:
        return "syntax error - expected decimal digit";
    case ExceptionID::kDictKeyNotString:
        return "syntax error - expected key to be of type string";
    case ExceptionID::kDictIncompletePair:
        return "syntax error - key-value pair missing value";
    case ExceptionID::kDictDuplicateKeys:
        return "syntax error - encountered duplicate keys";
    case ExceptionID::kDictBadOrder:
        return "syntax error - key-value pairs must be ordered";
    default:
        return "Bencode::ParseError::what, Not yet implemented";
    }
}

Bencode::DumpError::DumpError(ExceptionID id) : id_(id) {}

const char* Bencode::DumpError::what() const noexcept {
    switch (id_) {
    case DumpError::ExceptionID::kNull:
        return "Can't dump element of type null";
    default:
        return "Bencode::DumpError::what, Not yet implemented";
    }
}


std::istream& operator>>(std::istream& i, Bencode& o) {
    o = Bencode::Parse(i);
    return i;
}

std::ostream& operator<<(std::ostream& o, const Bencode& i) {
    std::string output = i.Dump();
    return o.write(output.data(), output.size());
}
