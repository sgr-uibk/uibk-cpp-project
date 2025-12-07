#pragma once

// ============================================================================
// Gameplay constants
// ============================================================================

// TODO: Many constants are not used anywhere, use, else remove dead code
namespace GameConfig
{
namespace Player
{
	constexpr int DEFAULT_MAX_HEALTH = 100;
	constexpr float SHOOT_COOLDOWN = 0.5f; // seconds between shots
	constexpr int INVENTORY_SIZE = 9;
	constexpr int MAX_SIMULTANEOUS_POWERUPS = 5;
	constexpr float CANNON_LENGTH = 22.f; // distance from tank center to cannon tip
} // namespace Player

namespace Projectile
{
	constexpr float SPEED = 300.f;      // pixels per second
	constexpr int BASE_DAMAGE = 25;     // base damage without powerups
	constexpr float SIZE = 8.f;         // collision box size
	constexpr float MAX_LIFETIME = 5.f; // seconds before auto-despawn
} // namespace Projectile

namespace ItemSpawn
{
	constexpr float SPAWN_INTERVAL = 3.f; // seconds between spawns
	constexpr int MAX_ITEMS_ON_MAP = 5;   // max concurrent items
	constexpr float ITEM_SIZE = 24.f;     // collision box size
} // namespace ItemSpawn

namespace Powerup
{
	constexpr float SPEED_BOOST_MULTIPLIER = 1.5f;
	constexpr int DAMAGE_BOOST_MULTIPLIER = 2;
	constexpr float RAPID_FIRE_COOLDOWN = 0.1f;
	constexpr float EFFECT_DURATION = 5.f;
	constexpr int HEALTH_PACK_HEAL = 50;
	constexpr int SHIELD_HP = 50;
} // namespace Powerup

namespace UI
{
	// Pause menu buttons (sized for 800x600, will scale with window)
	constexpr float BUTTON_WIDTH = 180.f;
	constexpr float BUTTON_HEIGHT = 40.f;
	constexpr float BUTTON_SPACING = 15.f;

	// Music button (wider for text)
	constexpr float MUSIC_BUTTON_WIDTH = 220.f;
	constexpr float MUSIC_BUTTON_HEIGHT = 35.f;

	// Hotbar/Inventory
	constexpr float HOTBAR_SLOT_SIZE = 50.f;
	constexpr float HOTBAR_SLOT_SPACING = 5.f;

	// Text labels
	namespace Text
	{
		// Pause menu
		constexpr char const *PAUSE_TITLE = "PAUSED";
		constexpr char const *PAUSE_WARNING = "WARNING: You can be killed while paused!";
		constexpr char const *BUTTON_RESUME = "Resume";
		constexpr char const *BUTTON_DISCONNECT = "Disconnect";
		constexpr char const *BUTTON_SETTINGS = "Settings";
		constexpr char const *BUTTON_BACK = "Back";

		// Settings menu
		constexpr char const *SETTINGS_TITLE = "SETTINGS";
		constexpr char const *MUSIC_ON = "Music: ON";
		constexpr char const *MUSIC_OFF = "Music: OFF";
		constexpr char const *KEYBINDS_HEADER = "KEYBINDS";

		// Keybind labels
		constexpr char const *KEYBIND_MOVE = "Move: W A S D";
		constexpr char const *KEYBIND_SHOOT = "Shoot: Space / Left Click";
		constexpr char const *KEYBIND_USE_ITEM = "Use Item: Q";
		constexpr char const *KEYBIND_SELECT_HOTBAR = "Select Hotbar: 1-9 / Mouse Wheel";
		constexpr char const *KEYBIND_PAUSE = "Pause: Escape";
	} // namespace Text
} // namespace UI

namespace Menu
{
	// Common layout
	constexpr float TITLE_Y = 80.f;
	constexpr int TITLE_SIZE = 50;

	// Button dimensions (sized for 800x600, will scale with window)
	constexpr float MAIN_BUTTON_WIDTH = 240.f;
	constexpr float MAIN_BUTTON_HEIGHT = 40.f;
	constexpr float MAIN_BUTTON_SPACING = 12.f;
	constexpr float MAIN_BUTTON_START_Y = 200.f;

	constexpr float LOBBY_BUTTON_WIDTH = 180.f;
	constexpr float LOBBY_BUTTON_HEIGHT = 40.f;
	constexpr float LOBBY_BUTTON_Y = 450.f;
	constexpr float LOBBY_BUTTON_OFFSET = 110.f;

	constexpr float SMALL_BUTTON_WIDTH = 160.f;
	constexpr float SMALL_BUTTON_HEIGHT = 32.f;

	// Player name input
	constexpr float PLAYER_NAME_BOX_WIDTH = 150.f;
	constexpr float PLAYER_NAME_BOX_HEIGHT = 30.f;
	constexpr int MAX_PLAYER_NAME_LENGTH = 12;

	// Server connection
	constexpr float SERVER_IP_BOX_WIDTH = 180.f;
	constexpr float SERVER_IP_BOX_HEIGHT = 35.f;
	constexpr float SERVER_PORT_BOX_WIDTH = 120.f;
	constexpr int MAX_IP_LENGTH = 15;

	// Lobby display
	constexpr float LOBBY_INFO_Y = 180.f;
	constexpr float PLAYERS_LABEL_Y = 240.f;
	constexpr float PLAYERS_START_Y = 280.f;
	constexpr float PLAYER_SPACING = 30.f;

	// Font sizes
	constexpr int BUTTON_TEXT_SIZE = 24;
	constexpr int SMALL_BUTTON_TEXT_SIZE = 20;
	constexpr int LABEL_TEXT_SIZE = 18;
	constexpr int HEADER_TEXT_SIZE = 20;

	// Colors (as functions to avoid issues with constexpr)
	inline sf::Color buttonColor()
	{
		return sf::Color(80, 80, 80);
	}
	inline sf::Color buttonHoverColor()
	{
		return sf::Color(100, 100, 100);
	}
	inline sf::Color buttonDisabledColor()
	{
		return sf::Color(50, 50, 50);
	}
	inline sf::Color inputBoxColor()
	{
		return sf::Color(40, 40, 40);
	}
	inline sf::Color outlineColor()
	{
		return sf::Color(100, 100, 100);
	}
	inline sf::Color outlineHoverColor()
	{
		return sf::Color(150, 150, 150);
	}
	inline sf::Color outlineActiveColor()
	{
		return sf::Color(200, 200, 100);
	}
} // namespace Menu
} // namespace GameConfig
