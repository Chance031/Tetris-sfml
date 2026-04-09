#include "Game.h"

#include <algorithm>
#include <array>
#include <filesystem>
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
    const sf::Color stateColor(160, 224, 255);

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

    DrawTextLine("RUN", {632.0f, 108.0f}, 18, labelColor);
    DrawTextLine("SCORE", {632.0f, 136.0f}, 16, labelColor);
    DrawTextLine(std::to_string(m_score), {632.0f, 158.0f}, 30, valueColor);

    DrawTextLine("LEVEL", {632.0f, 208.0f}, 16, labelColor);
    DrawTextLine(std::to_string(m_level), {632.0f, 230.0f}, 28, valueColor);

    DrawTextLine("LINES", {632.0f, 280.0f}, 16, labelColor);
    DrawTextLine(std::to_string(m_totalLines), {632.0f, 302.0f}, 28, valueColor);

    DrawTextLine("STATE", {742.0f, 136.0f}, 16, labelColor);
    std::string stateText = "TITLE";
    if (m_state == GameState::Playing)
        stateText = "PLAYING";
    else if (m_state == GameState::Paused)
        stateText = "PAUSED";
    else if (m_state == GameState::GameOver)
        stateText = "GAME OVER";
    const unsigned int stateSize = m_state == GameState::GameOver ? 17u : 20u;
    DrawTextLine(stateText, {742.0f, 158.0f}, stateSize, stateColor);

    DrawTextLine("B2B", {742.0f, 208.0f}, 16, labelColor);
    DrawTextLine(m_isBackToBackActive ? "ACTIVE" : "IDLE", {742.0f, 230.0f}, 20,
        m_isBackToBackActive ? accentColor : hintColor);

    DrawTextLine("COMBO", {742.0f, 280.0f}, 16, labelColor);
    DrawTextLine(std::to_string(std::max(m_combo, 0)), {742.0f, 302.0f}, 20, valueColor);

    card.setPosition({460.0f, 410.0f});
    card.setSize({374.0f, 130.0f});
    m_window.draw(card);

    DrawTextLine("PROGRESS", {478.0f, 421.0f}, 18, labelColor);
    DrawTextLine("COMBO FLOW", {478.0f, 446.0f}, 16, labelColor);

    sf::RectangleShape meter({338.0f, 14.0f});
    meter.setPosition({478.0f, 468.0f});
    meter.setFillColor(MeterTrackColor);
    m_window.draw(meter);

    const float comboWidth = std::min(338.0f, static_cast<float>(std::max(m_combo, 0)) * 42.0f);
    meter.setSize({comboWidth, 14.0f});
    meter.setFillColor(ComboMeterColor);
    m_window.draw(meter);

    DrawTextLine("LEVEL PROGRESS", {478.0f, 500.0f}, 16, labelColor);
    meter.setSize({338.0f, 14.0f});
    meter.setPosition({478.0f, 522.0f});
    meter.setFillColor(MeterTrackColor);
    m_window.draw(meter);

    const float levelRatio = std::min(1.0f, static_cast<float>(m_totalLines % 10) / 10.0f);
    meter.setSize({338.0f * levelRatio, 14.0f});
    meter.setFillColor(LevelMeterColor);
    m_window.draw(meter);

    card.setPosition({460.0f, 554.0f});
    card.setSize({374.0f, 128.0f});
    m_window.draw(card);

    DrawTextLine("RECENT", {478.0f, 570.0f}, 18, labelColor);
    DrawTextLine(m_lastClearMessage.empty() ? "No recent clear" : m_lastClearMessage,
        {478.0f, 600.0f}, 22, m_lastClearMessage.empty() ? hintColor : accentColor);

    DrawTextLine("DROP SPEED", {478.0f, 634.0f}, 16, labelColor);
    DrawTextLine(std::to_string(static_cast<int>(m_fallInterval.asMilliseconds())) + " ms",
        {614.0f, 632.0f}, 18, valueColor);
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
        DrawCenteredTextLine("PAUSED", 480.0f, 246.0f, 34, sf::Color(245, 247, 250));
        DrawCenteredTextLine("Take a breath, then jump back in.", 480.0f, 292.0f, 18, sf::Color(208, 214, 224));
        DrawCenteredTextLine("Esc : Resume", 480.0f, 318.0f, 16, sf::Color(200, 205, 214));
        DrawMenuOption("Resume", 480.0f, 340.0f, m_pauseMenuSelection == 0);
        DrawMenuOption("Restart", 480.0f, 378.0f, m_pauseMenuSelection == 1);
        DrawMenuOption("Quit", 480.0f, 416.0f, m_pauseMenuSelection == 2);
        DrawCenteredTextLine("Up/Down : Move   Enter : Confirm", 480.0f, 452.0f, 16, sf::Color(190, 196, 208));
    }
    else if (m_state == GameState::GameOver)
    {
        DrawCenteredTextLine("GAME OVER", 480.0f, 232.0f, 34, sf::Color(255, 210, 210));
        DrawCenteredTextLine("Final Score", 480.0f, 278.0f, 18, sf::Color(208, 214, 224));
        DrawCenteredTextLine(std::to_string(m_score), 480.0f, 306.0f, 30, sf::Color(255, 214, 120));
        DrawMenuOption("Restart", 480.0f, 350.0f, m_gameOverMenuSelection == 0);
        DrawMenuOption("Back to Title", 480.0f, 388.0f, m_gameOverMenuSelection == 1);
        DrawMenuOption("Quit", 480.0f, 426.0f, m_gameOverMenuSelection == 2);
        DrawCenteredTextLine("Up/Down : Move   Enter : Confirm", 480.0f, 458.0f, 16, sf::Color(190, 196, 208));
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
