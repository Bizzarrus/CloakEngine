#pragma once
#ifndef CE_API_COMPONENTS_MOVEABLE_H
#define CE_API_COMPONENTS_MOVEABLE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Components/Position.h"
#include "CloakEngine/Global/Math.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Components {
			inline namespace Moveable_v1 {
				class CLOAK_UUID("{36BDFC4D-7EB9-49F8-80A7-C0C44831EBA9}") IMoveable : public virtual Components::Position_v1::IPosition {
					public:
						typedef Components::Position_v1::IPosition Super;

						CLOAKENGINE_API CLOAK_CALL IMoveable(In API::World::IEntity* e, In type_id typeID) : IPosition(e, typeID), Component(e, typeID) {}
						virtual void CLOAK_CALL_THIS SetPosition(In const API::Global::Math::WorldPosition& pos) = 0;
						virtual void CLOAK_CALL_THIS SetPosition(In float X, In float Y, In float Z) = 0;
						virtual void CLOAK_CALL_THIS SetPosition(In float X, In float Y, In float Z, In int32_t nX, In int32_t nY, In int32_t nZ) = 0;
						virtual void CLOAK_CALL_THIS SetRotation(In const API::Global::Math::Quaternion& rot) = 0;
						virtual void CLOAK_CALL_THIS SetRotation(In const API::Global::Math::Vector& axis, In float angle) = 0;
						virtual void CLOAK_CALL_THIS SetRotation(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order) = 0;
						virtual void CLOAK_CALL_THIS SetScale(In const API::Global::Math::Vector& scale) = 0;
						virtual void CLOAK_CALL_THIS SetScale(In float scale) = 0;
						virtual void CLOAK_CALL_THIS SetScale(In float X, In float Y, In float Z) = 0;
						virtual void CLOAK_CALL_THIS SetLocalPosition(In const API::Global::Math::Vector& pos) = 0;
						virtual void CLOAK_CALL_THIS SetLocalPosition(In float X, In float Y, In float Z) = 0;
						virtual void CLOAK_CALL_THIS SetLocalRotation(In const API::Global::Math::Quaternion& rot) = 0;
						virtual void CLOAK_CALL_THIS SetLocalRotation(In const API::Global::Math::Vector& axis, In float angle) = 0;
						virtual void CLOAK_CALL_THIS SetLocalRotation(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order) = 0;
						virtual void CLOAK_CALL_THIS SetLocalScale(In const API::Global::Math::Vector& scale) = 0;
						virtual void CLOAK_CALL_THIS SetLocalScale(In float scale) = 0;
						virtual void CLOAK_CALL_THIS SetLocalScale(In float X, In float Y, In float Z) = 0;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In const API::Global::Math::Vector& v) = 0;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In const API::Global::Math::Quaternion& v) = 0;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order) = 0;
						virtual void CLOAK_CALL_THIS SetVelocity(In const API::Global::Math::Vector& v) = 0;
						virtual void CLOAK_CALL_THIS SetVelocity(In float X, In float Y, In float Z) = 0;
						virtual void CLOAK_CALL_THIS SetAcceleration(In const API::Global::Math::Vector& a) = 0;
						virtual void CLOAK_CALL_THIS SetAcceleration(In float X, In float Y, In float Z) = 0;
						virtual void CLOAK_CALL_THIS SetRelativeVelocity(In const API::Global::Math::Vector& v) = 0;
						virtual void CLOAK_CALL_THIS SetRelativeVelocity(In float X, In float Y, In float Z) = 0;
						virtual void CLOAK_CALL_THIS SetRelativeAcceleration(In const API::Global::Math::Vector& a) = 0;
						virtual void CLOAK_CALL_THIS SetRelativeAcceleration(In float X, In float Y, In float Z) = 0;

						virtual void CLOAK_CALL_THIS Move(In const API::Global::Math::Vector& offset) = 0;
						virtual void CLOAK_CALL_THIS Move(In float X, In float Y, In float Z) = 0;
						virtual void CLOAK_CALL_THIS MoveRelative(In const API::Global::Math::Vector& offset) = 0;
						virtual void CLOAK_CALL_THIS MoveRelative(In float X, In float Y, In float Z) = 0;
						virtual void CLOAK_CALL_THIS Rotate(In const API::Global::Math::Quaternion& rot) = 0;
						virtual void CLOAK_CALL_THIS Rotate(In const API::Global::Math::Vector& axis, In float angle) = 0;
						virtual void CLOAK_CALL_THIS Rotate(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order) = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif