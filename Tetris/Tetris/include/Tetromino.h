#pragma once

#include "Types.h"

#include <array>

class Tetromino
{
public:
    Tetromino() = default;
    explicit Tetromino(TetrominoType type);

    void SetType(TetrominoType newType);
    void SetPosition(int x, int y);
    void SetRotation(int newRotationDegrees);

    void Move(int dx, int dy);
    void RotateCW();
    void RotateCCW();

    std::array<Point, 4> GetBlockLocations() const;
    TetrominoType GetType() const;
    Point GetPosition() const;
    int GetRotationDegrees() const;
    int GetRotationIndex() const;

private:
    TetrominoType m_type = TetrominoType::I;
    Point m_position{0, 0};
    int m_rotation = 0;
};

std::array<Point, 5> GetSrsKicks(TetrominoType type, int oldRotationIndex, RotationDirection direction);
