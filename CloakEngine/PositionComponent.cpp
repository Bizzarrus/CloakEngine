#include "stdafx.h"
#include "Implementation/Components/Position.h"
#include "Implementation/World/Entity.h"

#pragma warning(push)
#pragma warning(disable: 4436)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			namespace Position_v1 {
				CLOAK_CALL Position::Position(In API::World::IEntity* parent, In type_id typeID) : IPosition(parent, typeID), Component(parent, typeID)
				{
					m_anchor = nullptr;
					m_ref = nullptr;
					m_node = nullptr;
					m_offset = API::Global::Math::Vector(0, 0, 0);
					m_scale = API::Global::Math::Vector(1, 1, 1);
					m_rot = API::Global::Math::Quaternion(0, 0, 0, 1);
					m_LTNTime = 0;
				}
				CLOAK_CALL Position::Position(In API::World::IEntity* parent, In API::World::Component* src) : Position(parent, src->TypeID)
				{
					Position* o = dynamic_cast<Position*>(src);
					if (o != nullptr)
					{
						m_node = o->m_node;
						m_offset = o->m_offset;
						m_rot = o->m_rot;
						m_scale = o->m_scale;
						if (m_node != nullptr) { m_node->Add(this, &m_ref); }
					}
				}
				CLOAK_CALL Position::~Position()
				{
					for (const auto& a : m_childs) { a->iRemoveParent(); }
					if (m_anchor != nullptr) { m_anchor->iRemoveChild(this); }
				}

				void CLOAK_CALL_THIS Position::OnEnable()
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					iUpdateNode(nullptr, m_node);
				}
				void CLOAK_CALL_THIS Position::OnDisable()
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					iUpdateNode(m_node, nullptr);
				}

				CLOAKENGINE_API API::Global::Math::WorldPosition CLOAK_CALL_THIS Position::GetPosition() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					if (m_anchor != nullptr) { return m_anchor->GetPosition() + (m_anchor->GetRotation().Rotate(m_offset * m_scale)); }
					if (m_node == nullptr) { return m_offset; }
					const API::Global::Math::IntVector pos = m_node->GetPosition();
					return API::Global::Math::WorldPosition(m_offset, pos);
				}
				CLOAKENGINE_API API::Global::Math::Vector CLOAK_CALL_THIS Position::GetLocalPosition() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					if (m_node == nullptr || m_anchor != nullptr) { return m_offset; }
					const API::Global::Math::IntVector pos = m_node->GetPosition();
					return static_cast<API::Global::Math::Vector>(API::Global::Math::WorldPosition(m_offset, pos));
				}
				CLOAKENGINE_API API::Global::Math::Quaternion CLOAK_CALL_THIS Position::GetRotation() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					if (m_anchor != nullptr) { return (m_anchor->GetRotation() * m_rot).Normalize(); }
					return m_rot;
				}
				CLOAKENGINE_API API::Global::Math::Quaternion CLOAK_CALL_THIS Position::GetLocalRotation() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					return m_rot;
				}
				CLOAKENGINE_API API::Global::Math::Vector CLOAK_CALL_THIS Position::GetScale() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					if (m_anchor != nullptr) { return m_anchor->GetScale() * m_scale; }
					return m_scale;
				}
				CLOAKENGINE_API API::Global::Math::Vector CLOAK_CALL_THIS Position::GetLocalScale() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					return m_scale;
				}
				/*
				CLOAKENGINE_API API::Files::Position CLOAK_CALL_THIS Position::GetNodePosition() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					if (m_anchor != nullptr)
					{
						API::Files::Position r = m_anchor->GetNodePosition();
						r.Offset += (m_anchor->GetRotation().Rotate(m_offset*m_scale));
						return r;
					}
					API::Files::Position r;
					r.Offset = m_offset;
					if (m_node != nullptr) { r.Node = m_node->GetPosition(); }
					else { r.Node.X = r.Node.Y = r.Node.Z = 0; }
					return r;
				}
				*/
				CLOAKENGINE_API API::Files::IWorldNode* CLOAK_CALL_THIS Position::GetNode() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					return m_node;
				}
				CLOAKENGINE_API API::RefPointer<API::Components::IPosition> CLOAK_CALL_THIS Position::GetParent() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					return m_anchor;
				}

				void CLOAK_CALL_THIS Position::SetParent(In IPosition* par)
				{
					Position* p = dynamic_cast<Position*>(par);
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (p != nullptr)
					{
						if (p != m_anchor)
						{
							const API::Global::Math::WorldPosition np = p->GetPosition();
							const API::Global::Math::Quaternion nr = p->GetRotation();
							const API::Global::Math::Vector ns = p->GetScale();

							API::Global::Math::WorldPosition op(0, 0, 0, 0, 0, 0);
							API::Global::Math::Quaternion or = API::Global::Math::Quaternion(0, 0, 0, 1);
							API::Global::Math::Vector os = API::Global::Math::Vector(1, 1, 1);
							if (m_anchor != nullptr) 
							{ 
								op = m_anchor->GetPosition();
								or = m_anchor->GetRotation();
								os = m_anchor->GetScale();
								m_anchor->iRemoveChild(this); 
							}
							m_anchor = p;
							m_anchor->iAddChild(this);
							API::Helper::ReadLock plock = m_anchor->m_parent->LockRead();
							iSetNode(m_anchor->m_node);
							plock.unlock();

							m_offset += static_cast<API::Global::Math::Vector>(op - np);
							m_rot = (nr.Inverse() * or * m_rot).Normalize();
							m_scale *= os / ns;
						}
					}
					else if (m_anchor != nullptr)
					{
						const API::Global::Math::WorldPosition op = m_anchor->GetPosition();
						const API::Global::Math::Quaternion or = m_anchor->GetRotation();
						const API::Global::Math::Vector os = m_anchor->GetScale();

						m_anchor->iRemoveChild(this);
						m_anchor = nullptr;

						m_offset += static_cast<API::Global::Math::Vector>(op);
						m_rot = (or * m_rot).Normalize();
						m_scale *= os;
					}
				}

				const API::Global::Math::Matrix& CLOAK_CALL_THIS Position::GetLocalToNode(In API::Global::Time time, Out_opt Impl::Files::WorldNode** node)
				{
					API::Helper::ReadLock rlock = m_parent->LockRead();
					if (m_LTNTime != time)
					{
						API::Helper::WriteLock wlock = m_parent->LockWrite();
						if (m_LTNTime != time)
						{
							m_LTN = API::Global::Math::Matrix::Scale(m_scale);
							m_LTN *= API::Global::Math::Quaternion::ToMatrix(m_rot);
							m_LTN *= API::Global::Math::Matrix::Translate(m_offset);
							if (m_anchor != nullptr) { m_LTN *= m_anchor->GetLocalToNode(time); }
							m_LTNTime = time;
						}
					}
					if (node != nullptr) { *node = m_node; }
					return m_LTN;
				}
				void CLOAK_CALL_THIS Position::SetNode(In API::Files::IWorldNode* node, In const API::Global::Math::Vector& offset, In const API::Global::Math::Quaternion& rot, In const API::Global::Math::Vector& scale)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_anchor != nullptr) { m_anchor->iRemoveChild(this); m_anchor = nullptr; }
					Impl::Files::WorldNode* n = nullptr;
					if (node == nullptr || SUCCEEDED(node->QueryInterface(CE_QUERY_ARGS(&n))))
					{
						iSetNode(n);
						SAVE_RELEASE(n);
					}
					m_offset = offset;
					m_rot = rot;
					m_scale = scale;
				}
				void CLOAK_CALL_THIS Position::RemoveFromNode(In_opt API::Files::IWorldNode* callNode)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					iUpdateNode(m_node, nullptr);
					if (m_node != nullptr) { m_node->Remove(m_ref); }
					m_node = nullptr;
				}

				void CLOAK_CALL_THIS Position::iUpdateNode(In Impl::Files::WorldNode* last, In Impl::Files::WorldNode* next)
				{
					if (last != next)
					{
						API::Helper::WriteLock lock = m_parent->LockWrite();

						Impl::World::Entity* e = nullptr;
						API::Global::Debug::Error err = GetEntity(CE_QUERY_ARGS(&e));
						if (CloakDebugCheckOK(err, err, true))
						{
							e->SetScene(next == nullptr ? nullptr : next->GetParentScene(), next);
							e->Release();
						}

						for (const auto& a : m_childs) { a->iSetNode(next); }
					}
				}
				void CLOAK_CALL_THIS Position::iRemoveChild(In Position* pos)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_childs.erase(pos);
				}
				void CLOAK_CALL_THIS Position::iAddChild(In Position* pos)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_childs.insert(pos);
				}
				void CLOAK_CALL_THIS Position::iRemoveParent()
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_anchor != nullptr)
					{
						const API::Global::Math::WorldPosition op = m_anchor->GetPosition();
						const API::Global::Math::Quaternion or = m_anchor->GetRotation();
						const API::Global::Math::Vector os = m_anchor->GetScale();

						m_offset += static_cast<API::Global::Math::Vector>(op);
						m_rot = (or *m_rot).Normalize();
						m_scale *= os;
					}
					m_anchor = nullptr;
				}
				void CLOAK_CALL_THIS Position::iSetNode(In Impl::Files::WorldNode* node)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (node != m_node)
					{
						AddRef();
						iUpdateNode(m_node, node);
						Impl::Files::ObjectRef or = m_ref;
						if (node != nullptr) { node->Add(this, &m_ref); }
						if (m_node != nullptr) { m_node->Remove(or); }
						m_node = node;
						Release();
					}
				}
			}
		}
	}
}

#pragma warning(pop)