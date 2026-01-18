#ifndef _METAINFO_H
#define _METAINFO_H

#include <string>

#include "bencode.h"

struct Piece {
    Piece(std::string_view hash_string, long length);
    std::vector<std::byte> hash;
    long length;
};

struct File {
    std::string path;
    long length;
    std::vector<Piece> pieces;
};

class Metainfo {
public:
    Metainfo(std::istream& input);
    std::string_view get_announce() const;
    std::string_view get_name() const;
    const std::vector<File>& get_file_list () const;
    const std::vector<Piece>& get_piece_list() const;
    long get_total_length() const;
    const std::vector<std::byte>& get_info_hash() const;

    class MetainfoError: public std::exception {
    public:
        enum class ExceptionID {
            kTopLevelNotDict,
            kMissingAnnounce,
            kAnnounceNotString,
            kMissingInfo,
            kInfoNotDict,
            kMissingName,
            kNameNotString,
            kMissingPieceLength,
            kPieceLengthNotInt,
            kPieceLengthInvalid,
            kMissingLengthAndFiles,
            kBothLengthAndFiles,
            kLengthNotInt,
            kLengthInvalid,
            kFilesNotList,
            kFilesEmpty,
            kFileNotDict,
            kFileMissingLength,
            kFileLengthNotInt,
            kFileMissingPath,
            kFilePathNotList,
            kFilePathEmpty,
            kSubPathNotString,
            kMissingPieces,
            kPiecesNotString,
            kPiecesInvalid,
            kPiecesLengthMismatch
        };
        MetainfoError(ExceptionID id);
        const char* what() const noexcept;
        const ExceptionID id_;
    };
private:
    void parse_announce();
    void parse_info();
    void parse_name();
    void parse_piece_length();
    void parse_single_file();
    void parse_file_list();
    void calculate_info_hash();
    Bencode top_;
    std::string_view announce_;
    Bencode info_;
    std::string_view name_;
    std::vector<File> file_list_;
    long piece_length_;
    std::vector<Piece> piece_list_;
    long total_length_;
};

#endif // _METAINFO_H
