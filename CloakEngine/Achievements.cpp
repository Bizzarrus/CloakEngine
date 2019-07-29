#include "stdafx.h"
#include "Engine/Steam/Achievements.h"
#include "CloakEngine/Global/Memory.h"

#include "CloakEngine/Global/Game.h"

#include "Implementation/Global/Steam.h"

#include <atomic>

//Steam achievement handler. See https://partner.steamgames.com/documentation/bootstrap_achieve for more infos
//There's also a BasicHandler in case steam support is deactivated

namespace CloakEngine {
	namespace Engine {
		namespace Steam {
			namespace Achievements {
				CLOAK_CALL Achievement::Achievement() : Achievement("") {}
				CLOAK_CALL Achievement::Achievement(In const std::string& id)
				{
					strID = id;
					Name = "";
					Description = "";
					Achieved = false;
				}
#ifdef USE_STEAM
				class SteamHandler : public AchievementHandler {
					public:
						CLOAK_CALL SteamHandler() : m_appID(SteamUtils()->GetAppID()), m_init(false)
						{
							CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
						}
						CLOAK_CALL ~SteamHandler()
						{
							SAVE_RELEASE(m_sync);
						}
						void CLOAK_CALL_THIS RegisterAchievement(In unsigned int ID, In const std::string& strID, In const API::Global::Steam::Achievement& a) override
						{
							API::Helper::Lock lock(m_sync);
							m_achievements[ID] = Achievement(strID);
						}
						void CLOAK_CALL_THIS SetAchieved(In unsigned int ID) override
						{
							if (m_init)
							{
								API::Helper::Lock lock(m_sync);
								auto it = m_achievements.find(ID);
								if (it != m_achievements.end())
								{
									SteamUserStats()->SetAchievement(it->second.strID.c_str());
									SteamUserStats()->StoreStats();
								}
							}
						}
						bool CLOAK_CALL_THIS RequestData() override
						{
							if (SteamUserStats() != nullptr && SteamUser() != nullptr && SteamUser()->BLoggedOn())
							{
								return SteamUserStats()->RequestCurrentStats();
							}
							return false;
						}
						Success(return == true) bool CLOAK_CALL_THIS GetAchievement(In unsigned int ID, Out API::Global::Steam::Achievement* a) override
						{
							API::Helper::Lock lock(m_sync);
							auto it = m_achievements.find(ID);
							if (it == m_achievements.end()) { return false; }
							a->Achieved = it->second.Achieved;
							a->Description = it->second.Description;
							a->Name = it->second.Name;
							return true;
						}

						void* CLOAK_CALL operator new(In size_t s) { return API::Global::Memory::MemoryHeap::Allocate(s); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }

						//Callbacks:

						STEAM_CALLBACK(SteamHandler, OnUserStatsReceived, UserStatsReceived_t)
						{
							if (pParam->m_nGameID == m_appID)
							{
								if (pParam->m_eResult == k_EResultOK)
								{
									m_init = true;
									API::Helper::Lock lock(m_sync);
									for (const auto& it : m_achievements)
									{
										Achievement* a = &it.second;
										SteamUserStats()->GetAchievement(a->strID.c_str(), &a->Achieved);
										a->Name = SteamUserStats()->GetAchievementDisplayAttribute(a->strID.c_str(), "name");
										a->Description = SteamUserStats()->GetAchievementDisplayAttribute(a->strID.c_str(), "desc");
									}
								}
								else
								{
									//recieved failed, callback->m_eResult contain error code
								}
							}
						}
						STEAM_CALLBACK(SteamHandler, OnUserStatsStored, UserStatsStored_t)
						{
							if (pParam->m_nGameID == m_appID)
							{
								if (pParam->m_eResult == k_EResultOK)
								{
									//Stored ok
								}
								else
								{
									//Stored failed, callback->m_eResult contain error code
								}
							}
						}
						STEAM_CALLBACK(SteamHandler, OnAchievementStored, UserAchievementStored_t)
						{
							if (pParam->m_nGameID == m_appID)
							{
								//Stored ok
							}
						}
					private:
						std::atomic<bool> m_init;
						const uint64 m_appID;
				};
#endif
				class BasicHandler : public AchievementHandler {
					public:
						CLOAK_CALL BasicHandler()
						{
							CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
						}
						CLOAK_CALL ~BasicHandler()
						{
							SAVE_RELEASE(m_sync);
						}
						void CLOAK_CALL_THIS RegisterAchievement(In unsigned int ID, In const std::string& strID, In const API::Global::Steam::Achievement& a) override
						{
							Achievement ac(strID);
							ac.Achieved = a.Achieved;
							ac.Description = a.Description;
							ac.Name = a.Name;
							API::Helper::Lock lock(m_sync);
							m_achievements[ID] = ac;
						}
						void CLOAK_CALL_THIS SetAchieved(In unsigned int ID) override
						{
							API::Helper::Lock lock(m_sync);
							auto it = m_achievements.find(ID);
							if (it != m_achievements.end())
							{
								it->second.Achieved = true;
							}
						}
						bool CLOAK_CALL_THIS RequestData() override
						{
							//TODO
							return true;
						}
						Success(return == true) bool CLOAK_CALL_THIS GetAchievement(In unsigned int ID, Out API::Global::Steam::Achievement* a) override
						{
							API::Helper::Lock lock(m_sync);
							auto it = m_achievements.find(ID);
							if (it == m_achievements.end()) { return false; }
							a->Achieved = it->second.Achieved;
							a->Description = it->second.Description;
							a->Name = it->second.Name;
							return true;
						}
						void* CLOAK_CALL operator new(In size_t s) { return API::Global::Memory::MemoryHeap::Allocate(s); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }
				};				

				AchievementHandler* GetNewHandler(In bool useSteam)
				{
#ifdef USE_STEAM
					if (useSteam) { return new SteamHandler(); }
#endif
					return new BasicHandler();
				}
			}
		}
	}
}