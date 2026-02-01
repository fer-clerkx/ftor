#include <gtest/gtest.h>
#include "../src/metainfo.h"

#include <cmath>
#include <numeric>

#include <openssl/sha.h>

#include "../src/bencode.h"

constexpr Bencode nominal_input() {
    long piece_length = 512 * (long)pow(2, 20);  // 512 MB
    std::vector<long> length_list {
        (long)pow(2, 40),  // 1 TB
        2 * (long)pow(2, 40)  // 2 TB
    };
    long total_length = std::reduce(length_list.begin(), length_list.end());
    std::size_t num_pieces = ceil(total_length / (double)piece_length);
    return Bencode {
        "announce", "http://test_announce.org:1337/tracker_1/tracker_2/",
        "info", {
            "name", "test_name",
            "piece length", piece_length,
            "files", {
                {
                    "length", length_list[0],
                    "path", Bencode::List {
                        "sub_dir_1",
                        "test_file_1"
                    }
                },
                {
                    "length", length_list[1],
                    "path", Bencode::List {
                        "sub_dir_2",
                        "test_file_2"
                    }
                }
            },
            "pieces", std::string(20 * num_pieces, 'a')
        }
    };
}

void check_metainfo_exception(std::istringstream& input,
    Metainfo::MetainfoError::ExceptionID expected_id)
{
    try {
        Metainfo dut(input);
        FAIL() << "Expected Metainfo::MetainfoError";
    }
    catch (const Metainfo::MetainfoError& e) {
        EXPECT_EQ(e.id_, expected_id);
    }
    catch (const std::exception& e) {
        FAIL() << "Expected Metainfo::MetainfoError, got: " << e.what();
    }
}

TEST(MetainfoTest, emptyInput) {
    std::istringstream input;
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kTopLevelNotDict);
}

TEST(MetainfoTest, encodingError) {
    std::istringstream input("a");
    EXPECT_THROW({Metainfo dut(input);}, Bencode::ParseError);
}

TEST(MetainfoTest, topLevelNotDict) {
    std::istringstream input("le");
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kTopLevelNotDict);
}

TEST(MetainfoTest, announceMissing) {
    std::istringstream input("de");
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kMissingAnnounce);
}

TEST(MetainfoTest, announceNotString) {
    Bencode input_elem = nominal_input();
    input_elem["announce"] = 0l;
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kAnnounceNotString);
}

TEST(MetainfoTest, announceContainsSpace) {
    Bencode input_elem = nominal_input();
    input_elem["announce"] = "http://test announce.org";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kAnnounceInvalidURL);
}

TEST(MetainfoTest, announceContainsCurlyBrace) {
    Bencode input_elem = nominal_input();
    input_elem["announce"] = "http://test{announce.org";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kAnnounceInvalidURL);
}

TEST(MetainfoTest, announceUsingHTTPS) {
    Bencode input_elem = nominal_input();
    input_elem["announce"] = "https://test_announce.org";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kAnnounceInvalidScheme);
}

TEST(MetainfoTest, announceUsingUDP) {
    Bencode input_elem = nominal_input();
    input_elem["announce"] = "udp://test_announce.org";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kAnnounceInvalidScheme);
}

TEST(MetainfoTest, announceGood) {
    std::istringstream input(nominal_input().Dump());
    Metainfo dut(input);
    EXPECT_EQ(dut.get_announce().scheme(), "http");
    EXPECT_EQ(dut.get_announce().host(), "test_announce.org");
    EXPECT_EQ(dut.get_announce().port(), "1337");
    EXPECT_EQ(dut.get_announce().path(), "/tracker_1/tracker_2/");
}

TEST(MetainfoTest, announceEmptyPath) {
    Bencode input_elem = nominal_input();
    input_elem["announce"] = "http://test_announce.org";
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    EXPECT_EQ(dut.get_announce().scheme(), "http");
    EXPECT_EQ(dut.get_announce().host(), "test_announce.org");
    EXPECT_EQ(dut.get_announce().path(), "/");
}

TEST(MetainfoTest, announcequery) {
    Bencode input_elem = nominal_input();
    input_elem["announce"] = "http://test_announce.org?foo=bar&hello=world";
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    EXPECT_EQ(dut.get_announce().scheme(), "http");
    EXPECT_EQ(dut.get_announce().host(), "test_announce.org");
    EXPECT_EQ(dut.get_announce().path(), "/");
    ASSERT_EQ(dut.get_announce().query().size(), 2);
    EXPECT_EQ(dut.get_announce().query()[0].key(), "foo");
    EXPECT_EQ(dut.get_announce().query()[0].val(), "bar");
    EXPECT_EQ(dut.get_announce().query()[1].key(), "hello");
    EXPECT_EQ(dut.get_announce().query()[1].val(), "world");
}

TEST(MetainfoTest, missingInfo) {
    Bencode input_elem = nominal_input();
    input_elem.erase("info");
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kMissingInfo);
}

TEST(MetainfoTest, infoNotDict) {
    Bencode input_elem = nominal_input();
    input_elem["info"] = "";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kInfoNotDict);
}

TEST(MetainfoTest, nameMissing) {
    Bencode input_elem = nominal_input();
    input_elem["info"].erase("name");
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kMissingName);
}

TEST(MetainfoTest, nameNotString) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["name"] = 0l;
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kNameNotString);
}

TEST(MetainfoTest, nameGood) {
    std::istringstream input(nominal_input().Dump());
    Metainfo dut(input);
    EXPECT_EQ(dut.get_name(), "test_name");
}

TEST(MetainfoTest, pieceLengthMissing) {
    Bencode input_elem = nominal_input();
    input_elem["info"].erase("piece length");
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kMissingPieceLength);
}

TEST(MetainfoTest, pieceLengthNotInteger) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["piece length"] = "";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kPieceLengthNotInt);
}

TEST(MetainfoTest, pieceLength0) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["piece length"] = 0l;
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kPieceLengthInvalid);
}

TEST(MetainfoTest, pieceLengthNegative) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["piece length"] = -1;
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kPieceLengthInvalid);
}

TEST(MetainfoTest, missingLengthAndFileList) {
    Bencode input_elem = nominal_input();
    input_elem["info"].erase("files");
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kMissingLengthAndFiles);
}

TEST(MetainfoTest, bothLengthAndFileList) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["length"] = 1;
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kBothLengthAndFiles);
}

TEST(MetainfoTest, lengthNotInt) {
    Bencode input_elem = nominal_input();
    input_elem["info"].erase("files");
    input_elem["info"]["length"] = "";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kLengthNotInt);
}

TEST(MetainfoTest, length0) {
    Bencode input_elem = nominal_input();
    input_elem["info"].erase("files");
    input_elem["info"]["length"] = 0l;
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kLengthInvalid);
}

TEST(MetainfoTest, lengthNegative) {
    Bencode input_elem = nominal_input();
    input_elem["info"].erase("files");
    input_elem["info"]["length"] = -1;
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kLengthInvalid);
}

TEST(MetainfoTest, getSingleFilePathAndLength) {
    Bencode input_elem = nominal_input();
    input_elem["info"].erase("files");
    input_elem["info"]["length"] = 512 * pow(2, 30);  // 512 GB
    input_elem["info"]["pieces"] = std::string(1024 * 20, 'a');
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    ASSERT_EQ(dut.get_file_list().size(), 1);
    EXPECT_EQ(dut.get_file_list()[0].path, "test_name");
    EXPECT_EQ(dut.get_file_list()[0].length, 512 * pow(2, 30));
}

TEST(MetainfoTest, getTotalLengthSingleFile) {
    Bencode input_elem = nominal_input();
    input_elem["info"].erase("files");
    input_elem["info"]["length"] = 512 * pow(2, 30);  // 512 GB
    input_elem["info"]["pieces"] = std::string(1024 * 20, 'a');
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    EXPECT_EQ(dut.get_total_length(), 512 * pow(2, 30));
}

TEST(MetainfoTest, fileListNotList) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"] = "";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kFilesNotList);
}

TEST(MetainfoTest, fileListEmpty) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"].clear();
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kFilesEmpty);
}

TEST(MetainfoTest, firstFileNotDict) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0] = "";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kFileNotDict);
}

TEST(MetainfoTest, secondFileNotDict) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][1] = "";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kFileNotDict);
}

TEST(MetainfoTest, fileNoLength) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0].erase("length");
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kFileMissingLength);
}

TEST(MetainfoTest, fileLengthNotInt) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0]["length"] = "";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kFileLengthNotInt);
}

TEST(MetainfoTest, fileNoPath) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0].erase("path");
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kFileMissingPath);
}

TEST(MetainfoTest, filePathNotList) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0]["path"] = "";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kFilePathNotList);
}

TEST(MetainfoTest, filePathEmpty) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0]["path"].clear();
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kFilePathEmpty);
}

TEST(MetainfoTest, fileSubPathNotString) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0]["path"][0] = 0l;
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kSubPathNotString);
}

TEST(MetainfoTest, getLenghtDir) {
    Bencode input_elem = nominal_input();
    // input_elem["info"]["files"][0]["length"] = 100;
    // input_elem["info"]["files"][1]["length"] = 200;
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    ASSERT_EQ(dut.get_file_list().size(), 2);
    EXPECT_EQ(dut.get_file_list()[0].length, pow(2, 40));
    EXPECT_EQ(dut.get_file_list()[1].length, 2 * pow(2, 40));
}

TEST(MetainfoTest, getTotalLengthDict) {
    Bencode input_elem = nominal_input();
    // input_elem["info"]["files"][0]["length"] = pow(2, 40);  // 1 TB
    // input_elem["info"]["files"][1]["length"] = 2 * pow(2, 40);  // 2 TB
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    EXPECT_EQ(dut.get_total_length(), 3 * pow(2, 40));
}

TEST(MetainfoTest, getPathDirSinglePath) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0]["path"].erase(0);
    input_elem["info"]["files"][1]["path"].erase(0);
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    ASSERT_EQ(dut.get_file_list().size(), 2);
    EXPECT_EQ(dut.get_file_list()[0].path, "test_file_1");
    EXPECT_EQ(dut.get_file_list()[1].path, "test_file_2");
}

TEST(MetainfoTest, getPathDirMultiPath) {
    Bencode input_elem = nominal_input();
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    ASSERT_EQ(dut.get_file_list().size(), 2);
    EXPECT_EQ(dut.get_file_list()[0].path, "sub_dir_1/test_file_1");
    EXPECT_EQ(dut.get_file_list()[1].path, "sub_dir_2/test_file_2");
}

TEST(MetainfoTest, missingPieces) {
    Bencode input_elem = nominal_input();
    input_elem["info"].erase("pieces");
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kMissingPieces);
}

TEST(MetainfoTest, piecesNotString) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["pieces"] = 0l;
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kPiecesNotString);
}

TEST(MetainfoTest, piecesEmpty) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["pieces"].clear();
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kPiecesInvalid);
}

TEST(MetainfoTest, piecesNotMultipleOf20) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["pieces"] = "0123456789012345678";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kPiecesInvalid);
}

TEST(MetainfoTest, piecesLengthMismatch) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["pieces"] = "01234567890123456789";
    std::istringstream input(input_elem.Dump());
    check_metainfo_exception(input,
        Metainfo::MetainfoError::ExceptionID::kPiecesLengthMismatch);
}

TEST(MetainfoTest, checkNumbeofPieces) {
    Bencode input_elem = nominal_input();
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    std::size_t expected_size
        = input_elem.at("info").at("pieces").get_string().size() / 20;
    EXPECT_EQ(dut.get_piece_list().size(), expected_size);
}

TEST(MetainfoTest, getSinglePieceHash) {
    Bencode input_elem = nominal_input();
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    EXPECT_EQ(dut.get_piece_list()[0].hash,
              std::vector<std::byte>(20, std::byte('a')));
}

TEST(MetainfoTest, getMultiplePieceHashes) {
    Bencode input_elem = nominal_input();
    long piece_length = input_elem["info"]["piece length"].get_int();
    input_elem["info"]["files"][0]["length"] = piece_length;
    input_elem["info"]["files"][1]["length"] = piece_length;
    input_elem["info"]["pieces"] = std::string(20, 'a') + std::string(20, 'b');
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    EXPECT_EQ(dut.get_piece_list()[0].hash,
              std::vector<std::byte>(20, std::byte('a')));
    EXPECT_EQ(dut.get_piece_list()[1].hash,
              std::vector<std::byte>(20, std::byte('b')));
}

TEST(MetainfoTest, getNormalPieceLength) {
    Bencode input_elem = nominal_input();
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    for (const Piece& piece : dut.get_piece_list())
        EXPECT_EQ(piece.length, 512 * pow(2, 20));
}

TEST(MetainfoTest, getTruncatedPieceLength) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0]["length"] = pow(2, 40) - 1;
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    std::vector<Piece> output = dut.get_piece_list();
    for (std::size_t i = 0; i < output.size() - 1; i++)
        EXPECT_EQ(output[i].length, 512*pow(2, 20));
    EXPECT_EQ(output.back().length, 512*pow(2, 20) - 1);
}

TEST(MetainfoTest, checkNumberOfPiecesFromFile) {
    Bencode input_elem = nominal_input();
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    std::size_t num_pieces = 0;
    for (const File& file : dut.get_file_list())
        num_pieces += file.pieces.size();
    EXPECT_EQ(num_pieces, dut.get_piece_list().size());
}

TEST(MetainfoTest, getPieceFromFile) {
    Bencode input_elem = nominal_input();
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    std::vector<File> file_list_output = dut.get_file_list();
    std::vector<Piece> piece_list_output = dut.get_piece_list();
    std::size_t piece_idx = 0;
    for (const File& file : file_list_output) {
        for (const Piece& file_piece : file.pieces) {
            EXPECT_EQ(file_piece.length, piece_list_output[piece_idx].length);
            EXPECT_EQ(file_piece.hash, piece_list_output[piece_idx].hash);
            piece_idx++;
        }
    }
}

TEST(MetainfoTest, checkNumberOfPiecesFromFileOverlapping) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0]["length"] = pow(2, 40) + 1;
    input_elem["info"]["pieces"] = std::string(
        input_elem.at("info").at("pieces").get_string().length() + 20,
        'a'
    );
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    std::size_t num_pieces = 0;
    for (const File& file : dut.get_file_list())
        num_pieces += file.pieces.size();
    // 1 overlapping piece
    EXPECT_EQ(num_pieces, dut.get_piece_list().size() + 1);
}

TEST(MetainfoTest, getPieceFromFileOverlapping) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0]["length"] = pow(2, 40) + 1;
    input_elem["info"]["pieces"] = std::string(
        input_elem.at("info").at("pieces").get_string().length() + 20,
        'a'
    );
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    std::vector<File> file_list_output = dut.get_file_list();
    std::vector<Piece> piece_list_output = dut.get_piece_list();
    std::size_t piece_idx = 0;
    for (const File& file : file_list_output) {
        for (const Piece& file_piece : file.pieces) {
            EXPECT_EQ(file_piece.length, piece_list_output[piece_idx].length);
            EXPECT_EQ(file_piece.hash, piece_list_output[piece_idx].hash);
            piece_idx++;
        }
        piece_idx--;
    }
}

TEST(MetainfoTest, getPieceFromFileMultiOverlapping) {
    Bencode input_elem = nominal_input();
    input_elem["info"]["files"][0]["length"] = 100;
    input_elem["info"]["files"][1]["length"] = 100;
    input_elem["info"]["pieces"] = std::string(20, 'a');
    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);
    std::vector<File> file_list_output = dut.get_file_list();
    std::vector<Piece> piece_list_output = dut.get_piece_list();
    for (const File& file : file_list_output) {
        EXPECT_EQ(file.pieces.size(), 1);
        EXPECT_EQ(file.pieces[0].length, piece_list_output[0].length);
        EXPECT_EQ(file.pieces[0].hash, piece_list_output[0].hash);
    }
}

TEST(MetainfoTest, getInfoHash) {
    Bencode input_elem = nominal_input();

    std::string info_dump = input_elem.at("info").Dump();
    std::vector<unsigned char> expected_hash(20);
    SHA1(reinterpret_cast<const unsigned char*>(info_dump.data()),
        info_dump.size(), expected_hash.data());

    std::istringstream input(input_elem.Dump());
    Metainfo dut(input);

    const std::vector<std::byte> output = dut.get_info_hash();
    ASSERT_EQ(output.size(), 20);
    for (std::size_t i = 0; i < 20; i++)
        EXPECT_EQ(std::to_integer<unsigned char>(output[i]), expected_hash[i]);
}
