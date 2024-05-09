#pragma once
#include "torrent_tracker.h"

TorrentTracker::TorrentTracker(const std::string& url)
        : url_(url)
{}

void TorrentTracker::UpdatePeers(const TorrentFile &tf, std::string peerId, int port)
{
    cpr::Response Resp = cpr::Get(
            cpr::Url{url_},
            cpr::Parameters {
                    {"info_hash", tf.infoHash},
                    {"peer_id", peerId},
                    {"port", std::to_string(port)},
                    {"uploaded", std::to_string(0)},
                    {"downloaded", std::to_string(0)},
                    {"left", std::to_string(tf.length)},
                    {"compact", std::to_string(1)}
            },
            cpr::Timeout{20000}
    );

    if(Resp.status_code != 200)
    {
        std::cerr << "Something went wrong... Status code: " << Resp.status_code << std::endl;
        return;
    }

    if(Resp.text.find("failure reason") != std::string::npos)
    {
        std::cerr << "Something went wrong... Text of responce: " << Resp.text << std::endl;
        return;
    }

    std::stringstream stream;
    stream << Resp.text;

    auto Dict = *std::dynamic_pointer_cast<bencode::Dictionary>(bencode::ParseBencode(stream, (char) stream.get()));

    peers_ = ParsePeer(std::dynamic_pointer_cast<bencode::String>(Dict["peers"])->get());
}

const std::vector<Peer>& TorrentTracker::GetPeers() const
{
    return peers_;
}

std::vector<Peer> TorrentTracker::ParsePeer(const std::string &peers)
{
    std::vector<Peer> result;
    for(size_t i = 0; i < peers.size(); i += 6)
    {
        result.emplace_back();

        std::string IP = peers.substr(i, 4);

        for(size_t j = 0; j < 4; ++j)
        {
            result.back().ip += std::to_string((uint8_t ) (unsigned char) IP[j]);

            if(j < 3) result.back().ip.push_back('.');
        }

        std::string port = peers.substr(i + 4, 2);

        result.back().port = (uint16_t((unsigned char) port[0]) << 8) + uint16_t((unsigned char) port[1]);
    }
    return result;
}