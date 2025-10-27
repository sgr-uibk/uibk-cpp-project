#include "GameWorld.h"

#include "ResourceManager.h"

GameWorld::GameWorld(sf::Vector2u windowDimensions)
	: m_windowDimensions(windowDimensions),
	  m_map(windowDimensions),
	  m_player("BlueTank", sf::Color(90, 170, 255), m_map, 150),
	  m_healthbar({20.f, 20.f}, {220.f, 28.f}),
	  m_battleMusic(MusicManager::inst().load("audio/battle_loop.ogg"))
{
	m_worldView = sf::View(sf::FloatRect({0, 0}, sf::Vector2<float>(windowDimensions)));
	m_hudView = m_worldView; // copy

	m_player.setHealthCallback([this](int const health, int const max) {
		m_healthbar.setMaxHealth(max);
		m_healthbar.setHealth(health);
	});

	m_battleMusic.play();
}

void GameWorld::update(float dt)
{
	bool const w = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W);
	bool const s = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S);
	bool const a = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A);
	bool const d = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D);

	const sf::Vector2f moveVec{static_cast<float>(d - a), static_cast<float>(s - w)};
	m_player.movement(moveVec);

	if(sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space))
		m_player.shoot();

#ifdef SFML_DEBUG
	if(sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::H))
		m_player.heal(1);

	if(sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::R))
		m_player.revive();
#endif

	m_player.update(dt);
	m_healthbar.update(dt);

	// Center on player
	sf::Vector2f ppos = m_player.getPosition();
	m_worldView.setCenter(ppos);
}

void GameWorld::draw(sf::RenderWindow &window) const
{
	window.setView(m_worldView);
	window.clear(sf::Color::White);
	m_map.draw(window);
	m_player.draw(window);

	// HUD
	window.setView(m_hudView);
	window.draw(m_healthbar);
}
