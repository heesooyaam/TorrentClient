#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <utility>

using namespace std::chrono_literals;

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId)
        : tf_(tf)
        , socket_(peer.ip, peer.port, 1000ms, 1000ms)
        , selfPeerId_(std::move(selfPeerId))
        , terminated_(false)
        , choked_(true)
{}

void PeerConnect::Run()
{
    std::cout << "CONNECTION STARTED" << std::endl;
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer" << std::endl;
            try
            {
                MainLoop();
            } catch(const std::exception& e)
            {
                std::cout << "Ooops... something wrong with MAIN LOOP:\n" << e.what() << std::endl;
                Terminate();
            }
        }
        else
        {
            std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
    }
    std::cout << "CONNECTION ENDED" << std::endl;
}

void PeerConnect::PerformHandshake()
{
    socket_.EstablishConnection();

    socket_.SendData(std::string(1, (char) 19) + "BitTorrent protocol00000000"
                     + tf_.infoHash
                     + selfPeerId_);

    std::string get = socket_.ReceiveData(68);

    if(get.front() != ((char) 19) || get.substr(1, 19) != "BitTorrent protocol" || tf_.infoHash != get.substr(28, 20))
    {
        throw std::runtime_error("Error: handshake failed");
    }
}

bool PeerConnect::EstablishConnection()
{
    try {
        PerformHandshake();
    } catch (const std::exception& e){
        std::cerr << "Fail in function PerformHandshake(), peer " << socket_.GetIp() << ":" <<
                  socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
    try {
        ReceiveBitfield();
    } catch (const std::exception& e) {
        std::cerr << "Fail in function RecieveBitfield(), peer " << socket_.GetIp() << ":" <<
            socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
    try {
        SendInterested();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Fail in function SendInterested(), peer " << socket_.GetIp() << ":" <<
                  socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
}

void PeerConnect::ReceiveBitfield()
{
    std::string data = socket_.ReceiveData();

    if((int) data.front() == 20)
    {
        data = socket_.ReceiveData();
    }

    MessageId ID = (MessageId) (int) data.front();

    if(ID == MessageId::BitField)
    {
        piecesAvailability_ = PeerPiecesAvailability(data.substr(1));
    }
    else if(ID == MessageId::Unchoke)
    {
        choked_ = false;
    }
    else
    {
        throw std::runtime_error("Error: cannot get bitfield");
    }
}

void PeerConnect::SendInterested()
{
    socket_.SendData(IntToBytes(1) + std::string(1, (char) MessageId::Interested));
}

void PeerConnect::Terminate()
{
    std::cerr << "Terminate" << std::endl;
    terminated_ = true;
}

void PeerConnect::MainLoop()
{
    /*
     * При проверке вашего решения на сервере этот метод будет переопределен.
     * Если вы провели handshake верно, то в этой функции будет работать обмен данными с пиром
     */
    std::cout << "Dummy main loop" << std::endl;
    Terminate();
}