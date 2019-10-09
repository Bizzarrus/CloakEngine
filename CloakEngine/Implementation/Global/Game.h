#pragma once
#ifndef CE_IMPL_GLOBAL_GAME_H
#define CE_IMPL_GLOBAL_GAME_H

#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "Engine/LinkedList.h"

#include <atomic>
#include <string>

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Game {
				constexpr float DEFAULT_SLEEP_TIME = 1000000.0f / 20.0f;
				//struct ThreadFPSInfo {
				//	API::Global::Time lastTime = 0;
				//	API::Global::Time prevTime = 0;
				//	unsigned int turns = 0;
				//	unsigned int lastTurns = 0;
				//	float lastFPS = 0;
				//};
				class ThreadFPSState {
					private:
						API::Global::Time m_last = 0;
						API::Global::Time m_prev = 0;
						uint32_t m_turns = 0;
						uint32_t m_lastTurns = 0;
						float m_lastFPS = 0;
					public:
						float CLOAK_CALL_THIS Update();
						float CLOAK_CALL_THIS Update(In API::Global::Time currentTime);
				};
				class ThreadSleepState {
					private:
						long double m_round = 0;
						API::Global::Time m_expectedAwake = 0;
					public:
						API::Global::Time CLOAK_CALL_THIS GetSleepTime(In float targetFrameTime);
						API::Global::Time CLOAK_CALL_THIS GetSleepTime(In API::Global::Time frameStart, In float targetFrameTime);
						API::Global::Time CLOAK_CALL_THIS GetSleepTime(In API::Global::Time frameStart, In API::Global::Time currentTime, In float targetFrameTime);
				};
				class ThreadComState {
					public:	
						static constexpr uint8_t MAX_LEVEL = 6;
						enum State { WAITING, UPDATED, FINISHED };
					private:
						uint8_t m_level = MAX_LEVEL;
						bool m_reduce = false;
					public:
						State CLOAK_CALL_THIS Check();
						void CLOAK_CALL_THIS Update();
						uint8_t CLOAK_CALL_THIS GetCurrentLevel() const;
				};
#ifdef _DEBUG
				typedef Engine::LinkedList<API::Helper::ISavePtr*>::const_iterator ptrAdr;
				void CLOAK_CALL removePtr(In ptrAdr);
				ptrAdr CLOAK_CALL registerPtr(In API::Helper::ISavePtr* ptr);
#endif
				void CLOAK_CALL registerThread();
				void CLOAK_CALL registerThreadStop();
#ifdef _DEBUG
				void CLOAK_CALL threadRespond(In unsigned int tID, In API::Global::Time time);
				void CLOAK_CALL disableThreadRespondCheck(In unsigned int tID);
#endif
				bool CLOAK_CALL canThreadRun();
				bool CLOAK_CALL canThreadStop();
				void CLOAK_CALL looseFocus();
				void CLOAK_CALL gainFocus();
				void CLOAK_CALL wakeThreads();
				void CLOAK_CALL updateThreadPause(In bool mode);
				bool CLOAK_CALL checkThreadWaiting();
				std::string CLOAK_CALL getModulePath();
				unsigned long long CLOAK_CALL getCurrentTimeMilliSeconds();
				unsigned long long CLOAK_CALL getCurrentTimeMicroSeconds();
				uint8_t CLOAK_CALL GetCurrentComLevel();
			}
		}
	}
}

#endif
