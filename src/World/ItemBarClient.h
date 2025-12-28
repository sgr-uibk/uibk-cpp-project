#pragma once
#include "Player/PlayerState.h"
#include <SFML/Graphics/RenderWindow.hpp>

class ItemBarClient
{
  public:
	ItemBarClient(PlayerState const &ownPlayer, sf::RenderWindow const &window);
	void draw(sf::RenderWindow &window) const;
	void moveSelection(int delta);
	void handleKeyboardEvent(sf::Keyboard::Scancode scan);
	void setSelection(int const slot)
	{
		m_selectedSlot = slot;
	}
	size_t getSelection() const
	{ // 0 is reserved for "no item used"
		return m_selectedSlot + 1;
	}
	bool handleItemUse() // clear-on-get
	{
		bool ret = m_useItemRequested;
		m_useItemRequested = false;
		return ret;
	}

  private:
	bool m_useItemRequested = false;
	size_t m_selectedSlot = 0;
	PlayerState const &m_ownPlayer;
	sf::RenderWindow const &m_window;
	sf::Font &m_font;
	static size_t constexpr numSlots = 9;
	static float constexpr slotSize = GameConfig::UI::HOTBAR_SLOT_SIZE;
	static float constexpr slotSpacing = GameConfig::UI::HOTBAR_SLOT_SPACING;
	static float constexpr hotbarWidth = numSlots * slotSize + (numSlots - 1) * slotSpacing;
};
