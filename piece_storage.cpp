#include "piece_storage.h"
#include <iostream>
#include <cassert>

PieceStorage::PieceStorage(const TorrentFile &tf)
{
    assert(tf.pieceHashes.size() == tf.length / tf.pieceLength + bool(tf.length % tf.pieceLength));
    for(int cur_piece_index = 0; cur_piece_index < tf.pieceHashes.size(); ++cur_piece_index)
    {
        PushPiece(std::make_shared<Piece>(
                cur_piece_index,
                (cur_piece_index != tf.pieceHashes.size() - 1) ? tf.pieceLength : (tf.length % tf.pieceLength) ? tf.length % tf.pieceLength : tf.pieceLength,
                tf.pieceHashes[cur_piece_index]));
    }
}
PiecePtr PieceStorage::GetNextPieceToDownload()
{
    if(remainPieces_.empty()) return nullptr;

    auto to_return = remainPieces_.front();
    remainPieces_.pop();
    return to_return;
}

void PieceStorage::PieceProcessed(const PiecePtr& piece)
{
    if(piece->HashMatches())
    {
        SavePieceToDisk(piece);
        ClearQueue();
        return;
    }

    piece->Reset();
    PushPiece(piece);
}

void PieceStorage::PushPiece(PiecePtr piece)
{
    remainPieces_.push(piece);
}

bool PieceStorage::QueueIsEmpty() const
{
    return remainPieces_.empty();
}

size_t PieceStorage::TotalPiecesCount() const
{
    return remainPieces_.size();
}

void PieceStorage::ClearQueue()
{
    while(!remainPieces_.empty())
    {
        remainPieces_.pop();
    }
}

void PieceStorage::SavePieceToDisk(PiecePtr piece) {
    // Эта функция будет переопределена при запуске вашего решения в проверяющей системе
    // Вместо сохранения на диск там пока что будет проверка, что часть файла скачалась правильно
    std::cout << "Downloaded piece " << piece->GetIndex() << std::endl;
    std::cerr << "Clear pieces list, don't want to download all of them" << std::endl;
    ClearQueue();
}
