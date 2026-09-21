#pragma once
#include "game/frontend/submenus/Player/AttackWaves.hpp"

#include <imgui.h>
#include <string>

namespace YimMenu::Features::NpcAttackOptions
{
    // settings shown in the "Npc Attack" section of the menu
    std::string& ConfiguredModel();
    int& ConfiguredCount();
    float& ConfiguredDistance();
    int& ConfiguredSideIndex();            // 0 = around the player, 1 = in front, 2 = behind
    AttackWaves::SpawnSide ConfiguredSide(); // resolved spawn side used by the feature

    // resolves a (partial) model name to a valid ped model, returns an empty string if nothing matches
    std::string ResolveModelName(const std::string& input);

    // ImGui completion callback for the model input field (Tab autocompletes)
    int ModelInputCallback(ImGuiInputTextCallbackData* data);
}
