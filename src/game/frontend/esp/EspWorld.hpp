#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "imgui.h"
#include <rage/vector.hpp>

namespace YimMenu
{
	// Minimal view of one ESP-tracked actor. Production code maps real
	// Ped/Player objects to this; the visual test feeds scripted fakes.
	// All expensive game/native queries are resolved by the provider,
	// so the shared draw code stays free of RDR2 dependencies.
	class IEspActor
	{
	public:
		virtual ~IEspActor() = default;

		virtual bool IsValid() const    = 0;
		virtual bool IsDead() const     = 0;
		virtual bool IsAnimal() const   = 0;
		virtual bool IsHorse() const    = 0;
		virtual bool IsSelf() const     = 0; // skipped, like the local player
		virtual bool IsSelected() const = 0; // blue name, like the selected player

		virtual std::string GetName() const = 0; // players; empty for peds
		virtual std::string GetInfo() const = 0; // model / net / script line for peds
		virtual float GetDistance() const   = 0; // precomputed distance to self
		virtual int GetHealth() const       = 0;
		virtual int GetMaxHealth() const    = 0;
		virtual bool IsVisible() const      = 0; // line-of-sight state (occluded targets render dimmed)

		virtual rage::fvector3 GetBone(int boneId) const = 0;
	};

	// Entity source + camera projection behind the ESP overlay.
	// Project() returns x == 0 when the point cannot be projected
	// (no camera / behind it), mirroring the in-game contract.
	class IEspWorld
	{
	public:
		virtual ~IEspWorld() = default;

		virtual bool IsAvailable() const = 0;
		virtual void ForEachPlayer(const std::function<void(IEspActor&)>& callback) = 0;
		virtual void ForEachPed(const std::function<void(IEspActor&)>& callback)    = 0;
		virtual ImVec2 Project(const rage::fvector3& world) const                   = 0;
		virtual ImVec2 GetScreenSize() const                                        = 0;
	};
}
