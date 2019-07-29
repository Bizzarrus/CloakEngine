#pragma once
#ifndef CE_API_INTERFACE_MOVEABLEFRAME_H
#define CE_API_INTERFACE_MOVEABLEFRAME_H

#include "CloakEngine/Interface/Frame.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace MoveableFrame_v1 {
				CLOAK_INTERFACE_ID("{9D860326-7FFD-4385-B778-9AAB04296995}") IMoveableFrame : public virtual IFrame{
					public:
						virtual void CLOAK_CALL_THIS Move(In float X, In float Y) = 0;
						virtual void CLOAK_CALL_THIS ResetPos() = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif
