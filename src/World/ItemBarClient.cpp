#include "ItemBarClient.h"
#include "ResourceManager.h"
#include "Item/PowerupSpriteManager.h"

ItemBarClient::ItemBarClient(PlayerState const &ownPlayer, sf::RenderWindow const &window)
	: m_ownPlayer(ownPlayer), m_window(window), m_font(FontManager::inst().load("Font/LiberationSans-Regular.ttf"))
{
}

void ItemBarClient::draw(sf::RenderWindow &window) const
{
	auto const &inventory = m_ownPlayer.getInventory();

	sf::Vector2f const windowSize(m_window.getSize());
	sf::Vector2f const start = {windowSize.x / 2.f - hotbarWidth / 2.f, windowSize.y - slotSize - 20.f};

	for(size_t i = 0; i < 9; ++i)
	{
		float x = start.x + i * (slotSize + slotSpacing);

		// slot background
		sf::RectangleShape slot(sf::Vector2f(slotSize, slotSize));
		slot.setPosition(sf::Vector2f(x, start.y));
		slot.setFillColor(sf::Color(40, 40, 40, 200));

		// selected slot highlight
		if(i == m_selectedSlot)
		{
			slot.setOutlineColor(GameConfig::UI::HOTBAR_SELECTED_OUTLINE);
			slot.setOutlineThickness(GameConfig::UI::HOTBAR_SELECTED_OUTLINE_THICKNESS);
		}
		else
		{
			slot.setOutlineColor(GameConfig::UI::HOTBAR_UNSELECTED_OUTLINE);
			slot.setOutlineThickness(GameConfig::UI::HOTBAR_UNSELECTED_OUTLINE_THICKNESS);
		}
		window.draw(slot);

		// inner glow for selected slot
		if(i == m_selectedSlot)
		{
			sf::RectangleShape innerGlow(sf::Vector2f(slotSize - 6.f, slotSize - 6.f));
			innerGlow.setPosition(sf::Vector2f(x + 3.f, start.y + 3.f));
			sf::Color glowColor = GameConfig::UI::HOTBAR_SELECTED_OUTLINE;
			glowColor.a = 50; // Semi-transparent
			innerGlow.setFillColor(glowColor);
			window.draw(innerGlow);
		}

		PowerupType itemType = inventory[i];
		if(itemType != PowerupType::NONE)
		{
			constexpr float spriteScale = 0.5f;

			sf::Sprite itemSprite = PowerupSpriteManager::inst().getSpriteForType(itemType, spriteScale);
			itemSprite.setPosition(sf::Vector2f(x + slotSize / 2.f, start.y + slotSize / 2.f));
			window.draw(itemSprite);
		}

		sf::Text slotNumber(m_font);
		slotNumber.setString(std::to_string(i + 1));
		slotNumber.setCharacterSize(14);
		slotNumber.setFillColor(sf::Color(200, 200, 200));
		slotNumber.setPosition(sf::Vector2f(x + 5.f, start.y + 2.f));
		window.draw(slotNumber);
	}
}

void ItemBarClient::moveSelection(int const delta)
{
	size_t const newSlot = (m_selectedSlot + delta + numSlots) % numSlots;
	m_selectedSlot = newSlot;
}

void ItemBarClient::handleKeyboardEvent(sf::Keyboard::Scancode scan)
{
	m_useItemRequested = scan == sf::Keyboard::Scancode::Q;

	if(scan >= sf::Keyboard::Scancode::Num1 && scan <= sf::Keyboard::Scancode::Num9)
	{
		int slot = static_cast<int>(scan) - static_cast<int>(sf::Keyboard::Scancode::Num1);
		setSelection(slot);
	}
}
