#pragma once
#ifndef CE_API_INTERFACE_LOADSCREEN_H
#define CE_API_INTERFACE_LOADSCREEN_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Interface/BasicGUI.h"
#include "CloakEngine/Files/Image.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace LoadScreen_v1 {
				CLOAK_INTERFACE_ID("{C38A51FD-CF60-4436-8B18-CAE498E36B8C}") ILoadScreen : public virtual Rendering::IDrawable2D, public virtual Helper::SavePtr_v1::ISavePtr{
					public:
						virtual void CLOAK_CALL_THIS SetGUI(In IBasicGUI* gui) = 0;
						virtual void CLOAK_CALL_THIS SetBackgroundImage(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS AddToPool() = 0;
						virtual void CLOAK_CALL_THIS RemoveFromPool() = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif
