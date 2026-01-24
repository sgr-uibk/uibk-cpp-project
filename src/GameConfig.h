#pragma once
#include <string>
#include <array>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>

// ============================================================================
// Gameplay constants
// ============================================================================

constexpr float TILE_WIDTH = 64.0f;
constexpr float TILE_HEIGHT = 32.0f;
constexpr float CARTESIAN_TILE_SIZE = TILE_HEIGHT;

// ============================================================================
// Map registry
// ============================================================================

namespace Maps
{
constexpr std::array<char const *, 1> MAP_PATHS = {"map/arena.json"};

constexpr int DEFAULT_MAP_INDEX = 0;

inline std::string getMapPath(int mapIndex)
{
	if(mapIndex < 0 || mapIndex >= static_cast<int>(MAP_PATHS.size()))
		return std::string(MAP_PATHS[DEFAULT_MAP_INDEX]);
	return std::string(MAP_PATHS[mapIndex]);
}
} // namespace Maps

namespace GameConfig
{
namespace Player
{
	constexpr float SHOOT_COOLDOWN = 0.5f; // seconds between shots
	constexpr int INVENTORY_SIZE = 9;
	constexpr int MAX_SIMULTANEOUS_POWERUPS = 5;
	constexpr float COLLISION_DAMAGE = 0.15f;
	constexpr float CANNON_LENGTH = 22.f; // distance from tank center to cannon tip

	constexpr char const *TANK_SPRITESHEET_PATH = "tanks/tiger/hull_spritesheet.png";
	constexpr int TANK_SPRITE_SIZE = 150;
	constexpr int TANK_SPRITE_COLUMNS = 8;
	constexpr float TANK_DAMAGED_THRESHOLD = 0.5f;
} // namespace Player

namespace Projectile
{
	constexpr float SPEED = 300.f;  // pixels per second
	constexpr int BASE_DAMAGE = 25; // base damage without powerups
} // namespace Projectile

namespace ItemSpawn
{
	constexpr float SPAWN_INTERVAL = 3.f; // seconds between spawns
	constexpr int MAX_ITEMS_ON_MAP = 5;   // max concurrent items
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
	// Pause menu buttons
	constexpr float BUTTON_WIDTH = 200.f;
	constexpr float BUTTON_HEIGHT = 50.f;
	constexpr float BUTTON_SPACING = 20.f;

	// Music button (wider for text)
	constexpr float MUSIC_BUTTON_WIDTH = 250.f;
	constexpr float MUSIC_BUTTON_HEIGHT = 40.f;

	// Hotbar/Inventory
	constexpr float HOTBAR_SLOT_SIZE = 50.f;
	constexpr float HOTBAR_SLOT_SPACING = 5.f;
	constexpr sf::Color HOTBAR_SELECTED_OUTLINE = sf::Color(255, 215, 0); // Gold
	constexpr float HOTBAR_SELECTED_OUTLINE_THICKNESS = 3.f;
	constexpr sf::Color HOTBAR_UNSELECTED_OUTLINE = sf::Color(100, 100, 100);
	constexpr float HOTBAR_UNSELECTED_OUTLINE_THICKNESS = 1.f;

	// Window size presets
	constexpr std::array<sf::Vector2u, 5> WINDOW_SIZE_PRESETS = {
		sf::Vector2u{800, 600},  // 0: Default 4:3
		sf::Vector2u{1024, 768}, // 1: Standard 4:3
		sf::Vector2u{1280, 720}, // 2: HD 16:9
		sf::Vector2u{1366, 768}, // 3: WXGA 16:9
		sf::Vector2u{1920, 1080} // 4: Full HD 16:9
	};

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
		constexpr char const *WINDOW_SIZE_PREFIX = "Window: ";
		constexpr char const *KEYBINDS_HEADER = "KEYBINDS";

		// Keybind labels
		constexpr char const *KEYBIND_MOVE = "Move: W A S D";
		constexpr char const *KEYBIND_SHOOT = "Shoot: Space / Left Click";
		constexpr char const *KEYBIND_USE_ITEM = "Use Item: Q";
		constexpr char const *KEYBIND_SELECT_HOTBAR = "Select Hotbar: 1-9 / Mouse Wheel";
		constexpr char const *KEYBIND_CAMERA_ZOOM = "Camera Zoom: + / -";
		constexpr char const *KEYBIND_PAUSE = "Pause: Escape";
	} // namespace Text

	// Powerup Cooldown Panel
	constexpr float POWERUP_PANEL_SLOT_SIZE = 50.f;
	constexpr float POWERUP_PANEL_SLOT_SPACING = 8.f;
	constexpr float POWERUP_PANEL_MARGIN = 20.f;
	constexpr float POWERUP_REUSE_COOLDOWN = 10.f;

	// Ammunition Display
	constexpr float AMMO_BULLET_WIDTH = 12.f;
	constexpr float AMMO_BULLET_HEIGHT = 40.f;
	constexpr float AMMO_BULLET_SPACING = 4.f;
	constexpr float AMMO_PANEL_MARGIN = 20.f;
	constexpr int AMMO_BULLET_COUNT = 10;

	// Metal plate styling
	inline sf::Color const METAL_PLATE_COLOR{60, 65, 70, 230};
	inline sf::Color const METAL_PLATE_HIGHLIGHT{90, 95, 100, 150};
	inline sf::Color const METAL_RIVET_COLOR{40, 42, 45};
} // namespace UI
} // namespace GameConfig
