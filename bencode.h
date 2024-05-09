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
        Integer(const int64_t& num);
        std::string encode() const override final;
        int64_t get() const;
    private:
        int64_t val;
    };

    class String : public bcType
    {
    public:
        String(const std::string& str);
        std::string encode() const override final;
        size_t size() const;
        const std::string& get() const;
    private:
        std::string str;
    };

    class List : public bcType
    {
    public:
        List(const std::vector<std::shared_ptr<bcType>>& data);
        std::string encode() const override final;
        const std::vector<std::shared_ptr<bcType>>& get() const;
        size_t size() const;
    private:
        std::vector<std::shared_ptr<bcType>> list;
    };

    class Dictionary : public bcType
    {
    public:
        Dictionary(const std::map<std::string, std::shared_ptr<bcType>>& dict);
        std::string encode() const override final;
        std::shared_ptr<bcType> operator[](const std::string& key);
        const std::map<std::string, std::shared_ptr<bcType>>& get() const;
        size_t size() const;
    private:
        std::map<std::string, std::shared_ptr<bcType>> dict;
    };

    template<typename streamType>
    std::shared_ptr<bcType> ParseBencode(streamType& stream, char curType)
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
                list.emplace_back(ParseBencode(stream, curType));
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
                std::string key = dynamic_pointer_cast<String>(ParseBencode(stream, curType))->get();
                curType = stream.get();
                dict[key] = ParseBencode(stream, curType);
                curType = stream.get();
            }
            return std::shared_ptr<bcType>(new Dictionary(dict));
        }
    }
}