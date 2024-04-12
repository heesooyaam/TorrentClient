#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace bencode
{
    class bcType
    {
    public:
        virtual std::string encode() const = 0;
        virtual ~bcType() = default;
    };
    class Integer : public bcType
    {
    public:
        Integer(const int64_t& num)
        : val(num)
        {}
        std::string encode() const override final
        {
            return "i" + std::to_string(val) + "e";
        }
        int64_t get() const
        {
            return val;
        }
    private:
        int64_t val;
    };
    class String : public bcType
    {
    public:
        String(const std::string& str)
        : str(str)
        {}
        std::string encode() const override final
        {
            return std::to_string(size()) + ":" + str;
        }
        size_t size() const
        {
            return str.size();
        }
        const std::string& get() const
        {
            return str;
        }
    private:
        std::string str;
    };
    class List : public bcType
    {
    public:
        List(const std::vector<std::shared_ptr<bcType>>& data)
                : list(data)
        {}
        std::string encode() const override final
        {
            std::string bencode = "l:";
            for(const auto& dataPtr : list)
            {
                bencode += dataPtr->encode();
            }
            bencode.push_back('e');
            return bencode;
        }
        const std::vector<std::shared_ptr<bcType>>& get() const
        {
            return list;
        }
        size_t size() const
        {
            return list.size();
        }
    private:
        std::vector<std::shared_ptr<bcType>> list;
    };
    class Dictionary : public bcType
    {
    public:
        Dictionary(const std::map<std::string, std::shared_ptr<bcType>>& dict)
        :dict(dict)
        {}
        std::string encode() const override final
        {
            std::string bencode = "d";
            for(const auto& [key, valuePtr] : dict)
            {
                bencode += String(key).encode() + valuePtr->encode();
            }
            bencode.push_back('e');
            return bencode;
        }
        std::shared_ptr<bcType> operator[](const std::string& key)
        {
            return dict[key];
        }
        const std::map<std::string, std::shared_ptr<bcType>>& get() const
        {
            return dict;
        }
        size_t size() const
        {
            return dict.size();
        }
    private:
        std::map<std::string, std::shared_ptr<bcType>> dict;
    };

    template<typename streamType>
    std::shared_ptr<bcType> parse(streamType& stream, char curType)
    {
        if(curType == 'i')
        {
            int64_t val;
            stream >> val;
            stream.get();
            return std::shared_ptr<bcType>(new Integer(val));
        }
        else if('0' <= curType && curType <= '9')
        {
            size_t length;
            std::string sizeStr;
            while(curType != ':')
            {
                sizeStr.push_back(curType);
                curType = stream.get();
            }
            length = std::stoll(sizeStr);

            std::string str;
            for(size_t i = 0; i < length; ++i)
            {
                str.push_back(stream.get());
            }
            return std::shared_ptr<bcType>(new String(str));
        }
        else if(curType == 'l')
        {
            std::vector<std::shared_ptr<bcType>> list;
            curType = stream.get();
            while(curType != 'e')
            {
                list.emplace_back(parse(stream, curType));
                curType = stream.get();
            }
            return std::shared_ptr<bcType>(new List(list));
        }
        else
        {
            std::map<std::string, std::shared_ptr<bcType>> dict;
            curType = stream.get();
            while(curType != 'e')
            {
                std::string key = dynamic_pointer_cast<String>(parse(stream, curType))->get();
                curType = stream.get();
                dict[key] = parse(stream, curType);
                curType = stream.get();
            }
            return std::shared_ptr<bcType>(new Dictionary(dict));
        }
    }
}