#include "ItemBarClient.h"
#include "ResourceManager.h"

ItemBarClient::ItemBarClient(PlayerState const &ownPlayer, sf::RenderWindow const &window)
	: m_ownPlayer(ownPlayer), m_window(window), m_font(FontManager::inst().load("Font/LiberationSans-Regular.ttf"))
{
}

void ItemBarClient::draw(sf::RenderWindow &window) const
{
	auto const &inventory = m_ownPlayer.getInventory();

	sf::Vector2f const windowSize(m_window.getSize());
	sf::Vector2f const start = {windowSize.x / 2.f - hotbarWidth / 2.f, windowSize.y - slotSize - 20.f};

	auto getColorForPowerup = [](PowerupType type) -> sf::Color {
		switch(type)
		{
		case PowerupType::HEALTH_PACK:
			return sf::Color::Green;
		case PowerupType::SPEED_BOOST:
			return sf::Color::Cyan;
		case PowerupType::DAMAGE_BOOST:
			return sf::Color::Red;
		case PowerupType::SHIELD:
			return sf::Color::Blue;
		case PowerupType::RAPID_FIRE:
			return sf::Color::Yellow;
		case PowerupType::NONE:
		default:
			return sf::Color(60, 60, 60);
		}
	};

	auto getNameForPowerup = [](PowerupType type) -> std::string {
		switch(type)
		{
		case PowerupType::HEALTH_PACK:
			return "HP";
		case PowerupType::SPEED_BOOST:
			return "SPD";
		case PowerupType::DAMAGE_BOOST:
			return "DMG";
		case PowerupType::SHIELD:
			return "SHD";
		case PowerupType::RAPID_FIRE:
			return "RPD";
		case PowerupType::NONE:
		default:
			return "";
		}
	};

	for(int i = 0; i < 9; ++i)
	{
		float x = start.x + i * (slotSize + slotSpacing);

		// slot background
		sf::RectangleShape slot(sf::Vector2f(slotSize, slotSize));
		slot.setPosition(sf::Vector2f(x, start.y));
		slot.setFillColor(sf::Color(40, 40, 40, 200));

		// selected slot highlight
		if(i == m_selectedSlot)
		{
			slot.setOutlineColor(sf::Color::White);
			slot.setOutlineThickness(3.f);
		}
		else
		{
			slot.setOutlineColor(sf::Color(100, 100, 100));
			slot.setOutlineThickness(1.f);
		}
		window.draw(slot);

		PowerupType itemType = inventory[i];
		if(itemType != PowerupType::NONE)
		{
			sf::RectangleShape itemIcon(sf::Vector2f(slotSize - 10.f, slotSize - 10.f));
			itemIcon.setPosition(sf::Vector2f(x + 5.f, start.y + 5.f));
			itemIcon.setFillColor(getColorForPowerup(itemType));
			window.draw(itemIcon);

			sf::Text itemText(m_font);
			itemText.setString(getNameForPowerup(itemType));
			itemText.setCharacterSize(12);
			itemText.setFillColor(sf::Color::White);
			sf::FloatRect textBounds = itemText.getLocalBounds();
			itemText.setPosition(sf::Vector2f(x + slotSize / 2.f - textBounds.size.x / 2.f,
			                                  start.y + slotSize / 2.f - textBounds.size.y / 2.f - 3.f));
			window.draw(itemText);
		}

		sf::Text slotNumber(m_font);
		slotNumber.setString(std::to_string(i + 1));
		slotNumber.setCharacterSize(14);
		slotNumber.setFillColor(sf::Color(200, 200, 200));
		sf::FloatRect numBounds = slotNumber.getLocalBounds();
		slotNumber.setPosition(sf::Vector2f(x + 5.f, start.y + 2.f));
		window.draw(slotNumber);
	}
}

void ItemBarClient::moveSelection(int const delta)
{
	int newSlot = m_selectedSlot;
	newSlot += delta;
	newSlot %= numSlots;
	assert(newSlot >= 0);
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
