#include "PowerupCooldownPanelClient.h"
#include "Item/PowerupSpriteManager.h"
#include <cmath>

PowerupCooldownPanel::PowerupCooldownPanel(PlayerState const &playerState, sf::RenderWindow const &window)
	: m_playerState(playerState), m_window(window)
{
	for(int i = 0; i < PlayerState::NUM_POWERUP_TYPES; ++i)
	{
		m_typeSlots[i] = TypeSlotState{};
		m_typeSlots[i].type = static_cast<PowerupType>(i);
	}
}

void PowerupCooldownPanel::update(float dt)
{
	for(int i = 1; i < PlayerState::NUM_POWERUP_TYPES; ++i)
	{
		auto type = static_cast<PowerupType>(i);
		auto &slot = m_typeSlots[i];

		bool isActive = m_playerState.hasPowerup(type);
		float reuseCooldown = m_playerState.m_powerupReuseCooldowns[i].getRemaining();

		if(isActive)
		{
			slot.phase = TypeSlotState::Phase::ACTIVE_DURATION;

			auto const *effect = m_playerState.getPowerup(type);
			if(effect)
			{
				slot.totalDuration = GameConfig::Powerup::EFFECT_DURATION;
				slot.remaining = effect->duration;
			}
		}
		else if(reuseCooldown > 0.f)
		{
			slot.phase = TypeSlotState::Phase::REUSE_COOLDOWN;
			slot.totalDuration = REUSE_COOLDOWN_DURATION;
			slot.remaining = reuseCooldown;
		}
		else
		{
			slot.phase = TypeSlotState::Phase::INACTIVE;
		}

		slot.flashTimer = std::max(0.f, slot.flashTimer - dt);
	}
}

void PowerupCooldownPanel::triggerCooldownFlash(PowerupType type)
{
	int typeIndex = static_cast<int>(type);
	if(typeIndex > 0 && typeIndex < PlayerState::NUM_POWERUP_TYPES)
	{
		m_typeSlots[typeIndex].flashTimer = FLASH_DURATION;
	}
}

void PowerupCooldownPanel::draw(sf::RenderWindow &window) const
{
	int activeCount = countActiveSlots();
	if(activeCount == 0)
		return;

	sf::Vector2f panelPos = calculatePanelPosition();
	sf::Vector2f panelSize = calculatePanelSize();

	drawMetalPlateBackground(window, panelPos, panelSize);

	float slotX = panelPos.x + PANEL_PADDING;
	float slotY = panelPos.y + PANEL_PADDING + SLOT_SIZE / 2.f;

	for(int i = 1; i < PlayerState::NUM_POWERUP_TYPES; ++i)
	{
		auto const &slot = m_typeSlots[i];
		if(slot.phase != TypeSlotState::Phase::INACTIVE)
		{
			drawSlot(window, slot, {slotX + SLOT_SIZE / 2.f, slotY});
			slotX += SLOT_SIZE + SLOT_SPACING;
		}
	}
}

void PowerupCooldownPanel::drawMetalPlateBackground(sf::RenderWindow &window, sf::Vector2f const &pos,
                                                    sf::Vector2f const &size) const
{
	using namespace GameConfig::UI;

	sf::RectangleShape plate(size);
	plate.setPosition(pos);
	plate.setFillColor(METAL_PLATE_COLOR);
	plate.setOutlineColor(METAL_PLATE_BORDER);
	plate.setOutlineThickness(METAL_PLATE_OUTLINE_THICKNESS);
	window.draw(plate);

	float const inset = METAL_HIGHLIGHT_INSET;
	sf::RectangleShape highlight({size.x - inset * 2.f, METAL_HIGHLIGHT_HEIGHT});
	highlight.setPosition({pos.x + inset, pos.y + inset});
	highlight.setFillColor(METAL_PLATE_HIGHLIGHT);
	window.draw(highlight);

	drawRivet(window, {pos.x + RIVET_INSET, pos.y + RIVET_INSET});
	drawRivet(window, {pos.x + size.x - RIVET_INSET, pos.y + RIVET_INSET});
	drawRivet(window, {pos.x + RIVET_INSET, pos.y + size.y - RIVET_INSET});
	drawRivet(window, {pos.x + size.x - RIVET_INSET, pos.y + size.y - RIVET_INSET});
}

void PowerupCooldownPanel::drawSlot(sf::RenderWindow &window, TypeSlotState const &slot, sf::Vector2f const &pos) const
{
	float flashIntensity = 0.f;
	if(slot.flashTimer > 0.f)
	{
		float pulsePhase = slot.flashTimer * 4.f;
		flashIntensity = std::abs(std::sin(pulsePhase * 3.14159f)) * 0.6f;
	}

	sf::CircleShape bg(SLOT_SIZE / 2.f);
	bg.setOrigin({SLOT_SIZE / 2.f, SLOT_SIZE / 2.f});
	bg.setPosition(pos);

	if(flashIntensity > 0.f)
	{
		uint8_t red = static_cast<uint8_t>(30 + 150 * flashIntensity);
		bg.setFillColor(sf::Color(red, 32, 35, 200));
		bg.setOutlineColor(sf::Color(180, 60, 60));
	}
	else
	{
		bg.setFillColor(sf::Color(30, 32, 35, 200));
		bg.setOutlineColor(sf::Color(80, 85, 90));
	}
	bg.setOutlineThickness(2.f);
	window.draw(bg);

	if(slot.phase == TypeSlotState::Phase::INACTIVE)
		return;

	float progress = 0.f;
	if(slot.totalDuration > 0.f)
	{
		progress = 1.f - (slot.remaining / slot.totalDuration);
	}

	PieChartShape pie(SLOT_SIZE / 2.f - 4.f);
	pie.setPosition(pos);

	if(slot.phase == TypeSlotState::Phase::ACTIVE_DURATION)
	{
		pie.setClockwise(true);
		pie.setFillColor(sf::Color(100, 200, 100, 150));
		pie.setProgress(progress);
	}
	else
	{
		pie.setClockwise(false);
		if(flashIntensity > 0.f)
		{
			uint8_t red = static_cast<uint8_t>(150 + 80 * flashIntensity);
			pie.setFillColor(sf::Color(red, 80, 80, 180));
		}
		else
		{
			pie.setFillColor(sf::Color(150, 150, 150, 150));
		}
		pie.setProgress(slot.remaining / slot.totalDuration);
	}

	window.draw(pie);

	if(slot.type != PowerupType::NONE)
	{
		sf::Sprite icon = PowerupSpriteManager::inst().getSpriteForType(slot.type, 0.28f);
		icon.setPosition(pos);
		if(slot.phase == TypeSlotState::Phase::REUSE_COOLDOWN)
		{
			if(flashIntensity > 0.f)
			{
				uint8_t red = static_cast<uint8_t>(180 + 75 * flashIntensity);
				icon.setColor(sf::Color(red, 100, 100));
			}
			else
			{
				icon.setColor(sf::Color(128, 128, 128));
			}
		}
		window.draw(icon);
	}
}

void PowerupCooldownPanel::drawRivet(sf::RenderWindow &window, sf::Vector2f const &pos) const
{
	using namespace GameConfig::UI;

	sf::CircleShape rivet(RIVET_RADIUS);
	rivet.setOrigin({RIVET_RADIUS, RIVET_RADIUS});
	rivet.setPosition(pos);
	rivet.setFillColor(METAL_RIVET_COLOR);
	rivet.setOutlineColor(RIVET_OUTLINE_COLOR);
	rivet.setOutlineThickness(RIVET_OUTLINE_THICKNESS);
	window.draw(rivet);
}

sf::Vector2f PowerupCooldownPanel::calculatePanelPosition() const
{
	sf::Vector2u windowSize = m_window.getSize();
	sf::Vector2f panelSize = calculatePanelSize();

	float x = MARGIN;
	float y = static_cast<float>(windowSize.y) - panelSize.y - MARGIN;

	return {x, y};
}

sf::Vector2f PowerupCooldownPanel::calculatePanelSize() const
{
	int activeCount = countActiveSlots();
	if(activeCount == 0)
		return {0.f, 0.f};

	float width = PANEL_PADDING * 2.f + activeCount * SLOT_SIZE + (activeCount - 1) * SLOT_SPACING;
	float height = PANEL_PADDING * 2.f + SLOT_SIZE;

	return {width, height};
}

int PowerupCooldownPanel::countActiveSlots() const
{
	return static_cast<int>(std::count_if(m_typeSlots.begin() + 1, m_typeSlots.end(),
	                                      [](auto const &s) { return s.phase != TypeSlotState::Phase::INACTIVE; }));
}
