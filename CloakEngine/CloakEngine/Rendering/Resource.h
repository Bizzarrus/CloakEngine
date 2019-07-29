#pragma once
#ifndef CE_API_RENDERING_RESOURCE_H
#define CE_API_RENDERING_RESOURCE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Rendering/Defines.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace Resource_v1 {
				CLOAK_INTERFACE_ID("{59F43BCB-7629-4AC6-9E0C-57EDB37ADC24}") IResource : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						
				};
			}
		}
	}
}

#endif
