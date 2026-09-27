# MenuVisualTest: standalone harness rendering the REAL menu shell and the REAL
# ESP draw code against a scripted fake scene. Links only game-independent
# sources (commands/settings/manager/items/fonts/ESP) plus test doubles.
# The in-game DLL target is untouched by this file.

set(VISUAL_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

set(VISUAL_TEST_SOURCES
    "${VISUAL_TEST_DIR}/main.cpp"
    "${VISUAL_TEST_DIR}/MockEspWorld.cpp"
    "${VISUAL_TEST_DIR}/EspLabPanel.cpp"
    "${VISUAL_TEST_DIR}/stubs/FiberPoolStub.cpp"

    # shared globals (g_Running etc. declared by common.hpp)
    "${SRC_DIR}/common.cpp"

    # shared, game-independent production code
    # (RdrEspWorld.cpp stays DLL-only: it binds ESP to live game objects)
    "${SRC_DIR}/game/frontend/MenuShell.cpp"
    "${SRC_DIR}/game/frontend/ESP.cpp"
    "${SRC_DIR}/game/frontend/fonts/MainFont.cpp"
    "${SRC_DIR}/game/frontend/submenus/Settings.cpp"
    "${SRC_DIR}/game/frontend/items/BoolCommandItem.cpp"
    "${SRC_DIR}/game/frontend/items/Button.cpp"
    "${SRC_DIR}/game/frontend/items/ColorCommandItem.cpp"
    "${SRC_DIR}/game/frontend/items/CommandItem.cpp"
    "${SRC_DIR}/game/frontend/items/ConditionalItem.cpp"
    "${SRC_DIR}/game/frontend/items/FloatCommandItem.cpp"
    "${SRC_DIR}/game/frontend/items/Group.cpp"
    "${SRC_DIR}/game/frontend/items/HotkeySetter.cpp"
    "${SRC_DIR}/game/frontend/items/ImGuiItem.cpp"
    "${SRC_DIR}/game/frontend/items/InputTextWithHint.cpp"
    "${SRC_DIR}/game/frontend/items/IntCommandItem.cpp"
    "${SRC_DIR}/game/frontend/items/ListCommandItem.cpp"
    "${SRC_DIR}/game/frontend/items/StringCommandItem.cpp"
    "${SRC_DIR}/core/commands/BoolCommand.cpp"
    "${SRC_DIR}/core/commands/ColorCommand.cpp"
    "${SRC_DIR}/core/commands/Command.cpp"
    "${SRC_DIR}/core/commands/Commands.cpp"
    "${SRC_DIR}/core/commands/FloatCommand.cpp"
    "${SRC_DIR}/core/commands/HotkeySystem.cpp"
    "${SRC_DIR}/core/commands/IntCommand.cpp"
    "${SRC_DIR}/core/commands/ListCommand.cpp"
    "${SRC_DIR}/core/commands/LoopedCommand.cpp"
    "${SRC_DIR}/core/commands/StringCommand.cpp"
    "${SRC_DIR}/core/frontend/manager/Category.cpp"
    "${SRC_DIR}/core/frontend/manager/Submenu.cpp"
    "${SRC_DIR}/core/frontend/manager/UIManager.cpp"
    "${SRC_DIR}/core/frontend/widgets/toggle/imgui_toggle.cpp"
    "${SRC_DIR}/core/frontend/widgets/toggle/imgui_toggle_palette.cpp"
    "${SRC_DIR}/core/frontend/widgets/toggle/imgui_toggle_presets.cpp"
    "${SRC_DIR}/core/frontend/widgets/toggle/imgui_toggle_renderer.cpp"
    "${SRC_DIR}/core/settings/Settings.cpp"
    "${SRC_DIR}/core/settings/IStateSerializer.cpp"
    "${SRC_DIR}/core/filemgr/BaseObj.cpp"
    "${SRC_DIR}/core/filemgr/File.cpp"
    "${SRC_DIR}/core/filemgr/FileMgr.cpp"
    "${SRC_DIR}/core/filemgr/Folder.cpp"
    "${SRC_DIR}/util/Joaat.cpp"

    # Dear ImGui Win32 + DX11 backends (core imgui lib is built by the DLL cmake)
    "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
)

add_executable(MenuVisualTest WIN32 ${VISUAL_TEST_SOURCES})
set_property(TARGET MenuVisualTest PROPERTY CXX_STANDARD 23)

# same language environment as the DLL: production headers rely on this PCH
target_precompile_headers(MenuVisualTest PRIVATE "${SRC_DIR}/common.hpp")

target_include_directories(MenuVisualTest PRIVATE
    "${SRC_DIR}"
    "${VISUAL_TEST_DIR}"
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
    "${imgui_SOURCE_DIR}/misc/cpp"
    "${RDR-Classes_SOURCE_DIR}"
    "${minhook_SOURCE_DIR}/include"
)

target_link_libraries(MenuVisualTest PRIVATE
    imgui
    nlohmann_json::nlohmann_json
    AsyncLogger
    d3d11.lib
    dxgi.lib
)

add_compile_definitions("_CRT_SECURE_NO_WARNINGS" "NOMINMAX" "WIN32_LEAN_AND_MEAN")

# land next to the cheat DLL so both exes live side by side
set_target_properties(MenuVisualTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
