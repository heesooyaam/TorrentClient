#pragma once
#include "byte_tools.h"
#include <algorithm>
#include <openssl/sha.h>
#include <vector>

int BytesToInt(std::string_view bytes)
{
    int Integer = 0;
    for (const auto& byte : bytes)
    {
        Integer <<= 8;
        Integer += (unsigned char) byte;
    }
    return Integer;
}

std::string IntToBytes(size_t X)
{
    std::string BigEndian(4, '0');

    for(auto& ch : BigEndian)
    {
        ch = (char) (X & 0xFF);
        X >>= 8;
    }

    std::reverse(BigEndian.begin(), BigEndian.end());

    return BigEndian;
}

std::string CalculateSHA1(const std::string& msg)
{
    unsigned char hash[20];
    SHA1((const unsigned char*) msg.c_str(), msg.size(), hash);
    return std::string(hash, hash + 20);
}
