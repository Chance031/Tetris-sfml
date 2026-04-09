#pragma once

#include "Board.h"

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <filesystem>
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
    void HandleKeyReleased(sf::Keyboard::Key key);
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
    void UpdateContinuousInput();
    void BeginHorizontalInput(int direction);
    void EndHorizontalInput(int direction);
    void BeginSoftDropInput();
    void EndSoftDropInput();
    int GetActiveHorizontalDirection() const;
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
    void DrawTextLine(std::string_view text, sf::Vector2f position, unsigned int size, sf::Color color);
    void LoadUIFont();
    void MarkTitleDirty();

private:
    static constexpr int InitialFallIntervalMs = 800;
    static constexpr int FallIntervalDecreasePerLevel = 50;
    static constexpr int MinFallIntervalMs = 100;
    static constexpr int LockDelayMs = 500;
    static constexpr int MaxLockResetCount = 15;
    static constexpr int HorizontalAutoShiftDelayMs = 140;
    static constexpr int HorizontalAutoRepeatMs = 45;
    static constexpr int SoftDropRepeatMs = 35;
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
    sf::Font m_font;
    bool m_hasFont = false;

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
    bool m_leftPressed = false;
    bool m_rightPressed = false;
    bool m_softDropPressed = false;
    bool m_hasReachedHorizontalRepeat = false;
    int m_lastHorizontalDirection = 0;

    int m_score = 0;
    int m_level = 1;
    int m_totalLines = 0;
    int m_combo = -1;
    int m_lockResetCount = 0;

    std::string m_lastClearMessage;

    sf::Clock m_fallClock;
    sf::Clock m_lockClock;
    sf::Clock m_horizontalInputClock;
    sf::Clock m_softDropClock;
    sf::Time m_fallInterval;

    std::mt19937 m_randomEngine;
    std::vector<TetrominoType> m_pieceBag;
    bool m_isWindowTitleDirty = true;
};
