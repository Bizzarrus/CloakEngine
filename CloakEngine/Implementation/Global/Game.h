#pragma once
#ifndef CE_IMPL_GLOBAL_GAME_H
#define CE_IMPL_GLOBAL_GAME_H

#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "Engine/LinkedList.h"

#include <atomic>
#include <string>

#define CLOAKENGINE_CAN_COM_MAX_LEVEL 6

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Game {
				constexpr float DEFAULT_SLEEP_TIME = 1000000.0f / 20.0f;
				struct ThreadSleepInfo {
					float roundOffset = 0;
					API::Global::Time exptTime = 0;
				};
				struct ThreadFPSInfo {
					API::Global::Time lastTime = 0;
					API::Global::Time prevTime = 0;
					unsigned int turns = 0;
					unsigned int lastTurns = 0;
					float lastFPS = 0;
				};
				struct ThreadCanComInfo {
					BYTE curCanCom = CLOAKENGINE_CAN_COM_MAX_LEVEL;
					bool reduce = false;
				};
#ifdef _DEBUG
				typedef Engine::LinkedList<API::Helper::ISavePtr*>::const_iterator ptrAdr;
#endif
				enum class ThreadCanComRes { NOT_READY, UPDATED, FINISHED };
				DWORD CLOAK_CALL threadSleep(In API::Global::Time curTime, In float timeToSleep, Inout ThreadSleepInfo* info);
#ifdef _DEBUG
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
				Success(return == true)
				bool CLOAK_CALL getFPS(In size_t threadNum, Out_opt float* fps = nullptr, Out_opt unsigned int* tID = nullptr);
				ThreadCanComRes CLOAK_CALL checkThreadCommunicateLevel(Inout ThreadCanComInfo* info);
				void CLOAK_CALL updateCommunicateLevel(Inout ThreadCanComInfo* info);
				BYTE CLOAK_CALL getThreadCommunicateLevel(In const ThreadCanComInfo& info);
				unsigned long long CLOAK_CALL getCurrentTimeMilliSeconds();
				unsigned long long CLOAK_CALL getCurrentTimeMicroSeconds();
				float CLOAK_CALL threadFPS(In API::Global::Time curTime, Inout ThreadFPSInfo* info);
				void CLOAK_CALL onGameLangUpdate();
				BYTE CLOAK_CALL GetCurrentComLevel();
			}
		}
	}
}

#endif
