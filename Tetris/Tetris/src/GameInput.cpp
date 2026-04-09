#include "Game.h"

#include <optional>

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
        ClearContinuousInputState();
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
        ClearContinuousInputState();
        m_state = GameState::Playing;
        m_fallClock.restart();

        if (m_isTouchingGround)
            m_lockClock.restart();

        return;
    }

    if (m_pauseMenuSelection == 1)
    {
        ClearContinuousInputState();
        StartNewSession();
        return;
    }

    m_window.close();
}

void Game::ActivateGameOverMenuSelection()
{
    if (m_gameOverMenuSelection == 0)
    {
        ClearContinuousInputState();
        StartNewSession();
        return;
    }

    if (m_gameOverMenuSelection == 1)
    {
        ClearContinuousInputState();
        m_state = GameState::Title;
        m_titleMenuSelection = 0;
        MarkTitleDirty();
        return;
    }

    m_window.close();
}

