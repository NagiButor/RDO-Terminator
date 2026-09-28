#include "FireNova.hpp"

#include "game/commands/PlayerCommand.hpp"
#include "game/backend/Players.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <mutex>
#include <random>
#include <vector>

namespace YimMenu::Features
{
	// Fire Nova drops a single meteor straight onto the target, Meteor Rain drops
	// several at random points around them.
	// The impact itself must not kill anyone: every damaging explosion is only
	// placed where no player is within kBlastSafeDist, otherwise it is replaced by
	// a harmless sound-only blast with the same camera shake. Fire is allowed to
	// burn and hurt people, it just isn't lit directly under someone's feet.
	//
	// Everything runs in one persistent script (MeteorDirector) as small state
	// machines. The previous version parked a FiberPool fiber on every fire for
	// 45 s; the pool only has 5 fibers, so NPC spawns and every other feature
	// queued up behind the fires.
	namespace
	{
		using Clock = std::chrono::steady_clock;
		using namespace std::chrono_literals;

		constexpr float kBlastSafeDist  = 12.f; // damaging explosions stay this far from players
		constexpr float kRainSafeDist   = 16.f; // rain impact points stay this far from players
		constexpr float kFireClearDist  = 2.5f; // no fire lit right under a player
		constexpr float kPi2            = 6.2831853f;
		constexpr int kMaxLiveFires     = 160;
		constexpr int kTrailSteps       = 40;
		constexpr auto kTrailStepTime   = 30ms;
		constexpr auto kRingDelay       = 150ms;
		constexpr auto kFireLifetime    = 45s;

		std::mt19937& Rng()
		{
			static std::mt19937 rng{std::random_device{}()};
			return rng;
		}

		float Rand(float a, float b)
		{
			return std::uniform_real_distribution<float>(a, b)(Rng());
		}

		float Dist3D(const rage::fvector3& a, const rage::fvector3& b)
		{
			const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
			return std::sqrt(dx * dx + dy * dy + dz * dz);
		}

		// ground under (x, y), probing from well above so hills don't swallow the ray
		bool GroundAt(float x, float y, float refZ, float& outZ)
		{
			return MISC::GET_GROUND_Z_FOR_3D_COORD(x, y, refZ + 40.f, &outZ, false);
		}

		bool IsWater(float x, float y, float groundZ)
		{
			float water = 0.f;
			return WATER::GET_WATER_HEIGHT(x, y, groundZ + 2.f, &water) && water > groundZ - 0.2f;
		}

		// positions of every player ped in the session, including ourselves
		std::vector<rage::fvector3> PlayerPositions()
		{
			std::vector<rage::fvector3> out;
			for (auto& [id, player] : Players::GetPlayers())
			{
				if (!player.IsValid())
					continue;
				auto ped = player.GetPed();
				if (ped.IsValid())
					out.push_back(ped.GetPosition());
			}
			if (auto self = Self::GetPed(); self.IsValid())
				out.push_back(self.GetPosition());
			return out;
		}

		float NearestPlayer(const rage::fvector3& p, const std::vector<rage::fvector3>& players)
		{
			float best = 1e9f;
			for (auto& pp : players)
				best = std::min(best, Dist3D(p, pp));
			return best;
		}

		// damaging explosion if nobody is close, otherwise a harmless boom with the same shake
		void Blast(const rage::fvector3& p, ExplosionTypes type, float scale, bool sound, float shake, const std::vector<rage::fvector3>& players)
		{
			if (NearestPlayer(p, players) >= kBlastSafeDist)
				FIRE::ADD_EXPLOSION(p.x, p.y, p.z, (int)type, scale, sound, false, shake);
			else if (sound || shake > 0.f)
				FIRE::ADD_EXPLOSION(p.x, p.y, p.z, (int)ExplosionTypes::ONLY_SOUND, 1.f, sound, true, shake);
		}

		// ---------------------------------------------------------------- fires

		struct LiveFire
		{
			int id;
			Clock::time_point expires;
		};

		std::vector<LiveFire> g_Fires;

		// one big, spreading flame; put out later by the director to stay under the fire cap
		void LightFire(const rage::fvector3& p, float scale, int generations, const std::vector<rage::fvector3>& players)
		{
			if ((int)g_Fires.size() >= kMaxLiveFires)
				return;
			if (IsWater(p.x, p.y, p.z))
				return;
			if (NearestPlayer(p, players) < kFireClearDist)
				return;

			const int fire = FIRE::START_SCRIPT_FIRE(p.x, p.y, p.z + 0.1f, generations, scale, false, "", 0.f, 0);
			if (fire)
				g_Fires.push_back({fire, Clock::now() + kFireLifetime + std::chrono::milliseconds((int)Rand(0.f, 8000.f))});
		}

		void ExpireFires()
		{
			const auto now = Clock::now();
			std::erase_if(g_Fires, [now](const LiveFire& f) {
				if (f.expires > now)
					return false;
				FIRE::REMOVE_SCRIPT_FIRE(f.id);
				return true;
			});
		}

		// -------------------------------------------------------------- meteors

		struct Meteor
		{
			rage::fvector3 start;
			rage::fvector3 impact;
			float fireRadius;
			int step = 0; // 0..kTrailSteps = trail, then impact stages
			Clock::time_point next = Clock::now();
		};

		// a Meteor Rain in progress: drops `left` more meteors around `target`
		struct Rain
		{
			Player target;
			int left;
			Clock::time_point next = Clock::now();
		};

		std::mutex g_Mutex; // guards the queues below: commands run on FiberPool fibers
		std::vector<Meteor> g_NewMeteors;
		std::vector<Rain> g_NewRains;

		void QueueMeteor(const rage::fvector3& impact, float fireRadius)
		{
			// entry point: high up, off to a random side so the streak is clearly diagonal
			const float heading = Rand(0.f, kPi2);
			Meteor m;
			m.start      = {impact.x + std::cos(heading) * 80.f, impact.y + std::sin(heading) * 80.f, impact.z + 130.f};
			m.impact     = impact;
			m.fireRadius = fireRadius;

			std::lock_guard lock(g_Mutex);
			g_NewMeteors.push_back(m);
		}

		// burning ground: dense core + jittered rings, lit centre-out one ring per call
		void BlazeRing(const Meteor& m, int ring, const std::vector<rage::fvector3>& players)
		{
			const auto& c = m.impact;
			if (ring == 0)
			{
				LightFire(c, 3.5f, 30, players);
				Blast(c, ExplosionTypes::MOLOTOV_TYPE_EXPLOSION_FIRE, 1.2f, false, 0.f, players);
				return;
			}

			const float radii[] = {0.f, 0.3f, 0.55f, 0.8f, 1.f};
			const int counts[]  = {0, 5, 9, 13, 16};
			const float offset  = Rand(0.f, kPi2);
			for (int i = 0; i < counts[ring]; i++)
			{
				const float a = offset + kPi2 * i / counts[ring] + Rand(-0.2f, 0.2f);
				const float r = m.fireRadius * radii[ring] + Rand(-0.6f, 0.6f);
				rage::fvector3 p{c.x + std::cos(a) * r, c.y + std::sin(a) * r, c.z};
				if (!GroundAt(p.x, p.y, c.z, p.z))
					continue;
				// big overlapping flames near the core, a bit smaller at the edge
				LightFire(p, ring == 4 ? 2.4f : 3.0f, 25, players);
				// a molotov burst ignites grass / brush around each point
				Blast(p, ExplosionTypes::MOLOTOV_TYPE_EXPLOSION_FIRE, 0.8f, false, 0.f, players);
			}
		}

		// advances one meteor by one step; returns false once it is finished
		bool StepMeteor(Meteor& m, const std::vector<rage::fvector3>& players)
		{
			if (m.step <= kTrailSteps)
			{
				// ease-in: the rock accelerates as it falls
				const float t = std::pow((float)m.step / kTrailSteps, 1.7f);
				const rage::fvector3 p{m.start.x + (m.impact.x - m.start.x) * t, m.start.y + (m.impact.y - m.start.y) * t, m.start.z + (m.impact.z - m.start.z) * t};

				if (p.z > m.impact.z + 3.f)
				{
					// burning head
					Blast(p, ExplosionTypes::MOLOTOV_STYLE_EXPLOSION1, 1.6f, false, 0.f, players);
					// thick smoke tail left behind, sparks shed every few steps
					Blast(p, ExplosionTypes::SMALL_EXPLOSION_NO_SOUND, 1.0f, false, 0.f, players);
					if (m.step % 3 == 0)
						Blast(p, ExplosionTypes::SPARKS_WITH_FIRE_AFTERWARDS, 0.6f, false, 0.f, players);
				}

				m.step++;
				m.next = Clock::now() + kTrailStepTime;
				return true;
			}

			const auto& c = m.impact;
			switch (m.step++ - kTrailSteps - 1)
			{
			case 0: // impact: rock debris, fireball, heavy rumble
				Blast(c, ExplosionTypes::BIG_ROCK_EXPLOSION, 1.3f, true, 3.0f, players);
				Blast(c, ExplosionTypes::BIG_EXPLOSION_25, 1.0f, true, 2.5f, players);
				// debris thrown out around the crater, even when the centre had to stay harmless
				for (int i = 0; i < 5; i++)
				{
					const float a = kPi2 * i / 5 + Rand(-0.3f, 0.3f);
					rage::fvector3 p{c.x + std::cos(a) * 14.f, c.y + std::sin(a) * 14.f, c.z};
					if (GroundAt(p.x, p.y, c.z, p.z))
						Blast(p, ExplosionTypes::SMALLER_ROCK_EXPLOSION1, 0.8f, false, 0.f, players);
				}
				m.next = Clock::now() + 120ms;
				return true;
			case 1: // rising mushroom of fire and smoke
				Blast({c.x, c.y, c.z + 3.f}, ExplosionTypes::MOLOTOV_STYLE_EXPLOSION2, 1.5f, false, 1.0f, players);
				Blast({c.x, c.y, c.z + 6.f}, ExplosionTypes::SMALL_EXPLOSION_NO_SOUND, 1.4f, false, 0.f, players);
				m.next = Clock::now() + kRingDelay;
				return true;
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
				BlazeRing(m, m.step - kTrailSteps - 4, players);
				m.next = Clock::now() + kRingDelay;
				return true;
			default:
				return false;
			}
		}

		bool StepRain(Rain& r)
		{
			if (r.left <= 0 || !r.target.IsValid())
				return false;
			auto ped = r.target.GetPed();
			if (!ped.IsValid())
				return false;

			// re-read the position each time: the target is probably running
			const auto center  = ped.GetPosition();
			const auto players = PlayerPositions();
			for (int attempt = 0; attempt < 24; attempt++)
			{
				const float a = Rand(0.f, kPi2);
				const float d = Rand(kRainSafeDist, 25.f);
				rage::fvector3 p{center.x + std::cos(a) * d, center.y + std::sin(a) * d, center.z};
				if (!GroundAt(p.x, p.y, center.z, p.z) || NearestPlayer(p, players) < kRainSafeDist)
					continue;
				QueueMeteor(p, 9.f);
				break;
			}

			r.left--;
			r.next = Clock::now() + std::chrono::milliseconds((int)Rand(600.f, 1300.f));
			return r.left > 0;
		}
	}

	void MeteorDirector()
	{
		std::vector<Meteor> meteors;
		std::vector<Rain> rains;

		while (true)
		{
			{
				std::lock_guard lock(g_Mutex);
				meteors.insert(meteors.end(), g_NewMeteors.begin(), g_NewMeteors.end());
				rains.insert(rains.end(), g_NewRains.begin(), g_NewRains.end());
				g_NewMeteors.clear();
				g_NewRains.clear();
			}

			const auto now = Clock::now();

			std::erase_if(rains, [now](Rain& r) {
				return r.next <= now && !StepRain(r);
			});

			if (!meteors.empty())
			{
				const auto players = PlayerPositions();
				std::erase_if(meteors, [&](Meteor& m) {
					return m.next <= now && !StepMeteor(m, players);
				});
			}

			ExpireFires();
			ScriptMgr::Yield();
		}
	}

	// One meteor, straight onto the player, leaving a crater of fire around them
	class FireNova : public PlayerCommand
	{
		using PlayerCommand::PlayerCommand;

		virtual void OnCall(Player player) override
		{
			if (!player.IsValid() || !player.GetPed().IsValid())
				return;

			rage::fvector3 impact = player.GetPed().GetPosition();
			GroundAt(impact.x, impact.y, impact.z, impact.z);
			QueueMeteor(impact, 10.f);
		}
	};

	// Several meteors fall one after another at random points 16-25 m around the player
	class FireRain : public PlayerCommand
	{
		using PlayerCommand::PlayerCommand;

		virtual void OnCall(Player player) override
		{
			if (!player.IsValid() || !player.GetPed().IsValid())
				return;

			std::lock_guard lock(g_Mutex);
			g_NewRains.push_back({player, 7});
		}
	};

	static FireNova _FireNova{"firenova", "Fire Nova", "A single meteor crashes straight onto the player and sets everything around them ablaze"};
	static FireRain _FireRain{"firerain", "Meteor Rain", "Meteors rain down around the player, each impact leaves a raging fire"};
}
