#include "Game.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <optional>
#include <sstream>

namespace
{
    constexpr sf::Color BackgroundColor(18, 18, 24);
    constexpr sf::Color BoardBackgroundColor(26, 30, 38);
    constexpr sf::Color BoardOutlineColor(70, 76, 88);
    constexpr sf::Color EmptyCellColor(33, 37, 46);
    constexpr sf::Color PanelColor(28, 32, 40);
    constexpr sf::Color PanelOutlineColor(80, 86, 98);
    constexpr sf::Color MeterTrackColor(40, 44, 54);
    constexpr sf::Color ComboMeterColor(255, 176, 64);
    constexpr sf::Color LevelMeterColor(96, 200, 128);

    sf::Color GetPieceColor(TetrominoType type, std::uint8_t alpha = 255)
    {
        switch (type)
        {
        case TetrominoType::I: return sf::Color(84, 212, 255, alpha);
        case TetrominoType::J: return sf::Color(76, 119, 255, alpha);
        case TetrominoType::L: return sf::Color(255, 171, 64, alpha);
        case TetrominoType::O: return sf::Color(255, 214, 64, alpha);
        case TetrominoType::S: return sf::Color(102, 214, 110, alpha);
        case TetrominoType::T: return sf::Color(190, 110, 255, alpha);
        case TetrominoType::Z: return sf::Color(255, 99, 99, alpha);
        default: return sf::Color(220, 220, 220, alpha);
        }
    }

    TetrominoType CellValueToType(std::uint8_t value)
    {
        return static_cast<TetrominoType>(static_cast<int>(value) - 1);
    }
}

Game::Game()
    : m_window(sf::VideoMode({960u, 720u}), "Tetris SFML")
    , m_fallInterval(sf::milliseconds(InitialFallIntervalMs))
    , m_randomEngine(std::random_device{}())
{
    m_window.setFramerateLimit(60);
    m_pieceBag.reserve(7);
    LoadUIFont();
}

void Game::Run()
{
    UpdateWindowTitle();

    while (m_window.isOpen())
    {
        ProcessEvents();
        Update();
        Render();
        UpdateWindowTitle();
    }
}

void Game::ProcessEvents()
{
    while (const std::optional event = m_window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
            return;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            HandleKeyPressed(keyPressed->code);
    }
}

void Game::HandleKeyPressed(sf::Keyboard::Key key)
{
    switch (m_state)
    {
    case GameState::Title:
        HandleTitleInput(key);
        break;
    case GameState::Playing:
        HandlePlayingInput(key);
        break;
    case GameState::Paused:
        HandlePausedInput(key);
        break;
    case GameState::GameOver:
        HandleGameOverInput(key);
        break;
    }
}

void Game::HandleTitleInput(sf::Keyboard::Key key)
{
    if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space)
        StartNewSession();
    else if (key == sf::Keyboard::Key::Escape)
        m_window.close();
}

void Game::HandlePlayingInput(sf::Keyboard::Key key)
{
    if (key == sf::Keyboard::Key::Escape)
    {
        m_window.close();
        return;
    }

    if (key == sf::Keyboard::Key::P)
    {
        m_state = GameState::Paused;
        MarkTitleDirty();
        return;
    }

    switch (key)
    {
    case sf::Keyboard::Key::Left:
    case sf::Keyboard::Key::A:
        if (TryMoveCurrentPiece(-1, 0, false))
            m_lastMoveWasRotation = false;
        break;
    case sf::Keyboard::Key::Right:
    case sf::Keyboard::Key::D:
        if (TryMoveCurrentPiece(1, 0, false))
            m_lastMoveWasRotation = false;
        break;
    case sf::Keyboard::Key::Down:
    case sf::Keyboard::Key::S:
        if (TryMoveCurrentPiece(0, 1, true))
            m_score += SoftDropScorePerCell;
        break;
    case sf::Keyboard::Key::Z:
        TryRotateCurrentPiece(RotationDirection::CounterClockwise);
        break;
    case sf::Keyboard::Key::X:
    case sf::Keyboard::Key::Up:
    case sf::Keyboard::Key::W:
        TryRotateCurrentPiece(RotationDirection::Clockwise);
        break;
    case sf::Keyboard::Key::Space:
        HardDropCurrentPiece();
        break;
    case sf::Keyboard::Key::C:
    case sf::Keyboard::Key::LShift:
    case sf::Keyboard::Key::RShift:
        HoldCurrentPiece();
        break;
    default:
        break;
    }

    MarkTitleDirty();
}

void Game::HandlePausedInput(sf::Keyboard::Key key)
{
    if (key == sf::Keyboard::Key::P || key == sf::Keyboard::Key::Enter)
    {
        m_state = GameState::Playing;
        m_fallClock.restart();

        if (m_isTouchingGround)
            m_lockClock.restart();
    }
    else if (key == sf::Keyboard::Key::Escape)
    {
        m_window.close();
        return;
    }

    MarkTitleDirty();
}

void Game::HandleGameOverInput(sf::Keyboard::Key key)
{
    if (key == sf::Keyboard::Key::R || key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space)
        StartNewSession();
    else if (key == sf::Keyboard::Key::Escape)
        m_window.close();

    MarkTitleDirty();
}

void Game::StartNewSession()
{
    m_board.Reset();
    m_score = 0;
    m_level = 1;
    m_totalLines = 0;
    m_combo = -1;
    m_isBackToBackActive = false;
    m_lastClearMessage.clear();
    m_isLockRequired = false;
    m_isTouchingGround = false;
    m_lastMoveWasRotation = false;
    m_hasHoldPiece = false;
    m_canHold = true;
    m_lockResetCount = 0;
    m_pieceBag.clear();
    m_state = GameState::Playing;

    m_fallInterval = sf::milliseconds(InitialFallIntervalMs);
    m_fallClock.restart();
    m_lockClock.restart();

    m_nextPiece = Tetromino(CreateRandomTetrominoType());
    SpawnNextPiece();
    MarkTitleDirty();
}

void Game::Update()
{
    if (m_state != GameState::Playing)
        return;

    if (m_isLockRequired)
    {
        ProcessLockAndResolve();
        return;
    }

    if (m_isTouchingGround && m_lockClock.getElapsedTime() >= sf::milliseconds(LockDelayMs))
    {
        m_isLockRequired = true;
        ProcessLockAndResolve();
        return;
    }

    if (m_fallClock.getElapsedTime() < m_fallInterval)
        return;

    m_fallClock.restart();
    TryMoveCurrentPiece(0, 1, true);
}

void Game::Render()
{
    m_window.clear(BackgroundColor);
    DrawBoard();
    DrawPanel();
    DrawOverlay();
    m_window.display();
}

void Game::UpdateWindowTitle()
{
    if (!m_isWindowTitleDirty)
        return;

    std::ostringstream title;
    title << "Tetris SFML | ";

    switch (m_state)
    {
    case GameState::Title:
        title << "Title - Enter/Space to start";
        break;
    case GameState::Playing:
        title << "Score " << m_score << " | Level " << m_level << " | Lines " << m_totalLines;
        if (!m_lastClearMessage.empty())
            title << " | " << m_lastClearMessage;
        break;
    case GameState::Paused:
        title << "Paused - Press P or Enter to resume";
        break;
    case GameState::GameOver:
        title << "Game Over - Press R or Enter to restart";
        break;
    }

    m_window.setTitle(title.str());
    m_isWindowTitleDirty = false;
}

int Game::CalculateScore(int clearedLines) const
{
    switch (clearedLines)
    {
    case 1: return 100 * m_level;
    case 2: return 300 * m_level;
    case 3: return 500 * m_level;
    case 4: return 800 * m_level;
    default: return 0;
    }
}

int Game::CalculateTSpinScore(int clearedLines) const
{
    switch (clearedLines)
    {
    case 0: return TSpinNoLineScore * m_level;
    case 1: return TSpinSingleScore * m_level;
    case 2: return TSpinDoubleScore * m_level;
    case 3: return TSpinTripleScore * m_level;
    default: return 0;
    }
}

void Game::UpdateLevel()
{
    m_level = (m_totalLines / 10) + 1;
    const int fallInterval = InitialFallIntervalMs - (m_level - 1) * FallIntervalDecreasePerLevel;
    m_fallInterval = sf::milliseconds(std::max(fallInterval, MinFallIntervalMs));
}

void Game::SpawnNextPiece()
{
    m_currentPiece = m_nextPiece;
    m_currentPiece.SetPosition(Board::Width / 2 - 1, 0);
    m_currentPiece.SetRotation(0);
    m_lastMoveWasRotation = false;

    m_nextPiece = Tetromino(CreateRandomTetrominoType());
    MarkTitleDirty();
}

void Game::ProcessLockAndResolve()
{
    const bool isTSpin = DetectTSpin();
    m_board.LockPiece(m_currentPiece);

    const int clearedLines = m_board.ClearLines();

    if (isTSpin && clearedLines == 0)
    {
        m_score += CalculateTSpinScore(0);
        m_lastClearMessage = "T-Spin";
    }
    else if (clearedLines == 0)
    {
        m_lastClearMessage.clear();
    }

    if (clearedLines > 0)
    {
        const int lineClearScore = isTSpin ? CalculateTSpinScore(clearedLines) : CalculateScore(clearedLines);
        const bool isDifficultClear = (clearedLines == 4) || isTSpin;
        const char* clearNames[] = {"", "Single", "Double", "Triple", "Tetris"};

        m_lastClearMessage = isTSpin
            ? std::string("T-Spin ") + clearNames[clearedLines]
            : clearNames[clearedLines];

        ++m_combo;
        m_totalLines += clearedLines;
        m_score += lineClearScore;
        m_score += m_combo * ComboScorePerStep * m_level;

        if (isDifficultClear)
        {
            if (m_isBackToBackActive)
                m_score += lineClearScore / 2;

            m_isBackToBackActive = true;
        }
        else
        {
            m_isBackToBackActive = false;
        }

        UpdateLevel();
    }
    else
    {
        m_combo = -1;
    }

    m_isLockRequired = false;
    ResetLockDelay();
    SpawnNextPiece();
    m_canHold = true;
    m_fallClock.restart();

    if (!m_board.CanPlace(m_currentPiece))
        m_state = GameState::GameOver;

    MarkTitleDirty();
}

bool Game::TryMoveCurrentPiece(int dx, int dy, bool lockOnFail)
{
    m_currentPiece.Move(dx, dy);

    if (m_board.CanPlace(m_currentPiece))
    {
        RefreshLockDelayAfterSuccessfulMove();
        return true;
    }

    m_currentPiece.Move(-dx, -dy);

    if (lockOnFail)
        StartLockDelay();

    return false;
}

bool Game::TryRotateCurrentPiece(RotationDirection direction)
{
    const int oldRotationIndex = m_currentPiece.GetRotationIndex();
    const auto kicks = GetSrsKicks(m_currentPiece.GetType(), oldRotationIndex, direction);
    const std::array<Point, 8> fallbackKicks{
        Point{-1, 0},
        Point{1, 0},
        Point{-2, 0},
        Point{2, 0},
        Point{0, -1},
        Point{-1, -1},
        Point{1, -1},
        Point{0, -2}
    };

    if (direction == RotationDirection::Clockwise)
        m_currentPiece.RotateCW();
    else
        m_currentPiece.RotateCCW();

    for (const Point& kick : kicks)
    {
        m_currentPiece.Move(kick.x, kick.y);

        if (m_board.CanPlace(m_currentPiece))
        {
            RefreshLockDelayAfterSuccessfulMove();
            m_lastMoveWasRotation = true;
            return true;
        }

        m_currentPiece.Move(-kick.x, -kick.y);
    }

    // The current SFML port uses simple 4x4 local shape data rather than a full
    // guideline pivot model, so pure SRS offsets can still miss valid wall kicks.
    // Try a small set of practical fallback offsets to keep near-wall rotation working.
    for (const Point& kick : fallbackKicks)
    {
        m_currentPiece.Move(kick.x, kick.y);

        if (m_board.CanPlace(m_currentPiece))
        {
            RefreshLockDelayAfterSuccessfulMove();
            m_lastMoveWasRotation = true;
            return true;
        }

        m_currentPiece.Move(-kick.x, -kick.y);
    }

    if (direction == RotationDirection::Clockwise)
        m_currentPiece.RotateCCW();
    else
        m_currentPiece.RotateCW();

    return false;
}

void Game::StartLockDelay()
{
    if (m_isTouchingGround)
        return;

    m_isTouchingGround = true;
    m_lockResetCount = 0;
    m_lockClock.restart();
}

void Game::ResetLockDelay()
{
    m_isTouchingGround = false;
    m_lockResetCount = 0;
    m_lockClock.restart();
}

void Game::RefreshLockDelayAfterSuccessfulMove()
{
    if (!IsCurrentPieceTouchingGround())
    {
        ResetLockDelay();
        return;
    }

    if (!m_isTouchingGround)
    {
        StartLockDelay();
        return;
    }

    if (m_lockResetCount >= MaxLockResetCount)
        return;

    ++m_lockResetCount;
    m_lockClock.restart();
}

bool Game::IsCurrentPieceTouchingGround() const
{
    Tetromino testPiece = m_currentPiece;
    testPiece.Move(0, 1);
    return !m_board.CanPlace(testPiece);
}

bool Game::DetectTSpin() const
{
    if (m_currentPiece.GetType() != TetrominoType::T || !m_lastMoveWasRotation)
        return false;

    const Point position = m_currentPiece.GetPosition();
    const Point center{position.x + 1, position.y + 1};
    const std::array<Point, 4> corners{
        Point{center.x - 1, center.y - 1},
        Point{center.x + 1, center.y - 1},
        Point{center.x - 1, center.y + 1},
        Point{center.x + 1, center.y + 1}
    };

    int blockedCorners = 0;

    for (const Point& corner : corners)
    {
        if (!m_board.IsInside(corner) || m_board.IsCellFilled(corner))
            ++blockedCorners;
    }

    return blockedCorners >= 3;
}

void Game::HardDropCurrentPiece()
{
    int droppedCells = 0;

    while (TryMoveCurrentPiece(0, 1, false))
        ++droppedCells;

    m_score += droppedCells * HardDropScorePerCell;
    m_isLockRequired = true;
    MarkTitleDirty();
}

void Game::HoldCurrentPiece()
{
    if (!m_canHold)
        return;

    const TetrominoType currentType = m_currentPiece.GetType();

    if (m_hasHoldPiece)
    {
        const TetrominoType heldType = m_holdPiece.GetType();
        m_holdPiece = Tetromino(currentType);
        m_currentPiece = Tetromino(heldType);
        m_currentPiece.SetPosition(Board::Width / 2 - 1, 0);
    }
    else
    {
        m_holdPiece = Tetromino(currentType);
        m_hasHoldPiece = true;
        SpawnNextPiece();
    }

    m_canHold = false;
    m_isLockRequired = false;
    m_lastMoveWasRotation = false;
    ResetLockDelay();
    m_fallClock.restart();

    if (!m_board.CanPlace(m_currentPiece))
        m_state = GameState::GameOver;

    MarkTitleDirty();
}

TetrominoType Game::CreateRandomTetrominoType()
{
    if (m_pieceBag.empty())
        RefillPieceBag();

    const TetrominoType type = m_pieceBag.back();
    m_pieceBag.pop_back();
    return type;
}

void Game::RefillPieceBag()
{
    m_pieceBag = {
        TetrominoType::I,
        TetrominoType::J,
        TetrominoType::L,
        TetrominoType::O,
        TetrominoType::S,
        TetrominoType::T,
        TetrominoType::Z
    };

    std::shuffle(m_pieceBag.begin(), m_pieceBag.end(), m_randomEngine);
}

Tetromino Game::GetGhostPiece() const
{
    Tetromino ghostPiece = m_currentPiece;

    while (true)
    {
        ghostPiece.Move(0, 1);

        if (!m_board.CanPlace(ghostPiece))
        {
            ghostPiece.Move(0, -1);
            break;
        }
    }

    return ghostPiece;
}

void Game::DrawBoard()
{
    sf::RectangleShape boardBackground({Board::Width * BlockSize, Board::Height * BlockSize});
    boardBackground.setPosition({BoardOffsetX, BoardOffsetY});
    boardBackground.setFillColor(BoardBackgroundColor);
    boardBackground.setOutlineColor(BoardOutlineColor);
    boardBackground.setOutlineThickness(2.0f);
    m_window.draw(boardBackground);

    sf::RectangleShape cell({BlockSize - 2.0f, BlockSize - 2.0f});

    for (int y = 0; y < Board::Height; ++y)
    {
        for (int x = 0; x < Board::Width; ++x)
        {
            cell.setPosition({BoardOffsetX + x * BlockSize + 1.0f, BoardOffsetY + y * BlockSize + 1.0f});
            cell.setFillColor(EmptyCellColor);
            m_window.draw(cell);

            const std::uint8_t cellValue = m_board.GetCellValue({x, y});
            if (cellValue == 0)
                continue;

            cell.setFillColor(GetPieceColor(CellValueToType(cellValue)));
            m_window.draw(cell);
        }
    }

    DrawPiece(GetGhostPiece(), 70);
    DrawPiece(m_currentPiece);
}

void Game::DrawPiece(const Tetromino& tetromino, std::uint8_t alpha, bool useBoardOffset)
{
    sf::RectangleShape cell({BlockSize - 2.0f, BlockSize - 2.0f});
    const float offsetX = useBoardOffset ? BoardOffsetX : 0.0f;
    const float offsetY = useBoardOffset ? BoardOffsetY : 0.0f;

    for (const Point& block : tetromino.GetBlockLocations())
    {
        cell.setPosition({offsetX + block.x * BlockSize + 1.0f, offsetY + block.y * BlockSize + 1.0f});
        cell.setFillColor(GetPieceColor(tetromino.GetType(), alpha));
        m_window.draw(cell);
    }
}

void Game::DrawMiniPiece(TetrominoType type, sf::Vector2f origin)
{
    Tetromino preview(type);
    preview.SetPosition(0, 0);
    preview.SetRotation(0);
    const auto previewBlocks = preview.GetBlockLocations();

    float minX = 10.0f;
    float maxX = -10.0f;
    float minY = 10.0f;
    float maxY = -10.0f;

    for (const Point& block : previewBlocks)
    {
        minX = std::min(minX, static_cast<float>(block.x));
        maxX = std::max(maxX, static_cast<float>(block.x));
        minY = std::min(minY, static_cast<float>(block.y));
        maxY = std::max(maxY, static_cast<float>(block.y));
    }

    const float miniBlockSize = BlockSize * 0.7f;
    const float width = (maxX - minX + 1.0f) * miniBlockSize;
    const float height = (maxY - minY + 1.0f) * miniBlockSize;
    const float startX = origin.x + (110.0f - width) * 0.5f;
    const float startY = origin.y + (110.0f - height) * 0.5f;
    sf::RectangleShape cell({miniBlockSize - 2.0f, miniBlockSize - 2.0f});

    for (const Point& block : previewBlocks)
    {
        cell.setPosition({
            startX + (block.x - minX) * miniBlockSize,
            startY + (block.y - minY) * miniBlockSize
        });
        cell.setFillColor(GetPieceColor(type));
        m_window.draw(cell);
    }
}

void Game::DrawPanel()
{
    const sf::Vector2f nextBoxPos{460.0f, 70.0f};
    const sf::Vector2f holdBoxPos{460.0f, 240.0f};

    sf::RectangleShape box({110.0f, 110.0f});
    box.setFillColor(PanelColor);
    box.setOutlineThickness(2.0f);
    box.setOutlineColor(PanelOutlineColor);

    box.setPosition(nextBoxPos);
    m_window.draw(box);
    DrawMiniPiece(m_nextPiece.GetType(), nextBoxPos);
    DrawTextLine("NEXT", {460.0f, 38.0f}, 22, sf::Color(230, 232, 238));

    box.setPosition(holdBoxPos);
    m_window.draw(box);
    DrawTextLine("HOLD", {460.0f, 208.0f}, 22, sf::Color(230, 232, 238));

    if (m_hasHoldPiece)
        DrawMiniPiece(m_holdPiece.GetType(), holdBoxPos);

    DrawTextLine("SCORE", {610.0f, 72.0f}, 20, sf::Color(170, 176, 190));
    DrawTextLine(std::to_string(m_score), {610.0f, 100.0f}, 28, sf::Color(245, 247, 250));

    DrawTextLine("LEVEL", {610.0f, 160.0f}, 20, sf::Color(170, 176, 190));
    DrawTextLine(std::to_string(m_level), {610.0f, 188.0f}, 28, sf::Color(245, 247, 250));

    DrawTextLine("LINES", {610.0f, 248.0f}, 20, sf::Color(170, 176, 190));
    DrawTextLine(std::to_string(m_totalLines), {610.0f, 276.0f}, 28, sf::Color(245, 247, 250));

    DrawTextLine("COMBO", {460.0f, 360.0f}, 18, sf::Color(170, 176, 190));
    DrawTextLine(std::to_string(std::max(m_combo, 0)), {648.0f, 356.0f}, 18, sf::Color(245, 247, 250));

    sf::RectangleShape meter({180.0f, 16.0f});
    meter.setPosition({460.0f, 390.0f});
    meter.setFillColor(MeterTrackColor);
    m_window.draw(meter);

    const float comboWidth = std::min(180.0f, static_cast<float>(std::max(m_combo, 0)) * 24.0f);
    meter.setSize({comboWidth, 16.0f});
    meter.setFillColor(ComboMeterColor);
    m_window.draw(meter);

    meter.setSize({180.0f, 16.0f});
    meter.setPosition({460.0f, 430.0f});
    meter.setFillColor(MeterTrackColor);
    m_window.draw(meter);

    DrawTextLine("LEVEL PROGRESS", {460.0f, 452.0f}, 18, sf::Color(170, 176, 190));

    const float levelRatio = std::min(1.0f, static_cast<float>(m_totalLines % 10) / 10.0f);
    meter.setSize({180.0f * levelRatio, 16.0f});
    meter.setFillColor(LevelMeterColor);
    m_window.draw(meter);

    std::string stateText = "TITLE";
    if (m_state == GameState::Playing)
        stateText = "PLAYING";
    else if (m_state == GameState::Paused)
        stateText = "PAUSED";
    else if (m_state == GameState::GameOver)
        stateText = "GAME OVER";

    DrawTextLine("STATE", {460.0f, 520.0f}, 18, sf::Color(170, 176, 190));
    DrawTextLine(stateText, {460.0f, 546.0f}, 24, sf::Color(245, 247, 250));

    if (!m_lastClearMessage.empty())
    {
        DrawTextLine("LAST CLEAR", {460.0f, 596.0f}, 18, sf::Color(170, 176, 190));
        DrawTextLine(m_lastClearMessage, {460.0f, 622.0f}, 22, sf::Color(255, 214, 120));
    }
}

void Game::DrawOverlay()
{
    if (m_state == GameState::Playing)
        return;

    sf::RectangleShape overlay({960.0f, 720.0f});
    overlay.setPosition({0.0f, 0.0f});

    if (m_state == GameState::Title)
        overlay.setFillColor(sf::Color(8, 10, 16, 170));
    else if (m_state == GameState::Paused)
        overlay.setFillColor(sf::Color(16, 22, 36, 150));
    else
        overlay.setFillColor(sf::Color(48, 14, 18, 150));

    m_window.draw(overlay);

    sf::RectangleShape panel({280.0f, 180.0f});
    panel.setPosition({340.0f, 230.0f});
    panel.setFillColor(sf::Color(24, 28, 36, 220));
    panel.setOutlineThickness(2.0f);
    panel.setOutlineColor(sf::Color(90, 96, 108));
    m_window.draw(panel);

    if (m_state == GameState::Title)
    {
        DrawTextLine("TETRIS", {410.0f, 265.0f}, 34, sf::Color(245, 247, 250));
        DrawTextLine("Enter or Space: Start", {380.0f, 320.0f}, 20, sf::Color(220, 224, 232));
        DrawTextLine("Esc: Quit", {430.0f, 350.0f}, 20, sf::Color(220, 224, 232));
    }
    else if (m_state == GameState::Paused)
    {
        DrawTextLine("PAUSED", {415.0f, 285.0f}, 30, sf::Color(245, 247, 250));
        DrawTextLine("P or Enter: Resume", {388.0f, 335.0f}, 20, sf::Color(220, 224, 232));
    }
    else if (m_state == GameState::GameOver)
    {
        DrawTextLine("GAME OVER", {390.0f, 278.0f}, 30, sf::Color(255, 210, 210));
        DrawTextLine("R or Enter: Restart", {390.0f, 328.0f}, 20, sf::Color(220, 224, 232));
        DrawTextLine("Final Score: " + std::to_string(m_score), {392.0f, 358.0f}, 20, sf::Color(255, 214, 120));
    }
}

void Game::DrawTextLine(std::string_view text, sf::Vector2f position, unsigned int size, sf::Color color)
{
    if (!m_hasFont)
        return;

    sf::Text label(m_font, std::string(text), size);
    label.setFillColor(color);
    label.setPosition(position);
    m_window.draw(label);
}

void Game::LoadUIFont()
{
    const std::array<std::filesystem::path, 7> candidates{
        // Prefer bundled project fonts so the UI looks the same on every PC.
        std::filesystem::path("asset/fonts/Hatmuri9.ttf"),
        std::filesystem::path("../asset/fonts/Hatmuri9.ttf"),
        std::filesystem::path("../../asset/fonts/Hatmuri9.ttf"),
        std::filesystem::path("assets/fonts/malgun.ttf"),
        std::filesystem::path("assets/fonts/arial.ttf"),
        std::filesystem::path("C:/Windows/Fonts/malgun.ttf"),
        std::filesystem::path("C:/Windows/Fonts/arial.ttf")
    };

    for (const auto& path : candidates)
    {
        if (std::filesystem::exists(path) && m_font.openFromFile(path))
        {
            m_hasFont = true;
            return;
        }
    }
}

void Game::MarkTitleDirty()
{
    m_isWindowTitleDirty = true;
}
