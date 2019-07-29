#pragma once
#ifndef CE_IMPL_GLOBAL_INPUT_H
#define CE_IMPL_GLOBAL_INPUT_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Input.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "Implementation/Helper/SavePtr.h"
#include "CloakEngine/Interface/BasicGUI.h"

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Input {
				constexpr size_t MAX_USER_COUNT = 4;
#pragma warning(push)
#pragma warning(disable: 4250)

				class KeyBinding : public API::Global::Input::IKeyBinding {
					public:
						CLOAK_CALL KeyBinding(In size_t slotCount, In bool exact, In API::Global::Input::KeyBindingFunc&& onClick);
						CLOAK_CALL KeyBinding(In const KeyBinding& k);
						CLOAK_CALL KeyBinding(In KeyBinding&& k);
						CLOAK_CALL ~KeyBinding();
						void CLOAK_CALL_THIS SetKeys(In size_t slot, In const API::Global::Input::KeyCode& keys) override;
						const API::Global::Input::KeyCode& CLOAK_CALL_THIS GetKeys(In size_t slot) const override;

						void CLOAK_CALL_THIS CheckKeys(In const API::Global::Input::KeyCode& nexthold, In DWORD user);
						KeyBinding& CLOAK_CALL_THIS operator=(In const KeyBinding& k);
						KeyBinding& CLOAK_CALL_THIS operator=(In KeyBinding&& k);
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						size_t m_slotCount;
						bool m_exact;
						bool m_entered;
						API::Helper::ISyncSection* m_sync = nullptr;
						API::Global::Input::KeyCode* m_keys;
						API::Global::Input::KeyBindingFunc m_func;
				};
				class CLOAK_UUID("{1258DDB7-4ED4-4462-8B9A-BF477E1ECF45}") KeyBindingPage : public virtual API::Global::Input::IKeyBindingPage, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL KeyBindingPage(In_opt KeyBindingPage* parent = nullptr, In_opt API::Global::Time timeout = 0, In_opt API::Global::Input::KeyPageFunc func = nullptr);
						CLOAK_CALL ~KeyBindingPage();
						CE::RefPointer<API::Global::Input::IKeyBindingPage> CLOAK_CALL_THIS CreateNewPage(In_opt API::Global::Time timeout = 0, In_opt API::Global::Input::KeyPageFunc func = nullptr) override;
						void CLOAK_CALL_THIS Enter(In API::Global::Input::User user) override;
						API::Global::Input::IKeyBinding* CLOAK_CALL_THIS CreateKeyBinding(In size_t slotCount, In bool exact, In API::Global::Input::KeyBindingFunc onKeyCode) override;
						
						void CLOAK_CALL_THIS Update(In API::Global::Time etime, In DWORD user);
						void CLOAK_CALL_THIS OnLeave(In DWORD user);
						void CLOAK_CALL_THIS CheckKeys(In const API::Global::Input::KeyCode& nexthold, In DWORD user);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						KeyBindingPage* const m_parent;
						const API::Global::Time m_timeout;
						API::Helper::ISyncSection* m_sync = nullptr;
						API::Global::Time m_enterTime[MAX_USER_COUNT];
						API::List<KeyBinding*> m_keyBindings;
						API::Global::Input::KeyPageFunc m_func;
				};

#pragma warning(pop)

				void CLOAK_CALL SetInputHandler(In API::Global::IInputEvent* handle);
				void CLOAK_CALL LooseFocus();
				void CLOAK_CALL OnInitialize();
				void CLOAK_CALL onLoad(In size_t threadID);
				void CLOAK_CALL onUpdate(In size_t threadID, In API::Global::Time etime);
				void CLOAK_CALL onStop(In size_t threadID);
				void CLOAK_CALL OnRelease();
				void CLOAK_CALL OnWindowClose();
				void CLOAK_CALL OnKeyEvent(In const API::Global::Input::KeyCode& key, In bool down, In DWORD user);
				void CLOAK_CALL OnKeyEvent(In const API::Global::Input::KeyCode& down, In const API::Global::Input::KeyCode& up, In DWORD user);
				void CLOAK_CALL OnResetKeys(In DWORD user);
				void CLOAK_CALL OnConnect(In DWORD user, In bool connected);
				void CLOAK_CALL OnMouseMoveEvent(In const API::Global::Math::Space2D::Point& pos, In const API::Global::Math::Space2D::Vector& move, In bool visible, In DWORD user);
				void CLOAK_CALL OnMouseSetEvent(In const API::Global::Math::Space2D::Point& pos, In bool visible, In DWORD user);
				void CLOAK_CALL OnThumbMoveEvent(In const API::Global::Math::Space2D::Point& left, In const API::Global::Math::Space2D::Point& right, In DWORD user);
				void CLOAK_CALL OnMouseScrollEvent(In float delta, In API::Global::Input::SCROLL_STATE button, In DWORD user);
				void CLOAK_CALL OnDoubleClick(In const API::Global::Input::KeyCode& key, In DWORD user);
				void CLOAK_CALL OnUpdateFocus(In API::Interface::IBasicGUI* lastFocus, In API::Interface::IBasicGUI* nextFocus);
			}
		}
	}
}

#endif