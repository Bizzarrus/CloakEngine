#include "stdafx.h"
#include "CloakEngine/Global/Steam.h"
#include "Implementation/Global/Steam.h"

#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Global/Log.h"

#include "Engine/Steam/Core.h"
#include "Engine/Steam/Achievements.h"

#include <atomic>

#ifdef USE_STEAM
#pragma comment(lib,"steam_api.lib")
#endif

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Steam {
				std::atomic<bool> g_init = false;
				Engine::Steam::Achievements::AchievementHandler* g_achievement = nullptr;

				void CLOAK_CALL Initialize(In bool use)
				{
					bool init = false;
#ifdef USE_STEAM
					if (use) { init = SteamAPI_Init(); }
#endif
					g_init = init;
					g_achievement = Engine::Steam::Achievements::GetNewHandler(init);

					API::Global::Log::WriteToLog("Steam initialized: " + std::to_string(g_init), API::Global::Log::Type::Info);

				}
				void CLOAK_CALL Update()
				{
#ifdef USE_STEAM
					if (g_init)
					{
						SteamAPI_RunCallbacks();
					}
#endif
				}
				void CLOAK_CALL RequestData()
				{
					
				}
				void CLOAK_CALL Release()
				{
#ifdef USE_STEAM
					if (g_init == true) { SteamAPI_Shutdown(); }
#endif
					delete g_achievement;
				}
			}
		}
	}
	namespace API {
		namespace Global {
			namespace Steam {
				bool CLOAKENGINE_API IsConnected() { return Impl::Global::Steam::g_init; }
				namespace Stat {
					void CLOAKENGINE_API Register(In unsigned int ID, In const char* strID, In Type type)
					{
						
					}
					void CLOAKENGINE_API SetValue(In unsigned int ID, In int value)
					{
						
					}
					void CLOAKENGINE_API SetValue(In unsigned int ID, In float value)
					{
						
					}
					void CLOAKENGINE_API SetValue(In unsigned int ID, In float value, In double time)
					{
						
					}
					void CLOAKENGINE_API GetValue(In unsigned int ID, Out int* value)
					{
						
					}
					void CLOAKENGINE_API GetValue(In unsigned int ID, Out float* value)
					{
						
					}
				}
				namespace Achievements {
					void CLOAKENGINE_API Register(In unsigned int ID, In const char* strID, In const Achievement& a) { Impl::Global::Steam::g_achievement->RegisterAchievement(ID, strID, a); }
					bool CLOAKENGINE_API Get(In unsigned int ID, Out Achievement* a) { return Impl::Global::Steam::g_achievement->GetAchievement(ID, a); }
				}
			}
		}
	}
}