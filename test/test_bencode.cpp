#include <gtest/gtest.h>
#include "../src/bencode.h"

//
// Initialization
//

TEST(BencodeTest, initializeNull) {
    Bencode dut;
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kNull);
    EXPECT_TRUE(dut.empty());
    EXPECT_EQ(dut.size(), 0);
}

TEST(BencodeTest, initializeNullInitializer) {
    Bencode dut {};
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kNull);
    EXPECT_TRUE(dut.empty());
    EXPECT_EQ(dut.size(), 0);
}

TEST(BencodeTest, initializeString) {
    std::string test_string = "Hello world";
    Bencode dut = test_string;
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kString);
    EXPECT_EQ(dut.get_string(), test_string);
    EXPECT_FALSE(dut.empty());
    EXPECT_EQ(dut.size(), 1);
}

TEST(BencodeTest, initializeStringRaw) {
    Bencode dut = "Hello world";
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kString);
    EXPECT_EQ(dut.get_string(), "Hello world");
    EXPECT_FALSE(dut.empty());
    EXPECT_EQ(dut.size(), 1);
}

TEST(BencodeTest, initializeInt) {
    long test_number = 64;
    Bencode dut = test_number;
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kInteger);
    EXPECT_EQ(dut.get_int(), test_number);
    EXPECT_FALSE(dut.empty());
    EXPECT_EQ(dut.size(), 1);
}

TEST(BencodeTest, initializeListEmpty) {
    Bencode dut(Bencode::List {});
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kList);
    EXPECT_TRUE(dut.empty());
    EXPECT_EQ(dut.size(), 0);
}

TEST(BencodeTest, initializeList) {
    long test_number = -5;
    std::string test_string = "hello";
    Bencode dut(Bencode::List {test_number, test_string, {}});
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kList);
    EXPECT_FALSE(dut.empty());
    EXPECT_EQ(dut.size(), 3);
    EXPECT_EQ(dut.at(0).get_int(), test_number);
    EXPECT_EQ(dut.at(1).get_string(), test_string);
    EXPECT_EQ(dut.at(2).Type(), Bencode::ValueType::kNull);
}

TEST(BencodeTest, initializeListInitializerUneven) {
    long test_number = -5;
    std::string test_string = "hello";
    Bencode dut {test_number, test_string, {}};
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kList);
    EXPECT_FALSE(dut.empty());
    EXPECT_EQ(dut.size(), 3);
    EXPECT_EQ(dut.at(0).get_int(), test_number);
    EXPECT_EQ(dut.at(1).get_string(), test_string);
    EXPECT_EQ(dut.at(2).Type(), Bencode::ValueType::kNull);
}

TEST(BencodeTest, initializeListInitializerNotAllString) {
    std::vector<std::string> test_string_list = {"hello", "world"};
    long test_number = -5;
    Bencode dut {test_string_list[0], test_number, {}, test_string_list[1]};
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kList);
    EXPECT_FALSE(dut.empty());
    EXPECT_EQ(dut.size(), 4);
    EXPECT_EQ(dut.at(0).get_string(), test_string_list[0]);
    EXPECT_EQ(dut.at(1).get_int(), test_number);
    EXPECT_EQ(dut.at(2).Type(), Bencode::ValueType::kNull);
    EXPECT_EQ(dut.at(3).get_string(), test_string_list[1]);
}

TEST(BencodeTest, initializeDict) {
    std::vector<std::string> key_list {"foo", "bar", "null"};
    long test_number = 643;
    std::string test_string = "Hello world";
    Bencode dut(Bencode::Dict {{key_list[0], test_number},
                               {key_list[1], test_string},
                               {key_list[2], {}}});
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kDictionary);
    EXPECT_FALSE(dut.empty());
    EXPECT_EQ(dut.size(), 3);
    EXPECT_EQ(dut.at(key_list[0]), test_number);
    EXPECT_EQ(dut.at(key_list[1]), test_string);
    EXPECT_EQ(dut.at(key_list[2]).Type(), Bencode::ValueType::kNull);
}

TEST(BencodeTest, initializeDictInitializer) {
    std::vector<std::string> key_list {"foo", "bar", "null"};
    long test_number = 643;
    std::string test_string = "Hello world";
    Bencode dut {key_list[0], test_number,
                 key_list[1], test_string,
                 key_list[2], {}};
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kDictionary);
    EXPECT_FALSE(dut.empty());
    EXPECT_EQ(dut.size(), 3);
    EXPECT_EQ(dut.at(key_list[0]), test_number);
    EXPECT_EQ(dut.at(key_list[1]), test_string);
    EXPECT_EQ(dut.at(key_list[2]).Type(), Bencode::ValueType::kNull);
}

// List access

TEST(BencodeTest, listAtGoodIndex) {
    long test_number = -10;
    Bencode dut {{}, test_number};
    EXPECT_EQ(dut.at(1).get_int(), test_number);
}

TEST(BencodeTest, listAtBadIndex) {
    Bencode dut {{}};
    EXPECT_THROW(dut.at(1), std::out_of_range);
}

TEST(BencodeTest, listSquareBracketGoodIndex) {
    long test_number = -10;
    Bencode dut {{}, test_number};
    EXPECT_EQ(dut[1].get_int(), test_number);
}

TEST(BencodeTest, listSquareBracketBadIndex) {
    Bencode dut {{}};
    EXPECT_THROW(dut[1], std::out_of_range);
}

// List modifiers

TEST(BencodeTest, clearFilledList) {
    Bencode dut {1};
    dut.clear();
    EXPECT_TRUE(dut.empty());
}

TEST(BencodeTest, clearEmptyList) {
    Bencode dut(Bencode::List {});
    dut.clear();
    EXPECT_TRUE(dut.empty());
}

TEST(BencodeTest, listEraseExistingElement) {
    Bencode dut {1, "2", 3};
    dut.erase(1);
    EXPECT_EQ(dut.size(), 2);
    EXPECT_EQ(dut[0].get_int(), 1);
    EXPECT_EQ(dut[1].get_int(), 3);
}

TEST(BencodeTest, listEraseNonExistingElement) {
    Bencode dut {1, 2, 3};
    EXPECT_THROW({dut.erase(3);}, std::out_of_range);
}

TEST(BencodeTest, listModifyElement) {
    Bencode dut {{}};
    dut[0] = 10;
    EXPECT_EQ(dut.size(), 1);
    EXPECT_EQ(dut[0], 10);
}

TEST(BencodeTest, listPushNewElement) {
    Bencode dut(Bencode::List {});
    dut.push_back(10);
    EXPECT_EQ(dut.size(), 1);
    EXPECT_EQ(dut[0].get_int(), 10);
    dut.push_back("Hello world");
    EXPECT_EQ(dut.size(), 2);
    EXPECT_EQ(dut[1].get_string(), "Hello world");
}

TEST(BencodeTest, listFromNull) {
    Bencode dut;
    dut.push_back(Bencode {});
    EXPECT_EQ(dut.size(), 1);
    EXPECT_EQ(dut[0].Type(), Bencode::ValueType::kNull);
}

// Dictionary modifiers

TEST(BencodeTest, clearFilledDict) {
    Bencode dut {{"foo"}, 1};
    dut.clear();
    EXPECT_TRUE(dut.empty());
}

TEST(BencodeTest, clearEmptyDict) {
    Bencode dut(Bencode::Dict {});
    dut.clear();
    EXPECT_TRUE(dut.empty());
}

TEST(BencodeTest, dictEraseExistingElement) {
    Bencode dut {"foo", 1,
                 "bar", 2,
                 "hello", 3};
    EXPECT_EQ(dut.erase("bar"), 1);
    EXPECT_EQ(dut.size(), 2);
    EXPECT_EQ(dut.at("foo").get_int(), 1);
    EXPECT_EQ(dut.at("hello").get_int(), 3);
}

TEST(BencodeTest, dictEraseNonExistingElement) {
    Bencode dut {"foo", 1};
    EXPECT_EQ(dut.erase("bar"), 0);
    EXPECT_EQ(dut.size(), 1);
}

TEST(BencodeTest, dictModifyElement) {
    Bencode dut {"foo", {}};
    dut["foo"] = 10;
    EXPECT_EQ(dut.size(), 1);
    EXPECT_EQ(dut.at("foo"), 10);
}

TEST(BencodeTest, dictInsertNewElement) {
    Bencode dut(Bencode::Dict {});
    dut["foo"] = 10;
    EXPECT_EQ(dut.size(), 1);
    EXPECT_EQ(dut.at("foo").get_int(), 10);
    dut["bar"] = "Hello world";
    EXPECT_EQ(dut.size(), 2);
    EXPECT_EQ(dut.at("bar").get_string(), "Hello world");
}

TEST(BencodeTest, dictFromNull) {
    Bencode dut;
    dut["foo"];
    EXPECT_EQ(dut.size(), 1);
    EXPECT_EQ(dut["foo"].Type(), Bencode::ValueType::kNull);
}

// Incorrect accesses

TEST(BencodeTest, nonStringAsString) {
    Bencode dut;
    EXPECT_THROW({dut.get_string();}, std::bad_variant_access);
    dut = 10;
    EXPECT_THROW({dut.get_string();}, std::bad_variant_access);
    dut = Bencode::List {};
    EXPECT_THROW({dut.get_string();}, std::bad_variant_access);
    dut = Bencode::Dict {};
    EXPECT_THROW({dut.get_string();}, std::bad_variant_access);
}

TEST(BencodeTest, nonIntAsInt) {
    Bencode dut;
    EXPECT_THROW({dut.get_int();}, std::bad_variant_access);
    dut = "Hello world";
    EXPECT_THROW({dut.get_int();}, std::bad_variant_access);
    dut = Bencode::List {};
    EXPECT_THROW({dut.get_int();}, std::bad_variant_access);
    dut = Bencode::Dict {};
    EXPECT_THROW({dut.get_int();}, std::bad_variant_access);
}

TEST(BencodeTest, idxAtOnNonList) {
    Bencode dut;
    EXPECT_THROW({dut.at(0);}, std::bad_variant_access);
    dut = 10;
    EXPECT_THROW({dut.at(0);}, std::bad_variant_access);
    dut = "Hello world";
    EXPECT_THROW({dut.at(0);}, std::bad_variant_access);
    dut = Bencode::Dict {};
    EXPECT_THROW({dut.at(0);}, std::bad_variant_access);
}

TEST(BencodeTest, idxSquareBracketOnNonList) {
    Bencode dut;
    EXPECT_THROW({dut[0];}, std::bad_variant_access);
    dut = 10;
    EXPECT_THROW({dut[0];}, std::bad_variant_access);
    dut = "Hello world";
    EXPECT_THROW({dut[0];}, std::bad_variant_access);
    dut = Bencode::Dict {};
    EXPECT_THROW({dut[0];}, std::bad_variant_access);
}

TEST(BencodeTest, keyAtOnNonDict) {
    Bencode dut;
    EXPECT_THROW({dut.at("foo");}, std::bad_variant_access);
    dut = 10;
    EXPECT_THROW({dut.at("foo");}, std::bad_variant_access);
    dut = "Hello world";
    EXPECT_THROW({dut.at("foo");}, std::bad_variant_access);
    dut = Bencode::List {};
    EXPECT_THROW({dut.at("foo");}, std::bad_variant_access);
}

TEST(BencodeTest, keySquareBracketOnNonDict) {
    // Null case should convert to dictionary
    Bencode dut = 10;
    EXPECT_THROW({dut["foo"];}, std::bad_variant_access);
    dut = "Hello world";
    EXPECT_THROW({dut["foo"];}, std::bad_variant_access);
    dut = Bencode::List {};
    EXPECT_THROW({dut["foo"];}, std::bad_variant_access);
}

// Value modifiers

TEST(BencodeTest, clearNull) {
    Bencode dut;
    dut.clear();
    EXPECT_EQ(dut.Type(), Bencode::ValueType::kNull);
}

TEST(BencodeTest, clearString) {
    Bencode dut = "Hello world";
    dut.clear();
    EXPECT_EQ(dut.get_string(), "");
}

TEST(BencodeTest, clearInt) {
    Bencode dut = 6;
    dut.clear();
    EXPECT_EQ(dut.get_int(), 0);
}

TEST(BencodeTest, keyEraseNonMap) {
    Bencode dut;
    EXPECT_THROW({dut.erase("foo");}, std::bad_variant_access);
    dut = "Hello world";
    EXPECT_THROW({dut.erase("foo");}, std::bad_variant_access);
    dut = 10;
    EXPECT_THROW({dut.erase("foo");}, std::bad_variant_access);
    dut = Bencode::List {};
    EXPECT_THROW({dut.erase("foo");}, std::bad_variant_access);
}

TEST(BencodeTest, idxEraseNonList) {
    Bencode dut;
    EXPECT_THROW({dut.erase(10);}, std::bad_variant_access);
    dut = "Hello world";
    EXPECT_THROW({dut.erase(10);}, std::bad_variant_access);
    dut = 10;
    EXPECT_THROW({dut.erase(10);}, std::bad_variant_access);
    dut = Bencode::Dict {};
    EXPECT_THROW({dut.erase(10);}, std::bad_variant_access);
}

TEST(BencodeTest, pushBackNonList) {
    // Null case should convert to list
    Bencode dut = "Hello world";
    EXPECT_THROW({dut.push_back("foo");}, std::bad_variant_access);
    dut = 10;
    EXPECT_THROW({dut.push_back("foo");}, std::bad_variant_access);
    dut = Bencode::Dict {};
    EXPECT_THROW({dut.push_back("foo");}, std::bad_variant_access);
}

// Iteration

TEST(BencodeTest, iterateOverList) {
    Bencode dut {0l, 1, 2};
    int i = 0;
    for (Bencode elem : dut) {
        EXPECT_EQ(elem.get_int(), i);
        i++;
    }
    EXPECT_EQ(i, dut.size());
}

TEST(BencodeTest, iterateOverDictKeys) {
    std::vector<std::string> key_list {"bar", "foo", "hello"};
    Bencode dut {key_list[1], {}, key_list[2], {}, key_list[0], {}};
    int i = 0;
    for (Bencode elem : dut) {
        EXPECT_EQ(elem.get_string(), key_list[i]);
        i++;
    }
    EXPECT_EQ(i, dut.size());
}

TEST(BencodeTest, iterateOverNull) {
    Bencode dut;
    EXPECT_THROW({dut.begin();}, std::bad_variant_access);
    EXPECT_THROW({dut.end();}, std::bad_variant_access);
}

TEST(BencodeTest, iterateOverString) {
    Bencode dut = "";
    EXPECT_THROW({dut.begin();}, std::bad_variant_access);
    EXPECT_THROW({dut.end();}, std::bad_variant_access);
}

TEST(BencodeTest, iterateOverInt) {
    Bencode dut = 3;
    EXPECT_THROW({dut.begin();}, std::bad_variant_access);
    EXPECT_THROW({dut.end();}, std::bad_variant_access);
}

TEST(BencodeTest, itemIterationOverDict) {
    std::vector<std::string> key_list {"bar", "foo"};
    std::vector<Bencode> value_list {0l, 1};
    Bencode dut {key_list[1], value_list[1], key_list[0], value_list[0]};
    int i = 0;
    for (const auto &[key, value] : dut.items()) {
        EXPECT_EQ(key, key_list[i]);
        EXPECT_EQ(value.get_int(), value_list[i].get_int());
        i++;
    }
    EXPECT_EQ(i, dut.size());
}

TEST(BencodeTest, itemIterationOverNonDict) {
    Bencode dut;
    EXPECT_THROW({dut.items();}, std::bad_variant_access);
    dut = "";
    EXPECT_THROW({dut.items();}, std::bad_variant_access);
    dut = 1;
    EXPECT_THROW({dut.items();}, std::bad_variant_access);
    dut = Bencode::List {};
    EXPECT_THROW({dut.items();}, std::bad_variant_access);
}

// Lookup

TEST(BencodeTest, containsExistingElem) {
    Bencode dut {"foo", 65, "bar", {}};
    EXPECT_TRUE(dut.contains("bar"));
}

TEST(BencodeTest, containsNonExistingElem) {
    Bencode dut {"foo", 65};
    EXPECT_FALSE(dut.contains("bar"));
}

TEST(BencodeTest, containsNonDict) {
    Bencode dut;
    EXPECT_FALSE(dut.contains("bar"));
    dut = "bar";
    EXPECT_FALSE(dut.contains("bar"));
    dut = 10;
    EXPECT_FALSE(dut.contains("bar"));
    dut = Bencode::List {"bar"};
    EXPECT_FALSE(dut.contains("bar"));
}

// Parsing

void check_parse_exception(std::istream& input,
                           Bencode::ParseError::ExceptionID expected_id) {
    try {
        Bencode::Parse(input);
        FAIL() << "Expected Bencode::ParseError";
    }
    catch (const Bencode::ParseError& e) {
        EXPECT_EQ(e.id_, expected_id);
    }
    catch (const std::exception& e) {
        FAIL() << "Expected Bencode::ParseError, got: " << e.what();
    }
}

TEST(BencodeTest, parseEmptyInput) {
    std::istringstream input("");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.Type(), Bencode::ValueType::kNull);
}

TEST(BencodeTest, parseBadPrefix) {
    std::istringstream input("a");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kBadPrefix);
}

// String

TEST(BencodeTest, parseStringNegativeLength) {
    std::istringstream input("-1");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kNegativeStringLength);
}

TEST(BencodeTest, parseStringLeading0) {
    std::istringstream input("01:f");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kLeading0);
}

TEST(BencodeTest, parseStringDouble0) {
    std::istringstream input("00:");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kLeading0);
}

TEST(BencodeTest, parseStringNonDecimalLength) {
    std::istringstream input("1a");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kStringMissingColon);
}

TEST(BencodeTest, parseStringNoColon) {
    std::istringstream input("3foo");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kStringMissingColon);
}

TEST(BencodeTest, parseStringUnexpectedEOF) {
    std::istringstream input("3:fo");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kUnexpectedEOF);
}

TEST(BencodeTest, parseStringSingleDigitLength) {
    std::istringstream input("3:foo");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.get_string(), "foo");
}

TEST(BencodeTest, parseStringEmpty) {
    std::istringstream input("0:");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.get_string(), "");
}

TEST(BencodeTest, parseStringDoubleDigitLength) {
    std::istringstream input("11:Hello world");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.get_string(), "Hello world");
}

TEST(BencodeTest, parseSingleStringWithMoreData) {
    std::istringstream input("11:Hello world3:foo");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kTooMuchData);
}

// Integer

TEST(BencodeTest, parseIntegerEmptyEOF) {
    std::istringstream input("i");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kUnexpectedEOF);
}

TEST(BencodeTest, parseIntegerEmptyWithPostfix) {
    std::istringstream input("ie");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kIntegerEmpty);
}

TEST(BencodeTest, parseIntegerEOF) {
    std::istringstream input("i6");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kUnexpectedEOF);
}

TEST(BencodeTest, parseIntegerMissingPostfix) {
    std::istringstream input("i6a");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kMissingPostfix);
}

TEST(BencodeTest, parseIntegerNonDecimal) {
    std::istringstream input("ia6e");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kIntegerNonDecimal);
}

TEST(BencodeTest, parseIntegerSingleDigit) {
    std::istringstream input("i6e");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.get_int(), 6l);
}

TEST(BencodeTest, parseIntegerDoubleDigit) {
    std::istringstream input("i43e");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.get_int(), 43l);
}

TEST(BencodeTest, parseIntegerNegative) {
    std::istringstream input("i-89e");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.get_int(), -89l);
}

// List

TEST(BencodeTest, parseListEmptyEOF) {
    std::istringstream input("l");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kUnexpectedEOF);
}

TEST(BencodeTest, parseListEmpty) {
    std::istringstream input("le");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.Type(), Bencode::ValueType::kList);
    EXPECT_TRUE(output.empty());
}

TEST(BencodeTest, parseListBadPrefix) {
    std::istringstream input("lae");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kBadPrefix);
}

TEST(BencodeTest, parseListSingleInteger) {
    std::istringstream input("li64ee");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.size(), 1);
    EXPECT_EQ(output[0].get_int(), 64l);
}

TEST(BencodeTest, parseListSingleString) {
    std::istringstream input("l3:fooe");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.size(), 1);
    EXPECT_EQ(output[0].get_string(), "foo");
}

TEST(BencodeTest, parseListSingleList) {
    std::istringstream input("llee");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.size(), 1);
    EXPECT_EQ(output[0].Type(), Bencode::ValueType::kList);
    EXPECT_TRUE(output[0].empty());
}

TEST(BencodeTest, parseListSingleDict) {
    std::istringstream input("ldee");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.size(), 1);
    EXPECT_EQ(output[0].Type(), Bencode::ValueType::kDictionary);
    EXPECT_TRUE(output[0].empty());
}

TEST(BencodeTest, parseListTripleElem) {
    std::istringstream input("llei-89e3:bare");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.size(), 3);
    EXPECT_EQ(output[0].Type(), Bencode::ValueType::kList);
    EXPECT_TRUE(output[0].empty());
    EXPECT_EQ(output[1].get_int(), -89l);
    EXPECT_EQ(output[2].get_string(), "bar");
}

// Dictionary

TEST(BencodeTest, parseDictEmptyEOF) {
    std::istringstream input("d");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kUnexpectedEOF);
}

TEST(BencodeTest, parseDictEmpty) {
    std::istringstream input("de");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.Type(), Bencode::ValueType::kDictionary);
    EXPECT_TRUE(output.empty());
}

TEST(BencodeTest, parseDictKeyBadPrefix) {
    std::istringstream input("dae");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kBadPrefix);
}

TEST(BencodeTest, parseDictKeyNotString) {
    std::istringstream input("di0e3:fooe");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kDictKeyNotString);
}

TEST(BencodeTest, parseDictValueBadPrefix) {
    std::istringstream input("d3:fooae");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kBadPrefix);
}

TEST(BencodeTest, parseDictKeyWithoutValue) {
    std::istringstream input("d3:fooe");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kDictIncompletePair);
}

TEST(BencodeTest, parseDictFilledEOF) {
    std::istringstream input("d3:foo3:bar");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kUnexpectedEOF);
}

TEST(BencodeTest, parseDictSingleInteger) {
    std::istringstream input("d3:fooi0ee");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.size(), 1);
    EXPECT_EQ(output.at("foo").get_int(), 0l);
}

TEST(BencodeTest, parseDictSingleString) {
    std::istringstream input("d3:foo3:bare");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.size(), 1);
    EXPECT_EQ(output.at("foo").get_string(), "bar");
}

TEST(BencodeTest, parseDictSingleList) {
    std::istringstream input("d3:foolee");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.size(), 1);
    EXPECT_EQ(output.at("foo").Type(), Bencode::ValueType::kList);
    EXPECT_TRUE(output.at("foo").empty());
}

TEST(BencodeTest, parseDictSingleDict) {
    std::istringstream input("d3:foodee");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.size(), 1);
    EXPECT_EQ(output.at("foo").Type(), Bencode::ValueType::kDictionary);
    EXPECT_TRUE(output.at("foo").empty());
}

TEST(BencodeTest, parseDictDuplicateKey) {
    std::istringstream input("d3:fooi2e3:foo5:helloe");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kDictDuplicateKeys);
}

TEST(BencodeTest, parseDictBadKeyOrder) {
    std::istringstream input("d3:fooi2e3:bar5:helloe");
    check_parse_exception(input,
        Bencode::ParseError::ExceptionID::kDictBadOrder);
}

TEST(BencodeTest, parseDictMultiElem) {
    std::istringstream input("d3:bari2e3:foo5:helloe");
    Bencode output = Bencode::Parse(input);
    EXPECT_EQ(output.size(), 2);
    EXPECT_EQ(output.at("bar").get_int(), 2l);
    EXPECT_EQ(output.at("foo").get_string(), "hello");
}

TEST(BencodeTest, extractionOperator) {
    std::istringstream input("11:Hello world");
    Bencode output {};
    input >> output;
    EXPECT_EQ(output.get_string(), "Hello world");
}

// Dump

void check_dump_exception(Bencode& data,
                           Bencode::DumpError::ExceptionID expected_id) {
    try {
        data.Dump();
        FAIL() << "Expected Bencode::DumpError";
    }
    catch (const Bencode::DumpError& e) {
        EXPECT_EQ(e.id_, expected_id);
    }
    catch (const std::exception& e) {
        FAIL() << "Expected Bencode::DumpError, got: " << e.what();
    }
}

TEST(BencodeTest, dumpNull) {
    Bencode data {};
    check_dump_exception(data, Bencode::DumpError::ExceptionID::kNull);
}

TEST(BencodeTest, dumpString) {
    Bencode data = "Hello world";
    std::string output = data.Dump();
    EXPECT_EQ(output, "11:Hello world");
}

TEST(BencodeTest, dumpEmptyString) {
    Bencode data = "";
    std::string output = data.Dump();
    EXPECT_EQ(output, "0:");
}

TEST(BencodeTest, dumpInteger) {
    Bencode data = 64;
    std::string output = data.Dump();
    EXPECT_EQ(output, "i64e");
}

TEST(BencodeTest, dumpNegativeInteger) {
    Bencode data = -9;
    std::string output = data.Dump();
    EXPECT_EQ(output, "i-9e");
}

TEST(BencodeTest, dump0Integer) {
    Bencode data = 0l;
    std::string output = data.Dump();
    EXPECT_EQ(output, "i0e");
}

TEST(BencodeTest, dumpEmptyList) {
    Bencode data = Bencode::List {};
    std::string output = data.Dump();
    EXPECT_EQ(output, "le");
}

TEST(BencodeTest, dumpListSingleElem) {
    Bencode data {"foo"};
    std::string output = data.Dump();
    EXPECT_EQ(output, "l3:fooe");
}

TEST(BencodeTest, dumpListMultiElem) {
    Bencode data {"foo", 64, Bencode::List {}, Bencode::Dict {}};
    std::string output = data.Dump();
    EXPECT_EQ(output, "l3:fooi64eledee");
}

TEST(BencodeTest, dumpEmptyDict) {
    Bencode data = Bencode::Dict {};
    std::string output = data.Dump();
    EXPECT_EQ(output, "de");
}

TEST(BencodeTest, dumpDictSingleElem) {
    Bencode data {"foo", "bar"};
    std::string output = data.Dump();
    EXPECT_EQ(output, "d3:foo3:bare");
}

TEST(BencodeTest, dumpDictMultiElem) {
    Bencode data {
        "foo", -89,
        "bar", Bencode::List {},
        "hello", Bencode::Dict {}
    };
    std::string output = data.Dump();
    EXPECT_EQ(output, "d3:barle3:fooi-89e5:hellodee");
}

TEST(BencodeTest, dumpToStream) {
    Bencode data {
        "foo", -89,
        "bar", Bencode::List {},
        "hello", Bencode::Dict {}
    };
    std::ostringstream output;
    output << data;
    EXPECT_EQ(output.str(), "d3:barle3:fooi-89e5:hellodee");
}
