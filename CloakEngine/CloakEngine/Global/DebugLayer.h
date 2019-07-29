#pragma once
#ifndef CE_API_GLOBAL_DEBUGLAYER_H
#define CE_API_GLOBAL_DEBUGLAYER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			inline namespace DebugLayer_v1 {
				CLOAK_INTERFACE_ID("{DA997F8B-5695-4D0B-A0BB-FFC690F74061}") IDebugLayer : public virtual Helper::SavePtr_v1::ISavePtr {
					public:
						
				};
			}
		}
	}
}

#endif