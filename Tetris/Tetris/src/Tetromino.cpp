#include "Tetromino.h"

namespace
{
    constexpr int ShapeData[7][4][4][4] =
    {
        {
            {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
            {{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}},
            {{0, 0, 0, 0}, {0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}},
            {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}}
        },
        {
            {{1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
            {{0, 1, 1, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
            {{0, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
            {{0, 1, 0, 0}, {0, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}}
        },
        {
            {{0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
            {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
            {{0, 0, 0, 0}, {1, 1, 1, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}},
            {{1, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}
        },
        {
            {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
            {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
            {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
            {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}
        },
        {
            {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
            {{0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
            {{0, 0, 0, 0}, {0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}},
            {{1, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}
        },
        {
            {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
            {{0, 1, 0, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
            {{0, 0, 0, 0}, {1, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
            {{0, 1, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}
        },
        {
            {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
            {{0, 0, 1, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
            {{0, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
            {{0, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}}
        }
    };
}

std::array<Point, 5> GetSrsKicks(TetrominoType type, int oldRotationIndex, RotationDirection direction)
{
    const bool isClockwise = direction == RotationDirection::Clockwise;

    if (type == TetrominoType::I)
    {
        switch (oldRotationIndex)
        {
        case 0:
            return isClockwise
                ? std::array<Point, 5>{Point{0, 0}, Point{-2, 0}, Point{1, 0}, Point{-2, 1}, Point{1, -2}}
                : std::array<Point, 5>{Point{0, 0}, Point{-1, 0}, Point{2, 0}, Point{-1, -2}, Point{2, 1}};
        case 1:
            return isClockwise
                ? std::array<Point, 5>{Point{0, 0}, Point{-1, 0}, Point{2, 0}, Point{-1, -2}, Point{2, 1}}
                : std::array<Point, 5>{Point{0, 0}, Point{-2, 0}, Point{1, 0}, Point{-2, 1}, Point{1, -2}};
        case 2:
            return isClockwise
                ? std::array<Point, 5>{Point{0, 0}, Point{2, 0}, Point{-1, 0}, Point{2, -1}, Point{-1, 2}}
                : std::array<Point, 5>{Point{0, 0}, Point{1, 0}, Point{-2, 0}, Point{1, 2}, Point{-2, -1}};
        case 3:
            return isClockwise
                ? std::array<Point, 5>{Point{0, 0}, Point{1, 0}, Point{-2, 0}, Point{1, 2}, Point{-2, -1}}
                : std::array<Point, 5>{Point{0, 0}, Point{2, 0}, Point{-1, 0}, Point{2, -1}, Point{-1, 2}};
        default:
            break;
        }
    }

    switch (oldRotationIndex)
    {
    case 0:
        return isClockwise
            ? std::array<Point, 5>{Point{0, 0}, Point{-1, 0}, Point{-1, -1}, Point{0, 2}, Point{-1, 2}}
            : std::array<Point, 5>{Point{0, 0}, Point{1, 0}, Point{1, -1}, Point{0, 2}, Point{1, 2}};
    case 1:
        return isClockwise
            ? std::array<Point, 5>{Point{0, 0}, Point{1, 0}, Point{1, 1}, Point{0, -2}, Point{1, -2}}
            : std::array<Point, 5>{Point{0, 0}, Point{-1, 0}, Point{-1, 1}, Point{0, -2}, Point{-1, -2}};
    case 2:
        return isClockwise
            ? std::array<Point, 5>{Point{0, 0}, Point{1, 0}, Point{1, -1}, Point{0, 2}, Point{1, 2}}
            : std::array<Point, 5>{Point{0, 0}, Point{-1, 0}, Point{-1, -1}, Point{0, 2}, Point{-1, 2}};
    case 3:
        return isClockwise
            ? std::array<Point, 5>{Point{0, 0}, Point{-1, 0}, Point{-1, 1}, Point{0, -2}, Point{-1, -2}}
            : std::array<Point, 5>{Point{0, 0}, Point{1, 0}, Point{1, 1}, Point{0, -2}, Point{1, -2}};
    default:
        return {};
    }
}

Tetromino::Tetromino(TetrominoType type) : m_type(type)
{
}

void Tetromino::SetType(TetrominoType newType)
{
    m_type = newType;
}

void Tetromino::SetPosition(int x, int y)
{
    m_position = {x, y};
}

void Tetromino::SetRotation(int newRotationDegrees)
{
    int normalized = newRotationDegrees % 360;
    if (normalized < 0)
        normalized += 360;

    m_rotation = (normalized / 90) % 4;
}

void Tetromino::Move(int dx, int dy)
{
    m_position.x += dx;
    m_position.y += dy;
}

void Tetromino::RotateCW()
{
    m_rotation = (m_rotation + 1) % 4;
}

void Tetromino::RotateCCW()
{
    m_rotation = (m_rotation + 3) % 4;
}

std::array<Point, 4> Tetromino::GetBlockLocations() const
{
    const int typeIndex = static_cast<int>(m_type);
    std::array<Point, 4> blockLocations{};
    int blockIndex = 0;

    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            if (ShapeData[typeIndex][m_rotation][y][x] == 0)
                continue;

            blockLocations[blockIndex] = {m_position.x + x, m_position.y + y};
            ++blockIndex;
        }
    }

    return blockLocations;
}

TetrominoType Tetromino::GetType() const
{
    return m_type;
}

Point Tetromino::GetPosition() const
{
    return m_position;
}

int Tetromino::GetRotationDegrees() const
{
    return m_rotation * 90;
}

int Tetromino::GetRotationIndex() const
{
    return m_rotation;
}
