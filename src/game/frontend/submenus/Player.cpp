#include "Player.hpp"

#include "core/commands/Commands.hpp"
#include "core/commands/IntCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/SavedLocations.hpp"
#include "game/backend/Self.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/frontend/submenus/Self.hpp"
#include "game/rdr/Natives.hpp"
#include "util/Math.hpp"
#include "util/Teleport.hpp"

#include <format>
#include <ranges>
#include <string>

namespace YimMenu::Submenus
{
    static float GetDistanceFromLocation(const SavedLocation& t)
    {
        return rage::fvector3(t.x, t.y, t.z).GetDistance(Self::GetPed().GetPosition());
    }

    void RenderCustomTeleport()
    {
        ImGui::BeginGroup();
        static std::string newLocationName{};
        static std::string category = "Default";
        static SavedLocation locationToDelete;

        if (!std::string(locationToDelete.name).empty())
            ImGui::OpenPopup("##deletelocation");

        if (ImGui::BeginPopupModal("##deletelocation", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        {
            ImGui::Text("Are you sure you want to delete %s?", locationToDelete.name.c_str());

            ImGui::Spacing();

            if (ImGui::Button("Yes"))
            {
                SavedLocations::DeleteSavedLocation(category, locationToDelete.name);
                locationToDelete.name = "";
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No"))
            {
                locationToDelete.name = "";
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PushItemWidth(300);
        InputTextWithHint("Category", "Category", &category).Draw();

        ImGui::PushItemWidth(200);
        InputTextWithHint("Location name", "New location", &newLocationName).Draw();
        ImGui::PopItemWidth();

        if (ImGui::Button("Save current location"))
        {
            FiberPool::Push([=] {
                if (newLocationName.empty())
                {
                    Notifications::Show("Custom Teleport", "Please enter a valid name.", NotificationType::Warning);
                }
                else if (SavedLocations::GetSavedLocationByName(newLocationName))
                {
                    Notifications::Show("Custom Teleport", std::format("Location with the name {} already exists", newLocationName));
                }
                else
                {
                    SavedLocation teleportLocation;
                    Entity teleportEntity = Self::GetPed();
                    if (auto vehicle = Self::GetVehicle())
                        teleportEntity = vehicle;
                    else if (auto mount = Self::GetMount())
                        teleportEntity = mount;

                    auto coords            = teleportEntity.GetPosition();
                    teleportLocation.name  = newLocationName;
                    teleportLocation.x     = coords.x;
                    teleportLocation.y     = coords.y;
                    teleportLocation.z     = coords.z;
                    teleportLocation.yaw   = ENTITY::GET_ENTITY_HEADING(teleportEntity.GetHandle());
                    teleportLocation.pitch = CAM::GET_GAMEPLAY_CAM_RELATIVE_PITCH();
                    teleportLocation.roll  = CAM::GET_GAMEPLAY_CAM_RELATIVE_HEADING();
                    SavedLocations::SaveNewLocation(category, teleportLocation);
                }
            });
        };

        ImGui::Separator();

        ImGui::Text("Double click to teleport\nShift click to delete");

        ImGui::Spacing();

        static std::string filter{};
        InputTextWithHint("##filter", "Search", &filter).Draw();

        ImGui::BeginGroup();
        ImGui::Text("Categories");
        if (ImGui::BeginListBox("##categories", {200, -1}))
        {
            for (auto& l : SavedLocations::GetAllSavedLocations() | std::ranges::views::keys)
            {
                if (ImGui::Selectable(l.data(), l == category))
                {
                    category = l;
                }

                if (category.empty())
                {
                    category = l;
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("Locations");
        if (ImGui::BeginListBox("##saved_locs", {200, -1}))
        {
            if (SavedLocations::GetAllSavedLocations().find(category) != SavedLocations::GetAllSavedLocations().end())
            {
                std::vector<SavedLocation> current_list{};
                if (!filter.empty())
                    current_list = SavedLocations::SavedLocationsFilteredList(filter);
                else
                    current_list = SavedLocations::GetAllSavedLocations().at(category);

                for (const auto& l : current_list)
                {
                    if (ImGui::Selectable(l.name.data(), false, ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                        {
                            locationToDelete = l;
                        }
                        else if (ImGui::IsMouseDoubleClicked(0))
                        {
                            FiberPool::Push([l] {
                                rage::fvector3 l_ = {l.x, l.y, l.z};
                                YimMenu::Teleport::TeleportEntity(Self::GetPed().GetHandle(), l_, false);
                            });
                        }
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        if (l.name.length() > 27)
                            ImGui::Text(l.name.data());
                        ImGui::Text("Distance: %f", GetDistanceFromLocation(l));
                        ImGui::EndTooltip();
                    }
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndGroup();

        ImGui::EndGroup();
    }

    Player::Player() :
        Submenu::Submenu("Player")
    {
        EnsureSelfHelpersLoaded();

        auto main = std::make_shared<Category>("Main");
        auto globalsGroup = std::make_shared<Group>("Globals");
        auto movementGroup = std::make_shared<Group>("Movement");
        auto toolsGroup = std::make_shared<Group>("Tools");

        main->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::TextDisabled("Survival, movement and everyday tools for your character.");
        }));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("godmode"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("neverwanted"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("invis"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("offtheradar"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("ghostmode"_J, "Ghost Mode"));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("noragdoll"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("antiafk"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("keepbarsfilled"_J));
        globalsGroup->AddItem(std::make_shared<ConditionalItem>("keepbarsfilled"_J, std::make_shared<BoolCommandItem>("keepdeadeyefilled"_J)));
        globalsGroup->AddItem(std::make_shared<ConditionalItem>("keepbarsfilled"_J, std::make_shared<BoolCommandItem>("keepstaminafilled"_J)));
        globalsGroup->AddItem(std::make_shared<ConditionalItem>("keepbarsfilled"_J, std::make_shared<BoolCommandItem>("keephealthfilled"_J)));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("keepcoresfilled"_J));
        globalsGroup->AddItem(std::make_shared<ConditionalItem>("keepcoresfilled"_J, std::make_shared<BoolCommandItem>("keepdeadeyecorefilled"_J)));
        globalsGroup->AddItem(std::make_shared<ConditionalItem>("keepcoresfilled"_J, std::make_shared<BoolCommandItem>("keepstaminacorefilled"_J)));
        globalsGroup->AddItem(std::make_shared<ConditionalItem>("keepcoresfilled"_J, std::make_shared<BoolCommandItem>("keephealthcorefilled"_J)));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("keepclean"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("antilasso"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("antihogtie"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("antimelee"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("drunk"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("superpunch"_J));
        globalsGroup->AddItem(std::make_shared<BoolCommandItem>("quickskin"_J));

        toolsGroup->AddItem(std::make_shared<CommandItem>("suicide"_J));
        toolsGroup->AddItem(std::make_shared<CommandItem>("clearcrimes"_J));
        toolsGroup->AddItem(std::make_shared<BoolCommandItem>("npcignore"_J));
        toolsGroup->AddItem(std::make_shared<BoolCommandItem>("eagleeye"_J));
        toolsGroup->AddItem(std::make_shared<BoolCommandItem>("overridewhistle"_J));
        toolsGroup->AddItem(std::make_shared<ConditionalItem>("overridewhistle"_J, std::make_shared<FloatCommandItem>("whistlepitch"_J, "Pitch")));
        toolsGroup->AddItem(std::make_shared<ConditionalItem>("overridewhistle"_J, std::make_shared<FloatCommandItem>("whistleclarity"_J, "Clarity")));
        toolsGroup->AddItem(std::make_shared<ConditionalItem>("overridewhistle"_J, std::make_shared<FloatCommandItem>("whistleshape"_J, "Shape")));

        static float playerScale = 1.0f;
        toolsGroup->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::Text("Player Scale");
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::InputFloat("##PlayerScale", &playerScale))
            {
                FiberPool::Push([] {
                    YimMenu::Self::GetPed().SetScale(playerScale);
                });
            }
        }));

        movementGroup->AddItem(std::make_shared<BoolCommandItem>("climbsteepslopes"_J));
        movementGroup->AddItem(std::make_shared<BoolCommandItem>("superjump"_J));
        movementGroup->AddItem(std::make_shared<BoolCommandItem>("superrun"_J));
        movementGroup->AddItem(std::make_shared<BoolCommandItem>("noclip"_J));
        movementGroup->AddItem(std::make_shared<ConditionalItem>("noclip"_J, std::make_shared<FloatCommandItem>("noclipspeed"_J)));
        movementGroup->AddItem(std::make_shared<BoolCommandItem>("freecam"_J));
        movementGroup->AddItem(std::make_shared<ConditionalItem>("freecam"_J, std::make_shared<FloatCommandItem>("freecamspeed"_J)));
        movementGroup->AddItem(std::make_shared<ConditionalItem>("freecam"_J, std::make_shared<BoolCommandItem>("freecamlightning"_J, "Freecam Lightning (LMB)")));

        main->AddItem(globalsGroup);
        main->AddItem(toolsGroup);
        main->AddItem(movementGroup);
        AddCategory(std::move(main));

        auto companions = std::make_shared<Category>("Companions");
        companions->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::TextDisabled("Bodyguards that follow you, fight for you and ride with you.");
        }));
        companions->AddItem(std::make_shared<ImGuiItem>([] { YimMenu::Submenus::RenderCompanionsCategory(); }));
        AddCategory(std::move(companions));

        auto teleport = std::make_shared<Category>("Teleport");
        auto teleportMiscGroup = std::make_shared<Group>("Destinations");

        teleport->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::TextDisabled("Instant travel for you, your horse and your wagon.");
        }));
        teleportMiscGroup->AddItem(std::make_shared<BoolCommandItem>("autotp"_J));
        teleportMiscGroup->AddItem(std::make_shared<CommandItem>("tptowaypoint"_J));
        teleportMiscGroup->AddItem(std::make_shared<CommandItem>("tptomount"_J));
        teleportMiscGroup->AddItem(std::make_shared<CommandItem>("tptotraintrack"_J));
        teleportMiscGroup->AddItem(std::make_shared<CommandItem>("tptomoonshineshack"_J));
        teleportMiscGroup->AddItem(std::make_shared<CommandItem>("tptonazar"_J));
        teleportMiscGroup->AddItem(std::make_shared<CommandItem>("tptoguarma"_J));
        teleportMiscGroup->AddItem(std::make_shared<ImGuiItem>([] {
            if (ImGui::Button("Bail to Loading Screen/ReSet Map"))
            {
                FiberPool::Push([] {
                    SCRIPTS::BAIL_TO_LANDING_PAGE(0);
                });
            }
        }));

        teleport->AddItem(teleportMiscGroup);
        AddCategory(std::move(teleport));

        auto customteleport = std::make_shared<Category>("Saved");
        customteleport->AddItem(std::make_shared<ImGuiItem>([] {
            RenderCustomTeleport();
        }));
        AddCategory(std::move(customteleport));

        auto animations = std::make_shared<Category>("Animations");
        animations->AddItem(std::make_shared<ImGuiItem>([] { YimMenu::Submenus::RenderAnimationsCategory(); }));
        AddCategory(std::move(animations));
    }
}
