#include "stdafx.h"
#include "Implementation/Components/Light.h"

#include "Engine/WorldStreamer.h"

#pragma warning(push)
#pragma warning(disable: 4436)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			namespace Light_v1 {
				constexpr float MIN_ROT_ANGLE = 1e-4f;

				CLOAK_CALL Light::Light(In API::World::IEntity* e, In type_id typeID) : Component(e, typeID), ILight(e, typeID)
				{
					m_color = API::Helper::Color::RGBA(0, 0, 0, 0);
				}
				CLOAK_CALL Light::Light(In API::World::IEntity* parent, In API::World::Component* src) : Light(parent, src->TypeID)
				{
					CLOAK_ASSUME(this != src);
					Light* o = dynamic_cast<Light*>(src);
					if (o != nullptr) { m_color = o->m_color; }
				}
				CLOAK_CALL Light::~Light()
				{

				}

				void CLOAK_CALL_THIS Light::SetColor(In const API::Helper::Color::RGBA& color)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_color = color;
				}
				void CLOAK_CALL_THIS Light::SetColor(In const API::Helper::Color::RGBA& color, In float intensity)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_color = API::Helper::Color::RGBA(color.R, color.G, color.B, intensity);
				}
				void CLOAK_CALL_THIS Light::SetColor(In const API::Helper::Color::RGBX& color, In float intensity)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_color = API::Helper::Color::RGBA(color.R, color.G, color.B, intensity);
				}
				void CLOAK_CALL_THIS Light::SetColor(In float R, In float G, In float B, In float intensity)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_color = API::Helper::Color::RGBA(R, G, B, intensity);
				}



				CLOAK_CALL DirectionalLight::DirectionalLight(In API::World::IEntity* e, In type_id typeID) : Component(e, typeID), ILight(e, typeID), Light(e, typeID), IDirectionalLight(e, typeID), IGraphicUpdate(e, typeID)
				{
					m_dir = API::Global::Math::Vector(0, -1, 0);
					m_angVelocity = API::Global::Math::Vector(0, 0, 0);
					m_lastUpdate = 0;
					m_curScene = nullptr;
				}
				CLOAK_CALL DirectionalLight::DirectionalLight(In API::World::IEntity* parent, In API::World::Component* src) : Component(parent, src->TypeID), ILight(parent, src->TypeID), Light(parent, src), IDirectionalLight(parent, src->TypeID), IGraphicUpdate(parent, src->TypeID)
				{
					m_dir = API::Global::Math::Vector(0, -1, 0);
					m_lastUpdate = 0;
					m_curScene = nullptr;
					DirectionalLight* o = dynamic_cast<DirectionalLight*>(src);
					if (o != nullptr) 
					{ 
						m_dir = o->m_dir; 
						m_angVelocity = o->m_angVelocity;
					}
				}
				CLOAK_CALL DirectionalLight::~DirectionalLight()
				{

				}

				void CLOAK_CALL_THIS DirectionalLight::SetDirection(In const API::Global::Math::Vector& dir)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_dir = dir.Normalize();
				}
				void CLOAK_CALL_THIS DirectionalLight::SetDirection(In float X, In float Y, In float Z)
				{
					SetDirection(API::Global::Math::Vector(X, Y, Z));
				}
				void CLOAK_CALL_THIS DirectionalLight::SetAngularVelocity(In const API::Global::Math::Vector& av)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_angVelocity = av;
					m_lastUpdate = API::Global::Game::GetGameTimeStamp();
				}
				void CLOAK_CALL_THIS DirectionalLight::SetAngularVelocity(In const API::Global::Math::Quaternion& av)
				{
					SetAngularVelocity(av.ToAngularVelocity());
				}
				void CLOAK_CALL_THIS DirectionalLight::SetAngularVelocity(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order)
				{
					SetAngularVelocity(API::Global::Math::Quaternion::Rotation(roll, pitch, yaw, order));
				}

				API::Components::Light_v1::DIRECTIONAL_LIGHT_INFO CLOAK_CALL_THIS DirectionalLight::GetInfo(In API::Rendering::WorldID worldID) const
				{
					API::Components::Light_v1::DIRECTIONAL_LIGHT_INFO r;
					FillInfo(worldID, &r);
					return r;
				}
				void CLOAK_CALL_THIS DirectionalLight::FillInfo(In API::Rendering::WorldID worldID, Out API::Components::Light_v1::DIRECTIONAL_LIGHT_INFO* res) const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					res->Color = m_color;
					res->Direction = m_dir;
					res->Visible = Engine::WorldStreamer::CheckVisible(worldID, m_curScene);
				}
				void CLOAK_CALL_THIS DirectionalLight::FillInfo(In API::Rendering::WorldID worldID, Out API::Components::Light_v1::LIGHT_INFO* res) const
				{
					res->Type = API::Components::Light_v1::LightType::Directional;
					FillInfo(worldID, &res->Directional);
				}

				void CLOAK_CALL_THIS DirectionalLight::Update(In API::Global::Time time, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_lastUpdate != time)
					{
						if (m_lastUpdate != 0)
						{
							const float adt = (time - m_lastUpdate)*1e-6f;
							const float avl = m_angVelocity.Length() * adt;
							if (avl > MIN_ROT_ANGLE)
							{
								API::Global::Math::Quaternion w = API::Global::Math::Quaternion(m_angVelocity, avl);
								m_dir = w.Rotate(m_dir).Normalize();
								m_lastUpdate = time;
							}
						}
						else { m_lastUpdate = time; }
					}
				}

				void CLOAK_CALL_THIS DirectionalLight::OnNodeChange(In API::Files::IScene* scene, In API::Files::IWorldNode* node)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_curScene = scene;
				}


				CLOAK_CALL PointLight::PointLight(In API::World::IEntity* e, In type_id typeID) : Component(e, typeID), ILight(e, typeID), Light(e, typeID), IPointLight(e, typeID), IGraphicUpdatePosition(e, typeID)
				{
					m_worldID = 0;
					m_updBounding = false;
					m_range = 0;
					m_bounding = new Impl::Files::BoundingSphere(API::Global::Math::Vector(0, 0, 0), m_range);
				}
				CLOAK_CALL PointLight::PointLight(In API::World::IEntity* parent, In API::World::Component* src) : Component(parent, src->TypeID), ILight(parent, src->TypeID), Light(parent, src), IPointLight(parent, src->TypeID), IGraphicUpdatePosition(parent, src->TypeID)
				{
					CLOAK_ASSUME(this != src);
					m_worldID = 0;
					m_updBounding = false;
					m_range = 0;
					PointLight* o = dynamic_cast<PointLight*>(src);
					if (o != nullptr) { m_range = o->m_range; }
					m_bounding = new Impl::Files::BoundingSphere(API::Global::Math::Vector(0, 0, 0), m_range);
				}
				CLOAK_CALL PointLight::~PointLight()
				{
					SAVE_RELEASE(m_bounding);
				}

				void CLOAK_CALL_THIS PointLight::SetRange(In float range)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_range = range;
					m_updBounding = true;
				}

				API::Components::Light_v1::POINT_LIGHT_INFO CLOAK_CALL_THIS PointLight::GetInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum) const
				{
					API::Components::Light_v1::POINT_LIGHT_INFO r;
					FillInfo(worldID, frustum, &r);
					return r;
				}
				void CLOAK_CALL_THIS PointLight::FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out API::Components::Light_v1::POINT_LIGHT_INFO* res) const
				{
					CLOAK_ASSUME(res != nullptr);
					API::Helper::ReadLock lock = m_parent->LockRead();
					res->Color = m_color;
					res->Range = m_range;
					res->Position = m_pos;
					res->Visible = worldID == m_worldID && m_bounding->Check(frustum, 0);
				}
				void CLOAK_CALL_THIS PointLight::FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out API::Components::Light_v1::LIGHT_INFO* res) const
				{
					res->Type = API::Components::Light_v1::LightType::Point;
					FillInfo(worldID, frustum, &res->Point);
				}

				float CLOAK_CALL_THIS PointLight::GetBoundingRadius() const 
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					return m_range;
				}
				void CLOAK_CALL_THIS PointLight::Update(In API::Global::Time time, In Impl::Components::Position* position, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					Impl::Files::WorldNode* node = nullptr;
					API::Global::Math::Matrix LTN = position->GetLocalToNode(time, &node);
					if (m_updBounding)
					{
						m_updBounding = false;
						m_bounding->Update(API::Global::Math::Vector(0, 0, 0), m_range);
					}
					m_bounding->Update();
					if (node != nullptr)
					{
						API::Global::Math::Matrix LTW = LTN * node->GetNodeToWorld(&m_worldID);
						m_bounding->SetLocalToWorld(LTW);
						m_pos = LTW.GetTranslation();
					}
					else
					{
						m_bounding->SetLocalToWorld(API::Global::Math::Matrix::Identity);
						m_pos = static_cast<API::Global::Math::Vector>(position->GetPosition());
					}
				}



				CLOAK_CALL SpotLight::SpotLight(In API::World::IEntity* e, In type_id typeID) : Component(e, typeID), ILight(e, typeID), Light(e, typeID), ISpotLight(e, typeID), IGraphicUpdatePosition(e, typeID)
				{
					m_worldID = 0;
					m_updBounding = false;
					m_range = 0;
					m_angle = 0;
					API::Global::Math::Vector axis[] = { API::Global::Math::Vector(0, 0, 0),API::Global::Math::Vector(0, 0, 0),API::Global::Math::Vector(0, 0, 0) };
					m_bounding = new Impl::Files::BoundingBox(API::Global::Math::Vector(0, 0, 0), axis);
				}
				CLOAK_CALL SpotLight::SpotLight(In API::World::IEntity* parent, In API::World::Component* src) : Component(parent, src->TypeID), ILight(parent, src->TypeID), Light(parent, src), ISpotLight(parent, src->TypeID), IGraphicUpdatePosition(parent, src->TypeID)
				{
					CLOAK_ASSUME(this != src);
					m_worldID = 0;
					m_updBounding = false;
					m_range = 0;
					m_angle = 0;
					SpotLight* o = dynamic_cast<SpotLight*>(src);
					if (o != nullptr)
					{
						m_range = o->m_range;
						m_angle = o->m_angle;
					}
					const float width = std::tan(m_angle / 2)*m_range;
					API::Global::Math::Vector axis[3] = { API::Global::Math::Vector(0, 0, m_range / 2), API::Global::Math::Vector(width, 0, 0), API::Global::Math::Vector(0, width, 0) };
					m_bounding = new Impl::Files::BoundingBox(axis[0], axis);
				}
				CLOAK_CALL SpotLight::~SpotLight()
				{
					SAVE_RELEASE(m_bounding);
				}

				void CLOAK_CALL_THIS SpotLight::SetRange(In float range)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_range = range;
					m_updBounding = true;
				}
				void CLOAK_CALL_THIS SpotLight::SetRange(In float range, In float angle)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_range = range;
					m_angle = angle;
					m_updBounding = true;
				}
				void CLOAK_CALL_THIS SpotLight::SetOpenAngle(In float angle)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_angle = angle;
					m_updBounding = true;
				}
				void CLOAK_CALL_THIS SpotLight::SetOpenAngle(In float angle, In float range) { SetRange(range, angle); }

				API::Components::Light_v1::SPOT_LIGHT_INFO CLOAK_CALL_THIS SpotLight::GetInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum) const
				{
					API::Components::Light_v1::SPOT_LIGHT_INFO r;
					FillInfo(worldID, frustum, &r);
					return r;
				}
				void CLOAK_CALL_THIS SpotLight::FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out API::Components::Light_v1::SPOT_LIGHT_INFO* res) const
				{
					CLOAK_ASSUME(res != nullptr);
					API::Helper::ReadLock lock = m_parent->LockRead();
					res->Color = m_color;
					res->Range = m_range;
					res->Position = m_pos;
					res->Angle = m_angle;
					res->Direction = m_dir;
					res->Visible = worldID == m_worldID && m_bounding->Check(frustum, 0);
				}
				void CLOAK_CALL_THIS SpotLight::FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out API::Components::Light_v1::LIGHT_INFO* res) const
				{
					res->Type = API::Components::Light_v1::LightType::Spot;
					FillInfo(worldID, frustum, &res->Spot);
				}

				float CLOAK_CALL_THIS SpotLight::GetBoundingRadius() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					return m_range;
				}
				void CLOAK_CALL_THIS SpotLight::Update(In API::Global::Time time, In Impl::Components::Position* position, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					Impl::Files::WorldNode* node = nullptr;
					API::Global::Math::Matrix LTN = position->GetLocalToNode(time, &node);
					if (m_updBounding)
					{
						m_updBounding = false;
						const float width = std::tan(m_angle / 2)*m_range;
						API::Global::Math::Vector axis[3] = { API::Global::Math::Vector(0, 0, m_range / 2), API::Global::Math::Vector(width, 0, 0), API::Global::Math::Vector(0, width, 0) };
						m_bounding->Update(axis[0], axis);
					}
					m_bounding->Update();
					if (node != nullptr)
					{
						API::Global::Math::Matrix LTW = LTN * node->GetNodeToWorld(&m_worldID);
						m_bounding->SetLocalToWorld(LTW);
						m_pos = LTW.GetTranslation();
						m_dir = (API::Global::Math::Vector(0, 0, 1)*LTW.RemoveTranslation()).Normalize();
					}
					else
					{
						m_bounding->SetLocalToWorld(API::Global::Math::Matrix::Identity);
						m_pos = static_cast<API::Global::Math::Vector>(position->GetPosition());
						m_dir = position->GetRotation().Rotate(API::Global::Math::Vector(0, 0, 1));
					}
				}
			}
		}
	}
}

#pragma warning(pop)