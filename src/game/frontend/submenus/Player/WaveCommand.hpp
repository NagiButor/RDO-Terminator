#pragma once
#include "AttackWaves.hpp"

#include "core/commands/LoopedCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/Players.hpp"

#include <string>

namespace YimMenu::Features::AttackWaves
{
    // Toggle that runs one AttackWaves::Config through a WaveRunner.
    // Army, Police, Wolves, Zombies and both Indian attacks are just configs.
    class WaveCommand : public LoopedCommand
    {
    public:
        using ConfigFn = Config (*)();

        WaveCommand(std::string name, std::string label, std::string description, ConfigFn config, WaveOptions options, std::string startText, std::string unitName) :
            LoopedCommand(std::move(name), std::move(label), std::move(description)),
            m_Config(config),
            m_Options(options),
            m_StartText(std::move(startText)),
            m_UnitName(std::move(unitName))
        {
        }

    private:
        void OnTick() override
        {
            m_Runner.Tick(m_Options, [this](int target, WaveRunner& runner) {
                return SpawnWave(target, m_Config(), runner.m_Attackers);
            });
        }

        void OnEnable() override
        {
            Notifications::Show(GetLabel(), m_StartText + " " + Players::GetSelected().GetName(), NotificationType::Info);
        }

        void OnDisable() override
        {
            m_Runner.Clear();
            Notifications::Show(GetLabel(), "Cleared " + m_UnitName, NotificationType::Info);
        }

        ConfigFn m_Config;
        WaveOptions m_Options;
        std::string m_StartText;
        std::string m_UnitName;
        WaveRunner m_Runner;
    };
}
