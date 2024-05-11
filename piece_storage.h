#pragma once

#include "torrent_file.h"
#include "piece.h"
#include <queue>
#include <string>
#include <unordered_set>
#include <shared_mutex>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <atomic>

/*
 * Хранилище информации о частях скачиваемого файла.
 * В этом классе отслеживается информация о том, какие части файла осталось скачать
 */
class PieceStorage {
public:
    PieceStorage(const TorrentFile& tf, const std::filesystem::path& outputDirectory);
    ~PieceStorage();

    /*
     * Отдает указатель на следующую часть файла, которую надо скачать
     */
    PiecePtr GetNextPieceToDownload();

    /*
     * Эта функция вызывается из PeerConnect, когда скачивание одной части файла завершено.
     * В рамках данного задания требуется очистить очередь частей для скачивания как только хотя бы одна часть будет успешно скачана.
     */
    void PieceProcessed(const PiecePtr& piece);

    /*
     * Остались ли не скачанные части файла?
     */
    bool QueueIsEmpty() const;

    /*
     * Отдает список номеров частей файла, которые были сохранены на диск
     */
    const std::vector<size_t>& GetPiecesSavedToDiscIndices() const;
    /*
     * Сколько частей файла всего
     */
    size_t TotalPiecesCount() const;

    /*
     * Сколько частей файла было сохранено на диск
     */
    size_t PiecesSavedToDiscCount() const;

    /*
     * Закрыть поток вывода в файл
     */
    void CloseOutputFile();

    /*
     * Запушить кусок в очередь
     */
    void PushPiece(PiecePtr piece);

    /*
     * Сколько частей файла в данный момент скачивается
     */
    size_t PiecesInProgressCount() const;
private:
    std::queue<PiecePtr> remainPieces_;  // очередь частей файла, которые осталось скачать
    std::vector<size_t> PiecesSavedToDisk_;
    const size_t TotalPiecesCounter_;
    const size_t OFFSET_;
    std::ofstream stream_;
    mutable std::shared_mutex queue_mutex_;
    mutable std::mutex stream_mutex_;

    /*
     * Сохранить piece на диск
     */
    void SavePieceToDisk(const PiecePtr& piece);

    /*
     * Почистить очередь
     */
    void ClearQueue();
};
