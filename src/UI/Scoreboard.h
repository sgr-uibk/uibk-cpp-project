#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Networking.h"

class Scoreboard
{
  public:
	struct PlayerStats
	{
		EntityId id;
		std::string name;
		int kills;
		int deaths;
	};

	enum class ButtonAction
	{
		NONE,
		RETURN_TO_LOBBY,
		MAIN_MENU
	};

	Scoreboard(sf::Font &font);

	void show(EntityId winnerId, std::vector<PlayerStats> const &playerStats);
	void draw(sf::RenderWindow &window) const;
	ButtonAction handleClick(sf::Vector2f mousePos);

	bool isShowing() const
	{
		return m_isShowing;
	}

  private:
	void setupButtons();

	sf::Font &m_font;
	bool m_isShowing;
	EntityId m_winnerId;
	std::vector<PlayerStats> m_playerStats;
	std::vector<sf::RectangleShape> m_buttons;
	std::vector<sf::Text> m_buttonTexts;
};
