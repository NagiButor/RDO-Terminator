#pragma once

#include <string>

#include "imgui.h"
#include "game/frontend/esp/EspWorld.hpp"

namespace YimMenu
{
    class ESP
    {
    public:
        static ImColor GetColorBasedOnDistance(float distance);
        static void DrawTextInfo(ImDrawList* drawList, const ImVec2& position, const std::string& text, ImColor color);
        static void DrawSkeleton(IEspActor& actor, const IEspWorld& world, ImDrawList* drawList, ImColor color);
        static void DrawHorseSkeleton(IEspActor& horse, const IEspWorld& world, ImDrawList* drawList, ImColor color);
        static void DrawPlayer(IEspActor& plyr, const IEspWorld& world, ImDrawList* drawList);
        static void DrawPeds(IEspActor& ped, const IEspWorld& world, ImDrawList* drawList);
        static void Draw(IEspWorld& world);
        static void Draw(); // production: draws from the live game world
    };
}
