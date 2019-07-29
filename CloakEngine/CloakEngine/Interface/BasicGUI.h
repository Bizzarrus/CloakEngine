#pragma once
#ifndef CE_API_INTERFACE_BASICGUI_H
#define CE_API_INTERFACE_BASICGUI_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Helper/EventBase.h"
#include "CloakEngine/Global/Graphic.h"
#include "CloakEngine/Global/Input.h"
#include "CloakEngine/Rendering/Context.h"
#include "CloakEngine/Helper/SavePtr.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface { inline namespace BasicGUI_v1 { class IBasicGUI; } }
		namespace Global {
			namespace Events {
				struct onResize {
					float newWidth;
					float newHeight;
				};
				struct onLooseFocus {
					Interface::IBasicGUI* nextFocus;
				};
				struct onGainFocus {
					Interface::IBasicGUI* lastFocus;
				};
				struct onParentChange {
					bool anchor;
					Interface::IBasicGUI* lastParent;
					Interface::IBasicGUI* nextParent;
				};
				struct onEnterChar {
					char16_t c;
				};
				struct onEnterKey {
					Input::KeyCode key;
					Input::KEY_STATE pressTpye;
					Input::User userID;
				};
				struct onClick {
					Input::MOUSE_STATE pressType;
					Input::MOUSE_STATE previousType;
					Input::KeyCode key;
					Input::KeyCode activeKeys;
					Input::User userID;
					Math::Space2D::Point clickPos;
					Math::Space2D::Point relativePos;
					Math::Space2D::Vector moveDir;
				};
				struct onScroll {
					Input::SCROLL_STATE button;
					Math::Space2D::Point clickPos;
					Math::Space2D::Point relativePos;
					float delta;
					Input::User userID;
				};
				struct onMouseEnter {
					const Interface::IBasicGUI* last;
					Math::Space2D::Point pos;
					Math::Space2D::Point relativePos;
				};
				struct onMouseLeave {
					const Interface::IBasicGUI* next;
				};
				struct onValueChange {
					float newValue;
				};
				struct onCheck {
					bool check;
				};
			}
		}
		namespace Interface {
			inline namespace BasicGUI_v1 {
				enum class AnchorPoint {
					CENTER = 0x0,
					LEFT = 0x1,
					RIGHT = 0x2,
					TOP = 0x4,
					BOTTOM = 0x8,
					TOPLEFT = 0x5,
					TOPRIGHT = 0x6,
					BOTTOMLEFT = 0x9,
					BOTTOMRIGHT = 0xa,
				};
				enum class SizeType {
					NORMAL,
					RELATIVE_WIDTH,
					RELATIVE_HEIGHT
				};
				struct Anchor;
				CLOAK_INTERFACE_ID("{2E3DD009-6472-4AAB-9F45-F5B1DE10D309}") IBasicGUI : public virtual Rendering::IDrawable2D, public virtual Helper::SavePtr_v1::ISavePtr, public virtual Helper::IEventCollector<Global::Events::onGainFocus, Global::Events::onResize, Global::Events::onLooseFocus, Global::Events::onParentChange, Global::Events::onMouseEnter, Global::Events::onMouseLeave>{
					public:
						/** This will make the GUI visible*/
						virtual void CLOAK_CALL_THIS Show() = 0;
						/** This will make the GUI invisible*/
						virtual void CLOAK_CALL_THIS Hide() = 0;
						virtual void CLOAK_CALL_THIS SetParent(In_opt IBasicGUI* parent) = 0;
						virtual void CLOAK_CALL_THIS SetSize(In float width, In float height, In_opt SizeType type = SizeType::NORMAL) = 0;
						virtual void CLOAK_CALL_THIS SetAnchor(In const Anchor& a) = 0;
						virtual void CLOAK_CALL_THIS SetFocus() = 0;
						virtual void CLOAK_CALL_THIS SetRotation(In float alpha) = 0;
						virtual void CLOAK_CALL_THIS SetZ(In UINT Z) = 0;
						virtual void CLOAK_CALL_THIS LooseFocus() = 0;
						virtual void CLOAK_CALL_THIS AddChild(In IBasicGUI* child) = 0;
						virtual void CLOAK_CALL_THIS UpdatePosition() = 0;
						/** This will check if the GUI is (theoretical) visible or not*/
						virtual bool CLOAK_CALL_THIS IsVisible() const = 0;
						/** This will check if the GUI will considered to be drawn*/
						virtual bool CLOAK_CALL_THIS IsVisibleOnScreen() const = 0;
						/** This will check if the GUI is actually inside the screen*/
						virtual bool CLOAK_CALL_THIS IsOnScreen() const = 0;
						virtual bool CLOAK_CALL_THIS IsClickable() const = 0;
						virtual bool CLOAK_CALL_THIS IsFocused() const = 0;
						virtual bool CLOAK_CALL_THIS IsEnabled() const = 0;
						virtual float CLOAK_CALL_THIS GetRotation() const = 0;
						virtual float CLOAK_CALL_THIS GetWidth() const = 0;
						virtual float CLOAK_CALL_THIS GetHeight() const = 0;
						Ret_maybenull virtual IBasicGUI* CLOAK_CALL_THIS GetParent() const = 0;
						virtual const Anchor& CLOAK_CALL_THIS GetAnchor() const = 0;
						virtual Global::Math::Space2D::Point CLOAK_CALL_THIS GetPositionOnScreen() const = 0;
						virtual API::Global::Math::Space2D::Point CLOAK_CALL_THIS GetRelativePos(In const API::Global::Math::Space2D::Point& pos) const = 0;
				};
				struct Anchor {
					AnchorPoint point = AnchorPoint::TOPLEFT;
					AnchorPoint relativePoint = AnchorPoint::TOPLEFT;
					IBasicGUI* relativeTarget = nullptr;
					Global::Math::Space2D::Vector offset;
				};
			}
			Ret_maybenull CLOAKENGINE_API IBasicGUI* CLOAK_CALL getFocused();
			CLOAKENGINE_API void CLOAK_CALL setShown(In IBasicGUI* gui, In bool visible);
			CLOAKENGINE_API void CLOAK_CALL registerForUpdate(In IBasicGUI* gui);
		}
	}
}

#pragma warning(pop)

#endif