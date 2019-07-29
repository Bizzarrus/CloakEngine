#pragma once
#ifndef CE_IMPL_INTERFACE_BASICGUI_H
#define CE_IMPL_INETRFACE_BASICGUI_H

#include "CloakEngine/Interface/BasicGUI.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Helper/EventBase.h"

#include "Engine/LinkedList.h"

#include <atomic>

#define GUI_SET_TEXTURE(img,target) if(target!=img){if(target!=nullptr){target->unload(); target->Release();} target=img; if(target!=nullptr){target->AddRef(); target->RequestLOD(API::Files::LOD::ULTRA); target->load();}API::Interface::registerForUpdate(this);}

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			void CLOAK_CALL Initialize();
			void CLOAK_CALL Release();
			API::Interface::IBasicGUI** CLOAK_CALL GetVisibleGUIs(In API::Global::Memory::PageStackAllocator* alloc, Out size_t* count);
			size_t CLOAK_CALL BeginToUpdate();
			API::Interface::IBasicGUI* CLOAK_CALL GetUpdateGUI(In size_t s);
			void CLOAK_CALL EndToUpdate();
			inline namespace BasicGUI_v1 {
				class CLOAK_UUID("{30D96485-EA10-4A29-A1F4-C445292C46A5}") BasicGUI : public virtual API::Interface::BasicGUI_v1::IBasicGUI, public Helper::SavePtr_v1::SavePtr, IMPL_EVENT(onGainFocus), IMPL_EVENT(onResize), IMPL_EVENT(onLooseFocus), IMPL_EVENT(onParentChange), IMPL_EVENT(onMouseEnter), IMPL_EVENT(onMouseLeave) {
					public:
						CLOAK_CALL_THIS BasicGUI();
						virtual CLOAK_CALL_THIS ~BasicGUI();
						virtual void CLOAK_CALL_THIS Show() override;
						virtual void CLOAK_CALL_THIS Hide() override;
						virtual void CLOAK_CALL_THIS SetParent(In_opt API::Interface::BasicGUI_v1::IBasicGUI* parent) override;
						virtual void CLOAK_CALL_THIS SetSize(In float width, In float height, In_opt API::Interface::SizeType type = API::Interface::SizeType::NORMAL) override;
						virtual void CLOAK_CALL_THIS SetAnchor(In const API::Interface::BasicGUI_v1::Anchor& a) override;
						virtual void CLOAK_CALL_THIS SetFocus() override;
						virtual void CLOAK_CALL_THIS LooseFocus() override;
						virtual void CLOAK_CALL_THIS AddChild(In API::Interface::BasicGUI_v1::IBasicGUI* child) override;
						virtual void CLOAK_CALL_THIS UpdatePosition() override;
						virtual void CLOAK_CALL_THIS SetRotation(In float alpha) override;
						virtual void CLOAK_CALL_THIS SetZ(In UINT Z) override;
						virtual bool CLOAK_CALL_THIS IsVisible() const override;
						virtual bool CLOAK_CALL_THIS IsVisibleOnScreen() const override;
						virtual bool CLOAK_CALL_THIS IsOnScreen() const override;
						virtual bool CLOAK_CALL_THIS IsClickable() const override;
						virtual bool CLOAK_CALL_THIS IsFocused() const override;
						virtual bool CLOAK_CALL_THIS IsEnabled() const override;
						virtual float CLOAK_CALL_THIS GetRotation() const override;
						virtual float CLOAK_CALL_THIS GetWidth() const override;
						virtual float CLOAK_CALL_THIS GetHeight() const override;
						Ret_maybenull virtual API::Interface::BasicGUI_v1::IBasicGUI* CLOAK_CALL_THIS GetParent() const override;
						virtual const API::Interface::BasicGUI_v1::Anchor& CLOAK_CALL_THIS GetAnchor() const override;
						virtual API::Global::Math::Space2D::Point CLOAK_CALL_THIS GetPositionOnScreen() const override;
						virtual API::Global::Math::Space2D::Point CLOAK_CALL_THIS GetRelativePos(In const API::Global::Math::Space2D::Point& pos) const override;

						virtual void CLOAK_CALL_THIS Draw(In API::Rendering::Context_v1::IContext2D* context) override;
						virtual void CLOAK_CALL_THIS UpdateDrawInformations(In API::Global::Time time, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager) override;
						virtual bool CLOAK_CALL_THIS SetUpdateFlag() override;

						virtual void CLOAK_CALL_THIS RegisterChild(In API::Interface::BasicGUI_v1::IBasicGUI*);
						virtual void CLOAK_CALL_THIS RegisterAnchChild(In API::Interface::BasicGUI_v1::IBasicGUI*);
						virtual void CLOAK_CALL_THIS UnregisterChild(In API::Interface::BasicGUI_v1::IBasicGUI*);
						virtual void CLOAK_CALL_THIS UnregisterAnchChild(In API::Interface::BasicGUI_v1::IBasicGUI*);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						virtual void CLOAK_CALL_THIS iHide();
						virtual bool CLOAK_CALL_THIS iUpdateDrawInfo(In unsigned long long time);
						virtual void CLOAK_CALL_THIS iDraw(In API::Rendering::Context_v1::IContext2D* context);

						bool m_visible;
						std::atomic<bool> m_updateFlag;
						float m_W;
						float m_H;
						float m_Width;
						float m_Height;
						API::Interface::SizeType m_sizeType;
						UINT m_Z;
						UINT m_ZSize;
						API::Global::Math::Space2D::Point m_pos;
						API::Interface::BasicGUI_v1::Anchor m_anch;
						API::Interface::BasicGUI_v1::IBasicGUI* m_parent;
						API::Helper::ISyncSection* m_sync;
						API::Helper::ISyncSection* m_syncDraw;
						Engine::LinkedList<API::Interface::BasicGUI_v1::IBasicGUI*> m_childs;
						Engine::LinkedList<API::Interface::BasicGUI_v1::IBasicGUI*> m_anchChilds;
						struct {
							float alpha;
							float sin_a;
							float cos_a;
						} m_rotation;

						UINT m_d_Z;
						UINT m_d_ZSize;
						size_t m_d_childSize;
						bool m_d_visible;
						bool m_d_ios;
						API::Interface::BasicGUI_v1::IBasicGUI** m_d_childs;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif