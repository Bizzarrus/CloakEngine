#pragma once
#ifndef CE_API_INTERFACE_PROGRESSBAR_H
#define CE_API_INTERFACE_PROGRESSBAR_H

#include "CloakEngine/Interface/Frame.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace ProgressBar_v1 {
				CLOAK_INTERFACE_ID("{B5133B0D-F432-4E4E-9D10-41481CF324AA}") IProgressBar : public virtual Frame_v1::IFrame {
					public:
						virtual void CLOAK_CALL_THIS SetProgressImage(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetProgress(In In_range(0, 1) float progress) = 0;
						virtual float CLOAK_CALL_THIS GetProgress() const = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif