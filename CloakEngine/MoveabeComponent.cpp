#include "stdafx.h"
#include "Implementation/Components/Moveable.h"

#pragma warning(push)
#pragma warning(disable: 4436)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			namespace Moveable_v1 {
				constexpr float MIN_ROT_ANGLE = 1e-4f;

				CLOAK_CALL Moveable::Moveable(In API::World::IEntity* parent, In type_id typeID) : Position(parent, typeID), IPosition(parent, typeID), IMoveable(parent, typeID), Component(parent, typeID)
				{
					m_velocity = API::Global::Math::Vector(0, 0, 0);
					m_relVelocity = API::Global::Math::Vector(0, 0, 0);
					m_acceleration = API::Global::Math::Vector(0, 0, 0);
					m_relAcceleration = API::Global::Math::Vector(0, 0, 0);
					m_angVelocity = API::Global::Math::Vector(0, 0, 0);
					m_lastRotUpdate = 0;
					m_lastUpdate = 0;
				}
				CLOAK_CALL Moveable::Moveable(In API::World::IEntity* parent, In API::World::Component* src) : Position(parent, src), IPosition(parent, src->TypeID), IMoveable(parent, src->TypeID), Component(parent, src->TypeID)
				{
					Moveable* c = dynamic_cast<Moveable*>(src);
					if (c != nullptr)
					{
						m_velocity = c->m_velocity;
						m_relVelocity = c->m_relVelocity;
						m_acceleration = c->m_acceleration;
						m_relAcceleration = c->m_relAcceleration;
						m_angVelocity = c->m_angVelocity;
					}
					m_lastRotUpdate = 0;
					m_lastUpdate = 0;
				}
				CLOAK_CALL Moveable::~Moveable()
				{

				}

				void CLOAK_CALL_THIS Moveable::SetPosition(In const API::Global::Math::WorldPosition& pos)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_anchor != nullptr) { m_offset = static_cast<API::Global::Math::Vector>(pos - m_anchor->GetPosition()); }
					else if (m_node != nullptr)
					{
						const API::Global::Math::IntVector& np = m_node->GetPosition();
						API::Global::Math::WorldPosition wp = pos;
						wp.Node -= np;
						m_offset = static_cast<API::Global::Math::Vector>(wp);
					}
					else { m_offset = static_cast<API::Global::Math::Vector>(pos); }
					if (m_parent == nullptr && m_node != nullptr)
					{
						Impl::Files::WorldNode* o = m_node;
						m_node->SnapToNode(&o, &m_offset);
						iSetNode(o);
					}
				}
				void CLOAK_CALL_THIS Moveable::SetPosition(In float X, In float Y, In float Z)
				{
					SetPosition(API::Global::Math::WorldPosition(X, Y, Z));
				}
				void CLOAK_CALL_THIS Moveable::SetPosition(In float X, In float Y, In float Z, In int32_t nX, In int32_t nY, In int32_t nZ)
				{
					SetPosition(API::Global::Math::WorldPosition(X, Y, Z, nX, nY, nZ));
				}
				void CLOAK_CALL_THIS Moveable::SetRotation(In const API::Global::Math::Quaternion& rot)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_anchor != nullptr) { m_rot = (m_anchor->GetRotation().Inverse() * rot).Normalize(); }
					else { m_rot = rot; }
				}
				void CLOAK_CALL_THIS Moveable::SetRotation(In const API::Global::Math::Vector& axis, In float angle)
				{
					SetRotation(API::Global::Math::Quaternion(axis, angle));
				}
				void CLOAK_CALL_THIS Moveable::SetRotation(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order)
				{
					SetRotation(API::Global::Math::Quaternion::Rotation(roll, pitch, yaw, order));
				}
				void CLOAK_CALL_THIS Moveable::SetScale(In const API::Global::Math::Vector& scale)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_anchor != nullptr) { m_scale = scale / m_anchor->GetScale(); }
					else { m_scale = scale; }
				}
				void CLOAK_CALL_THIS Moveable::SetScale(In float scale)
				{
					SetScale(API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS Moveable::SetScale(In float X, In float Y, In float Z)
				{
					SetScale(API::Global::Math::Vector(X, Y, Z));
				}
				void CLOAK_CALL_THIS Moveable::SetLocalPosition(In const API::Global::Math::Vector& pos)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_parent == nullptr && m_node != nullptr)
					{
						const API::Global::Math::IntVector& np = m_node->GetPosition();
						m_offset = static_cast<API::Global::Math::Vector>(API::Global::Math::WorldPosition(pos, -np));
						Impl::Files::WorldNode* o = m_node;
						m_node->SnapToNode(&o, &m_offset);
						iSetNode(o);
					}
					else { m_offset = pos; }
				}
				void CLOAK_CALL_THIS Moveable::SetLocalPosition(In float X, In float Y, In float Z)
				{
					SetLocalPosition(API::Global::Math::Vector(X, Y, Z));
				}
				void CLOAK_CALL_THIS Moveable::SetLocalRotation(In const API::Global::Math::Quaternion& rot)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_rot = rot;
				}
				void CLOAK_CALL_THIS Moveable::SetLocalRotation(In const API::Global::Math::Vector& axis, In float angle)
				{
					SetLocalRotation(API::Global::Math::Quaternion::Rotation(axis, angle));
				}
				void CLOAK_CALL_THIS Moveable::SetLocalRotation(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order)
				{
					SetLocalRotation(API::Global::Math::Quaternion::Rotation(roll, pitch, yaw, order));
				}
				void CLOAK_CALL_THIS Moveable::SetLocalScale(In const API::Global::Math::Vector& scale)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_scale = scale;
				}
				void CLOAK_CALL_THIS Moveable::SetLocalScale(In float scale)
				{
					SetLocalScale(API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS Moveable::SetLocalScale(In float X, In float Y, In float Z)
				{
					SetLocalScale(API::Global::Math::Vector(X, Y, Z));
				}
				void CLOAK_CALL_THIS Moveable::SetAngularVelocity(In const API::Global::Math::Vector& v)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_angVelocity = v;
					m_lastRotUpdate = API::Global::Game::GetGameTimeStamp();
				}
				void CLOAK_CALL_THIS Moveable::SetAngularVelocity(In const API::Global::Math::Quaternion& v)
				{
					SetAngularVelocity(v.ToAngularVelocity());
				}
				void CLOAK_CALL_THIS Moveable::SetAngularVelocity(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order)
				{
					SetAngularVelocity(API::Global::Math::Quaternion::Rotation(roll, pitch, yaw, order));
				}
				void CLOAK_CALL_THIS Moveable::SetVelocity(In const API::Global::Math::Vector& v)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_velocity = v;
				}
				void CLOAK_CALL_THIS Moveable::SetVelocity(In float X, In float Y, In float Z)
				{
					SetVelocity(API::Global::Math::Vector(X, Y, Z));
				}
				void CLOAK_CALL_THIS Moveable::SetAcceleration(In const API::Global::Math::Vector& a)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_acceleration = a;
				}
				void CLOAK_CALL_THIS Moveable::SetAcceleration(In float X, In float Y, In float Z)
				{
					SetAcceleration(API::Global::Math::Vector(X, Y, Z));
				}
				void CLOAK_CALL_THIS Moveable::SetRelativeVelocity(In const API::Global::Math::Vector& v)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_relVelocity = v;
				}
				void CLOAK_CALL_THIS Moveable::SetRelativeVelocity(In float X, In float Y, In float Z)
				{
					SetRelativeVelocity(API::Global::Math::Vector(X, Y, Z));
				}
				void CLOAK_CALL_THIS Moveable::SetRelativeAcceleration(In const API::Global::Math::Vector& a)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_relAcceleration = a;
				}
				void CLOAK_CALL_THIS Moveable::SetRelativeAcceleration(In float X, In float Y, In float Z)
				{
					SetRelativeAcceleration(API::Global::Math::Vector(X, Y, Z));
				}

				void CLOAK_CALL_THIS Moveable::Move(In const API::Global::Math::Vector& offset)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_offset += offset;
					if (m_parent == nullptr && m_node != nullptr)
					{
						Impl::Files::WorldNode* o = m_node;
						m_node->SnapToNode(&o, &m_offset);
						iSetNode(o);
					}
				}
				void CLOAK_CALL_THIS Moveable::Move(In float X, In float Y, In float Z)
				{
					Move(API::Global::Math::Vector(X, Y, Z));
				}
				void CLOAK_CALL_THIS Moveable::MoveRelative(In const API::Global::Math::Vector& offset)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_offset += m_rot.Rotate(offset);
					if (m_parent == nullptr && m_node != nullptr)
					{
						Impl::Files::WorldNode* o = m_node;
						m_node->SnapToNode(&o, &m_offset);
						iSetNode(o);
					}
				}
				void CLOAK_CALL_THIS Moveable::MoveRelative(In float X, In float Y, In float Z)
				{
					MoveRelative(API::Global::Math::Vector(X, Y, Z));
				}
				void CLOAK_CALL_THIS Moveable::Rotate(In const API::Global::Math::Quaternion& rot)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_rot = (m_rot * rot).Normalize();
				}
				void CLOAK_CALL_THIS Moveable::Rotate(In const API::Global::Math::Vector& axis, In float angle)
				{
					Rotate(API::Global::Math::Quaternion::Rotation(axis, angle));
				}
				void CLOAK_CALL_THIS Moveable::Rotate(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order)
				{
					Rotate(API::Global::Math::Quaternion::Rotation(roll, pitch, yaw, order));
				}

				//GetLocalToNode guesses the current position/rotation, based on the last information given by the physic simulation
				const API::Global::Math::Matrix& CLOAK_CALL_THIS Moveable::GetLocalToNode(In API::Global::Time time, Out_opt Impl::Files::WorldNode** node)
				{
					API::Helper::ReadLock rlock = m_parent->LockRead();
					if (m_LTNTime != time)
					{
						API::Helper::WriteLock wlock = m_parent->LockWrite();
						if (m_LTNTime != time)
						{
							API::Global::Math::Quaternion rot = m_rot;
							API::Global::Math::Vector scale = m_scale;
							API::Global::Math::Vector pos = m_offset;
							if (m_lastUpdate != 0 && m_lastUpdate < time)
							{
								const float dt = (time - m_lastUpdate)*1e-3f;
								const float adt = m_lastRotUpdate < time ? ((time - m_lastRotUpdate)*1e-3f) : 0;
								const float avl = m_angVelocity.Length() * adt;
								if (avl > MIN_ROT_ANGLE)
								{
									API::Global::Math::Quaternion w = API::Global::Math::Quaternion(m_angVelocity, avl);
									rot = (rot * w).Normalize();
								}
								pos = m_offset + (((m_velocity + (m_acceleration * dt)) + rot.Rotate(m_relVelocity + (m_relAcceleration * dt))) * dt);
							}
							m_LTN = API::Global::Math::Matrix::Scale(scale);
							m_LTN *= API::Global::Math::Quaternion::ToMatrix(rot);
							m_LTN *= API::Global::Math::Matrix::Translate(pos);
							if (m_anchor != nullptr) { m_LTN *= m_anchor->GetLocalToNode(time); }
							m_LTNTime = time;
						}
					}
					if (node != nullptr) { *node = m_node; }
					return m_LTN;
				}

				//Update is called during physic simulation only
				void CLOAK_CALL_THIS Moveable::Update(In API::Global::Time time)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_lastUpdate < time)
					{
						if (m_lastUpdate != 0)
						{
							const float dt = (time - m_lastUpdate)*1e-3f;
							const float adt = m_lastRotUpdate < time ? ((time - m_lastRotUpdate)*1e-3f) : 0;
							const float avl = m_angVelocity.Length() * adt;
							if (avl > MIN_ROT_ANGLE)
							{
								API::Global::Math::Quaternion w = API::Global::Math::Quaternion(m_angVelocity, avl);
								m_rot = (m_rot * w).Normalize();
								m_lastRotUpdate = time;
							}
							m_velocity += m_acceleration * dt;
							m_relVelocity += m_relAcceleration * dt;
							m_offset += (m_velocity * dt) + (m_rot.Rotate(m_relVelocity) * dt);
							if (m_node != nullptr)
							{
								Impl::Files::WorldNode* n = m_node;
								m_node->SnapToNode(&n, &m_offset);
								iSetNode(n);
							}
						}
						else { m_lastRotUpdate = time; }
						m_lastUpdate = time;
					}
				}
			}
		}
	}
}

#pragma warning(pop)