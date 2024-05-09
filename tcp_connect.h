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
    void EstablishConnection();
    void set_default_mode(int default_flags) const;
    void set_nonblock_mode(int current_flags) const;
    /*
     * Послать данные в сокет
     * Полезная информация:
     * - https://man7.org/linux/man-pages/man2/send.2.html
     */
    void LoopSend(const std::string& str) const;
    void SendData(const std::string& data) const;

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
    std::shared_ptr<char[]> LoopRecieve(size_t bufferSize) const;
    std::string ReceiveData(size_t bufferSize = 0) const;

    /*
     * Закрыть сокет
     */
    void CloseConnection();

    const std::string& GetIp() const;
    int GetPort() const;
private:
    const std::string ip_;
    const int port_;
    std::chrono::milliseconds connectTimeout_, readTimeout_;
    int sock_;
    bool closed;
};
