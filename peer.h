#pragma once

#include <vector>
#include <string>

struct Peer {
    std::string ip;
    int port;
};

std::vector<Peer> ParsePeer(const std::string& peers)
{
    std::vector<Peer> result;

    for(size_t i = 0; i < peers.size(); i += 6)
    {
        result.emplace_back();

        std::string IP = peers.substr(i, 4);

        for(size_t j = 0; j < 4; ++j)
        {
            result.back().ip += std::to_string((uint8_t ) IP[j]);

            if(j != 3) result.back().ip.push_back('.');
        }

        std::string port = peers.substr(i + 4, 2);

        result.back().port = (uint16_t(port[0]) << 8) + uint16_t(port[1]);
    }
    return result;
}
