#pragma once
#ifndef CE_API_COMPONENTS_AUDIOSOURCE_H
#define CE_API_COMPONENTS_AUDIOSOURCE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/World/Entity.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Components {
			inline namespace AudioSource_v1 {
				CLOAK_COMPONENT("{32C84A3E-7603-4A5D-864F-C0EC0C97DE3C}", IAudioSource) {
					public:
						CLOAKENGINE_API CLOAK_CALL IAudioSource(In API::World::IEntity* e, In type_id typeID) : Component(e, typeID) {}
				};
			}
		}
	}
}

#pragma warning(pop)

#endif