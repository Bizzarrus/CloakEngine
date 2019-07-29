#pragma once
#ifndef CE_IMPL_INTERFACE_LOADSCREEN_H
#define CE_IMPL_INTERFACE_LOADSCREEN_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Interface/LoadScreen.h"
#include "CloakEngine/Helper/SyncSection.h"

#include "Implementation/Helper/SavePtr.h"

#include "Engine/LinkedList.h"

#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace LoadScreen_v1 {
				class CLOAK_UUID("{1B24684F-0688-41E1-8302-80EC18C95BF7}") LoadScreen : public virtual API::Interface::ILoadScreen, public Helper::SavePtr_v1::SavePtr {
					public:
						static void CLOAK_CALL Initialize();
						static void CLOAK_CALL Terminate();
						static void CLOAK_CALL ShowRandom(In API::Rendering::Context_v1::IContext2D* context);
						static void CLOAK_CALL Hide();

						CLOAK_CALL LoadScreen();
						virtual CLOAK_CALL ~LoadScreen();
						virtual void CLOAK_CALL_THIS SetGUI(In API::Interface::IBasicGUI* gui) override;
						virtual void CLOAK_CALL_THIS SetBackgroundImage(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS AddToPool() override;
						virtual void CLOAK_CALL_THIS RemoveFromPool() override;

						virtual void CLOAK_CALL_THIS Draw(In API::Rendering::Context_v1::IContext2D* context) override;
						virtual void CLOAK_CALL_THIS UpdateDrawInformations(In API::Global::Time time, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager) override;
						virtual bool CLOAK_CALL_THIS SetUpdateFlag() override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS iShow();
						virtual void CLOAK_CALL_THIS iHide();

						API::Interface::IBasicGUI* m_gui;
						API::Files::ITexture* m_img;
						API::Helper::ISyncSection* m_sync;
						API::Rendering::VERTEX_2D m_vertex;
						Engine::LinkedList<LoadScreen*>::const_iterator m_poolPos;
						std::atomic<bool> m_updateFlag;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif
