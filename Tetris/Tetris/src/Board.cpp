#include "Board.h"

#include <cstring>

void Board::Reset()
{
    std::memset(m_cells, 0, sizeof(m_cells));
}

bool Board::CanPlace(const Tetromino& tetromino) const
{
    const auto blocks = tetromino.GetBlockLocations();

    for (const Point& block : blocks)
    {
        if (!IsInside(block) || !IsCellEmpty(block))
            return false;
    }

    return true;
}

void Board::LockPiece(const Tetromino& tetromino)
{
    const auto blocks = tetromino.GetBlockLocations();
    const std::uint8_t cellValue = static_cast<std::uint8_t>(static_cast<int>(tetromino.GetType()) + 1);

    for (const Point& block : blocks)
    {
        if (IsInside(block))
            m_cells[block.y][block.x] = cellValue;
    }
}

int Board::ClearLines()
{
    int clearedLines = 0;
    int y = Height - 1;

    while (y >= 0)
    {
        bool isFullLine = true;

        for (int x = 0; x < Width; ++x)
        {
            if (m_cells[y][x] == 0)
            {
                isFullLine = false;
                break;
            }
        }

        if (!isFullLine)
        {
            --y;
            continue;
        }

        ++clearedLines;

        for (int row = y; row > 0; --row)
        {
            for (int x = 0; x < Width; ++x)
                m_cells[row][x] = m_cells[row - 1][x];
        }

        for (int x = 0; x < Width; ++x)
            m_cells[0][x] = 0;
    }

    return clearedLines;
}

bool Board::IsInside(Point point) const
{
    return point.x >= 0 && point.x < Width && point.y >= 0 && point.y < Height;
}

bool Board::IsCellEmpty(Point point) const
{
    return IsInside(point) && m_cells[point.y][point.x] == 0;
}

bool Board::IsCellFilled(Point point) const
{
    return IsInside(point) && m_cells[point.y][point.x] != 0;
}

std::uint8_t Board::GetCellValue(Point point) const
{
    return IsInside(point) ? m_cells[point.y][point.x] : 0;
}
