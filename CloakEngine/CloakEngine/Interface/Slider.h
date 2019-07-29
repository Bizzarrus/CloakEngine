#pragma once
#ifndef CE_API_INTERFACE_SLIDER_H
#define CE_API_INTERFACE_SLIDER_H

#include "CloakEngine/Interface/Frame.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace Slider_v1 {
				CLOAK_INTERFACE_ID("{ECA3C42A-05B0-42FE-9496-6F54F3F5F422}") ISlider : public virtual Frame_v1::IFrame, public virtual Helper::IEventCollector<Global::Events::onEnterKey, Global::Events::onClick, Global::Events::onValueChange, Global::Events::onScroll>{
					public:
						virtual void CLOAK_CALL_THIS SetMin(In float min) = 0;
						virtual void CLOAK_CALL_THIS SetMax(In float max) = 0;
						virtual void CLOAK_CALL_THIS SetMinMax(In float min, In float max) = 0;
						virtual void CLOAK_CALL_THIS SetValue(In float val) = 0;
						virtual void CLOAK_CALL_THIS SetThumbTexture(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetLeftArrowTexture(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetRightArrowTexture(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetArrowWidth(In float w) = 0;
						virtual void CLOAK_CALL_THIS SetThumbWidth(In float w) = 0;
						virtual void CLOAK_CALL_THIS SetStepSize(In float ss) = 0;
						virtual float CLOAK_CALL_THIS GetStepSize() const = 0;
						virtual float CLOAK_CALL_THIS GetValue() const = 0;
						virtual float CLOAK_CALL_THIS GetMin() const = 0;
						virtual float CLOAK_CALL_THIS GetMax() const = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif