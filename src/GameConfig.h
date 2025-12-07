#pragma once

// ============================================================================
// Gameplay constants
// ============================================================================

namespace GameConfig
{
namespace Player
{
	constexpr float SHOOT_COOLDOWN = 0.5f; // seconds between shots
	constexpr float CANNON_LENGTH = 22.f;  // distance from tank center to cannon tip
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
} // namespace GameConfig
