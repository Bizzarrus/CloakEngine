#pragma once
#ifndef CE_IMPL_INTERFACE_SLIDER_H
#define CE_IMPL_INTERFACE_SLIDER_H

#include "CloakEngine/Interface/Slider.h"
#include "CloakEngine/Interface/Button.h"
#include "Implementation/Interface/Frame.h"

#include <atomic>

#pragma warning(push)
#pragma warning(disable:4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace Slider_v1 {
				class CLOAK_UUID("{C6D70056-66EB-4725-B30F-4D64A0707C98}") Slider : public virtual API::Interface::Slider_v1::ISlider, public Frame_v1::Frame, IMPL_EVENT(onClick), IMPL_EVENT(onEnterKey), IMPL_EVENT(onValueChange), IMPL_EVENT(onScroll) {
					public:
						CLOAK_CALL_THIS Slider();
						virtual CLOAK_CALL_THIS ~Slider();
						virtual void CLOAK_CALL_THIS SetMin(In float min) override;
						virtual void CLOAK_CALL_THIS SetMax(In float max) override;
						virtual void CLOAK_CALL_THIS SetMinMax(In float min, In float max) override;
						virtual void CLOAK_CALL_THIS SetValue(In float val) override;
						virtual void CLOAK_CALL_THIS SetThumbTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetLeftArrowTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetRightArrowTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetArrowWidth(In float w) override;
						virtual void CLOAK_CALL_THIS SetThumbWidth(In float w) override;
						virtual void CLOAK_CALL_THIS SetStepSize(In float ss) override;
						virtual float CLOAK_CALL_THIS GetStepSize() const override;
						virtual float CLOAK_CALL_THIS GetValue() const override;
						virtual float CLOAK_CALL_THIS GetMin() const override;
						virtual float CLOAK_CALL_THIS GetMax() const override;
						virtual bool CLOAK_CALL_THIS IsClickable() const override;

						virtual void CLOAK_CALL_THIS Initialize() override;

						virtual void CLOAK_CALL_THIS Draw(In API::Rendering::Context_v1::IContext2D* context) override;
						void CLOAK_CALL_THIS DragThumb(In const API::Global::Events::onClick& click);
						void CLOAK_CALL_THIS SetThumbPos(In const API::Global::Events::onClick& click);
						virtual void CLOAK_CALL_THIS OnScroll(In const API::Global::Events::onScroll& ev);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						float m_min;
						float m_max;
						float m_val;
						float m_arrW;
						float m_thumbW;
						float m_step;
						API::Interface::IButton* m_thumb;
						API::Interface::IButton* m_lArrow;
						API::Interface::IButton* m_rArrow;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif