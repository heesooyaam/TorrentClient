#pragma once

#include <string>
#include <vector>
#include "bencode.h"

struct TorrentFile {
    std::string announce;
    std::string comment;
    std::vector<std::string> pieceHashes;
    size_t pieceLength;
    size_t length;
    std::string name;
    std::string infoHash;
};

void CountPieceHashes(std::vector<std::string>& pieceHashes, bencode::Dictionary& info);
void CountInfoHash(std::string& infoHash, const bencode::Dictionary& dict);
TorrentFile LoadTorrentFile(const std::string& filename);
