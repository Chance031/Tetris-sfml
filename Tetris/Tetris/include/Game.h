#pragma once

#include "Board.h"

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <random>
#include <string>
#include <vector>

class Game
{
public:
    Game();
    void Run();

private:
    void ProcessEvents();
    void HandleKeyPressed(sf::Keyboard::Key key);
    void HandleTitleInput(sf::Keyboard::Key key);
    void HandlePlayingInput(sf::Keyboard::Key key);
    void HandlePausedInput(sf::Keyboard::Key key);
    void HandleGameOverInput(sf::Keyboard::Key key);

    void StartNewSession();
    void Update();
    void Render();
    void UpdateWindowTitle();

    int CalculateScore(int clearedLines) const;
    int CalculateTSpinScore(int clearedLines) const;
    void UpdateLevel();

    void SpawnNextPiece();
    void ProcessLockAndResolve();

    bool TryMoveCurrentPiece(int dx, int dy, bool lockOnFail);
    bool TryRotateCurrentPiece(RotationDirection direction);
    void StartLockDelay();
    void ResetLockDelay();
    void RefreshLockDelayAfterSuccessfulMove();
    bool IsCurrentPieceTouchingGround() const;
    bool DetectTSpin() const;
    void HardDropCurrentPiece();
    void HoldCurrentPiece();
    TetrominoType CreateRandomTetrominoType();
    void RefillPieceBag();
    Tetromino GetGhostPiece() const;

    void DrawBoard();
    void DrawPiece(const Tetromino& tetromino, std::uint8_t alpha = 255, bool useBoardOffset = true);
    void DrawMiniPiece(TetrominoType type, sf::Vector2f origin);
    void DrawPanel();
    void DrawOverlay();

private:
    static constexpr int InitialFallIntervalMs = 800;
    static constexpr int FallIntervalDecreasePerLevel = 50;
    static constexpr int MinFallIntervalMs = 100;
    static constexpr int LockDelayMs = 500;
    static constexpr int MaxLockResetCount = 15;
    static constexpr int SoftDropScorePerCell = 1;
    static constexpr int HardDropScorePerCell = 2;
    static constexpr int ComboScorePerStep = 50;
    static constexpr int TSpinNoLineScore = 400;
    static constexpr int TSpinSingleScore = 800;
    static constexpr int TSpinDoubleScore = 1200;
    static constexpr int TSpinTripleScore = 1600;

    static constexpr float BlockSize = 32.0f;
    static constexpr float BoardOffsetX = 60.0f;
    static constexpr float BoardOffsetY = 40.0f;

    sf::RenderWindow m_window;

    Board m_board;
    Tetromino m_currentPiece;
    Tetromino m_nextPiece;
    Tetromino m_holdPiece;

    GameState m_state = GameState::Title;
    bool m_isLockRequired = false;
    bool m_isTouchingGround = false;
    bool m_lastMoveWasRotation = false;
    bool m_hasHoldPiece = false;
    bool m_canHold = true;
    bool m_isBackToBackActive = false;

    int m_score = 0;
    int m_level = 1;
    int m_totalLines = 0;
    int m_combo = -1;
    int m_lockResetCount = 0;

    std::string m_lastClearMessage;

    sf::Clock m_fallClock;
    sf::Clock m_lockClock;
    sf::Time m_fallInterval;

    std::mt19937 m_randomEngine;
    std::vector<TetrominoType> m_pieceBag;
};
