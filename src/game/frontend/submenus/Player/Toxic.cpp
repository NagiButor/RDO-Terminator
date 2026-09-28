#include "Toxic.hpp"
#include "game/frontend/items/Items.hpp"

namespace YimMenu::Submenus
{
    // "Griefing": messes with the player's stats, mount, vehicle and inventory without killing them
    std::shared_ptr<Category> BuildToxicMenu()
    {
        auto menu = std::make_shared<Category>("Griefing");

        auto stats = std::make_shared<Group>("Honor & Bounty");
        stats->AddItem(std::make_shared<PlayerCommandItem>("maxhonor"_J));
        stats->AddItem(std::make_shared<PlayerCommandItem>("minhonor"_J));
        stats->AddItem(std::make_shared<PlayerCommandItem>("increasebounty"_J));
        stats->AddItem(std::make_shared<PlayerCommandItem>("startparlay"_J));
        stats->AddItem(std::make_shared<PlayerCommandItem>("endparlay"_J));

        auto ticker = std::make_shared<Group>("Messages");
        ticker->AddItem(std::make_shared<PlayerCommandItem>("sendticker"_J));
        ticker->AddItem(std::make_shared<ListCommandItem>("tickermessage"_J, "Message"));

        auto mount = std::make_shared<Group>("Mount");
        mount->AddItem(std::make_shared<PlayerCommandItem>("buckoff"_J, "Buck Off Horse"));
        mount->AddItem(std::make_shared<PlayerCommandItem>("kickhorse"_J));
        mount->AddItem(std::make_shared<PlayerCommandItem>("killhorse"_J));
        mount->AddItem(std::make_shared<PlayerCommandItem>("deletehorse"_J));
        mount->AddItem(std::make_shared<PlayerCommandItem>("sendstablemountevent"_J));
        mount->AddItem(std::make_shared<ListCommandItem>("mountinstance"_J, "Instance"));
        mount->AddItem(std::make_shared<ListCommandItem>("stablemountevent"_J, "Event"));

        auto other = std::make_shared<Group>("Vehicle & Weapons");
        other->AddItem(std::make_shared<PlayerCommandItem>("deletevehicle"_J));
        other->AddItem(std::make_shared<PlayerCommandItem>("removeweapons"_J, "Remove Weapons"));

        menu->AddItem(stats);
        menu->AddItem(ticker);
        menu->AddItem(mount);
        menu->AddItem(other);

        return menu;
    }
}
