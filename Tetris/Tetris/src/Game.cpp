#include "Game.h"

#include <algorithm>
#include <array>

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
    ClearContinuousInputState();
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

    // 가이드라인 기준으로 줄을 지우지 않은 T-Spin은 기존 B2B를 유지하지만,
    // 그 동작만으로 새 B2B를 시작하지는 않습니다.
    // 일반적인 무클리어 배치도 현재 B2B 상태를 그대로 둡니다.

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

    // 현재 조각 회전 데이터는 완전한 가이드라인 피벗 모델이 아니라 4x4 회전 배열 기반이라,
    // 순수 SRS만으로는 벽에 붙은 상태의 단순한 회전을 놓칠 수 있습니다.
    // 그래서 옆 벽 상황에 한해서만 최소한의 수평 보정을 추가로 확인합니다.
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

void Game::ClearContinuousInputState()
{
    m_leftPressed = false;
    m_rightPressed = false;
    m_softDropPressed = false;
    m_hasReachedHorizontalRepeat = false;
    m_lastHorizontalDirection = 0;
    m_horizontalInputClock.restart();
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
