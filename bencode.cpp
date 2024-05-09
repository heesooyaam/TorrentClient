#pragma once
#include "bencode.h"

/******************INTEGER******************/
bencode::Integer::Integer(const int64_t &num) : val(num) {}
std::string bencode::Integer::encode() const
{
    return "i" + std::to_string(val) + "e";
}
int64_t bencode::Integer::get() const
{
    return val;
}

/******************STRING******************/
bencode::String::String(const std::string& str): str(str) {}
std::string bencode::String::encode() const
{
    return std::to_string(size()) + ":" + str;
}
size_t bencode::String::size() const
{
    return str.size();
}
const std::string& bencode::String::get() const
{
    return str;
}

/******************LIST******************/
bencode::List::List(const std::vector<std::shared_ptr<bencode::bcType>>& data) : list(data) {}
std::string bencode::List::encode() const
{
    std::string bencode = "l:";
    for(const auto& dataPtr : list)
    {
        bencode += dataPtr->encode();
    }
    bencode.push_back('e');
    return bencode;
}
const std::vector<std::shared_ptr<bencode::bcType>>& bencode::List::get() const
{
    return list;
}
size_t bencode::List::size() const
{
    return list.size();
}

/******************DICTIONARY******************/
bencode::Dictionary::Dictionary(const std::map<std::string, std::shared_ptr<bcType>>& dict) : dict(dict) {}
std::string bencode::Dictionary::encode() const
{
    std::string bencode = "d";
    for(const auto& [key, valuePtr] : dict)
    {
        bencode += String(key).encode() + valuePtr->encode();
    }
    bencode.push_back('e');
    return bencode;
}
std::shared_ptr<bencode::bcType> bencode::Dictionary::operator[](const std::string& key)
{
    return dict[key];
}
const std::map<std::string, std::shared_ptr<bencode::bcType>>& bencode::Dictionary::get() const
{
    return dict;
}
size_t bencode::Dictionary::size() const
{
    return dict.size();
}