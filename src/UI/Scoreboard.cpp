#include "Scoreboard.h"
#include "GameConfig.h"
#include <algorithm>
#include <spdlog/spdlog.h>

Scoreboard::Scoreboard(sf::Font &font) : m_font(font), m_isShowing(false), m_winnerId(0)
{
}

void Scoreboard::show(EntityId winnerId, const std::vector<PlayerStats> &playerStats)
{
	m_isShowing = true;
	m_winnerId = winnerId;
	m_playerStats = playerStats;

	setupButtons();

	spdlog::info("Scoreboard displayed, winner: {}", winnerId);
}

void Scoreboard::setupButtons()
{
	m_buttons.clear();
	m_buttonTexts.clear();

	std::vector<std::string> buttonLabels = {"Return to Lobby", "Quit"};
	float buttonWidth = GameConfig::UI::BUTTON_WIDTH;
	float buttonHeight = GameConfig::UI::BUTTON_HEIGHT;
	float spacing = GameConfig::UI::BUTTON_SPACING;
	float centerX = WINDOW_DIM.x / 2.f - buttonWidth / 2.f;
	float startY = static_cast<float>(WINDOW_DIM.y) - 150.f;

	for(size_t i = 0; i < buttonLabels.size(); ++i)
	{
		sf::RectangleShape button;
		button.setSize({buttonWidth, buttonHeight});
		button.setPosition(sf::Vector2f(centerX, startY + i * (buttonHeight + spacing)));
		button.setFillColor(sf::Color(80, 80, 80));
		button.setOutlineColor(sf::Color::White);
		button.setOutlineThickness(2.f);
		m_buttons.push_back(button);

		sf::Text text(m_font);
		text.setString(buttonLabels[i]);
		text.setCharacterSize(24);
		text.setFillColor(sf::Color::White);

		sf::FloatRect textBounds = text.getLocalBounds();
		text.setPosition(
			sf::Vector2f(centerX + buttonWidth / 2.f - textBounds.size.x / 2.f - textBounds.position.x,
		                 startY + i * (buttonHeight + spacing) + buttonHeight / 2.f - textBounds.size.y / 2.f - 5.f));
		m_buttonTexts.push_back(text);
	}
}

void Scoreboard::draw(sf::RenderWindow &window) const
{
	if(!m_isShowing)
		return;

	window.setView(window.getDefaultView());
	sf::RectangleShape overlay{sf::Vector2f(window.getSize())};
	overlay.setFillColor(sf::Color(0, 0, 0, 200));
	window.draw(overlay);

	sf::View hudView(sf::FloatRect({0.f, 0.f}, sf::Vector2f(WINDOW_DIM)));
	window.setView(hudView);

	sf::Text title(m_font);
	title.setString("GAME OVER");
	title.setCharacterSize(56);
	title.setFillColor(sf::Color::White);
	sf::FloatRect titleBounds = title.getLocalBounds();
	title.setPosition(sf::Vector2f(WINDOW_DIM.x / 2.f - titleBounds.size.x / 2.f, 40.f));
	window.draw(title);

	sf::Text winnerText(m_font);
	std::string winnerName = "Unknown";

	if(!m_playerStats.empty())
	{
		for(const auto &stats : m_playerStats)
		{
			if(stats.id == m_winnerId)
			{
				winnerName = stats.name;
				break;
			}
		}
	}

	if(m_winnerId == 0)
	{
		winnerText.setString("Draw!");
		winnerText.setFillColor(sf::Color(150, 150, 150));
	}
	else
	{
		winnerText.setString("Winner: " + winnerName);
		winnerText.setFillColor(sf::Color(200, 200, 200));
	}

	winnerText.setCharacterSize(40);
	sf::FloatRect winnerBounds = winnerText.getLocalBounds();
	winnerText.setPosition(sf::Vector2f(WINDOW_DIM.x / 2.f - winnerBounds.size.x / 2.f, 120.f));
	window.draw(winnerText);

	if(!m_playerStats.empty())
	{
		std::vector<PlayerStats> sortedStats = m_playerStats;
		std::sort(sortedStats.begin(), sortedStats.end(),
		          [](const PlayerStats &a, const PlayerStats &b) { return a.kills > b.kills; });

		sf::Text statsHeader(m_font);
		statsHeader.setString("PLAYER STATISTICS");
		statsHeader.setCharacterSize(28);
		statsHeader.setFillColor(sf::Color(200, 200, 100));
		sf::FloatRect headerBounds = statsHeader.getLocalBounds();
		statsHeader.setPosition(sf::Vector2f(WINDOW_DIM.x / 2.f - headerBounds.size.x / 2.f, 200.f));
		window.draw(statsHeader);

		float tableStartY = 250.f;
		float nameX = WINDOW_DIM.x / 2.f - 150.f;
		float killsX = WINDOW_DIM.x / 2.f + 30.f;
		float deathsX = WINDOW_DIM.x / 2.f + 100.f;

		sf::Text nameHeader(m_font);
		nameHeader.setString("Player");
		nameHeader.setCharacterSize(20);
		nameHeader.setFillColor(sf::Color(180, 180, 180));
		nameHeader.setPosition(sf::Vector2f(nameX, tableStartY));
		window.draw(nameHeader);

		sf::Text killsHeader(m_font);
		killsHeader.setString("Kills");
		killsHeader.setCharacterSize(20);
		killsHeader.setFillColor(sf::Color(180, 180, 180));
		killsHeader.setPosition(sf::Vector2f(killsX, tableStartY));
		window.draw(killsHeader);

		sf::Text deathsHeader(m_font);
		deathsHeader.setString("Deaths");
		deathsHeader.setCharacterSize(20);
		deathsHeader.setFillColor(sf::Color(180, 180, 180));
		deathsHeader.setPosition(sf::Vector2f(deathsX, tableStartY));
		window.draw(deathsHeader);

		float rowHeight = 30.f;
		for(size_t i = 0; i < sortedStats.size(); ++i)
		{
			float rowY = tableStartY + 35.f + i * rowHeight;
			const auto &stats = sortedStats[i];

			sf::Color textColor = (stats.id == m_winnerId) ? sf::Color(255, 215, 0) : sf::Color::White;

			sf::Text nameText(m_font);
			nameText.setString(stats.name);
			nameText.setCharacterSize(18);
			nameText.setFillColor(textColor);
			nameText.setPosition(sf::Vector2f(nameX, rowY));
			window.draw(nameText);

			sf::Text killsText(m_font);
			killsText.setString(std::to_string(stats.kills));
			killsText.setCharacterSize(18);
			killsText.setFillColor(textColor);
			killsText.setPosition(sf::Vector2f(killsX + 15.f, rowY));
			window.draw(killsText);

			sf::Text deathsText(m_font);
			deathsText.setString(std::to_string(stats.deaths));
			deathsText.setCharacterSize(18);
			deathsText.setFillColor(textColor);
			deathsText.setPosition(sf::Vector2f(deathsX + 20.f, rowY));
			window.draw(deathsText);
		}
	}

	for(size_t i = 0; i < m_buttons.size(); ++i)
	{
		window.draw(m_buttons[i]);
		window.draw(m_buttonTexts[i]);
	}
}

Scoreboard::ButtonAction Scoreboard::handleClick(sf::Vector2f mousePos)
{
	if(!m_isShowing)
		return ButtonAction::NONE;

	for(size_t i = 0; i < m_buttons.size(); ++i)
	{
		if(m_buttons[i].getGlobalBounds().contains(mousePos))
		{
			if(i == 0) // Return to Lobby
			{
				spdlog::info("Returning to lobby from scoreboard");
				return ButtonAction::RETURN_TO_LOBBY;
			}
			else if(i == 1) // Quit
			{
				spdlog::info("Disconnecting from scoreboard");
				return ButtonAction::MAIN_MENU;
			}
		}
	}

	return ButtonAction::NONE;
}
