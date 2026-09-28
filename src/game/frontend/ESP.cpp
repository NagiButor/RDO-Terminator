#include "ESP.hpp"
#include "game/frontend/esp/EspWorld.hpp"
#include "game/frontend/Menu.hpp"
#include "game/frontend/Theme.hpp"
#include "core/commands/BoolCommand.hpp"
#include "core/commands/ColorCommand.hpp"
#include "core/commands/IntCommand.hpp"
#include "core/commands/ListCommand.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

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
    constexpr int horseTorsoBone  = 14410;
    constexpr int horsePelvisBone = 56200;

    constexpr int horseFrontLeftUpperArmBone = 37873;
    constexpr int horseFrontLeftForearmBone  = 53675;
    constexpr int horseFrontLeftHandBone     = 34606;
    constexpr int horseFrontLeftFingerBone0  = 41403;
    constexpr int horseFrontLeftFingerBone1  = 41404;

    constexpr int horseFrontRightUpperArmBone = 46065;
    constexpr int horseFrontRightForearmBone  = 54187;
    constexpr int horseFrontRightHandBone     = 22798;
    constexpr int horseFrontRightFingerBone0  = 16827;
    constexpr int horseFrontRightFingerBone1  = 16828;

    constexpr int horseRearLeftThighBone = 65478;
    constexpr int horseRearLeftCalfBone  = 55120;
    constexpr int horseRearLeftFootBone  = 45454;
    constexpr int horseRearLeftToeBone0  = 53081;
    constexpr int horseRearLeftToeBone1  = 53082;

    constexpr int horseRearRightThighBone = 6884;
    constexpr int horseRearRightCalfBone  = 43312;
    constexpr int horseRearRightFootBone  = 33646;
    constexpr int horseRearRightToeBone0  = 41273;
    constexpr int horseRearRightToeBone1  = 41274;

    // text tags fade out beyond this, boxes/bars stay at minimum alpha
    constexpr float kTextFadeDistance = 150.f;
    constexpr float kBoxFadeDistance  = 300.f;
    constexpr float kHpNumberDistance = 60.f;
    constexpr float kSkeletonDistance = 250.f;
} // namespace

namespace YimMenu::Features
{
    static const std::vector<std::pair<int, const char*>> kBoxStyles  = {{0, "Off"}, {1, "Corners"}, {2, "Full"}, {3, "Filled"}};
    static const std::vector<std::pair<int, const char*>> kColorModes = {{0, "Distance"}, {1, "Health"}, {2, "Custom"}};

    // Players
    BoolCommand _ESPDrawPlayers("espdrawplayers", "Draw Players", "Should the ESP draw players?");
    BoolCommand _ESPDrawDeadPlayers("espdrawdeadplayers", "Draw Dead Players", "Should the ESP draw dead players?");
    BoolCommand _ESPName("espnameplayers", "Show Player Name", "Should the ESP draw player names?");
    BoolCommand _ESPHealth("esphealthplayers", "Show Player Health", "Should the ESP draw player HP bars?");
    BoolCommand _ESPDistance("espdistanceplayers", "Show Player Distance", "Should the ESP draw player distance?");
    BoolCommand _ESPSkeleton("espskeletonplayers", "Show Player Skeleton", "Should the ESP draw player skeletons?");
    IntCommand _ESPRenderDistancePlayers("esprenddistplayers", "Render Distance", "Max ESP render distance for players (m)", 100, 20000, 500);
    ListCommand _ESPBoxPlayers("espboxplayers", "Box Style", "Bounding box around players", kBoxStyles, 1);
    ListCommand _ESPColorModePlayers("espcolormodeplayers", "Color By", "What decides the box / snapline color", kColorModes, 0);
    BoolCommand _ESPSnaplinePlayers("espsnaplineplayers", "Show Snapline", "Line from the screen edge to the player");

    ColorCommand _NameColorPlayers("namecolorplayers", "Player Name Color", "Changes the color of the name ESP for players", ImVec4{1.0f, 1.0f, 1.0f, 1.0f});
    ColorCommand _DistanceColorPlayers("distancecolorplayers", "Player Distance Color", "Changes the color of the distance ESP for players", ImVec4{1.0f, 1.0f, 1.0f, 1.0f});
    ColorCommand _SkeletonColorPlayers("skeletoncolorplayers", "Player Skeleton Color", "Changes the color of the skeleton ESP for players", ImVec4{1.0f, 1.0f, 1.0f, 1.0f});
    ColorCommand _BoxColorPlayers("boxcolorplayers", "Player Box Color", "Box color when Color By is Custom", ImVec4{0.92f, 0.10f, 0.13f, 1.0f});

    // Peds
    BoolCommand _ESPDrawPeds("espdrawpeds", "Draw Peds", "Should the ESP draw peds?");
    BoolCommand _ESPDrawDeadPeds("espdrawdeadpeds", "Draw Dead Peds", "Should the ESP draw dead peds?");
    BoolCommand _ESPAnimalsPeds("espanimalspeds", "Include Animals", "Draw animals and horses too", true);
    BoolCommand _ESPModelPeds("espmodelspeds", "Show Ped Model", "Should the ESP draw ped models?");
    BoolCommand _ESPNetworkInfoPeds("espnetinfopeds", "Show Ped Network Info", "Should the ESP draw network info?");
    BoolCommand _ESPScriptInfoPeds("espscriptinfopeds", "Show Ped Script Info", "Should the ESP draw script info?");
    BoolCommand _ESPHealthPeds("esphealthpeds", "Show Ped Health", "Should the ESP draw ped HP bars?");
    BoolCommand _ESPDistancePeds("espdistancepeds", "Show Ped Distance", "Should the ESP draw distance?");
    BoolCommand _ESPSkeletonPeds("espskeletonpeds", "Show Ped Skeleton", "Should the ESP draw the skeleton?");
    BoolCommand _ESPSkeletonHorse("espskeletonhorse", "Show Horse Skeleton", "Should the ESP draw horse skeletons?");
    IntCommand _ESPRenderDistancePeds("esprenddistpeds", "Render Distance", "Max ESP render distance for peds (m)", 100, 20000, 300);
    ListCommand _ESPBoxPeds("espboxpeds", "Box Style", "Bounding box around peds", kBoxStyles, 1);
    ListCommand _ESPColorModePeds("espcolormodepeds", "Color By", "What decides the box / snapline color", kColorModes, 0);
    BoolCommand _ESPSnaplinePeds("espsnaplinepeds", "Show Snapline", "Line from the screen edge to the ped");

    ColorCommand _HashColorPeds("hashcolorpeds", "Ped Hash Color", "Changes the color of the hash ESP for peds", ImVec4{1.0f, 1.0f, 1.0f, 1.0f});
    ColorCommand _DistanceColorPeds("distancecolorpeds", "Ped Distance Color", "Changes the color of the distance ESP for peds", ImVec4{1.0f, 1.0f, 1.0f, 1.0f});
    ColorCommand _SkeletonColorPeds("skeletoncolorpeds", "Ped Skeleton Color", "Changes the color of the skeleton ESP for peds", ImVec4{1.0f, 1.0f, 1.0f, 1.0f});
    ColorCommand _SkeletonColorHorse("skeletoncolorhorse", "Horse Skeleton Color", "Changes the color of the skeleton ESP for horses", ImVec4{1.0f, 1.0f, 1.0f, 1.0f});
    ColorCommand _BoxColorPeds("boxcolorpeds", "Ped Box Color", "Box color when Color By is Custom", ImVec4{0.95f, 0.75f, 0.25f, 1.0f});

    // Style (shared by players and peds)
    BoolCommand _ESPVisibilityCheck("espvisibilitycheck", "Visibility Check", "Dim targets behind walls", false);
    BoolCommand _ESPOutline("espoutline", "Outlines", "Dark outline under boxes, bars and skeletons for contrast", true);
    BoolCommand _ESPTagBackground("esptagbg", "Tag Background", "Draw names / distance on a translucent plate", true);
    BoolCommand _ESPSkeletonHead("espskeletonhead", "Skeleton Head Circle", "Draw a circle for the head on skeletons", true);
    BoolCommand _ESPDistanceFade("espdistancefade", "Fade With Distance", "Far targets are drawn more transparent", true);
    ListCommand _ESPSnaplineOrigin("espsnaporigin", "Snapline Origin", "Where snaplines start on the screen", {{0, "Bottom"}, {1, "Center"}, {2, "Top"}}, 0);
    ListCommand _ESPFontStyle("espfontstyle", "Tag Font", "Font used for ESP tags", {{0, "Clean"}, {1, "Pixel"}}, 0);
} // namespace YimMenu::Features

namespace YimMenu
{
    static const ImColor Green  = ImColor(0.30f, 0.86f, 0.46f, 1.f);
    static const ImColor Yellow = ImColor(0.98f, 0.80f, 0.26f, 1.f);
    static const ImColor Orange = ImColor(0.98f, 0.56f, 0.22f, 1.f);
    static const ImColor Red    = ImColor(0.95f, 0.28f, 0.30f, 1.f);
    static const ImColor Blue   = ImColor(0.36f, 0.71f, 1.00f, 1.f);
    static const ImColor Gray   = ImColor(0.62f, 0.63f, 0.66f, 1.f);

    static ImColor WithAlpha(ImColor color, float alpha)
    {
        ImVec4 v = color;
        v.w *= alpha;
        return ImColor(v);
    }

    static ImColor Lerp(ImColor a, ImColor b, float t)
    {
        const ImVec4 x = a, y = b;
        return ImColor(x.x + (y.x - x.x) * t, x.y + (y.y - x.y) * t, x.z + (y.z - x.z) * t, x.w + (y.w - x.w) * t);
    }

    static ImU32 Shadow(float alpha)
    {
        return IM_COL32(0, 0, 0, (int)(std::clamp(alpha, 0.f, 1.f) * 200.f));
    }

    // IEspWorld contract: x == 0 means the point could not be projected
    static bool Projected(const ImVec2& p)
    {
        return p.x != 0.f;
    }

    static ImColor HealthColor(float frac)
    {
        frac = std::clamp(frac, 0.f, 1.f);
        return frac < 0.5f ? Lerp(Red, Yellow, frac * 2.f) : Lerp(Yellow, Green, (frac - 0.5f) * 2.f);
    }

    static float HealthFrac(const IEspActor& actor)
    {
        return actor.GetMaxHealth() > 0 ? std::clamp((float)actor.GetHealth() / (float)actor.GetMaxHealth(), 0.f, 1.f) : 0.f;
    }

    static std::string FormatDistance(float meters)
    {
        if (meters >= 1000.f)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f km", meters / 1000.f);
            return buf;
        }
        return std::to_string((int)meters) + " m";
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

    float ESP::DrawTag(ImDrawList* drawList, float centerX, float y, const std::string& text, ImColor color, bool above, ImColor accent)
    {
        if (text.empty())
            return 0.f;

        const ImVec2 size = ImGui::CalcTextSize(text.c_str());
        const float padX  = 5.f;
        const float padY  = 2.f;
        const float h     = size.y + padY * 2.f;
        const float top   = above ? y - h : y;
        const ImVec2 textPos{std::floor(centerX - size.x / 2.f), std::floor(top + padY)};
        const float alpha = color.Value.w;

        if (Features::_ESPTagBackground.GetState())
        {
            const ImVec2 bgMin{textPos.x - padX, top};
            const ImVec2 bgMax{textPos.x + size.x + padX, top + h};
            drawList->AddRectFilled(bgMin, bgMax, IM_COL32(8, 9, 13, (int)(alpha * 175.f)), 3.f);
            if (accent.Value.w > 0.f)
                drawList->AddRectFilled(bgMin, {bgMin.x + 2.f, bgMax.y}, accent, 3.f, ImDrawFlags_RoundCornersLeft);
            drawList->AddText(textPos, color, text.c_str());
        }
        else
        {
            DrawOutlinedText(drawList, textPos, text, color);
        }
        return h + 2.f;
    }

    bool ESP::CalcBox(IEspActor& actor, const IEspWorld& world, ImVec2& outMin, ImVec2& outMax)
    {
        const ImVec2 head = world.Project(actor.GetBone(headBone));
        if (!Projected(head))
            return false;

        const rage::fvector3 leftFoot  = actor.GetBone(leftFootBone);
        const rage::fvector3 rightFoot = actor.GetBone(rightFootBone);
        const rage::fvector3 midFeet{(leftFoot.x + rightFoot.x) / 2.f, (leftFoot.y + rightFoot.y) / 2.f, (leftFoot.z + rightFoot.z) / 2.f};
        const ImVec2 feet = world.Project(midFeet);
        if (!Projected(feet))
            return false;

        // head bone sits at eye level: pad the top so the box covers the skull
        const float h = feet.y - head.y;
        if (h < 4.f)
            return false;

        // far away the box would shrink to a couple of pixels: clamp to a readable minimum
        const float fullH    = h * 1.12f;
        const float clampedH = std::max(fullH, 12.f);
        const float w        = std::max(10.f, clampedH * 0.42f);
        const float cy       = feet.y - fullH / 2.f;
        const float cx       = (head.x + feet.x) / 2.f;
        outMin               = {std::floor(cx - w / 2.f), std::floor(cy - clampedH / 2.f)};
        outMax               = {std::floor(cx + w / 2.f), std::floor(cy + clampedH / 2.f)};
        return true;
    }

    void ESP::DrawCornerBox(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, ImColor color, float thickness)
    {
        const float w   = boxMax.x - boxMin.x;
        const float h   = boxMax.y - boxMin.y;
        const float len = std::clamp(std::min(w, h) * 0.28f, 5.f, 40.f);

        auto corners = [&](ImU32 col, float t) {
            drawList->AddLine(boxMin, {boxMin.x + len, boxMin.y}, col, t);
            drawList->AddLine(boxMin, {boxMin.x, boxMin.y + len}, col, t);
            drawList->AddLine({boxMax.x, boxMin.y}, {boxMax.x - len, boxMin.y}, col, t);
            drawList->AddLine({boxMax.x, boxMin.y}, {boxMax.x, boxMin.y + len}, col, t);
            drawList->AddLine({boxMin.x, boxMax.y}, {boxMin.x + len, boxMax.y}, col, t);
            drawList->AddLine({boxMin.x, boxMax.y}, {boxMin.x, boxMax.y - len}, col, t);
            drawList->AddLine(boxMax, {boxMax.x - len, boxMax.y}, col, t);
            drawList->AddLine(boxMax, {boxMax.x, boxMax.y - len}, col, t);
        };

        if (Features::_ESPOutline.GetState())
            corners(Shadow(color.Value.w), thickness + 2.f);
        corners(color, thickness);
    }

    void ESP::DrawFullBox(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, ImColor color, float thickness)
    {
        if (Features::_ESPOutline.GetState())
            drawList->AddRect(boxMin, boxMax, Shadow(color.Value.w), 2.f, 0, thickness + 2.f);
        drawList->AddRect(boxMin, boxMax, color, 2.f, 0, thickness);
    }

    void ESP::DrawFilledBox(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, ImColor color, float thickness)
    {
        // soft vertical gradient: nearly clear at the head, denser at the feet
        const ImU32 top    = WithAlpha(color, 0.04f);
        const ImU32 bottom = WithAlpha(color, 0.22f);
        drawList->AddRectFilledMultiColor(boxMin, boxMax, top, top, bottom, bottom);
        DrawFullBox(drawList, boxMin, boxMax, color, thickness);
    }

    void ESP::DrawSnapline(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, const ImVec2& screenSize, ImColor color)
    {
        const int origin = Features::_ESPSnaplineOrigin.GetState();
        ImVec2 from{screenSize.x / 2.f, screenSize.y};
        ImVec2 to{(boxMin.x + boxMax.x) / 2.f, boxMax.y};
        if (origin == 1)
        {
            from.y = screenSize.y / 2.f;
            to.y   = (boxMin.y + boxMax.y) / 2.f;
        }
        else if (origin == 2)
        {
            from.y = 0.f;
            to.y   = boxMin.y;
        }

        if (Features::_ESPOutline.GetState())
            drawList->AddLine(from, to, Shadow(color.Value.w * 0.5f), 3.f);
        drawList->AddLine(from, to, WithAlpha(color, 0.75f), 1.f);
    }

    void ESP::DrawSideHealthBar(ImDrawList* drawList, const ImVec2& boxMin, const ImVec2& boxMax, float frac, float distance, float alpha)
    {
        frac = std::clamp(frac, 0.f, 1.f);

        const float barW = 3.f;
        const float barX = boxMin.x - barW - 4.f;
        const ImVec2 bgMin{barX, boxMin.y};
        const ImVec2 bgMax{barX + barW, boxMax.y};
        const float h = bgMax.y - bgMin.y;

        drawList->AddRectFilled({bgMin.x - 1.f, bgMin.y - 1.f}, {bgMax.x + 1.f, bgMax.y + 1.f}, IM_COL32(0, 0, 0, (int)(alpha * 190.f)));
        if (frac > 0.f)
        {
            const ImColor col  = WithAlpha(HealthColor(frac), alpha);
            const ImColor dark = WithAlpha(Lerp(HealthColor(frac), ImColor(0.f, 0.f, 0.f, 1.f), 0.35f), alpha);
            const float fillTop = bgMax.y - h * frac;
            drawList->AddRectFilledMultiColor({bgMin.x, fillTop}, bgMax, col, col, dark, dark);
        }

        // quarter ticks, only when the bar is tall enough to read them
        if (h > 40.f)
            for (int i = 1; i < 4; i++)
            {
                const float ty = std::floor(bgMin.y + h * (float)i / 4.f);
                drawList->AddLine({bgMin.x, ty}, {bgMax.x, ty}, IM_COL32(0, 0, 0, (int)(alpha * 150.f)), 1.f);
            }

        if (distance < kHpNumberDistance && frac < 0.995f)
        {
            const std::string hp = std::to_string((int)std::round(frac * 100.f));
            const ImVec2 ts      = ImGui::CalcTextSize(hp.c_str());
            const float ty       = std::clamp(bgMax.y - h * frac - ts.y / 2.f, bgMin.y - ts.y / 2.f, bgMax.y - ts.y / 2.f);
            DrawOutlinedText(drawList, {std::floor(bgMin.x - ts.x - 3.f), std::floor(ty)}, hp, WithAlpha(ImColor(1.f, 1.f, 1.f, 1.f), alpha));
        }
    }

    static void Bone(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 col, ImU32 shadow, bool outline)
    {
        // an unprojectable bone would draw a line to the screen corner
        if (!Projected(a) || !Projected(b))
            return;
        if (outline)
            dl->AddLine(a, b, shadow, 3.5f);
        dl->AddLine(a, b, col, 1.5f);
    }

    void ESP::DrawSkeleton(IEspActor& actor, const IEspWorld& world, ImDrawList* drawList, ImColor color)
    {
        const auto P = [&](int bone) {
            return world.Project(actor.GetBone(bone));
        };
        const ImVec2 head = P(headBone), neck = P(neckBone), torso = P(torsoBone);
        const ImVec2 lSh = P(leftShoulderBone), rSh = P(rightShoulderBone);
        const ImVec2 lEl = P(leftElbowBone), rEl = P(rightElbowBone);
        const ImVec2 lHa = P(leftHandBone), rHa = P(rightHandBone);
        const ImVec2 lKn = P(leftKneeBone), rKn = P(rightKneeBone);
        const ImVec2 lFt = P(leftFootBone), rFt = P(rightFootBone);

        const bool outline = Features::_ESPOutline.GetState();
        const ImU32 shadow = Shadow(color.Value.w * 0.8f);

        Bone(drawList, neck, lSh, color, shadow, outline);
        Bone(drawList, lSh, lEl, color, shadow, outline);
        Bone(drawList, lEl, lHa, color, shadow, outline);
        Bone(drawList, neck, rSh, color, shadow, outline);
        Bone(drawList, rSh, rEl, color, shadow, outline);
        Bone(drawList, rEl, rHa, color, shadow, outline);
        Bone(drawList, neck, torso, color, shadow, outline);
        Bone(drawList, torso, lKn, color, shadow, outline);
        Bone(drawList, lKn, lFt, color, shadow, outline);
        Bone(drawList, torso, rKn, color, shadow, outline);
        Bone(drawList, rKn, rFt, color, shadow, outline);

        if (Features::_ESPSkeletonHead.GetState() && Projected(head) && Projected(neck))
        {
            const float dx = head.x - neck.x, dy = head.y - neck.y;
            const float r  = std::clamp(std::sqrt(dx * dx + dy * dy) * 0.75f, 2.f, 60.f);
            if (outline)
                drawList->AddCircle(head, r, shadow, 20, 3.5f);
            drawList->AddCircle(head, r, color, 20, 1.5f);
            // neck line stops at the circle instead of crossing the face
            const float len = std::sqrt(dx * dx + dy * dy);
            if (len > r)
                Bone(drawList, {head.x - dx / len * r, head.y - dy / len * r}, neck, color, shadow, outline);
        }
        else
        {
            Bone(drawList, head, neck, color, shadow, outline);
        }
    }

    void ESP::DrawHorseSkeleton(IEspActor& horse, const IEspWorld& world, ImDrawList* drawList, ImColor color)
    {
        const auto P = [&](int bone) {
            return world.Project(horse.GetBone(bone));
        };
        const bool outline = Features::_ESPOutline.GetState();
        const ImU32 shadow = Shadow(color.Value.w * 0.8f);
        const auto chain   = [&](std::initializer_list<ImVec2> pts) {
            const ImVec2* prev = nullptr;
            for (const ImVec2& p : pts)
            {
                if (prev)
                    Bone(drawList, *prev, p, color, shadow, outline);
                prev = &p;
            }
        };

        const ImVec2 neck   = P(horseNeckBone0);
        const ImVec2 pelvis = P(horsePelvisBone);
        chain({P(horseHeadBone), neck, P(horseTorsoBone), pelvis});
        chain({neck, P(horseFrontLeftUpperArmBone), P(horseFrontLeftForearmBone), P(horseFrontLeftHandBone), P(horseFrontLeftFingerBone0), P(horseFrontLeftFingerBone1)});
        chain({neck, P(horseFrontRightUpperArmBone), P(horseFrontRightForearmBone), P(horseFrontRightHandBone), P(horseFrontRightFingerBone0), P(horseFrontRightFingerBone1)});
        chain({pelvis, P(horseRearLeftThighBone), P(horseRearLeftCalfBone), P(horseRearLeftFootBone), P(horseRearLeftToeBone0), P(horseRearLeftToeBone1)});
        chain({pelvis, P(horseRearRightThighBone), P(horseRearRightCalfBone), P(horseRearRightFootBone), P(horseRearRightToeBone0), P(horseRearRightToeBone1)});
    }

    // Everything the shared actor renderer needs to know about one ESP flavor.
    struct ESP::ActorStyle
    {
        bool m_DrawDead;
        int m_RenderDistance;
        int m_BoxMode;
        int m_ColorMode;
        ImColor m_CustomColor;
        bool m_Snapline;
        bool m_Health;
        bool m_Distance;
        ImColor m_DistanceColor;
        bool m_Skeleton;
        ImColor m_SkeletonColor;
        bool m_HorseSkeleton;
        ImColor m_HorseSkeletonColor;
        std::string m_Title; // name (players) or info lines (peds), '\n' separated
        ImColor m_TitleColor;
    };

    void ESP::DrawActor(IEspActor& actor, const IEspWorld& world, ImDrawList* drawList, const ActorStyle& style)
    {
        if (!actor.IsValid() || actor.IsSelf() || (actor.IsDead() && !style.m_DrawDead))
            return;
        if (!Projected(world.Project(actor.GetBone(torsoBone))))
            return;

        const float distance = actor.GetDistance();
        if (distance > (float)style.m_RenderDistance)
            return;

        const bool dead     = actor.IsDead();
        const bool showText = distance <= kTextFadeDistance;
        float fade          = Features::_ESPDistanceFade.GetState() ? std::clamp(1.15f - distance / kBoxFadeDistance, 0.3f, 1.f) : 1.f;
        if (!actor.IsVisible())
            fade *= 0.45f;
        const float frac = HealthFrac(actor);

        ImColor baseColor;
        if (dead)
            baseColor = Gray;
        else if (actor.IsSelected())
            baseColor = Blue;
        else if (style.m_ColorMode == 1)
            baseColor = HealthColor(frac);
        else if (style.m_ColorMode == 2)
            baseColor = style.m_CustomColor;
        else
            baseColor = GetColorBasedOnDistance(distance);
        baseColor = WithAlpha(baseColor, fade);

        // skeleton first so boxes, bars and tags stay readable on top of it
        if (!actor.IsAnimal() && style.m_Skeleton && distance < kSkeletonDistance)
            DrawSkeleton(actor, world, drawList, WithAlpha(style.m_SkeletonColor, fade));
        if (actor.IsHorse() && style.m_HorseSkeleton && distance < kSkeletonDistance)
            DrawHorseSkeleton(actor, world, drawList, WithAlpha(style.m_HorseSkeletonColor, fade));

        ImVec2 boxMin{}, boxMax{};
        bool hasBox = CalcBox(actor, world, boxMin, boxMax);
        if (!hasBox)
        {
            // animals / odd poses: fall back to a small anchor box around the torso
            const ImVec2 t = world.Project(actor.GetBone(torsoBone));
            boxMin         = {t.x - 8.f, t.y - 12.f};
            boxMax         = {t.x + 8.f, t.y + 12.f};
        }

        if (hasBox)
        {
            if (style.m_BoxMode == 1)
                DrawCornerBox(drawList, boxMin, boxMax, baseColor);
            else if (style.m_BoxMode == 2)
                DrawFullBox(drawList, boxMin, boxMax, baseColor);
            else if (style.m_BoxMode == 3)
                DrawFilledBox(drawList, boxMin, boxMax, baseColor);

            if (style.m_Snapline && actor.IsVisible())
                DrawSnapline(drawList, boxMin, boxMax, world.GetScreenSize(), baseColor);
        }

        ImFont* font = Features::_ESPFontStyle.GetState() == 1 ? Menu::Font::g_EspFont : (Theme::g_SmallFont ? Theme::g_SmallFont : Menu::Font::g_EspFont);
        if (font)
            ImGui::PushFont(font);

        if (style.m_Health && !dead)
            DrawSideHealthBar(drawList, boxMin, boxMax, frac, distance, fade);

        const float centerX = (boxMin.x + boxMax.x) / 2.f;
        if (showText && !style.m_Title.empty())
        {
            // stack lines upwards from the box top, last line closest to the box
            std::vector<std::string> lines;
            size_t start = 0;
            while (start <= style.m_Title.size())
            {
                const size_t end = style.m_Title.find('\n', start);
                std::string line = style.m_Title.substr(start, end == std::string::npos ? std::string::npos : end - start);
                while (!line.empty() && line.back() == ' ')
                    line.pop_back();
                if (!line.empty())
                    lines.push_back(std::move(line));
                if (end == std::string::npos)
                    break;
                start = end + 1;
            }

            float y = boxMin.y - 3.f;
            for (auto it = lines.rbegin(); it != lines.rend(); ++it)
                y -= DrawTag(drawList, centerX, y, *it, WithAlpha(style.m_TitleColor, fade), true, it == lines.rbegin() ? baseColor : ImColor(0, 0, 0, 0));
        }

        float belowY = boxMax.y + 3.f;
        if (dead && showText)
            belowY += DrawTag(drawList, centerX, belowY, "DEAD", WithAlpha(Gray, fade), false);
        if (style.m_Distance && showText)
            DrawTag(drawList, centerX, belowY, FormatDistance(distance), WithAlpha(style.m_DistanceColor, fade), false);

        if (font)
            ImGui::PopFont();
    }

    void ESP::DrawPlayer(IEspActor& plyr, const IEspWorld& world, ImDrawList* drawList)
    {
        ActorStyle style{};
        style.m_DrawDead       = Features::_ESPDrawDeadPlayers.GetState();
        style.m_RenderDistance = Features::_ESPRenderDistancePlayers.GetState();
        style.m_BoxMode        = Features::_ESPBoxPlayers.GetState();
        style.m_ColorMode      = Features::_ESPColorModePlayers.GetState();
        style.m_CustomColor    = ImColor(Features::_BoxColorPlayers.GetState());
        style.m_Snapline       = Features::_ESPSnaplinePlayers.GetState();
        style.m_Health         = Features::_ESPHealth.GetState();
        style.m_Distance       = Features::_ESPDistance.GetState();
        style.m_DistanceColor  = ImColor(Features::_DistanceColorPlayers.GetState());
        style.m_Skeleton       = Features::_ESPSkeleton.GetState();
        style.m_SkeletonColor  = ImColor(Features::_SkeletonColorPlayers.GetState());
        style.m_HorseSkeleton  = false;
        if (Features::_ESPName.GetState())
            style.m_Title = plyr.GetName();
        style.m_TitleColor = plyr.IsSelected() ? Blue : ImColor(Features::_NameColorPlayers.GetState());
        DrawActor(plyr, world, drawList, style);
    }

    void ESP::DrawPeds(IEspActor& ped, const IEspWorld& world, ImDrawList* drawList)
    {
        if (!Features::_ESPAnimalsPeds.GetState() && ped.IsAnimal())
            return;

        ActorStyle style{};
        style.m_DrawDead           = Features::_ESPDrawDeadPeds.GetState();
        style.m_RenderDistance     = Features::_ESPRenderDistancePeds.GetState();
        style.m_BoxMode            = Features::_ESPBoxPeds.GetState();
        style.m_ColorMode          = Features::_ESPColorModePeds.GetState();
        style.m_CustomColor        = ImColor(Features::_BoxColorPeds.GetState());
        style.m_Snapline           = Features::_ESPSnaplinePeds.GetState();
        style.m_Health             = Features::_ESPHealthPeds.GetState();
        style.m_Distance           = Features::_ESPDistancePeds.GetState();
        style.m_DistanceColor      = ImColor(Features::_DistanceColorPeds.GetState());
        style.m_Skeleton           = Features::_ESPSkeletonPeds.GetState();
        style.m_SkeletonColor      = ImColor(Features::_SkeletonColorPeds.GetState());
        style.m_HorseSkeleton      = Features::_ESPSkeletonHorse.GetState();
        style.m_HorseSkeletonColor = ImColor(Features::_SkeletonColorHorse.GetState());
        style.m_Title              = ped.GetInfo();
        style.m_TitleColor         = ImColor(Features::_HashColorPeds.GetState());
        DrawActor(ped, world, drawList, style);
    }

    void ESP::Draw(IEspWorld& world)
    {
        if (!world.IsAvailable())
            return;

        if (const auto drawList = ImGui::GetBackgroundDrawList())
        {
            // peds first: players are the more important layer and end up on top
            if (Features::_ESPDrawPeds.GetState())
            {
                world.ForEachPed([&](IEspActor& ped) {
                    DrawPeds(ped, world, drawList);
                });
            }
            if (Features::_ESPDrawPlayers.GetState())
            {
                world.ForEachPlayer([&](IEspActor& player) {
                    DrawPlayer(player, world, drawList);
                });
            }
        }
    }
} // namespace YimMenu
