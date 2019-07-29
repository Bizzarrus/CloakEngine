#pragma once
#ifndef CE_API_RENDERING_QUERY_H
#define CE_API_RENDERING_QUERY_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Rendering/Defines.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace Query_v1 {
				CLOAK_INTERFACE_ID("{EB71CA4D-5DF3-49BE-BD72-18BA892ED1A9}") IQuery : public virtual API::Helper::SavePtr_v1::ISavePtr {
					public:
						virtual QUERY_TYPE CLOAK_CALL_THIS GetType() const = 0;
						virtual const QUERY_DATA& CLOAK_CALL_THIS GetData() = 0;
				};
			}
		}
	}
}

#endif