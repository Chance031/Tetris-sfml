#pragma once

struct Point
{
    int x = 0;
    int y = 0;
};

enum class GameState
{
    Title,
    Playing,
    Paused,
    GameOver
};

enum class TetrominoType
{
    I,
    J,
    L,
    O,
    S,
    T,
    Z
};

enum class RotationDirection
{
    Clockwise,
    CounterClockwise
};
