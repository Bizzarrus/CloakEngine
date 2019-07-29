#pragma once
#ifndef CE_IMPL_COMPONENTS_MOVEABLE_H
#define CE_IMPL_COMPONENTS_MOVEABLE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Components/Moveable.h"
#include "Implementation/Components/Position.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			inline namespace Moveable_v1 {
				class CLOAK_UUID("{D0E34452-F0C2-4B41-8677-723349257802}") Moveable : public virtual API::Components::Moveable_v1::IMoveable, public Impl::Components::Position_v1::Position {
					public:
						typedef type_tuple<API::Components::Moveable_v1::IMoveable, Impl::Components::Position_v1::Position> Super;

						CLOAK_CALL Moveable(In API::World::IEntity* parent, In type_id typeID);
						CLOAK_CALL Moveable(In API::World::IEntity* parent, In API::World::Component* src);
						virtual CLOAK_CALL ~Moveable();

						virtual void CLOAK_CALL_THIS SetPosition(In const API::Global::Math::WorldPosition& pos) override;
						virtual void CLOAK_CALL_THIS SetPosition(In float X, In float Y, In float Z) override;
						virtual void CLOAK_CALL_THIS SetPosition(In float X, In float Y, In float Z, In int32_t nX, In int32_t nY, In int32_t nZ) override;
						virtual void CLOAK_CALL_THIS SetRotation(In const API::Global::Math::Quaternion& rot) override;
						virtual void CLOAK_CALL_THIS SetRotation(In const API::Global::Math::Vector& axis, In float angle) override;
						virtual void CLOAK_CALL_THIS SetRotation(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order) override;
						virtual void CLOAK_CALL_THIS SetScale(In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS SetScale(In float scale) override;
						virtual void CLOAK_CALL_THIS SetScale(In float X, In float Y, In float Z) override;
						virtual void CLOAK_CALL_THIS SetLocalPosition(In const API::Global::Math::Vector& pos) override;
						virtual void CLOAK_CALL_THIS SetLocalPosition(In float X, In float Y, In float Z) override;
						virtual void CLOAK_CALL_THIS SetLocalRotation(In const API::Global::Math::Quaternion& rot) override;
						virtual void CLOAK_CALL_THIS SetLocalRotation(In const API::Global::Math::Vector& axis, In float angle) override;
						virtual void CLOAK_CALL_THIS SetLocalRotation(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order) override;
						virtual void CLOAK_CALL_THIS SetLocalScale(In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS SetLocalScale(In float scale) override;
						virtual void CLOAK_CALL_THIS SetLocalScale(In float X, In float Y, In float Z) override;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In const API::Global::Math::Vector& v) override;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In const API::Global::Math::Quaternion& v) override;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order) override;
						virtual void CLOAK_CALL_THIS SetVelocity(In const API::Global::Math::Vector& v) override;
						virtual void CLOAK_CALL_THIS SetVelocity(In float X, In float Y, In float Z) override;
						virtual void CLOAK_CALL_THIS SetAcceleration(In const API::Global::Math::Vector& a) override;
						virtual void CLOAK_CALL_THIS SetAcceleration(In float X, In float Y, In float Z) override;
						virtual void CLOAK_CALL_THIS SetRelativeVelocity(In const API::Global::Math::Vector& v) override;
						virtual void CLOAK_CALL_THIS SetRelativeVelocity(In float X, In float Y, In float Z) override;
						virtual void CLOAK_CALL_THIS SetRelativeAcceleration(In const API::Global::Math::Vector& a) override;
						virtual void CLOAK_CALL_THIS SetRelativeAcceleration(In float X, In float Y, In float Z) override;

						virtual void CLOAK_CALL_THIS Move(In const API::Global::Math::Vector& offset) override;
						virtual void CLOAK_CALL_THIS Move(In float X, In float Y, In float Z) override;
						virtual void CLOAK_CALL_THIS MoveRelative(In const API::Global::Math::Vector& offset) override;
						virtual void CLOAK_CALL_THIS MoveRelative(In float X, In float Y, In float Z) override;
						virtual void CLOAK_CALL_THIS Rotate(In const API::Global::Math::Quaternion& rot) override;
						virtual void CLOAK_CALL_THIS Rotate(In const API::Global::Math::Vector& axis, In float angle) override;
						virtual void CLOAK_CALL_THIS Rotate(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order) override;

						virtual const API::Global::Math::Matrix& CLOAK_CALL_THIS GetLocalToNode(In API::Global::Time time, Out_opt Impl::Files::WorldNode** node = nullptr) override;
						//This function should only be called once per frame, from physic simulation thread
						virtual void CLOAK_CALL_THIS Update(In API::Global::Time time);
					protected:
						API::Global::Math::Vector m_velocity;
						API::Global::Math::Vector m_relVelocity;
						API::Global::Math::Vector m_acceleration;
						API::Global::Math::Vector m_relAcceleration;
						API::Global::Math::Vector m_angVelocity;
						API::Global::Time m_lastUpdate;
						API::Global::Time m_lastRotUpdate;
				};
			}
		}
	}
}

#pragma warning(pop)
#endif