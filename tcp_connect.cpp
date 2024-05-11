#pragma once
#include "tcp_connect.h"

void TcpConnect::EstablishConnection()
{
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_ == -1)
    {
        throw std::runtime_error("Error: cannot create socket:(");
    }

    closed = false;

    sockaddr_in sock_address;
    memset(&sock_address, '0', sizeof(sock_address));
    sock_address.sin_family = AF_INET;
    sock_address.sin_addr.s_addr = inet_addr(ip_.c_str());
    sock_address.sin_port = htons(port_);

    int default_flags = fcntl(sock_, F_GETFL, 0);
    set_nonblock_mode(default_flags);

    int connection_result = connect(sock_, (const sockaddr*) &sock_address, sizeof(sock_address));

    if(connection_result == 0)
    {
        set_default_mode(default_flags);
        return;
    }

    pollfd arr;
    memset(&arr, '0', sizeof(arr));
    arr =
            {
                    .fd = sock_,
                    .events = POLLOUT
            };

    connection_result = poll(&arr, 1, connectTimeout_.count());

    if(connection_result == -1)
    {
        set_default_mode(default_flags);
        throw std::runtime_error("Error: something went wrong...");
    }
    else if(connection_result == 0)
    {
        set_default_mode(default_flags);
        throw std::runtime_error("Error: time limit exceeded");
    }

    set_default_mode(default_flags);
}
void TcpConnect::set_default_mode(int default_flags) const
{
    fcntl(sock_, F_SETFL, default_flags);
    if(fcntl(sock_, F_GETFL, 0) != (default_flags))
    {
        throw std::runtime_error("Error: cannot set default mode");
    }
}
void TcpConnect::set_nonblock_mode(int current_flags) const
{
    fcntl(sock_, F_SETFL, current_flags | O_NONBLOCK);
    if(fcntl(sock_, F_GETFL, 0) != (current_flags | O_NONBLOCK))
    {
        throw std::runtime_error("Error: cannot set O_NONBLOCK mode");
    }
}
std::shared_ptr<char[]> TcpConnect::LoopRecieve(size_t bufferSize) const
{
    std::shared_ptr<char[]> buffer(new char[bufferSize]);
    size_t needToGet = bufferSize;

    while(needToGet > 0)
    {
        int curSession = recv(sock_, buffer.get() + (bufferSize - needToGet), needToGet, 0);

        if(curSession <= 0)
        {
            throw std::runtime_error("Error: cannot send data");
        }

        needToGet -= curSession;
    }
    return buffer;
}
std::string TcpConnect::ReceiveData(size_t bufferSize) const
{
    if(sock_ < 0)
    {
        throw std::runtime_error("Error: socket is not connected");
    }

    timeval TimeOut =
            {
                    .tv_sec = readTimeout_.count() / 1000,
                    .tv_usec = readTimeout_.count() % 1000 * 1000
            };
    timeval DefaultTimeOut =
            {
                    .tv_sec = 0,
                    .tv_usec = 0
            };

    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*) &TimeOut, sizeof(TimeOut));

    if(bufferSize == 0)
    {
        std::shared_ptr<char[]> size;
        try {
            size = LoopRecieve(4);
        } catch (...) {
            setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*) &DefaultTimeOut, sizeof(DefaultTimeOut));
            throw;
        }
        bufferSize = BytesToInt(std::string_view(size.get(), 4));
    }

    if(!bufferSize) return "";

    std::shared_ptr<char[]> buffer;

    try {
        buffer = LoopRecieve(bufferSize);
    } catch(...) {
        setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*) &DefaultTimeOut, sizeof(DefaultTimeOut));
        throw;
    }

    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*) &DefaultTimeOut, sizeof(DefaultTimeOut));
    return std::string(buffer.get(), buffer.get() + bufferSize);
}
void TcpConnect::LoopSend(const std::string &data) const
{
    size_t notSent = data.size();
    while(notSent > 0)
    {
        int curSession = send(sock_, data.c_str() + (data.size() - notSent), notSent, 0);

        if(curSession < 0)
        {
            throw std::runtime_error("Error: cannot send data");
        }

        notSent -= curSession;
    }
}
void TcpConnect::SendData(const std::string& data) const
{
    if(sock_ < 0)
    {
        throw std::runtime_error("Error: bad socket");
    }

    try {
        LoopSend(data);
    } catch(...) {
        throw;
    }
}
void TcpConnect::CloseConnection()
{
    if(!closed)
    {
        closed = true;
        close(sock_);
    }
}
const std::string& TcpConnect::GetIp() const
{
    return ip_;
}
int TcpConnect::GetPort() const
{
    return port_;
}
