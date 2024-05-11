#include "piece_storage.h"
#include <iostream>
#include <cassert>

using shared_lock = std::shared_lock<std::shared_mutex>;
using unique_lock = std::unique_lock<std::shared_mutex>;
using lock_guard = std::lock_guard<std::shared_mutex>;

PieceStorage::PieceStorage(const TorrentFile &tf, const std::filesystem::path& outputDirectory)
: TotalPiecesCounter_(tf.pieceHashes.size())
, OFFSET_(tf.pieceLength)
, stream_(outputDirectory / tf.name, std::ios::binary | std::ios::out)
{
    if(!stream_.is_open())
    {
        throw std::runtime_error("Error: cannot create or find file");
    }

    std::cerr << "Logger: Piece Storage was created, OFFSET_ = " << OFFSET_ << std::endl;

    stream_.seekp(tf.length - 1);
    stream_.write("\0", 1);

    assert(tf.pieceHashes.size() == tf.length / tf.pieceLength + bool(tf.length % tf.pieceLength)
            && OFFSET_ == tf.pieceLength);

    for(int cur_piece_index = 0; cur_piece_index < tf.pieceHashes.size(); ++cur_piece_index)
    {
        PushPiece(std::make_shared<Piece>(
                cur_piece_index,
                (cur_piece_index != tf.pieceHashes.size() - 1) ? tf.pieceLength : (tf.length % tf.pieceLength) ? tf.length % tf.pieceLength : tf.pieceLength,
                tf.pieceHashes[cur_piece_index]));
    }
}
PieceStorage::~PieceStorage()
{
    CloseOutputFile();
}
PiecePtr PieceStorage::GetNextPieceToDownload()
{
    lock_guard lock(queue_mutex_);
    
    if(remainPieces_.empty()) return nullptr;

    auto to_return = remainPieces_.front();
    remainPieces_.pop();
    return to_return;
}

void PieceStorage::PieceProcessed(const PiecePtr& piece)
{
    if(!piece->HashMatches())
    {
        piece->Reset();

        lock_guard lock(queue_mutex_);
        remainPieces_.push(piece);
    }
    else
    {
        SavePieceToDisk(piece);
    }
}

bool PieceStorage::QueueIsEmpty() const
{
    shared_lock lock(queue_mutex_);
    return remainPieces_.empty();
}

const std::vector<size_t>& PieceStorage::GetPiecesSavedToDiscIndices() const
{
    return PiecesSavedToDisk_;
}

size_t PieceStorage::TotalPiecesCount() const
{
    return TotalPiecesCounter_;
}

void PieceStorage::CloseOutputFile()
{
    std::lock_guard<std::mutex> lock(stream_mutex_);
    if(stream_.is_open()) stream_.close();
}

size_t PieceStorage::PiecesSavedToDiscCount() const
{
    shared_lock lock(queue_mutex_);
    return PiecesSavedToDisk_.size();
}

void PieceStorage::PushPiece(PiecePtr piece)
{
    lock_guard lock(queue_mutex_);
    remainPieces_.push(piece);
}

void PieceStorage::ClearQueue()
{
    lock_guard lock(queue_mutex_);
    while(!remainPieces_.empty())
    {
        remainPieces_.pop();
    }
}

size_t PieceStorage::PiecesInProgressCount() const
{
    shared_lock lock(queue_mutex_);
    return TotalPiecesCounter_ - remainPieces_.size() - PiecesSavedToDisk_.size();
}

void PieceStorage::SavePieceToDisk(const PiecePtr& piece)
{
    std::lock_guard<std::mutex> lock(stream_mutex_);

    std::cerr << "Logger: trying to save piece with idx = " << piece->GetIndex() << std::endl;

    if(!stream_.is_open())
    {
        throw std::runtime_error("Error: cannot save piece");
    }

    {
        lock_guard q_lock(queue_mutex_);
        PiecesSavedToDisk_.emplace_back(piece->GetIndex());
    }

    assert(piece->GetData().size() == piece->Length());

    stream_.seekp(piece->GetIndex() * OFFSET_, std::ios::beg);
    stream_.write(piece->GetData().data(), piece->Length());

    std::cerr << "Logger: piece with idx = " << piece->GetIndex() << " was successfully saved, its length = " << piece->Length() << std::endl;
}