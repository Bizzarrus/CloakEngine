#pragma once
#ifndef CE_E_STEAM_ACHIEVEMENTS_H
#define CE_E_STEAM_ACHIEVEMENTS_H

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Steam.h"
#include "Engine/Steam/Core.h"

#include <string>
#include <unordered_map>

namespace CloakEngine {
	namespace Engine {
		namespace Steam {
			namespace Achievements {
				struct Achievement {
					std::string strID;
					std::string Name;
					std::string Description;
					bool Achieved;
					CLOAK_CALL Achievement();
					CLOAK_CALL Achievement(In const std::string& id);
				};
				CLOAK_INTERFACE_BASIC AchievementHandler {
					public:
						virtual CLOAK_CALL ~AchievementHandler() {}
						virtual bool CLOAK_CALL_THIS RequestData() = 0;
						virtual void CLOAK_CALL_THIS RegisterAchievement(In unsigned int ID, In const std::string& strID, In const API::Global::Steam::Achievement& a) = 0;
						virtual void CLOAK_CALL_THIS SetAchieved(In unsigned int ID) = 0;
						Success(return == true) virtual bool CLOAK_CALL_THIS GetAchievement(In unsigned int ID, Out API::Global::Steam::Achievement* a) = 0;
					protected:
						API::HashMap<unsigned int, Achievement> m_achievements;
						API::Helper::ISyncSection* m_sync;
				};
				AchievementHandler* GetNewHandler(In bool useSteam);
			}
		}
	}
}

#endif
