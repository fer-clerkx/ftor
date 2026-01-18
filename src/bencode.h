#ifndef _BENCODE_H
#define _BENCODE_H

#include <vector>
#include <map>
#include <string>
#include <variant>
#include <iostream>

class Bencode {
public:
    using List = std::vector<Bencode>;
    using Dict = std::map<std::string, Bencode>;

    // Constructors
    Bencode();
    Bencode(std::string value);
    Bencode(const char *raw_string);
    Bencode(long value);
    Bencode(List value);
    Bencode(Dict value);
    Bencode(std::initializer_list<Bencode> init);

    // Deserialize / Serialize
    static Bencode Parse(std::istream& input);
private:
    static Bencode ParseRecursive(std::istream& input);
    static Bencode ParseString(std::istream& input);
    static Bencode ParseInteger(std::istream& input);
    static Bencode ParseList(std::istream& input);
    static Bencode ParseDictionary(std::istream& input);
public:
    std::string Dump() const;

    // Inspection
    enum class ValueType {
        kNull,
        kString,
        kInteger,
        kList,
        kDictionary
    };
    ValueType Type() const;

    // Value access
    std::string_view get_string() const;
    long get_int() const;

    // Element access
    const Bencode& at(std::size_t idx) const;
    const Bencode& at(const std::string& key) const;
    Bencode& operator[](std::size_t idx);
    Bencode& operator[](const std::string& key);

    // Iteration
    class const_iterator {
    public:
        const_iterator(List::const_iterator it);
        const_iterator(Dict::const_iterator it);
        void operator++();
        bool operator!=(const const_iterator& other) const;
        Bencode operator*() const;
    private:
        enum class IteratorType {
            List,
            Dictionary
        };
        IteratorType type_;
        std::variant<List::const_iterator, Dict::const_iterator> it_;
    };
    const_iterator begin() const;
    const_iterator end() const;

    class ConstIterationProxy {
        class const_iterator {
            Dict::const_iterator it_;
        public:
            const_iterator(Dict::const_iterator it);
            void operator++();
            bool operator!=(const const_iterator& other) const;
            std::pair<std::string, Bencode> operator*() const;
        };
    public:
        ConstIterationProxy(const Dict& data);
        const_iterator begin() const;
        const_iterator end() const;
    private:
        const Dict data_;
    };
    ConstIterationProxy items() const;

    // Lookup
    bool contains(const std::string& key) const;

    // Capacity
    std::size_t size() const;
    bool empty() const;

    // Modifiers
    void clear();
    std::size_t erase(const std::string& key);
    void erase(std::size_t idx);
    void push_back(Bencode elem);

    // Comparison
    bool operator==(const Bencode& rhs) const;

    // Exceptions
    class ParseError: public std::exception {
    public:
        enum class ExceptionID {
            kUnexpectedEOF,
            kLeading0,
            kTooMuchData,
            kMissingPostfix,
            kBadPrefix,
            kNegativeStringLength,
            kStringMissingColon,
            kIntegerEmpty,
            kIntegerNonDecimal,
            kDictKeyNotString,
            kDictIncompletePair,
            kDictDuplicateKeys,
            kDictBadOrder
        };
        ParseError(ExceptionID id);
        const char* what() const noexcept;
        const ExceptionID id_;
    };

    class DumpError: public std::exception {
    public:
        enum class ExceptionID {
            kNull
        };
        DumpError(ExceptionID id);
        const char* what() const noexcept;
        const ExceptionID id_;
    };

private:
    std::variant<std::monostate, std::string, long, List, Dict> data_;
};

// Deserialize / Serialize operators
std::istream& operator>>(std::istream& i, Bencode& o);
std::ostream& operator<<(std::ostream& o, const Bencode& i);


#endif // _BENCODE_H
