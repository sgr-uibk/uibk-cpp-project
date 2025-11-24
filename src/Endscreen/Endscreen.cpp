#include "Endscreen.h"
#include <ResourceManager.h>
#include <spdlog/spdlog.h>
#include <sstream>

Endscreen::Endscreen(const sf::Vector2u &)
{
	try
	{
		m_font = ResourceManager<sf::Font>::inst().load("Font/LiberationSans-Regular.ttf");
	}
	catch(const std::exception &e)
	{
		SPDLOG_ERROR("Failed to load font for Endscreen: {}", e.what());
		throw;
	}

	m_title = std::make_unique<sf::Text>(m_font);
	m_placementText = std::make_unique<sf::Text>(m_font);
	m_continueText = std::make_unique<sf::Text>(m_font);
}

std::string cleanString(const std::string& input) {
    return std::string(input.c_str());
}

void Endscreen::setup(const PlayerState &winner, const std::vector<PlayerState> &players)
{
	std::vector<PlayerState> sortedPlayers;
	for (auto &p : players)
	{
		if (p.getPlayerId() != 0)
			sortedPlayers.push_back(p);
	}
	std::sort(sortedPlayers.begin(), sortedPlayers.end(),
	          [](const PlayerState &a, const PlayerState &b) { return a.getKills() > b.getKills(); });

	const float centerX = 800.f / 2.f;
	float currentY = 100.f;

	// Titel
	m_title->setFont(m_font);
	m_title->setString("Game Over");
	m_title->setCharacterSize(60);
	m_title->setFillColor(sf::Color(255, 165, 0));
	sf::FloatRect titleBounds = m_title->getLocalBounds();
	m_title->setOrigin({titleBounds.position.x + titleBounds.size.x / 2.f,
	                   titleBounds.position.y + titleBounds.size.y / 2.f});
	m_title->setPosition({centerX, currentY});
	currentY += titleBounds.size.y + 50.f; // Abstand nach unten

	// Winner & Placements
	std::stringstream ss;
	std::string winnerName = cleanString(winner.m_name);
	ss << "Winner: " << winnerName << "\n\nPlacements:\n";
	for (size_t i = 0; i < sortedPlayers.size(); ++i)
	{
		std::string playerName = cleanString(sortedPlayers[i].m_name);
		ss << (i + 1) << ". " << playerName << ": " << sortedPlayers[i].getKills() << " Kills\n";
	}

	m_placementText->setFont(m_font);
	m_placementText->setString(ss.str());
	m_placementText->setCharacterSize(32);
	m_placementText->setFillColor(sf::Color::White);

	sf::FloatRect placementBounds = m_placementText->getLocalBounds();
	m_placementText->setOrigin({placementBounds.position.x + placementBounds.size.x / 2.f,
	                           placementBounds.position.y}); // oben
	m_placementText->setPosition({centerX, currentY});
	currentY += placementBounds.size.y + 50.f;

	// Continue Text
	m_continueText->setFont(m_font);
	m_continueText->setCharacterSize(24);
	m_continueText->setFillColor(sf::Color::Green);
	m_continueText->setString("Weiter in 3s");

	sf::FloatRect continueBounds = m_continueText->getLocalBounds();
	m_continueText->setOrigin({continueBounds.position.x + continueBounds.size.x / 2.f,
	                          continueBounds.position.y});
	m_continueText->setPosition({centerX, currentY});

	m_timer = 3.0f;
	m_setupDone = true;

	for (auto &p : sortedPlayers)
	{
		SPDLOG_INFO("Player name raw: '{}'", p.m_name);
	}
}

void Endscreen::update(float dt)
{
	if (!m_setupDone)
		return;

	m_timer -= dt;
	if (m_timer <= 0.f)
	{
		m_exit = true;
		m_timer = 0.f;
	}

	int secondsLeft = static_cast<int>(std::ceil(m_timer));
	m_continueText->setString("Weiter in " + std::to_string(secondsLeft) + "s");

	sf::FloatRect bounds = m_continueText->getLocalBounds();
	m_continueText->setOrigin({bounds.position.x + bounds.size.x / 2.f,
	                          bounds.position.y});
}


void Endscreen::draw(sf::RenderWindow &window) const
{
	if(!m_setupDone)
		return;

	sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
	overlay.setFillColor(sf::Color(0, 0, 0, 200));

	window.draw(overlay);
	window.draw(*m_title);
	window.draw(*m_placementText);
	window.draw(*m_continueText);
}
