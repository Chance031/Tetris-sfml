#pragma once

#include "Tetromino.h"

#include <cstdint>

class Board
{
public:
    static constexpr int Width = 10;
    static constexpr int Height = 20;

    void Reset();

    bool CanPlace(const Tetromino& tetromino) const;
    void LockPiece(const Tetromino& tetromino);
    int ClearLines();

    bool IsInside(Point point) const;
    bool IsCellEmpty(Point point) const;
    bool IsCellFilled(Point point) const;
    std::uint8_t GetCellValue(Point point) const;

private:
    std::uint8_t m_cells[Height][Width] = {};
};
