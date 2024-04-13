#pragma once
#include "bencode.h"
#include "torrent_file.h"
#include <cassert>
#include <openssl/sha.h>

void countInfoHash(std::string& infoHash, const bencode::Dictionary& dict)
{
    assert(infoHash.empty());
    unsigned char hash[20];
    std::string dictHash = dict.encode();
    SHA1((const unsigned char*) dictHash.c_str(), dictHash.size(), hash);
    for(int i = 0; i < 20; ++i)
    {
        infoHash.push_back(hash[i]);
    }
}
void countPieceHashes(std::vector<std::string>& pieceHashes, bencode::Dictionary& info)
{
    std::string pieces = std::dynamic_pointer_cast<bencode::String>(info["pieces"])->get();
    for(size_t i = 0; i < pieces.size(); i += 20)
    {
        pieceHashes.emplace_back(pieces.substr(i, 20));
    }
}
TorrentFile LoadTorrentFile(const std::string& filename)
{
    TorrentFile torrent;
    std::ifstream stream(filename);
    auto fileDict = *std::dynamic_pointer_cast<bencode::Dictionary>(bencode::parse(stream, static_cast<char>(stream.get())));

    torrent.announce = std::dynamic_pointer_cast<bencode::String>(fileDict["announce"])->get();
    torrent.comment = std::dynamic_pointer_cast<bencode::String>(fileDict["comment"])->get();

    auto infoDict = *std::dynamic_pointer_cast<bencode::Dictionary>(fileDict["info"]);
    countInfoHash(torrent.infoHash, infoDict);

    torrent.length = std::dynamic_pointer_cast<bencode::Integer>(infoDict["length"])->get();
    torrent.pieceLength = std::dynamic_pointer_cast<bencode::Integer>(infoDict["piece length"])->get();
    torrent.name = std::dynamic_pointer_cast<bencode::String>(infoDict["name"])->get();
    countPieceHashes(torrent.pieceHashes, infoDict);

    return torrent;
}
