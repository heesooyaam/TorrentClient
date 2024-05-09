#pragma once
#include "message.h"

Message Message::Parse(const std::string& messageString) {
    if(messageString.empty())
    {
        return {MessageId::KeepAlive, 0, ""};
    }
    else
    {
        return {static_cast<MessageId>(messageString.front()),
                messageString.size(),
                messageString.substr(1)};
    }
}
Message Message::Init(MessageId id, const std::string& payload)
{
    return {id, payload.size() + 1, payload};
}
std::string Message::ToString() const
{
    return IntToBytes(messageLength) + std::string(1, (char) id) + payload;
}