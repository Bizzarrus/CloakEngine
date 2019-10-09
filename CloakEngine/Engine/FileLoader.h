#pragma once
#ifndef CE_E_FILELOADER_H
#define CE_E_FILELOADER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Files/FileHandler.h"
#include "CloakEngine/Global/Graphic.h"

#include "Engine/LinkedList.h"

#include <atomic>

namespace CloakEngine {
	namespace Engine {
		namespace FileLoader {
			inline namespace FileLoader_v2 {
				enum class LoadType : uint8_t { NONE = 0, LOAD = 1, UNLOAD = 2, RELOAD = 3, DELAYED = 4, PROCESSING = 5 };
				struct LoadState {
					LoadType Type;
					API::Files::Priority Priority;
				};

				class ILoad : public virtual API::Helper::ISavePtr {
					public:
						CLOAK_CALL ILoad();
						virtual CLOAK_CALL ~ILoad();

						bool CLOAK_CALL_THIS CheckLoadAction(In size_t threadID, In uint64_t stateID);
						bool CLOAK_CALL_THIS CanBeDequeued() const;
						void CLOAK_CALL_THIS ExecuteLoadAction(In size_t threadID, In const LoadState& state, In API::Global::Threading::Priority tp);
						void CLOAK_CALL_THIS CheckSettings(In const API::Global::Graphic::Settings& nset);
					protected:
						void CLOAK_CALL_THIS iRequestLoad(In_opt API::Files::Priority prio = API::Files::Priority::LOW);
						void CLOAK_CALL_THIS iRequestUnload(In_opt API::Files::Priority prio = API::Files::Priority::LOW);
						void CLOAK_CALL_THIS iRequestReload(In_opt API::Files::Priority prio = API::Files::Priority::LOW);
						void CLOAK_CALL_THIS iRequestDelayedLoad(In_opt API::Files::Priority prio = API::Files::Priority::LOW);
						void CLOAK_CALL_THIS iSetPriority(In API::Files::Priority p);

						virtual bool CLOAK_CALL_THIS iCheckSetting(In const API::Global::Graphic::Settings& nset) const;
						virtual bool CLOAK_CALL_THIS iLoadFile(Out bool* keepLoadingBackground) = 0;
						virtual void CLOAK_CALL_THIS iUnloadFile() = 0;
						virtual void CLOAK_CALL_THIS iDelayedLoadFile(Out bool* keepLoadingBackground) = 0;
						std::atomic<bool> m_isLoaded;
					private:
						void CLOAK_CALL_THIS iQueueRequest(In LoadType type, In API::Files::Priority priority);
						bool CLOAK_CALL_THIS iProcessLoadAction(In size_t threadID);
						void CLOAK_CALL_THIS iPushInQueue(In API::Files::Priority priority);

						std::atomic<uint64_t> m_stateID;
						std::atomic<LoadState> m_state;
						std::atomic<bool> m_canBeQueued;
						std::atomic<bool> m_canDequeue;
						std::atomic<Engine::LinkedList<ILoad*>::const_iterator> m_ptr;
				};

				void CLOAK_CALL onStart();
				void CLOAK_CALL onStartLoading();
				void CLOAK_CALL waitOnLoading();
				void CLOAK_CALL onStop();
				void CLOAK_CALL updateSetting(In const API::Global::Graphic::Settings& nset);
				bool CLOAK_CALL isLoading(In_opt API::Files::Priority prio);
			}
		}
	}
}

#endif