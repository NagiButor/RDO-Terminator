#include "Trolling.hpp"

namespace YimMenu::Submenus
{
	// "Pranks": harmless jokes - cages and attachments, nothing that damages the player
	std::shared_ptr<Category> BuildTrollingMenu()
	{
		auto menu = std::make_shared<Category>("Pranks");

		auto cages = std::make_shared<Group>("Cages");
		cages->AddItem(std::make_shared<PlayerCommandItem>("cageplayersmall"_J));
		cages->AddItem(std::make_shared<PlayerCommandItem>("cageplayerlarge"_J));
		cages->AddItem(std::make_shared<PlayerCommandItem>("cageplayercircus"_J));

		auto attachments = std::make_shared<Group>("Attachments");
		attachments->AddItem(std::make_shared<PlayerCommandItem>("spank"_J));
		attachments->AddItem(std::make_shared<PlayerCommandItem>("spankdatass"_J, "Spank Dat Ass"));
		attachments->AddItem(std::make_shared<PlayerCommandItem>("rideonshoulders"_J));
		attachments->AddItem(std::make_shared<PlayerCommandItem>("rideemcowboy"_J, "Rideem Cowboy"));
		attachments->AddItem(std::make_shared<PlayerCommandItem>("touchplayer"_J));
		attachments->AddItem(std::make_shared<PlayerCommandItem>("slap"_J));
		attachments->AddItem(std::make_shared<CommandItem>("cancelattachment"_J));

		menu->AddItem(cages);
		menu->AddItem(attachments);

		return menu;
	}
}
