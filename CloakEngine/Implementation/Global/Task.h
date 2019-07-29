#pragma once
#ifndef CE_IMPL_GLOBAL_TASK_H
#define CE_IMPL_GLOBAL_TASK_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Task.h"

#include <functional>

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Task {
				//Can be used to speed up following calls of HelpWork/HelpWorkOnce. This value should not be modified and not be 
				//communicated across tasks or threads!
				CLOAK_INTERFACE_BASIC IHelpWorker {
					public:
						virtual bool CLOAK_CALL_THIS HelpWork(In_opt API::Global::Threading::Help help = API::Global::Threading::Help::Allways) = 0;
						//Keeps working until the given function returns true
						virtual void CLOAK_CALL_THIS HelpWorkUntil(In std::function<bool()> func, In_opt API::Global::Threading::Help help = API::Global::Threading::Help::Allways, In_opt bool ignoreLocks = false) = 0;
						//Keeps working while the given function returns true
						virtual void CLOAK_CALL_THIS HelpWorkWhile(In std::function<bool()> func, In_opt API::Global::Threading::Help help = API::Global::Threading::Help::Allways, In_opt bool ignoreLocks = false) = 0;
				};
				CLOAK_INTERFACE_BASIC IRunWorker{
					public:
						virtual bool CLOAK_CALL_THIS RunWork(In_opt API::Global::Threading::Help help = API::Global::Threading::Help::Allways) = 0;
						virtual bool CLOAK_CALL_THIS RunWorkOrWait(In_opt API::Global::Threading::Help help = API::Global::Threading::Help::Allways) = 0;
						virtual bool CLOAK_CALL_THIS RunWorkOrWait(In API::Global::Threading::Help help, In API::Global::Time waitEndMilliSeconds) = 0;
						virtual bool CLOAK_CALL_THIS RunWorkOrWait(In API::Global::Time waitEndMilliSeconds) = 0;
				};

				API::Global::Task CLOAK_CALL Sleep(In API::Global::Time milliSeconds, In API::Global::Threading::Kernel&& kernel);
				API::Global::Task CLOAK_CALL Sleep(In API::Global::Time milliSeconds, In const API::Global::Threading::Kernel& kernel);

				void* CLOAK_CALL Allocate(In size_t size);
				void* CLOAK_CALL TryResizeBlock(In void* ptr, In size_t size);
				void* CLOAK_CALL Reallocate(In void* ptr, In size_t size, In_opt bool moveData = true);
				void CLOAK_CALL Free(In void* ptr);
				size_t CLOAK_CALL GetMaxAlloc();

				IHelpWorker* CLOAK_CALL GetCurrentWorker();

				IRunWorker* CLOAK_CALL Initialize(In bool windowThread);
				void CLOAK_CALL Release();
				void CLOAK_CALL Finish();
			}
		}
	}
}

#endif