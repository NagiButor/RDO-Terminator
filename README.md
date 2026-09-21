# Mi5hlen Menu

A powerful, community-driven mod menu for **Red Dead Redemption 2** and **Red Dead Online** by Rockstar Games.
**Strictly for educational purposes.**

- **Author & maintainer:** Mi5hlen
- **Telegram:** [@Mi5hlen](https://t.me/Mi5hlen) — support, updates & builds
- Based on **Terminus / HorseMenu** (upstream: [YimMenu/HorseMenu](https://github.com/YimMenu/HorseMenu))

> This is a **BETA** build — some things may not work as intended. Bug reports and ideas are always welcome on Telegram.

---

## Getting Started

1. **Grab a popular injector** from GitHub and inject the DLL into `rdr2.exe`.
2. Inject **at the main menu**, not "in-game".
3. Open / close the menu with **F5** or **INSERT**.
4. Settings, logs and dictionaries are stored in: `%AppData%\Mi5hlenMenu`

> **Need help?** Message me on Telegram: [@Mi5hlen](https://t.me/Mi5hlen)

---

## Features & Improvements

| **Area**     | **What's inside**                                                                                            |
|--------------|--------------------------------------------------------------------------------------------------------------|
| **GUI**      | Modern theming, theme editor/presets, resizable UI, color editor, placement saving, menu toggle (F5/INSERT)   |
| **ESP**      | Modernized and fixed ESP overlay                                                                              |
| **Self**     | Give all weapons/ammo, searchable animation dictionary, music player improvements                              |
| **Teleport** | Teleport to Guarma, loading screen/reset toggles                                                              |
| **Player**   | "BoomPlus" tab for custom explosions, "BeatDown"/"Beat N Kick" events, Indian Attack Mode, Ride'em Cowboy      |
| **World**    | Reworked spawners, legendary presets, Kill'em All, Bodyguard system, save last 20 spawned peds                 |
| **Recovery** | Give American Wild Flower collection                                                                          |
| **Settings** | Customizable everything — menu, GUI, theming and more!                                                        |
| **Other**    | AniDict.json / MusicDict.json creation for advanced users                                                     |

---

## V 2.2 Update

- **Scenario Player** — added by *evl wayz*
- **One Click Dailys** — added by *Mi5hlen*
- **Train Speed** — added by *Mi5hlen*
- **Old DeadEye Target select** — added by *Mi5hlen*

---

## V 2.1 Update

- Fixes: updated Companion and BodyGuard settings, minor variation-logic fixes, reverted RemoteBolas
- **Added:** Perfect Animal/Pelt option
- Spawn perfect dead animals
- Poodle Fun in Boom Plus
- More trains, direction control, and the ability to delete the trains you spawn
- Now you can **GIVE the American Wild Flower Collection to others**

---

## Building from source (Windows)

**Requirements:** Visual Studio 2022 (MSVC x64), CMake 3.20+, Ninja.
All third-party dependencies (imgui, minhook, nlohmann/json, AsyncLogger, Vulkan-Headers, RDR-Classes) are downloaded automatically by CMake via `FetchContent`.

```powershell
# run from "x64 Native Tools Command Prompt for VS 2022"
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DFETCHCONTENT_UPDATES_DISCONNECTED=ON -S . -B build -G Ninja
cmake --build build --target Mi5hlenMenu
```

**Result:** `build\Mi5hlenMenu.dll` — this is the file you inject into `rdr2.exe`.

---

## Credits & Scripting Attribution

- **Upstream base:**
  - [YimMenu/HorseMenu](https://github.com/YimMenu/HorseMenu) — the original foundation and codebase.

- **Project lead, integration, main scripting & fixes:**
  - **Mi5hlen** — Telegram: [@Mi5hlen](https://t.me/Mi5hlen)
    - *Self/Weapons:* Give All Weapons & Ammo
    - *Self/Animations:* Searchable Animation Dictionary, last 20 used, music player overhaul, dictionary and loop toggle
    - *Teleport/Main:* Teleport to Guarma, loading screen/reset menu toggle
    - *Player:* "BoomPlus" (custom explosions), BeatDown/Beat N Kick, Ride'em Cowboy, Spank Dat Ass, Indian Attack Mode
    - *Player/Kick:* Population Reset improvements, Beat N Kick scripting
    - *World/Spawners:* Ped Spawner overhaul, Legendary presets, Kill'em All, BodyGuard system, save last 20 spawned peds
    - *Recovery:* Give American Wild Flower collection
    - *Settings/GUI:* ESP fixes, menu toggle system, menu theming, custom color editor, menu resizing & placement saving
    - *Misc:* BAT file and AniDict.json creation
    - *V2.2 Additions:* One Click Dailys, Train Speed, Old DeadEye Target select

- **evl wayz:**
    - Suggested and helped with GUI re-size feature
    - *V2.2 Addition:* Scenario Player

- **Observed Cat:**
    - Scripted the "Old deadEye" section
    - Added the feature to give collectibles to other players (Players/Helpful)

- **MaestroDelFuego:**
    - Inspiration for the V2 Theme Editor & Preset system

- **PlumbWick3d:**
    - Provided the custom command-line injector used in the AIO package
    - Maintains [Wicked Menu Official Discord](https://discord.gg/ycbDQGqx3Q) for RDR2 & GTAV modding

- **Special Thanks:**
    - All staff and members of the HorseMenu/Terminus and Wicked Menu Discord communities, for ideas, bug reports and inspiration!

---

## Community & Support

- **Telegram:** [@Mi5hlen](https://t.me/Mi5hlen) — the fastest way to reach me: support, updates, custom builds
- **Discord:** [Wicked Menu Official](https://discord.gg/ycbDQGqx3Q) — RDR2 & GTAV modding community

---

**"Made with passion, powered by the community, and always evolving."**
