#pragma once
#ifndef CE_API_COMPONENTS_POSITION_H
#define CE_API_COMPONENTS_POSITION_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/World/Entity.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Files/Scene.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Components {
			inline namespace Position_v1 {
				CLOAK_COMPONENT("{66E65DA6-E184-46BF-BAA2-8D0A67D035BB}", IPosition) {
					public:
						CLOAKENGINE_API CLOAK_CALL IPosition(In API::World::IEntity* e, In type_id typeID) : Component(e, typeID) {}
						virtual CLOAKENGINE_API API::Global::Math::WorldPosition CLOAK_CALL_THIS GetPosition() const = 0;
						virtual CLOAKENGINE_API API::Global::Math::Vector CLOAK_CALL_THIS GetLocalPosition() const = 0;
						virtual CLOAKENGINE_API API::Global::Math::Quaternion CLOAK_CALL_THIS GetRotation() const = 0;
						virtual CLOAKENGINE_API API::Global::Math::Quaternion CLOAK_CALL_THIS GetLocalRotation() const = 0;
						virtual CLOAKENGINE_API API::Global::Math::Vector CLOAK_CALL_THIS GetScale() const = 0;
						virtual CLOAKENGINE_API API::Global::Math::Vector CLOAK_CALL_THIS GetLocalScale() const = 0;
						virtual CLOAKENGINE_API API::Files::IWorldNode* CLOAK_CALL_THIS GetNode() const = 0;
						virtual CLOAKENGINE_API API::RefPointer<IPosition> CLOAK_CALL_THIS GetParent() const = 0;
						
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS SetParent(In IPosition* par) = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif