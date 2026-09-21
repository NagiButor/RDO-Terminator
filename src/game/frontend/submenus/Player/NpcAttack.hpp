#pragma once
#include "game/frontend/submenus/Player/AttackWaves.hpp"

#include <imgui.h>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace YimMenu::Features::NpcAttackOptions
{
    // Converts text typed in the Russian keyboard layout into the Latin
    // characters on the same physical keys (e.g. "иуфк" -> "bear").
    // The menu font has no Cyrillic glyphs, so RU-layout input shows as "???"
    // and never matches a model/weapon name -- converting it on the fly
    // makes the fields usable no matter which layout is active.
    inline std::string FixRuKeyboardLayout(const std::string& input)
    {
        // key = UTF-8 bytes of the Cyrillic char (first << 8 | second)
        static const std::unordered_map<std::uint16_t, char> kRuToEn = {
            {0xD0B9, 'q'}, {0xD186, 'w'}, {0xD183, 'e'}, {0xD0BA, 'r'}, {0xD0B5, 't'}, {0xD0BD, 'y'},
            {0xD0B3, 'u'}, {0xD188, 'i'}, {0xD189, 'o'}, {0xD0B7, 'p'}, {0xD185, '['}, {0xD18A, ']'},
            {0xD184, 'a'}, {0xD18B, 's'}, {0xD0B2, 'd'}, {0xD0B0, 'f'}, {0xD0BF, 'g'}, {0xD180, 'h'},
            {0xD0BE, 'j'}, {0xD0BB, 'k'}, {0xD0B4, 'l'}, {0xD0B6, ';'}, {0xD18D, '\''},
            {0xD18F, 'z'}, {0xD187, 'x'}, {0xD181, 'c'}, {0xD0BC, 'v'}, {0xD0B8, 'b'}, {0xD182, 'n'},
            {0xD18C, 'm'}, {0xD0B1, ','}, {0xD18E, '.'}, {0xD191, '`'},
            {0xD099, 'Q'}, {0xD0A6, 'W'}, {0xD0A3, 'E'}, {0xD09A, 'R'}, {0xD095, 'T'}, {0xD09D, 'Y'},
            {0xD093, 'U'}, {0xD0A8, 'I'}, {0xD0A9, 'O'}, {0xD097, 'P'}, {0xD0A5, '{'}, {0xD0AA, '}'},
            {0xD0A4, 'A'}, {0xD0AB, 'S'}, {0xD092, 'D'}, {0xD090, 'F'}, {0xD09F, 'G'}, {0xD0A0, 'H'},
            {0xD09E, 'J'}, {0xD09B, 'K'}, {0xD094, 'L'}, {0xD096, ':'}, {0xD0AD, '"'},
            {0xD0AF, 'Z'}, {0xD0A7, 'X'}, {0xD0A1, 'C'}, {0xD09C, 'V'}, {0xD098, 'B'}, {0xD0A2, 'N'},
            {0xD0AC, 'M'}, {0xD091, '<'}, {0xD0AE, '>'}, {0xD081, '~'},
        };

        bool has_cyrillic = false;
        for (const auto c : input)
        {
            if ((c & 0x80) != 0)
            {
                has_cyrillic = true;
                break;
            }
        }
        if (!has_cyrillic)
            return input;

        std::string out;
        out.reserve(input.size());
        for (std::size_t i = 0; i < input.size();)
        {
            const auto c = (unsigned char)input[i];
            if (c < 0x80)
            {
                out.push_back((char)c);
                ++i;
            }
            else if ((c == 0xD0 || c == 0xD1) && i + 1 < input.size())
            {
                const auto key = (std::uint16_t)((std::uint16_t(c) << 8) | (unsigned char)input[i + 1]);
                const auto it  = kRuToEn.find(key);
                if (it != kRuToEn.end())
                    out.push_back(it->second);
                // unmappable Cyrillic is dropped: the font cannot render it anyway
                i += 2;
            }
            else
            {
                // some other multibyte sequence -- skip one UTF-8 code point
                std::size_t len = 1;
                if ((c & 0xE0) == 0xC0)
                    len = 2;
                else if ((c & 0xF0) == 0xE0)
                    len = 3;
                else if ((c & 0xF8) == 0xF0)
                    len = 4;
                i += len;
            }
        }
        return out;
    }

    // ImGui edit-callback helper: transliterates RU-layout text to Latin
    // right as it is typed, so the field never holds "???" glyphs.
    inline bool ApplyRuFixToImGuiInput(ImGuiInputTextCallbackData* data)
    {
        if (!data || data->EventFlag != ImGuiInputTextFlags_CallbackEdit)
            return false;

        const std::string cur(data->Buf, (std::size_t)data->BufTextLen);
        const std::string fixed = FixRuKeyboardLayout(cur);
        if (fixed == cur)
            return false;

        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, fixed.c_str());
        return true;
    }

    // settings shown in the "Npc Attack" section of the menu
    std::string& ConfiguredModel();
    int& ConfiguredCount();
    float& ConfiguredDistance();
    int& ConfiguredHealth(); // 0 = keep the game default, >0 = HP given to every spawned NPC
    bool& ConfiguredLasso(); // humans attack with a lasso (hogtie) instead of a gunfight
    int& ConfiguredSideIndex();            // 0 = around the player, 1 = in front, 2 = behind
    AttackWaves::SpawnSide ConfiguredSide(); // resolved spawn side used by the feature

    // resolves a (partial) model name to a valid ped model, returns an empty string if nothing matches
    std::string ResolveModelName(const std::string& input);

    // ImGui completion callback for the model input field (Tab autocompletes)
    int ModelInputCallback(ImGuiInputTextCallbackData* data);

    // weapon shown in the "Npc Attack" section; empty / "None" = no weapon (hands, teeth, claws)
    std::string& ConfiguredWeapon();
    // resolves a (partial) weapon name to a valid weapon, returns an empty string if nothing matches
    std::string ResolveWeaponName(const std::string& input);
    // ImGui completion callback for the weapon input field (Tab autocompletes)
    int WeaponInputCallback(ImGuiInputTextCallbackData* data);

    // builds the current attack config (model + weapon + count + distance + side)
    AttackWaves::Config BuildConfig();
    // spawns exactly one NPC with the current settings on the selected player
    bool SpawnSingleNow();
    // deletes every living NPC spawned by this feature (singles + wave)
    void ClearAllNow();
}
