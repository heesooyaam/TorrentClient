#pragma once

#include <string>
#include <chrono>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <fcntl.h>
#include <cstring>
#include <sys/poll.h>
#include "byte_tools.h"
#include <memory>
#include <cassert>

/*
 * Обертка над низкоуровневой структурой сокета.
 */
class TcpConnect {
public:
    TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout)
            : ip_(ip)
            , port_(port)
            , connectTimeout_(connectTimeout)
            , readTimeout_(readTimeout)
            , closed(true)
    {}

    ~TcpConnect()
    {
        CloseConnection();
    }

    /*
     * Установить tcp соединение.
     * Если соединение занимает более `connectTimeout` времени, то прервать подключение и выбросить исключение.
     * Полезная информация:
     * - https://man7.org/linux/man-pages/man7/socket.7.html
     * - https://man7.org/linux/man-pages/man2/connect.2.html
     * - https://man7.org/linux/man-pages/man2/fcntl.2.html (чтобы включить неблокирующий режим работы операций)
     * - https://man7.org/linux/man-pages/man2/select.2.html
     * - https://man7.org/linux/man-pages/man2/setsockopt.2.html
     * - https://man7.org/linux/man-pages/man2/close.2.html
     * - https://man7.org/linux/man-pages/man3/errno.3.html
     * - https://man7.org/linux/man-pages/man3/strerror.3.html
     */
    void EstablishConnection()
    {
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if(sock_ == -1)
        {
            throw std::runtime_error("Error: cannot create socket:(");
        }

        closed = true;

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
    void set_default_mode(int default_flags) const
    {
        fcntl(sock_, F_SETFL, default_flags);
        if(fcntl(sock_, F_GETFL, 0) != (default_flags))
        {
            throw std::runtime_error("Error: cannot set default mode");
        }
    }
    void set_nonblock_mode(int current_flags) const
    {
        fcntl(sock_, F_SETFL, current_flags | O_NONBLOCK);
        if(fcntl(sock_, F_GETFL, 0) != (current_flags | O_NONBLOCK))
        {
            throw std::runtime_error("Error: cannot set O_NONBLOCK mode");
        }
    }
    /*
     * Послать данные в сокет
     * Полезная информация:
     * - https://man7.org/linux/man-pages/man2/send.2.html
     */
    void SendData(const std::string& data) const
    {
        if(sock_ < 0)
        {
            throw std::runtime_error("Error: bad socket");
        }

        auto c_data = data.c_str();
        size_t notSent = data.size();
        while(notSent > 0)
        {
            int curSession = send(sock_, c_data, notSent, 0);

            if(curSession < 0)
            {
                throw std::runtime_error("Error: cannot send data");
            }

            notSent -= curSession;
            c_data += curSession;
        }
    }

    /*
     * Прочитать данные из сокета.
     * Если передан `bufferSize`, то прочитать `bufferSize` байт.
     * Если параметр `bufferSize` не передан, то сначала прочитать 4 байта, а затем прочитать количество байт, равное
     * прочитанному значению.
     * Первые 4 байта (в которых хранится длина сообщения) интерпретируются как целое число в формате big endian,
     * см https://wiki.theory.org/BitTorrentSpecification#Data_Types
     * Полезная информация:
     * - https://man7.org/linux/man-pages/man2/poll.2.html
     * - https://man7.org/linux/man-pages/man2/recv.2.html
     */
    std::string ReceiveData(size_t bufferSize = 0) const
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
            char size[4];
            char* ptr = size;
            size_t needToGet = 4;
            while(needToGet > 0)
            {
                int curSession = recv(sock_, ptr, needToGet, 0);

                if(curSession <= 0)
                {
                    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*) &DefaultTimeOut, sizeof(DefaultTimeOut));
                    throw std::runtime_error("Error: cannot send data");
                }

                needToGet -= curSession;
                ptr += curSession;
            }
            std::string sz = {size[0], size[1], size[2], size[3]};
            bufferSize = BytesToInt(sz);
        }
        char* buffer(new char[bufferSize]);
        char* ptr = buffer;
        size_t needToGet = bufferSize;
        while(needToGet > 0)
        {
            int curSession = recv(sock_, ptr, needToGet, 0);

            if(curSession <= 0)
            {
                delete[] buffer;
                setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*) &DefaultTimeOut, sizeof(DefaultTimeOut));
                throw std::runtime_error("Error: cannot send data");
            }

            needToGet -= curSession;
            ptr += curSession;
        }

        std::string res;

        for(size_t i = 0; i < bufferSize; ++i)
        {
            res.push_back(buffer[i]);
        }

        delete[] buffer;
        setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*) &DefaultTimeOut, sizeof(DefaultTimeOut));
        return res;
    }

    /*
     * Закрыть сокет
     */
    void CloseConnection()
    {
        if(!closed)
        {
            closed = true;
            close(sock_);
        }
    }

    const std::string& GetIp() const
    {
        return ip_;
    }
    int GetPort() const
    {
        return port_;
    }
private:
    const std::string ip_;
    const int port_;
    std::chrono::milliseconds connectTimeout_, readTimeout_;
    int sock_;
    bool closed;
};
