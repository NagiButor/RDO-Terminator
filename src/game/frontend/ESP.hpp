#pragma once

#include <string>

#include "imgui.h"
#include "game/frontend/esp/EspWorld.hpp"

namespace YimMenu
{
    // Overlay building blocks: skeleton, corner/full box, snapline,
    // side HP bar and outlined mono tags. All draw through IEspWorld so the
    // same code renders live game objects and scripted test fakes.
    class ESP
    {
    public:
        static ImColor GetColorBasedOnDistance(float distance);
        static void DrawTextInfo(ImDrawList* drawList, const ImVec2& position, const std::string& text, ImColor color);
        static void DrawOutlinedText(ImDrawList* drawList, const ImVec2& position, const std::string& text, ImColor color);
        static void DrawCenteredText(ImDrawList* drawList, float centerX, float y, const std::string& text, ImColor color);
        static bool CalcBox(IEspActor& actor, const IEspWorld& world, ImVec2& outMin, ImVec2& outMax);
        static void DrawCornerBox(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, ImColor color, float thickness = 1.5f);
        static void DrawFullBox(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, ImColor color, float thickness = 1.5f);
        static void DrawSnapline(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, const ImVec2& screenSize, ImColor color);
        static void DrawSideHealthBar(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, float frac, float distance);
        static void DrawSkeleton(IEspActor& actor, const IEspWorld& world, ImDrawList* drawList, ImColor color);
        static void DrawHorseSkeleton(IEspActor& horse, const IEspWorld& world, ImDrawList* drawList, ImColor color);
        static void DrawPlayer(IEspActor& plyr, const IEspWorld& world, ImDrawList* drawList);
        static void DrawPeds(IEspActor& ped, const IEspWorld& world, ImDrawList* drawList);
        static void Draw(IEspWorld& world);
        static void Draw(); // production: draws from the live game world
    };
}
