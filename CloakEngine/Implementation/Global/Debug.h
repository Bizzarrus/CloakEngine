#pragma once
#ifndef CE_IMPL_GLOBAL_DEBUG_H
#define CE_IMPL_GLOBAL_DEBUG_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Global/GameEvents.h"
#include "Engine/LinkedList.h"

#include <string>

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Debug {
				void CLOAK_CALL writeToConsole(In const std::string& text, In_opt API::Global::Log::Source src = API::Global::Log::Source::Engine);
				void CLOAK_CALL updateConsole(In size_t threadID, In API::Global::Time etime);
				void CLOAK_CALL releaseConsole(In size_t threadID);
				void CLOAK_CALL enableConsole(In size_t threadID);
				void CLOAK_CALL setDebugEvent(In API::Global::IDebugEvent* ev);
				API::Global::Debug::Error CLOAK_CALL getLastError();
				void CLOAK_CALL Initialize();
				void CLOAK_CALL Release();

#ifdef _DEBUG
				struct ThreadDebugData {
					bool print = false;
					unsigned int step = 0;
					int line = 0;
					std::string file = "";
					unsigned long long time = 0;
				};
				typedef Engine::LinkedList<ThreadDebugData>::iterator ThreadDebug;
				ThreadDebug CLOAK_CALL StartThreadDebug(In unsigned int step, In_opt std::string file = "", In_opt int line = 0);
				void CLOAK_CALL StepThreadDebug(In ThreadDebug debug, In unsigned int step, In_opt int line = 0);
				void CLOAK_CALL StopThreadDebug(In ThreadDebug debug);
#endif
			}
		}
	}
}

//#define NO_THREAD_DEBUG
#if defined(_DEBUG) && !defined(NO_THREAD_DEBUG)
#define DebugThreadStart(step) const CloakEngine::Impl::Global::Debug::ThreadDebug __ce_thdb_=CloakEngine::Impl::Global::Debug::StartThreadDebug(step,__FILE__,__LINE__); unsigned int __ce_thdbs_=step
#define DebugThreadStep(step) CloakEngine::Impl::Global::Debug::StepThreadDebug(__ce_thdb_,step,__LINE__); __ce_thdbs_=step
#define DebugThreadNext() CloakEngine::Impl::Global::Debug::StepThreadDebug(__ce_thdb_,++__ce_thdbs_,__LINE__)
#define DebugThreadStop() CloakEngine::Impl::Global::Debug::StopThreadDebug(__ce_thdb_)
#else
#define DebugThreadStart(step)
#define DebugThreadStep(step)
#define DebugThreadNext()
#define DebugThreadStop()
#endif

#endif