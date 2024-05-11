#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <utility>

using namespace std::chrono_literals;

PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield) : bitfield_(bitfield){}

bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const
{
    return bitfield_[pieceIndex >> 3] & (1 << (7 - pieceIndex % (1 << 3)));
}
void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex)
{
    bitfield_[pieceIndex >> 3] |= (1 << (7 - pieceIndex % (1 << 3)));
}
size_t PeerPiecesAvailability::Size() const
{
    size_t cnt = 0;
    for(auto&& byte : bitfield_)
    {
        cnt += std::__popcount(byte);
    }
    return cnt;
}
PeerConnect::PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId, PieceStorage& pieceStorage)
        : tf_(tf)
        , socket_(peer.ip, peer.port, 500ms, 500ms)
        , selfPeerId_(std::move(selfPeerId))
        , terminated_(false)
        , choked_(true)
        , pieceStorage_(pieceStorage)
        , pendingBlock_(false)
        , failed_(false) {}

void PeerConnect::Run()
{
    std::cerr << "###CONNECTION STARTED###" << std::endl;
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cerr << "Connection established to peer" << std::endl;
            try
            {
                MainLoop();
            } catch(const std::exception& e)
            {
                failed_ = true;
                std::cerr << "Ooops... something wrong in MAIN LOOP: " << e.what() << std::endl;
                Terminate();
            }
        }
        else
        {
            failed_ = true;
            std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
    }
    std::cerr << "###CONNECTION ENDED###" << std::endl;
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

    MessageId ID = (MessageId) (uint8_t) data.front();

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

void PeerConnect::RequestPiece()
{
    if(!pieceInProgress_)
    {
        pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
        int loop_size = pieceStorage_.TotalPiecesCount();
        while(pieceInProgress_ && loop_size-- && !piecesAvailability_.IsPieceAvailable(pieceInProgress_->GetIndex()))
        {
            pieceStorage_.PushPiece(pieceInProgress_);
            pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
        }
    }

    if(pieceInProgress_)
    {
        if(!pendingBlock_)
        {
            auto block = pieceInProgress_->FirstMissingBlock();

            if(block)
            {
                std::cerr << "Logger: trying to send Request" << std::endl;
                auto message = Message::Init(MessageId::Request,
                                             IntToBytes(pieceInProgress_->GetIndex())
                                             + IntToBytes(block->offset)
                                             + IntToBytes(block->length));
                socket_.SendData(message.ToString());
                pendingBlock_ = true;
                pieceInProgress_->SetPended(block->offset);
                std::cerr << "Logger: Request was sent" << std::endl;
            }
            else
            {
                pieceInProgress_ = nullptr;
            }
        }
    }
}

void PeerConnect::MainLoop() {
    while (!terminated_) {

        if(pieceStorage_.QueueIsEmpty())
        {
            std::cerr << "Logger: pieces queue is empty -> terminating..." << std::endl;
            Terminate();
            return;
        }

        auto message = Message::Parse(socket_.ReceiveData());

        if(message.id == MessageId::Choke)
        {
            std::cerr << "Logger: got message Choke" << std::endl;
            choked_ = true;
            Terminate();
        }
        else if(message.id == MessageId::Unchoke)
        {
            std::cerr << "Logger: got message Unchoke" << std::endl;
            choked_ = false;
        }
        else if(message.id == MessageId::KeepAlive)
        {
            std::cerr << "Logger: got meassage Keep Alive" << std::endl;
            continue;
        }
        else if(message.id == MessageId::Have)
        {
            std::cerr << "Logger: got message Have" << std::endl;
            piecesAvailability_.SetPieceAvailability(BytesToInt(message.payload));
        }
        else if(message.id == MessageId::Piece)
        {
            std::cerr << "Logger: got message Piece" << std::endl;
            size_t idx = BytesToInt(message.payload.substr(0, 4));
            auto begin = BytesToInt(message.payload.substr(4, 4));
            auto block = message.payload.substr(8);

            if(pieceInProgress_ && idx == pieceInProgress_->GetIndex())
            {
                pieceInProgress_->SaveBlock(begin, block);
                if(pieceInProgress_->AllBlocksRetrieved())
                {
                    pieceStorage_.PieceProcessed(pieceInProgress_);
                    pieceInProgress_ = nullptr;
                }
            }
            pendingBlock_ = false;
        }

        if (!choked_ && !pendingBlock_) {
            RequestPiece();
        }
    }
}

bool PeerConnect::Failed() const
{
    return failed_;
}
