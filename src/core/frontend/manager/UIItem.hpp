#pragma once
#include "util/Joaat.hpp"

namespace YimMenu
{
	// Preferably, all items should be saved in a global instance to make UI elements searchable
	class UIItem
	{
	public:
		virtual ~UIItem() = default;
		virtual void Draw() = 0;

		// Layout hints for Group: hidden items take no row, inline items
		// (e.g. color swatches) share the row of the item before them.
		virtual bool IsVisible() const
		{
			return true;
		}
		virtual bool IsInline() const
		{
			return false;
		}
	};
}
