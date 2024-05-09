#include "byte_tools.h"
#include "piece.h"
namespace {
constexpr size_t BLOCK_SIZE = 1 << 14;
}
Block::Block()
: status(Status::Missing) {}


Piece::Piece(size_t index, size_t length, std::string hash)
: index_(index)
, length_(length)
, hash_(std::move(hash))
, retrieved_counter(0)
, blocks_(length / BLOCK_SIZE + bool(length % BLOCK_SIZE), Block())
{
    SplitIntoBlocks();
}

void Piece::SplitIntoBlocks()
{
    for(int i = 0; i < blocks_.size(); ++i)
    {
        blocks_[i].length = (i != blocks_.size() - 1) ? BLOCK_SIZE : (length_ % BLOCK_SIZE) ? length_ % BLOCK_SIZE : BLOCK_SIZE;
        blocks_[i].piece = index_;
        blocks_[i].offset = i * BLOCK_SIZE;
    }
}

size_t Piece::GetIndex() const
{
    return index_;
}

void Piece::SaveBlock(size_t blockOffset, std::string data)
{
    int block_index = blockOffset / BLOCK_SIZE;

    if(blocks_[block_index].status != Block::Status::Retrieved)
    {
        ++retrieved_counter;
    }

    blocks_[block_index].data = std::move(data);
    blocks_[block_index].status = Block::Status::Retrieved;
}

bool Piece::AllBlocksRetrieved() const
{
    return retrieved_counter == blocks_.size();
}

std::string Piece::GetData() const
{
    std::string data;
    for(const auto& block : blocks_)
    {
        data += block.data;
    }
    return data;
}

const std::string& Piece::GetHash() const
{
    return hash_;
}

std::string Piece::GetDataHash() const
{
    return CalculateSHA1(GetData());
}

void Piece::Reset()
{
    retrieved_counter = 0;
    for(auto& block : blocks_)
    {
        block.status = Block::Status::Missing;
        block.data.clear();
    }
}

bool Piece::HashMatches() const
{
    return hash_ == GetDataHash();
}

Block* Piece::FirstMissingBlock()
{
    for(auto& block : blocks_)
    {
        if(block.status == Block::Status::Missing) return &block;
    }

    return nullptr;
}

void Piece::SetPended(size_t offset)
{
    int block_index = offset / BLOCK_SIZE;

    blocks_[block_index].status = Block::Status::Pending;
}



