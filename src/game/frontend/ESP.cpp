#include "ESP.hpp"
#include "game/frontend/esp/EspWorld.hpp"
#include "game/frontend/Menu.hpp"
#include "core/commands/BoolCommand.hpp"
#include "core/commands/ColorCommand.hpp"
#include "core/commands/IntCommand.hpp"
#include "core/commands/ListCommand.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
    // Human
    constexpr int headBone          = 21030;
    constexpr int neckBone          = 14283;
    constexpr int torsoBone         = 14410;
    constexpr int leftHandBone      = 34606;
    constexpr int rightHandBone     = 22798;
    constexpr int leftFootBone      = 45454;
    constexpr int rightFootBone     = 33646;
    constexpr int leftElbowBone     = 22711;
    constexpr int rightElbowBone    = 2992;
    constexpr int leftKneeBone      = 22173;
    constexpr int rightKneeBone     = 63133;
    constexpr int leftShoulderBone  = 44903;
    constexpr int rightShoulderBone = 4312;

    // Horse
    constexpr int horseHeadBone   = 21030;
    constexpr int horseNeckBone0  = 14283;
    constexpr int horseNeckBone1  = 14284;
    constexpr int horseNeckBone2  = 14285;
    constexpr int horseNeckBone3  = 14286;
    constexpr int horseNeckBone4  = 14287;
    constexpr int horseNeckBone5  = 14288;
    constexpr int horseTorsoBone  = 14410;
    constexpr int horsePelvisBone = 56200;
    constexpr int horseTailBone   = 30992;

    // Front left leg
    constexpr int horseFrontLeftUpperArmBone = 37873;
    constexpr int horseFrontLeftForearmBone  = 53675;
    constexpr int horseFrontLeftHandBone     = 34606;
    constexpr int horseFrontLeftFingerBone0  = 41403;
    constexpr int horseFrontLeftFingerBone1  = 41404;

    // Front right leg
    constexpr int horseFrontRightUpperArmBone = 46065;
    constexpr int horseFrontRightForearmBone  = 54187;
    constexpr int horseFrontRightHandBone     = 22798;
    constexpr int horseFrontRightFingerBone0  = 16827;
    constexpr int horseFrontRightFingerBone1  = 16828;

    // Rear left leg
    constexpr int horseRearLeftThighBone = 65478;
    constexpr int horseRearLeftCalfBone  = 55120;
    constexpr int horseRearLeftFootBone  = 45454;
    constexpr int horseRearLeftToeBone0  = 53081;
    constexpr int horseRearLeftToeBone1  = 53082;

    // Rear right leg
    constexpr int horseRearRightThighBone = 6884;
    constexpr int horseRearRightCalfBone  = 43312;
    constexpr int horseRearRightFootBone  = 33646;
    constexpr int horseRearRightToeBone0  = 41273;
    constexpr int horseRearRightToeBone1  = 41274;

    // text tags fade out beyond this, boxes/bars stay at minimum alpha
    constexpr float kTextFadeDistance = 150.f;
    constexpr float kBoxFadeDistance  = 300.f;
    constexpr float kHpNumberDistance = 60.f;
} // namespace

namespace YimMenu::Features
{
    // Players
    BoolCommand _ESPDrawPlayers("espdrawplayers", "Draw Players", "Should the ESP draw players?");
    BoolCommand _ESPDrawDeadPlayers("espdrawdeadplayers", "Draw Dead Players", "Should the ESP draw dead players?");
    BoolCommand _ESPName("espnameplayers", "Show Player Name", "Should the ESP draw player names?");
    BoolCommand _ESPHealth("esphealthplayers", "Show Player Health", "Should the ESP draw player HP bars?");
    BoolCommand _ESPDistance("espdistanceplayers", "Show Player Distance", "Should the ESP draw player distance?");
    BoolCommand _ESPSkeleton("espskeletonplayers", "Show Player Skeleton", "Should the ESP draw player skeletons?");
    IntCommand _ESPRenderDistancePlayers("esprenddistplayers", "Render Distance", "Max ESP render distance for players (m)", 100, 20000, 500);
    ListCommand _ESPBoxPlayers("espboxplayers", "Box Style", "Bounding box around players", {{0, "Off"}, {1, "Corners"}, {2, "Full"}}, 1);
    BoolCommand _ESPSnaplinePlayers("espsnaplineplayers", "Show Snapline", "Line from the bottom of the screen to the player");

    // Initialize colors with alpha set to 1.0f (fully opaque)
    ColorCommand _NameColorPlayers("namecolorplayers", "Player Name Color", "Changes the color of the name ESP for players", ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
    ColorCommand _DistanceColorPlayers("distancecolorplayers", "Player Distance Color", "Changes the color of the distance ESP for players", ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
    ColorCommand _SkeletonColorPlayers("skeletoncolorplayers", "Player Skeleton Color", "Changes the color of the skeleton ESP for players", ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });

    // Peds
    BoolCommand _ESPDrawPeds("espdrawpeds", "Draw Peds", "Should the ESP draw peds?");
    BoolCommand _ESPDrawDeadPeds("espdrawdeadpeds", "Draw Dead Peds", "Should the ESP draw dead peds?");
    BoolCommand _ESPModelPeds("espmodelspeds", "Show Ped Model", "Should the ESP draw ped models?");
    BoolCommand _ESPNetworkInfoPeds("espnetinfopeds", "Show Ped Network Info", "Should the ESP draw network info?");
    BoolCommand _ESPScriptInfoPeds("espscriptinfopeds", "Show Ped Script Info", "Should the ESP draw script info?");
    BoolCommand _ESPDistancePeds("espdistancepeds", "Show Ped Distance", "Should the ESP draw distance?");
    BoolCommand _ESPSkeletonPeds("espskeletonpeds", "Show Ped Skeleton", "Should the ESP draw the skeleton?");
    BoolCommand _ESPSkeletonHorse("espskeletonhorse", "Show Horse Skeleton", "Should the ESP draw horse skeletons?");
    IntCommand _ESPRenderDistancePeds("esprenddistpeds", "Render Distance", "Max ESP render distance for peds (m)", 100, 20000, 300);
    ListCommand _ESPBoxPeds("espboxpeds", "Box Style", "Bounding box around peds", {{0, "Off"}, {1, "Corners"}, {2, "Full"}}, 1);
    BoolCommand _ESPSnaplinePeds("espsnaplinepeds", "Show Snapline", "Line from the bottom of the screen to the ped");

    // Global: dim targets without line of sight (game only, throttled in the adapter)
    BoolCommand _ESPVisibilityCheck("espvisibilitycheck", "Visibility Check", "Dim targets behind walls", false);

    // Initialize colors with alpha set to 1.0f (fully opaque)
    ColorCommand _HashColorPeds("hashcolorpeds", "Ped Hash Color", "Changes the color of the hash ESP for peds", ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
    ColorCommand _DistanceColorPeds("distancecolorpeds", "Ped Distance Color", "Changes the color of the distance ESP for peds", ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
    ColorCommand _SkeletonColorPeds("skeletoncolorpeds", "Ped Skeleton Color", "Changes the color of the skeleton ESP for peds", ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
    ColorCommand _SkeletonColorHorse("skeletoncolorhorse", "Horse Skeleton Color", "Changes the color of the skeleton ESP for horses", ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
} // namespace YimMenu::Features

namespace YimMenu
{
    static ImColor Green  = ImColor(0.29f, 0.69f, 0.34f, 1.f);
    static ImColor Orange = ImColor(0.69f, 0.49f, 0.29f, 1.f);
    static ImColor Red    = ImColor(0.69f, 0.29f, 0.29f, 1.f);
    static ImColor Blue   = ImColor(0.36f, 0.71f, 0.89f, 1.f);

    static ImColor WithAlpha(ImColor color, float alpha)
    {
        ImVec4 v = color;
        v.w *= alpha;
        return ImColor(v);
    }

    ImColor ESP::GetColorBasedOnDistance(float distance)
    {
        if (distance < 100.f)
            return Green;
        else if (distance < 300.f)
            return Orange;
        else
            return Red;
    }

    void ESP::DrawTextInfo(ImDrawList* drawList, const ImVec2& position, const std::string& text, ImColor color)
    {
        DrawOutlinedText(drawList, position, text, color);
    }

    void ESP::DrawOutlinedText(ImDrawList* drawList, const ImVec2& position, const std::string& text, ImColor color)
    {
        const ImU32 outline = IM_COL32(0, 0, 0, (int)(color.Value.w * 230));
        drawList->AddText({position.x - 1.f, position.y}, outline, text.c_str());
        drawList->AddText({position.x + 1.f, position.y}, outline, text.c_str());
        drawList->AddText({position.x, position.y - 1.f}, outline, text.c_str());
        drawList->AddText({position.x, position.y + 1.f}, outline, text.c_str());
        drawList->AddText(position, color, text.c_str());
    }

    void ESP::DrawCenteredText(ImDrawList* drawList, float centerX, float y, const std::string& text, ImColor color)
    {
        const ImVec2 size = ImGui::CalcTextSize(text.c_str());
        DrawOutlinedText(drawList, {centerX - size.x / 2.f, y}, text, color);
    }

    bool ESP::CalcBox(IEspActor& actor, const IEspWorld& world, ImVec2& outMin, ImVec2& outMax)
    {
        const ImVec2 head = world.Project(actor.GetBone(headBone));
        if (head.x == 0)
            return false;

        const rage::fvector3 leftFoot  = actor.GetBone(leftFootBone);
        const rage::fvector3 rightFoot = actor.GetBone(rightFootBone);
        const rage::fvector3 midFeet{(leftFoot.x + rightFoot.x) / 2.f, (leftFoot.y + rightFoot.y) / 2.f, (leftFoot.z + rightFoot.z) / 2.f};
        const ImVec2 feet = world.Project(midFeet);
        if (feet.x == 0)
            return false;

        const float h = feet.y - head.y;
        if (h < 4.f)
            return false;

        // far away the box would shrink to a couple of pixels (and the old
        // floating HP bar fallback would kick in): clamp to a readable minimum
        const float clampedH = std::max(h, 12.f);
        const float w        = std::max(10.f, clampedH * 0.45f);
        const float cy       = head.y + h / 2.f;
        outMin               = {head.x - w / 2.f, cy - clampedH / 2.f};
        outMax               = {head.x + w / 2.f, cy + clampedH / 2.f};
        return true;
    }

    void ESP::DrawCornerBox(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, ImColor color, float thickness)
    {
        const float w   = boxMax.x - boxMin.x;
        const float h   = boxMax.y - boxMin.y;
        const float len = std::clamp(std::min(w, h) * 0.3f, 6.f, 40.f);

        // top-left
        drawList->AddLine(boxMin, {boxMin.x + len, boxMin.y}, color, thickness);
        drawList->AddLine(boxMin, {boxMin.x, boxMin.y + len}, color, thickness);
        // top-right
        drawList->AddLine({boxMax.x, boxMin.y}, {boxMax.x - len, boxMin.y}, color, thickness);
        drawList->AddLine({boxMax.x, boxMin.y}, {boxMax.x, boxMin.y + len}, color, thickness);
        // bottom-left
        drawList->AddLine({boxMin.x, boxMax.y}, {boxMin.x + len, boxMax.y}, color, thickness);
        drawList->AddLine({boxMin.x, boxMax.y}, {boxMin.x, boxMax.y - len}, color, thickness);
        // bottom-right
        drawList->AddLine(boxMax, {boxMax.x - len, boxMax.y}, color, thickness);
        drawList->AddLine(boxMax, {boxMax.x, boxMax.y - len}, color, thickness);
    }

    void ESP::DrawFullBox(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, ImColor color, float thickness)
    {
        drawList->AddRect(boxMin, boxMax, color, 0.f, 0, thickness);
    }

    void ESP::DrawSnapline(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, const ImVec2& screenSize, ImColor color)
    {
        drawList->AddLine({screenSize.x / 2.f, screenSize.y}, {(boxMin.x + boxMax.x) / 2.f, boxMax.y}, WithAlpha(color, 0.6f), 1.f);
    }

    void ESP::DrawSideHealthBar(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, float frac, float distance)
    {
        const float barW = 4.f;
        const float barX = boxMin.x - barW - 3.f;
        const ImVec2 bgMin{barX, boxMin.y};
        const ImVec2 bgMax{barX + barW, boxMax.y};
        drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180));
        ImColor bar_color = frac > 0.5f ? Green : (frac > 0.25f ? Orange : Red);
        const float h     = bgMax.y - bgMin.y;
        drawList->AddRectFilled({bgMin.x, bgMax.y - h * frac}, bgMax, bar_color);

        if (distance < kHpNumberDistance)
            DrawOutlinedText(drawList, {bgMin.x - 2.f, bgMax.y + 2.f}, std::to_string((int)(frac * 100.f)), IM_COL32(255, 255, 255, 255));
    }

    void ESP::DrawSkeleton(IEspActor& actor, const IEspWorld& world, ImDrawList* drawList, ImColor color)
    {
        auto headPos          = world.Project(actor.GetBone(headBone));
        auto neckPos          = world.Project(actor.GetBone(neckBone));
        auto torsoPos         = world.Project(actor.GetBone(torsoBone));
        auto leftShoulderPos  = world.Project(actor.GetBone(leftShoulderBone));
        auto rightShoulderPos = world.Project(actor.GetBone(rightShoulderBone));
        auto leftElbowPos     = world.Project(actor.GetBone(leftElbowBone));
        auto rightElbowPos    = world.Project(actor.GetBone(rightElbowBone));
        auto leftHandPos      = world.Project(actor.GetBone(leftHandBone));
        auto rightHandPos     = world.Project(actor.GetBone(rightHandBone));
        auto leftKneePos      = world.Project(actor.GetBone(leftKneeBone));
        auto rightKneePos     = world.Project(actor.GetBone(rightKneeBone));
        auto leftFootPos      = world.Project(actor.GetBone(leftFootBone));
        auto rightFootPos     = world.Project(actor.GetBone(rightFootBone));

        drawList->AddLine(headPos, neckPos, color, 1.5f);
        drawList->AddLine(neckPos, leftShoulderPos, color, 1.5f);
        drawList->AddLine(leftShoulderPos, leftElbowPos, color, 1.5f);
        drawList->AddLine(leftElbowPos, leftHandPos, color, 1.5f);
        drawList->AddLine(neckPos, rightShoulderPos, color, 1.5f);
        drawList->AddLine(rightShoulderPos, rightElbowPos, color, 1.5f);
        drawList->AddLine(rightElbowPos, rightHandPos, color, 1.5f);
        drawList->AddLine(neckPos, torsoPos, color, 1.5f);
        drawList->AddLine(torsoPos, leftKneePos, color, 1.5f);
        drawList->AddLine(leftKneePos, leftFootPos, color, 1.5f);
        drawList->AddLine(torsoPos, rightKneePos, color, 1.5f);
        drawList->AddLine(rightKneePos, rightFootPos, color, 1.5f);
    }

    void ESP::DrawHorseSkeleton(IEspActor& horse, const IEspWorld& world, ImDrawList* drawList, ImColor color)
    {
        // Head and neck
        auto headPos   = world.Project(horse.GetBone(horseHeadBone));
        auto neckPos0  = world.Project(horse.GetBone(horseNeckBone0));
        auto torsoPos  = world.Project(horse.GetBone(horseTorsoBone));
        auto pelvisPos = world.Project(horse.GetBone(horsePelvisBone));

        drawList->AddLine(headPos, neckPos0, color, 1.5f);
        drawList->AddLine(neckPos0, torsoPos, color, 1.5f);
        drawList->AddLine(torsoPos, pelvisPos, color, 1.5f);

        // Front left leg
        auto frontLeftUpperArmPos = world.Project(horse.GetBone(horseFrontLeftUpperArmBone));
        auto frontLeftForearmPos  = world.Project(horse.GetBone(horseFrontLeftForearmBone));
        auto frontLeftHandPos     = world.Project(horse.GetBone(horseFrontLeftHandBone));
        auto frontLeftFinger0Pos  = world.Project(horse.GetBone(horseFrontLeftFingerBone0));
        auto frontLeftFinger1Pos  = world.Project(horse.GetBone(horseFrontLeftFingerBone1));

        drawList->AddLine(neckPos0, frontLeftUpperArmPos, color, 1.5f);
        drawList->AddLine(frontLeftUpperArmPos, frontLeftForearmPos, color, 1.5f);
        drawList->AddLine(frontLeftForearmPos, frontLeftHandPos, color, 1.5f);
        drawList->AddLine(frontLeftHandPos, frontLeftFinger0Pos, color, 1.5f);
        drawList->AddLine(frontLeftFinger0Pos, frontLeftFinger1Pos, color, 1.5f);

        // Front right leg
        auto frontRightUpperArmPos = world.Project(horse.GetBone(horseFrontRightUpperArmBone));
        auto frontRightForearmPos  = world.Project(horse.GetBone(horseFrontRightForearmBone));
        auto frontRightHandPos     = world.Project(horse.GetBone(horseFrontRightHandBone));
        auto frontRightFinger0Pos  = world.Project(horse.GetBone(horseFrontRightFingerBone0));
        auto frontRightFinger1Pos  = world.Project(horse.GetBone(horseFrontRightFingerBone1));

        drawList->AddLine(neckPos0, frontRightUpperArmPos, color, 1.5f);
        drawList->AddLine(frontRightUpperArmPos, frontRightForearmPos, color, 1.5f);
        drawList->AddLine(frontRightForearmPos, frontRightHandPos, color, 1.5f);
        drawList->AddLine(frontRightHandPos, frontRightFinger0Pos, color, 1.5f);
        drawList->AddLine(frontRightFinger0Pos, frontRightFinger1Pos, color, 1.5f);

        // Rear left leg
        auto rearLeftThighPos = world.Project(horse.GetBone(horseRearLeftThighBone));
        auto rearLeftCalfPos  = world.Project(horse.GetBone(horseRearLeftCalfBone));
        auto rearLeftFootPos  = world.Project(horse.GetBone(horseRearLeftFootBone));
        auto rearLeftToe0Pos  = world.Project(horse.GetBone(horseRearLeftToeBone0));
        auto rearLeftToe1Pos  = world.Project(horse.GetBone(horseRearLeftToeBone1));

        drawList->AddLine(pelvisPos, rearLeftThighPos, color, 1.5f);
        drawList->AddLine(rearLeftThighPos, rearLeftCalfPos, color, 1.5f);
        drawList->AddLine(rearLeftCalfPos, rearLeftFootPos, color, 1.5f);
        drawList->AddLine(rearLeftFootPos, rearLeftToe0Pos, color, 1.5f);
        drawList->AddLine(rearLeftToe0Pos, rearLeftToe1Pos, color, 1.5f);

        // Rear right leg
        auto rearRightThighPos = world.Project(horse.GetBone(horseRearRightThighBone));
        auto rearRightCalfPos  = world.Project(horse.GetBone(horseRearRightCalfBone));
        auto rearRightFootPos  = world.Project(horse.GetBone(horseRearRightFootBone));
        auto rearRightToe0Pos  = world.Project(horse.GetBone(horseRearRightToeBone0));
        auto rearRightToe1Pos  = world.Project(horse.GetBone(horseRearRightToeBone1));

        drawList->AddLine(pelvisPos, rearRightThighPos, color, 1.5f);
        drawList->AddLine(rearRightThighPos, rearRightCalfPos, color, 1.5f);
        drawList->AddLine(rearRightCalfPos, rearRightFootPos, color, 1.5f);
        drawList->AddLine(rearRightFootPos, rearRightToe0Pos, color, 1.5f);
        drawList->AddLine(rearRightToe0Pos, rearRightToe1Pos, color, 1.5f);
    }

    void ESP::DrawPlayer(IEspActor& plyr, const IEspWorld& world, ImDrawList* drawList)
    {
        if (!plyr.IsValid() || plyr.IsSelf() || world.Project(plyr.GetBone(torsoBone)).x == 0 || (plyr.IsDead() && !Features::_ESPDrawDeadPlayers.GetState()))
            return;

        const float distanceToPlayer = plyr.GetDistance();
        if (distanceToPlayer > (float)Features::_ESPRenderDistancePlayers.GetState())
            return;
        const bool showText          = distanceToPlayer <= kTextFadeDistance;
        float fade                   = std::clamp(1.15f - distanceToPlayer / kBoxFadeDistance, 0.25f, 1.f);
        if (!plyr.IsVisible())
            fade *= 0.45f;

        ImColor baseColor = plyr.IsSelected() ? Blue : GetColorBasedOnDistance(distanceToPlayer);
        baseColor         = WithAlpha(baseColor, fade);

        if (Menu::Font::g_EspFont)
            ImGui::PushFont(Menu::Font::g_EspFont);

        ImVec2 boxMin{}, boxMax{};
        const bool hasBox = CalcBox(plyr, world, boxMin, boxMax);
        const int boxMode = Features::_ESPBoxPlayers.GetState();
        if (hasBox && boxMode == 1)
            DrawCornerBox(drawList, boxMin, boxMax, baseColor);
        else if (hasBox && boxMode == 2)
            DrawFullBox(drawList, boxMin, boxMax, baseColor);

        if (hasBox && Features::_ESPSnaplinePlayers.GetState() && plyr.IsVisible())
            DrawSnapline(drawList, boxMin, boxMax, world.GetScreenSize(), baseColor);

        const float centerX = hasBox ? (boxMin.x + boxMax.x) / 2.f : world.Project(plyr.GetBone(headBone)).x;
        if (Features::_ESPName.GetState() && showText)
            DrawCenteredText(drawList, centerX, boxMin.y - 13.f, plyr.GetName(),
                WithAlpha(plyr.IsSelected() ? Blue : ImColor(Features::_NameColorPlayers.GetState()), fade));

        if (Features::_ESPDistance.GetState() && showText)
            DrawCenteredText(drawList, centerX, boxMax.y + 3.f, std::to_string(static_cast<int>(distanceToPlayer)) + "m",
                WithAlpha(ImColor(Features::_DistanceColorPlayers.GetState()), fade));

        if (Features::_ESPHealth.GetState())
        {
            const float frac = plyr.GetMaxHealth() > 0 ? std::clamp((float)plyr.GetHealth() / (float)plyr.GetMaxHealth(), 0.f, 1.f) : 0.f;
            if (hasBox)
            {
                DrawSideHealthBar(drawList, boxMin, boxMax, frac, distanceToPlayer);
            }
            else if (Features::_ESPBoxPlayers.GetState() == 0)
            {
                // legacy horizontal bar, only when boxes are fully off
                auto headPos = world.Project(plyr.GetBone(headBone));
                const float bar_w = 60.f;
                const ImVec2 bar_min{headPos.x - bar_w / 2.f, headPos.y + 38};
                const ImVec2 bar_max{headPos.x + bar_w / 2.f, headPos.y + 43};
                drawList->AddRectFilled(bar_min, bar_max, IM_COL32(0, 0, 0, 160));
                ImColor bar_color = frac > 0.5f ? Green : (frac > 0.25f ? Orange : Red);
                drawList->AddRectFilled(bar_min, {bar_min.x + bar_w * frac, bar_max.y}, bar_color);
                if (showText)
                    DrawOutlinedText(drawList, {headPos.x - bar_w / 2.f, headPos.y + 45}, std::to_string(plyr.GetHealth()), IM_COL32(255, 255, 255, 255));
            }
        }

        if (Menu::Font::g_EspFont)
            ImGui::PopFont();

        if (Features::_ESPSkeleton.GetState() && !plyr.IsAnimal() && distanceToPlayer < 250.f)
        {
            DrawSkeleton(plyr, world, drawList, WithAlpha(ImGui::ColorConvertFloat4ToU32(Features::_SkeletonColorPlayers.GetState()), fade));
        }
    }

    void ESP::DrawPeds(IEspActor& ped, const IEspWorld& world, ImDrawList* drawList)
    {
        if (!ped.IsValid() || ped.IsSelf() || world.Project(ped.GetBone(torsoBone)).x == 0 || (ped.IsDead() && !Features::_ESPDrawDeadPeds.GetState()))
            return;

        const float distanceToPed = ped.GetDistance();
        if (distanceToPed > (float)Features::_ESPRenderDistancePeds.GetState())
            return;
        const bool showText       = distanceToPed <= kTextFadeDistance;
        float fade                = std::clamp(1.15f - distanceToPed / kBoxFadeDistance, 0.25f, 1.f);
        if (!ped.IsVisible())
            fade *= 0.45f;

        ImColor baseColor = WithAlpha(GetColorBasedOnDistance(distanceToPed), fade);

        if (Menu::Font::g_EspFont)
            ImGui::PushFont(Menu::Font::g_EspFont);

        ImVec2 boxMin{}, boxMax{};
        const bool hasBox = CalcBox(ped, world, boxMin, boxMax);
        const int boxMode = Features::_ESPBoxPeds.GetState();
        if (hasBox && boxMode == 1)
            DrawCornerBox(drawList, boxMin, boxMax, baseColor);
        else if (hasBox && boxMode == 2)
            DrawFullBox(drawList, boxMin, boxMax, baseColor);

        if (hasBox && Features::_ESPSnaplinePeds.GetState() && ped.IsVisible())
            DrawSnapline(drawList, boxMin, boxMax, world.GetScreenSize(), baseColor);

        const float centerX = hasBox ? (boxMin.x + boxMax.x) / 2.f : world.Project(ped.GetBone(headBone)).x;
        const std::string info = ped.GetInfo();
        if (!info.empty() && showText)
            DrawCenteredText(drawList, centerX, boxMin.y - 13.f,
                info, WithAlpha(ImGui::ColorConvertFloat4ToU32(Features::_HashColorPeds.GetState()), fade));

        if (Features::_ESPDistancePeds.GetState() && showText)
            DrawCenteredText(drawList, centerX, boxMax.y + 3.f, std::to_string(static_cast<int>(distanceToPed)) + "m",
                WithAlpha(ImColor(Features::_DistanceColorPeds.GetState()), fade));

        if (Menu::Font::g_EspFont)
            ImGui::PopFont();

        if (Features::_ESPSkeletonPeds.GetState() && !ped.IsAnimal() && distanceToPed < 250.f)
        {
            DrawSkeleton(ped, world, drawList, WithAlpha(ImGui::ColorConvertFloat4ToU32(Features::_SkeletonColorPeds.GetState()), fade));
        }

        if (Features::_ESPSkeletonHorse.GetState() && ped.IsHorse() && distanceToPed < 250.f)
        {
            DrawHorseSkeleton(ped, world, drawList, WithAlpha(ImGui::ColorConvertFloat4ToU32(Features::_SkeletonColorHorse.GetState()), fade));
        }
    }

    void ESP::Draw(IEspWorld& world)
    {
        if (!world.IsAvailable())
            return;

        if (const auto drawList = ImGui::GetBackgroundDrawList())
        {
            if (Features::_ESPDrawPlayers.GetState())
            {
                world.ForEachPlayer([&](IEspActor& player) {
                    DrawPlayer(player, world, drawList);
                });
            }
            if (Features::_ESPDrawPeds.GetState())
            {
                world.ForEachPed([&](IEspActor& ped) {
                    DrawPeds(ped, world, drawList);
                });
            }
        }
    }
} // namespace YimMenu
