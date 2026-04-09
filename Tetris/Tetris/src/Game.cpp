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

        if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>())
            HandleKeyReleased(keyReleased->code);
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

void Game::HandleKeyReleased(sf::Keyboard::Key key)
{
    if (m_state != GameState::Playing)
        return;

    switch (key)
    {
    case sf::Keyboard::Key::Left:
    case sf::Keyboard::Key::A:
        EndHorizontalInput(-1);
        break;
    case sf::Keyboard::Key::Right:
    case sf::Keyboard::Key::D:
        EndHorizontalInput(1);
        break;
    case sf::Keyboard::Key::Down:
    case sf::Keyboard::Key::S:
        EndSoftDropInput();
        break;
    default:
        break;
    }
}

void Game::HandleTitleInput(sf::Keyboard::Key key)
{
    if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W)
        ChangeMenuSelection(m_titleMenuSelection, -1, 2);
    else if (key == sf::Keyboard::Key::Down || key == sf::Keyboard::Key::S)
        ChangeMenuSelection(m_titleMenuSelection, 1, 2);
    else if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space)
        ActivateTitleMenuSelection();

    MarkTitleDirty();
}

void Game::HandlePlayingInput(sf::Keyboard::Key key)
{
    if (key == sf::Keyboard::Key::Escape)
    {
        m_state = GameState::Paused;
        m_pauseMenuSelection = 0;
        MarkTitleDirty();
        return;
    }

    switch (key)
    {
    case sf::Keyboard::Key::Left:
    case sf::Keyboard::Key::A:
        BeginHorizontalInput(-1);
        if (TryMoveCurrentPiece(-1, 0, false))
            m_lastMoveWasRotation = false;
        break;
    case sf::Keyboard::Key::Right:
    case sf::Keyboard::Key::D:
        BeginHorizontalInput(1);
        if (TryMoveCurrentPiece(1, 0, false))
            m_lastMoveWasRotation = false;
        break;
    case sf::Keyboard::Key::Down:
    case sf::Keyboard::Key::S:
        BeginSoftDropInput();
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
    if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W)
        ChangeMenuSelection(m_pauseMenuSelection, -1, 3);
    else if (key == sf::Keyboard::Key::Down || key == sf::Keyboard::Key::S)
        ChangeMenuSelection(m_pauseMenuSelection, 1, 3);
    else if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space)
        ActivatePauseMenuSelection();
    else if (key == sf::Keyboard::Key::Escape)
        m_pauseMenuSelection = 0, ActivatePauseMenuSelection();

    MarkTitleDirty();
}

void Game::HandleGameOverInput(sf::Keyboard::Key key)
{
    if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W)
        ChangeMenuSelection(m_gameOverMenuSelection, -1, 3);
    else if (key == sf::Keyboard::Key::Down || key == sf::Keyboard::Key::S)
        ChangeMenuSelection(m_gameOverMenuSelection, 1, 3);
    else if (key == sf::Keyboard::Key::R)
    {
        m_gameOverMenuSelection = 0;
        ActivateGameOverMenuSelection();
    }
    else if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space)
        ActivateGameOverMenuSelection();

    MarkTitleDirty();
}

void Game::ChangeMenuSelection(int& selection, int delta, int itemCount)
{
    selection = (selection + delta + itemCount) % itemCount;
}

void Game::ActivateTitleMenuSelection()
{
    if (m_titleMenuSelection == 0)
    {
        StartNewSession();
        return;
    }

    m_window.close();
}

void Game::ActivatePauseMenuSelection()
{
    if (m_pauseMenuSelection == 0)
    {
        m_state = GameState::Playing;
        m_fallClock.restart();

        if (m_isTouchingGround)
            m_lockClock.restart();

        return;
    }

    if (m_pauseMenuSelection == 1)
    {
        StartNewSession();
        return;
    }

    m_window.close();
}

void Game::ActivateGameOverMenuSelection()
{
    if (m_gameOverMenuSelection == 0)
    {
        StartNewSession();
        return;
    }

    if (m_gameOverMenuSelection == 1)
    {
        m_state = GameState::Title;
        m_titleMenuSelection = 0;
        MarkTitleDirty();
        return;
    }

    m_window.close();
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
    m_hasTouchedGround = false;
    m_lastMoveWasRotation = false;
    m_hasHoldPiece = false;
    m_canHold = true;
    m_lockResetCount = 0;
    m_titleMenuSelection = 0;
    m_pauseMenuSelection = 0;
    m_gameOverMenuSelection = 0;
    m_leftPressed = false;
    m_rightPressed = false;
    m_softDropPressed = false;
    m_hasReachedHorizontalRepeat = false;
    m_lastHorizontalDirection = 0;
    m_pieceBag.clear();
    m_state = GameState::Playing;

    m_fallInterval = sf::milliseconds(InitialFallIntervalMs);
    m_fallClock.restart();
    m_lockClock.restart();
    m_horizontalInputClock.restart();
    m_softDropClock.restart();

    m_nextPiece = Tetromino(CreateRandomTetrominoType());
    SpawnNextPiece();
    MarkTitleDirty();
}

void Game::Update()
{
    if (m_state != GameState::Playing)
        return;

    UpdateContinuousInput();

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
        title << "Paused - Press Esc or Enter to resume";
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
    m_isTouchingGround = false;
    m_hasTouchedGround = false;
    m_lockResetCount = 0;
    m_lastMoveWasRotation = false;

    m_nextPiece = Tetromino(CreateRandomTetrominoType());
    MarkTitleDirty();
}

void Game::ProcessLockAndResolve()
{
    const bool isTSpin = DetectTSpin();
    m_board.LockPiece(m_currentPiece);

    const int clearedLines = m_board.ClearLines();
    const bool isDifficultLineClear = (clearedLines == 4) || (isTSpin && clearedLines > 0);

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
        const char* clearNames[] = {"", "Single", "Double", "Triple", "Tetris"};

        m_lastClearMessage = isTSpin
            ? std::string("T-Spin ") + clearNames[clearedLines]
            : clearNames[clearedLines];

        ++m_combo;
        m_totalLines += clearedLines;
        m_score += lineClearScore;
        m_score += m_combo * ComboScorePerStep * m_level;

        if (isDifficultLineClear)
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

    // Guideline behavior: a no-line T-Spin preserves the current B2B chain,
    // but it does not start a new one by itself. Other no-line placements
    // also leave the chain untouched.

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
    const std::array<Point, 4> horizontalWallKicks{
        Point{-1, 0},
        Point{1, 0},
        Point{-2, 0},
        Point{2, 0}
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

    // The current piece definitions are still 4x4 grid rotations rather than a
    // full pivot-based guideline model, so pure SRS can miss simple wall-adjacent
    // saves. Keep only a minimal horizontal fallback for side-wall cases.
    for (const Point& kick : horizontalWallKicks)
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
    m_hasTouchedGround = true;
    m_lockClock.restart();
}

void Game::ResetLockDelay()
{
    m_isTouchingGround = false;
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
    m_hasTouchedGround = true;
    m_lockClock.restart();
}

void Game::UpdateContinuousInput()
{
    const int horizontalDirection = GetActiveHorizontalDirection();
    if (horizontalDirection == 0)
    {
        m_hasReachedHorizontalRepeat = false;
    }
    else
    {
        const sf::Time horizontalElapsed = m_horizontalInputClock.getElapsedTime();
        const sf::Time repeatDelay = m_hasReachedHorizontalRepeat
            ? sf::milliseconds(HorizontalAutoRepeatMs)
            : sf::milliseconds(HorizontalAutoShiftDelayMs);

        if (horizontalElapsed >= repeatDelay)
        {
            if (TryMoveCurrentPiece(horizontalDirection, 0, false))
                m_lastMoveWasRotation = false;

            m_hasReachedHorizontalRepeat = true;
            m_horizontalInputClock.restart();
            MarkTitleDirty();
        }
    }

    if (!m_softDropPressed)
        return;

    if (m_softDropClock.getElapsedTime() < sf::milliseconds(SoftDropRepeatMs))
        return;

    if (TryMoveCurrentPiece(0, 1, true))
        m_score += SoftDropScorePerCell;

    m_softDropClock.restart();
    MarkTitleDirty();
}

void Game::BeginHorizontalInput(int direction)
{
    if (direction < 0)
        m_leftPressed = true;
    else if (direction > 0)
        m_rightPressed = true;

    m_lastHorizontalDirection = direction;
    m_hasReachedHorizontalRepeat = false;
    m_horizontalInputClock.restart();
}

void Game::EndHorizontalInput(int direction)
{
    if (direction < 0)
        m_leftPressed = false;
    else if (direction > 0)
        m_rightPressed = false;

    const int activeDirection = GetActiveHorizontalDirection();
    m_lastHorizontalDirection = activeDirection;
    m_hasReachedHorizontalRepeat = false;
    m_horizontalInputClock.restart();
}

void Game::BeginSoftDropInput()
{
    m_softDropPressed = true;
    m_softDropClock.restart();
}

void Game::EndSoftDropInput()
{
    m_softDropPressed = false;
    m_softDropClock.restart();
}

int Game::GetActiveHorizontalDirection() const
{
    if (m_leftPressed && m_rightPressed)
        return m_lastHorizontalDirection;

    if (m_leftPressed)
        return -1;

    if (m_rightPressed)
        return 1;

    return 0;
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
    std::array<bool, 4> isBlocked{};

    for (std::size_t index = 0; index < corners.size(); ++index)
    {
        if (!m_board.IsInside(corners[index]) || m_board.IsCellFilled(corners[index]))
        {
            ++blockedCorners;
            isBlocked[index] = true;
        }
    }

    if (blockedCorners < 3)
        return false;

    std::array<int, 2> frontCornerIndices{};
    switch (m_currentPiece.GetRotationIndex())
    {
    case 0:
        frontCornerIndices = {0, 1};
        break;
    case 1:
        frontCornerIndices = {1, 3};
        break;
    case 2:
        frontCornerIndices = {2, 3};
        break;
    case 3:
        frontCornerIndices = {0, 2};
        break;
    default:
        return false;
    }

    return isBlocked[frontCornerIndices[0]] && isBlocked[frontCornerIndices[1]];
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
    const sf::Color cardFill(22, 26, 34);
    const sf::Color cardOutline(76, 82, 96);
    const sf::Color labelColor(156, 164, 180);
    const sf::Color valueColor(244, 246, 250);
    const sf::Color accentColor(255, 214, 120);
    const sf::Color hintColor(198, 203, 214);

    sf::RectangleShape card;
    card.setFillColor(cardFill);
    card.setOutlineThickness(2.0f);
    card.setOutlineColor(cardOutline);

    DrawTextLine("TETRIS HUD", {458.0f, 26.0f}, 28, valueColor);
    DrawTextLine("Hold: C   Pause: Esc", {460.0f, 58.0f}, 16, hintColor);

    const sf::Vector2f nextCardPos{460.0f, 92.0f};
    const sf::Vector2f holdCardPos{460.0f, 244.0f};
    card.setSize({132.0f, 132.0f});

    card.setPosition(nextCardPos);
    m_window.draw(card);
    DrawTextLine("NEXT", {476.0f, 106.0f}, 18, labelColor);
    DrawMiniPiece(m_nextPiece.GetType(), {471.0f, 124.0f});

    card.setPosition(holdCardPos);
    m_window.draw(card);
    DrawTextLine("HOLD", {476.0f, 258.0f}, 18, labelColor);
    if (m_hasHoldPiece)
        DrawMiniPiece(m_holdPiece.GetType(), {471.0f, 276.0f});
    else
        DrawTextLine("EMPTY", {490.0f, 322.0f}, 16, hintColor);

    const sf::Vector2f statsCardPos{614.0f, 92.0f};
    card.setPosition(statsCardPos);
    card.setSize({220.0f, 284.0f});
    m_window.draw(card);

    DrawTextLine("SCORE", {632.0f, 108.0f}, 18, labelColor);
    DrawTextLine(std::to_string(m_score), {632.0f, 136.0f}, 30, valueColor);

    DrawTextLine("LEVEL", {632.0f, 192.0f}, 18, labelColor);
    DrawTextLine(std::to_string(m_level), {632.0f, 220.0f}, 28, valueColor);

    DrawTextLine("LINES", {632.0f, 268.0f}, 18, labelColor);
    DrawTextLine(std::to_string(m_totalLines), {632.0f, 296.0f}, 28, valueColor);

    DrawTextLine("COMBO", {632.0f, 328.0f}, 16, labelColor);
    DrawTextLine(std::to_string(std::max(m_combo, 0)), {760.0f, 326.0f}, 18, valueColor);
    DrawTextLine("B2B", {632.0f, 350.0f}, 16, labelColor);
    DrawTextLine(m_isBackToBackActive ? "ACTIVE" : "READY", {724.0f, 348.0f}, 18,
        m_isBackToBackActive ? accentColor : hintColor);

    card.setPosition({460.0f, 410.0f});
    card.setSize({374.0f, 122.0f});
    m_window.draw(card);

    DrawTextLine("COMBO FLOW", {478.0f, 426.0f}, 18, labelColor);

    sf::RectangleShape meter({338.0f, 14.0f});
    meter.setPosition({478.0f, 458.0f});
    meter.setFillColor(MeterTrackColor);
    m_window.draw(meter);

    const float comboWidth = std::min(338.0f, static_cast<float>(std::max(m_combo, 0)) * 42.0f);
    meter.setSize({comboWidth, 14.0f});
    meter.setFillColor(ComboMeterColor);
    m_window.draw(meter);

    DrawTextLine("LEVEL PROGRESS", {478.0f, 484.0f}, 18, labelColor);
    meter.setSize({338.0f, 14.0f});
    meter.setPosition({478.0f, 514.0f});
    meter.setFillColor(MeterTrackColor);
    m_window.draw(meter);

    const float levelRatio = std::min(1.0f, static_cast<float>(m_totalLines % 10) / 10.0f);
    meter.setSize({338.0f * levelRatio, 14.0f});
    meter.setFillColor(LevelMeterColor);
    m_window.draw(meter);

    std::string stateText = "TITLE";
    if (m_state == GameState::Playing)
        stateText = "PLAYING";
    else if (m_state == GameState::Paused)
        stateText = "PAUSED";
    else if (m_state == GameState::GameOver)
        stateText = "GAME OVER";

    card.setPosition({460.0f, 554.0f});
    card.setSize({374.0f, 112.0f});
    m_window.draw(card);

    DrawTextLine("STATE", {478.0f, 570.0f}, 18, labelColor);
    DrawTextLine(stateText, {478.0f, 600.0f}, 28, valueColor);

    if (!m_lastClearMessage.empty())
    {
        DrawTextLine("LAST CLEAR", {640.0f, 570.0f}, 18, labelColor);
        DrawTextLine(m_lastClearMessage, {640.0f, 600.0f}, 22, accentColor);
    }
    else
    {
        DrawTextLine("Build a combo for bonus points", {478.0f, 634.0f}, 16, hintColor);
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

    sf::RectangleShape panel({360.0f, 300.0f});
    panel.setPosition({300.0f, 192.0f});
    panel.setFillColor(sf::Color(24, 28, 36, 220));
    panel.setOutlineThickness(2.0f);
    panel.setOutlineColor(sf::Color(90, 96, 108));
    m_window.draw(panel);

    if (m_state == GameState::Title)
    {
        DrawCenteredTextLine("TETRIS", 480.0f, 250.0f, 36, sf::Color(245, 247, 250));
        DrawCenteredTextLine("Falling blocks, clean timing.", 480.0f, 300.0f, 18, sf::Color(208, 214, 224));
        DrawMenuOption("Start", 480.0f, 350.0f, m_titleMenuSelection == 0);
        DrawMenuOption("Quit", 480.0f, 388.0f, m_titleMenuSelection == 1);
        DrawCenteredTextLine("Up/Down : Move   Enter : Confirm", 480.0f, 450.0f, 16, sf::Color(190, 196, 208));
    }
    else if (m_state == GameState::Paused)
    {
        DrawCenteredTextLine("PAUSED", 480.0f, 268.0f, 34, sf::Color(245, 247, 250));
        DrawCenteredTextLine("Take a breath, then jump back in.", 480.0f, 318.0f, 18, sf::Color(208, 214, 224));
        DrawCenteredTextLine("Esc : Resume", 480.0f, 344.0f, 16, sf::Color(200, 205, 214));
        DrawMenuOption("Resume", 480.0f, 360.0f, m_pauseMenuSelection == 0);
        DrawMenuOption("Restart", 480.0f, 398.0f, m_pauseMenuSelection == 1);
        DrawMenuOption("Quit", 480.0f, 436.0f, m_pauseMenuSelection == 2);
        DrawCenteredTextLine("Up/Down : Move   Enter : Confirm", 480.0f, 470.0f, 16, sf::Color(190, 196, 208));
    }
    else if (m_state == GameState::GameOver)
    {
        DrawCenteredTextLine("GAME OVER", 480.0f, 254.0f, 34, sf::Color(255, 210, 210));
        DrawCenteredTextLine("Final Score", 480.0f, 306.0f, 18, sf::Color(208, 214, 224));
        DrawCenteredTextLine(std::to_string(m_score), 480.0f, 334.0f, 30, sf::Color(255, 214, 120));
        DrawMenuOption("Restart", 480.0f, 382.0f, m_gameOverMenuSelection == 0);
        DrawMenuOption("Back to Title", 480.0f, 420.0f, m_gameOverMenuSelection == 1);
        DrawMenuOption("Quit", 480.0f, 458.0f, m_gameOverMenuSelection == 2);
        DrawCenteredTextLine("Up/Down : Move   Enter : Confirm", 480.0f, 486.0f, 16, sf::Color(190, 196, 208));
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

void Game::DrawCenteredTextLine(std::string_view text, float centerX, float y, unsigned int size, sf::Color color)
{
    if (!m_hasFont)
        return;

    sf::Text label(m_font, std::string(text), size);
    label.setFillColor(color);

    const sf::FloatRect bounds = label.getLocalBounds();
    label.setPosition({centerX - (bounds.position.x + bounds.size.x * 0.5f), y});
    m_window.draw(label);
}

void Game::DrawMenuOption(std::string_view text, float centerX, float y, bool selected)
{
    const sf::Color optionColor = selected ? sf::Color(255, 214, 120) : sf::Color(220, 225, 234);
    const std::string label = selected ? "> " + std::string(text) + " <" : std::string(text);
    DrawCenteredTextLine(label, centerX, y, 20, optionColor);
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
