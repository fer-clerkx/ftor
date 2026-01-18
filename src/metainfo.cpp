#include <cmath>

#include "metainfo.h"

Piece::Piece(std::string_view hash_string, long length) : length(length) {
    for (char digit : hash_string)
        hash.push_back(std::byte(digit));
}

Metainfo::Metainfo(std::istream& input) : top_(Bencode::Parse(input)) {
    if (top_.Type() != Bencode::ValueType::kDictionary)
        throw MetainfoError(MetainfoError::ExceptionID::kTopLevelNotDict);

    parse_announce();
    parse_info();
    parse_name();
    parse_piece_length();

    if (!info_.contains("length") && !info_.contains("files"))
        throw MetainfoError(MetainfoError::ExceptionID::kMissingLengthAndFiles);
    if (info_.contains("length") && info_.contains("files"))
        throw MetainfoError(MetainfoError::ExceptionID::kBothLengthAndFiles);
    if (info_.contains("length"))
        parse_single_file();
    else
        parse_file_list();

    if (!info_.contains("pieces"))
        throw MetainfoError(MetainfoError::ExceptionID::kMissingPieces);
    if (info_.at("pieces").Type() != Bencode::ValueType::kString)
        throw MetainfoError(MetainfoError::ExceptionID::kPiecesNotString);
    std::string_view pieces = info_.at("pieces").get_string();
    if (pieces.length() == 0 || pieces.length() % 20 != 0)
        throw MetainfoError(MetainfoError::ExceptionID::kPiecesInvalid);
    for (auto it = pieces.begin(); it != pieces.end(); it += 20) {
        std::string_view hash(it, it + 20);
        piece_list_.push_back(Piece(hash, 0));
    }

    if (ceil(total_length_ / (double)piece_length_) != piece_list_.size())
        throw MetainfoError(MetainfoError::ExceptionID::kPiecesLengthMismatch);
    for (std::size_t i = 0; i < piece_list_.size() - 1; i++)
        piece_list_[i].length = piece_length_;
    long final_length = total_length_ - (piece_list_.size()-1) * piece_length_;
    piece_list_.back().length = final_length;

    std::size_t piece_idx = 0;
    long file_length_sum = 0;
    for (File& file : file_list_) {
        if (file_length_sum < 0)
            file.pieces.push_back(piece_list_[piece_idx-1]);
        file_length_sum += file.length;
        while (file_length_sum > 0) {
            file.pieces.push_back(piece_list_[piece_idx]);
            file_length_sum -= piece_list_[piece_idx].length;
            piece_idx++;
        }
    }
}

void Metainfo::parse_announce() {
    if (!top_.contains("announce"))
        throw MetainfoError(MetainfoError::ExceptionID::kMissingAnnounce);
    if (top_.at("announce").Type() != Bencode::ValueType::kString)
        throw MetainfoError(MetainfoError::ExceptionID::kAnnounceNotString);
    announce_ = top_.at("announce").get_string();
}

void Metainfo::parse_info() {
    if (!top_.contains("info"))
        throw MetainfoError(MetainfoError::ExceptionID::kMissingInfo);
    if (top_.at("info").Type() != Bencode::ValueType::kDictionary)
        throw MetainfoError(MetainfoError::ExceptionID::kInfoNotDict);
    info_ = top_.at("info");
}

void Metainfo::parse_name() {
    if (!info_.contains("name"))
        throw MetainfoError(MetainfoError::ExceptionID::kMissingName);
    if (info_.at("name").Type() != Bencode::ValueType::kString)
        throw MetainfoError(MetainfoError::ExceptionID::kNameNotString);
    name_ = info_.at("name").get_string();
}

void Metainfo::parse_piece_length() {
    if (!info_.contains("piece length"))
        throw MetainfoError(MetainfoError::ExceptionID::kMissingPieceLength);
    if (info_.at("piece length").Type() != Bencode::ValueType::kInteger)
        throw MetainfoError(MetainfoError::ExceptionID::kPieceLengthNotInt);
    piece_length_ = info_.at("piece length").get_int();
    if (piece_length_ < 1)
        throw MetainfoError(MetainfoError::ExceptionID::kPieceLengthInvalid);
}

void Metainfo::parse_single_file() {
    if (info_.at("length").Type() != Bencode::ValueType::kInteger)
        throw MetainfoError(MetainfoError::ExceptionID::kLengthNotInt);
    long length = info_.at("length").get_int();
    if (length < 1)
        throw MetainfoError(MetainfoError::ExceptionID::kLengthInvalid);
    file_list_.push_back(
        File{std::string(get_name()), length, {}});
    total_length_ = length;
}

void Metainfo::parse_file_list() {
    if (info_.at("files").Type() != Bencode::ValueType::kList)
        throw MetainfoError(MetainfoError::ExceptionID::kFilesNotList);
    if (info_.at("files").empty())
        throw MetainfoError(MetainfoError::ExceptionID::kFilesEmpty);
    total_length_ = 0;
    for (const Bencode& file : info_.at("files")) {
        file_list_.push_back(File {});
        if (file.Type() != Bencode::ValueType::kDictionary)
            throw MetainfoError(MetainfoError::ExceptionID::kFileNotDict);

        if (!file.contains("length"))
            throw MetainfoError(
                MetainfoError::ExceptionID::kFileMissingLength);
        if (file.at("length").Type() != Bencode::ValueType::kInteger)
            throw MetainfoError(
                MetainfoError::ExceptionID::kFileLengthNotInt);
        file_list_.back().length = file.at("length").get_int();
        total_length_ += file.at("length").get_int();

        if (!file.contains("path"))
            throw MetainfoError(
                MetainfoError::ExceptionID::kFileMissingPath);
        if (file.at("path").Type() != Bencode::ValueType::kList)
            throw MetainfoError(
                MetainfoError::ExceptionID::kFilePathNotList);
        if (file.at("path").empty())
            throw MetainfoError(MetainfoError::ExceptionID::kFilePathEmpty);
        for (const Bencode& sub_path : file.at("path")) {
            if (sub_path.Type() != Bencode::ValueType::kString)
                throw MetainfoError(
                    MetainfoError::ExceptionID::kSubPathNotString);
            file_list_.back().path += std::string(sub_path.get_string()) + '/';
        }
        file_list_.back().path.pop_back();
    }
}


std::string_view Metainfo::get_announce() const {
    return announce_;
}

std::string_view Metainfo::get_name() const {
    return name_;
}

const std::vector<File>& Metainfo::get_file_list() const {
    return file_list_;
}

const std::vector<Piece>& Metainfo::get_piece_list() const {
    return piece_list_;
}

long Metainfo::get_total_length() const {
    return total_length_;
}


Metainfo::MetainfoError::MetainfoError(MetainfoError::ExceptionID id)
    : id_(id) {}

const char* Metainfo::MetainfoError::what() const noexcept {
    switch (id_) {
    case MetainfoError::ExceptionID::kTopLevelNotDict:
        return "input error - root element is not a dictionary";
    case MetainfoError::ExceptionID::kMissingAnnounce:
        return "input error - missing announce field";
    case MetainfoError::ExceptionID::kAnnounceNotString:
        return "input error - expected announce field to be of type string";
    case MetainfoError::ExceptionID::kMissingInfo:
        return "input error - missing info field";
    case MetainfoError::ExceptionID::kInfoNotDict:
        return "input error - expected info field to be of type dictionary";
    case MetainfoError::ExceptionID::kMissingName:
        return "input error - missing name field";
    case MetainfoError::ExceptionID::kNameNotString:
        return "input error - expected name field to be of type string";
    case MetainfoError::ExceptionID::kMissingPieceLength:
        return "input error - missing piece length field";
    case MetainfoError::ExceptionID::kPieceLengthNotInt:
        return "input error - "
            "expected piece length field to be of type integer";
    case MetainfoError::ExceptionID::kPieceLengthInvalid:
        return "input error - piece length value must be greater than 0";
    case MetainfoError::ExceptionID::kMissingLengthAndFiles:
        return "input error - expected either length or files field";
    case MetainfoError::ExceptionID::kBothLengthAndFiles:
        return "input error - input can't contain both length and files fields";
    case MetainfoError::ExceptionID::kLengthNotInt:
        return "input error - expected length field to be of type integer";
    case MetainfoError::ExceptionID::kLengthInvalid:
        return "input error - length value must be greater than 0";
    case MetainfoError::ExceptionID::kFilesNotList:
        return "input error - expected files field to be of type list";
    case MetainfoError::ExceptionID::kFilesEmpty:
        return "input error - expected files field to be not empty";
    case MetainfoError::ExceptionID::kFileNotDict:
        return "input error - expected file field to be of type dictionary";
    case MetainfoError::ExceptionID::kFileMissingLength:
        return "input error - missing file length field";
    case MetainfoError::ExceptionID::kFileLengthNotInt:
        return "input error - expected file length field to be of type integer";
    case MetainfoError::ExceptionID::kFileMissingPath:
        return "input error - missing file path field";
    case MetainfoError::ExceptionID::kFilePathNotList:
        return "input error - expected file path field to be of type list";
    case MetainfoError::ExceptionID::kFilePathEmpty:
        return "input error - expected file path field to be not empty";
    case MetainfoError::ExceptionID::kSubPathNotString:
        return "input error - expected file path element to be of type string";
    case MetainfoError::ExceptionID::kMissingPieces:
        return "input error - missing pieces field";
    case MetainfoError::ExceptionID::kPiecesNotString:
        return "input error - expected pieces field to be of type string";
    case MetainfoError::ExceptionID::kPiecesInvalid:
        return "input error - "
            "length of pieces string must be greater than 0 and multiple of 20";
    case MetainfoError::ExceptionID::kPiecesLengthMismatch:
        return "input error - number of pieces doesn't match total file length";
    default:
        return "Metainfo::MetainfoError::what(), not yet implemented";
    }
}
