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
			class ILoad;
			enum class LoadType { NONE, LOAD, UNLOAD, RELOAD, DELAYED };
			struct loadInfo {
				ILoad* obj;
			};

			class ILoad : public virtual API::Helper::ISavePtr {
				public:
					CLOAK_CALL_THIS ILoad();
					virtual CLOAK_CALL_THIS ~ILoad();
					virtual bool CLOAK_CALL_THIS onLoad();
					virtual void CLOAK_CALL_THIS onUnload();
					virtual bool CLOAK_CALL_THIS onDelayedLoad();
					virtual void CLOAK_CALL_THIS setPriority(In API::Files::Priority p, In Engine::LinkedList<loadInfo>::const_iterator ptr);
					virtual void CLOAK_CALL_THIS setWaitForLoad();
					virtual void CLOAK_CALL_THIS updateSetting(In const API::Global::Graphic::Settings& nset);
					
					void CLOAK_CALL_THIS onStartLoadAction(In_opt bool loadBackground = false);
					void CLOAK_CALL_THIS onProcessLoadAction(In LoadType loadType);
				protected:
					virtual void CLOAK_CALL_THIS iRequestLoad(In_opt API::Files::Priority prio = API::Files::Priority::LOW);
					virtual void CLOAK_CALL_THIS iRequestUnload(In_opt API::Files::Priority prio = API::Files::Priority::LOW);
					virtual void CLOAK_CALL_THIS iRequestReload(In_opt API::Files::Priority prio = API::Files::Priority::LOW);
					virtual void CLOAK_CALL_THIS iRequestDelayedLoad(In_opt API::Files::Priority prio = API::Files::Priority::LOW);
					virtual bool CLOAK_CALL_THIS iCheckSetting(In const API::Global::Graphic::Settings& nset) const;
					virtual bool CLOAK_CALL_THIS iLoadFile(Out bool* keepLoadingBackground) = 0;
					virtual void CLOAK_CALL_THIS iUnloadFile() = 0;
					virtual void CLOAK_CALL_THIS iDelayedLoadFile(Out bool* keepLoadingBackground) = 0;
					void CLOAK_CALL_THIS iSetPriority(In API::Files::Priority p);
					std::atomic<bool> m_isLoaded;
				private:
					std::atomic<LoadType> m_loadAction;
					std::atomic<bool> m_loaded;
					std::atomic<bool> m_loading;
					std::atomic<bool> m_waitForLoad;
					std::atomic<API::Files::Priority> m_priority;
					std::atomic<Engine::LinkedList<loadInfo>::const_iterator> m_loadPtr;
					Engine::LinkedList<ILoad*>::const_iterator m_ptr;
			};
			void CLOAK_CALL onStart();
			void CLOAK_CALL onStop();
			void CLOAK_CALL updateSetting(In const API::Global::Graphic::Settings& nset);
			bool CLOAK_CALL isLoading(In_opt API::Files::Priority prio = API::Files::Priority::NORMAL);
		}
	}
}

#endif