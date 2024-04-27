#pragma once

#include "peer.h"
#include "torrent_file.h"
#include "bencode.h"
#include "FileParser.h"
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <cpr/cpr.h>

class TorrentTracker {
public:
    /*
     * url - адрес трекера, берется из поля announce в .torrent-файле
     */
    TorrentTracker(const std::string& url)
    : url_(url)
    {}

    /*
     * Получить список пиров у трекера и сохранить его для дальнейшей работы.
     * Запрос пиров происходит посредством HTTP GET запроса, данные передаются в формате bencode.
     * Такой же формат использовался в .torrent файле.
     * Подсказка: посмотрите, что было написано в main.cpp в домашнем задании torrent-file
     *
     * tf: структура с разобранными данными из .torrent файла из предыдущего домашнего задания.
     * peerId: id, под которым представляется наш клиент.
     * port: порт, на котором наш клиент будет слушать входящие соединения (пока что мы не слушаем и на этот порт никто
     *  не сможет подключиться).
     */
    void UpdatePeers(const TorrentFile& tf, std::string peerId, int port)
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

        auto Dict = *std::dynamic_pointer_cast<bencode::Dictionary>(bencode::parse(stream, (char) stream.get()));

        peers_ = std::move(ParsePeer(std::dynamic_pointer_cast<bencode::String>(Dict["peers"])->get()));
    }

    /*
     * Отдает полученный ранее список пиров
     */
    const std::vector<Peer>& GetPeers() const
    {
        return peers_;
    }

private:
    std::string url_;
    std::vector<Peer> peers_;
};

