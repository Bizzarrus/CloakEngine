#pragma once
#ifndef CE_IMPL_COMPONENTS_LIGHT_H
#define CE_IMPL_COMPONENTS_LIGHT_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Components/Light.h"
#include "CloakEngine/Rendering/Context.h"
#include "Implementation/Files/BoundingVolume.h"
#include "Implementation/Components/GraphicUpdate.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			inline namespace Light_v1 {
				class Light : public virtual API::Components::Light_v1::ILight {
					public:
						CLOAK_CALL Light(In API::World::IEntity* e, In type_id typeID);
						CLOAK_CALL Light(In API::World::IEntity* parent, In API::World::Component* src);
						virtual CLOAK_CALL ~Light();

						virtual void CLOAK_CALL_THIS SetColor(In const API::Helper::Color::RGBA& color) override;
						virtual void CLOAK_CALL_THIS SetColor(In const API::Helper::Color::RGBA& color, In float intensity = 1.0f) override;
						virtual void CLOAK_CALL_THIS SetColor(In const API::Helper::Color::RGBX& color, In float intensity = 1.0f) override;
						virtual void CLOAK_CALL_THIS SetColor(In float R, In float G, In float B, In float intensity = 1.0f) override;
					protected:
						API::Helper::Color::RGBA m_color;
				};
				class CLOAK_UUID("{18D4E213-0BBB-465A-B45B-4AD5C66146CF}") DirectionalLight : public Light, public virtual API::Components::Light_v1::IDirectionalLight, public virtual Impl::Components::IGraphicUpdate {
					public:
						typedef type_tuple<Impl::Components::IGraphicUpdate, API::Components::Light_v1::IDirectionalLight> Super;

						CLOAK_CALL DirectionalLight(In API::World::IEntity* e, In type_id typeID);
						CLOAK_CALL DirectionalLight(In API::World::IEntity* parent, In API::World::Component* src);
						virtual CLOAK_CALL ~DirectionalLight();

						virtual void CLOAK_CALL_THIS SetDirection(In const API::Global::Math::Vector& dir) override;
						virtual void CLOAK_CALL_THIS SetDirection(In float X, In float Y, In float Z) override;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In const API::Global::Math::Vector& av) override;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In const API::Global::Math::Quaternion& av) override;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order) override;

						virtual API::Components::Light_v1::DIRECTIONAL_LIGHT_INFO CLOAK_CALL_THIS GetInfo(In API::Rendering::WorldID worldID) const override;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, Out API::Components::Light_v1::DIRECTIONAL_LIGHT_INFO* res) const override;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, Out API::Components::Light_v1::LIGHT_INFO* res) const override;

						virtual void CLOAK_CALL_THIS Update(In API::Global::Time time, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager) override;

						virtual void CLOAK_CALL_THIS OnNodeChange(In API::Files::IScene* scene, In API::Files::IWorldNode* node) override;
					protected:
						API::Global::Math::Vector m_dir;
						API::Global::Math::Vector m_angVelocity;
						API::Global::Time m_lastUpdate;
						API::Files::IScene* m_curScene;
				};
				class CLOAK_UUID("{B1B2B875-FD3A-4B8D-AF8C-2908FCE7F364}") PointLight : public Light, public virtual API::Components::Light_v1::IPointLight, public virtual Impl::Components::IGraphicUpdatePosition {
					public:
						typedef type_tuple<Impl::Components::IGraphicUpdatePosition, API::Components::Light_v1::IPointLight> Super;

						CLOAK_CALL PointLight(In API::World::IEntity* e, In type_id typeID);
						CLOAK_CALL PointLight(In API::World::IEntity* parent, In API::World::Component* src);
						virtual CLOAK_CALL ~PointLight();

						virtual void CLOAK_CALL_THIS SetRange(In float range) override;

						virtual API::Components::Light_v1::POINT_LIGHT_INFO CLOAK_CALL_THIS GetInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum) const override;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out API::Components::Light_v1::POINT_LIGHT_INFO* res) const override;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out API::Components::Light_v1::LIGHT_INFO* res) const override;

						virtual float CLOAK_CALL_THIS GetBoundingRadius() const override;
						virtual void CLOAK_CALL_THIS Update(In API::Global::Time time, In Impl::Components::Position* position, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager) override;
					protected:
						API::Global::Math::Vector m_pos;
						bool m_updBounding;
						float m_range;
						API::Rendering::WorldID m_worldID;
						Impl::Files::BoundingSphere* m_bounding;
				};
				class CLOAK_UUID("{F8B8AD21-BF58-468D-9F2A-D74EB389136D}") SpotLight : public Light, public virtual API::Components::Light_v1::ISpotLight, public virtual Impl::Components::IGraphicUpdatePosition {
					public:
						typedef type_tuple<Impl::Components::IGraphicUpdatePosition, API::Components::Light_v1::ISpotLight> Super;

						CLOAK_CALL SpotLight(In API::World::IEntity* e, In type_id typeID);
						CLOAK_CALL SpotLight(In API::World::IEntity* parent, In API::World::Component* src);
						virtual CLOAK_CALL ~SpotLight();

						virtual void CLOAK_CALL_THIS SetRange(In float range) override;
						virtual void CLOAK_CALL_THIS SetRange(In float range, In float angle) override;
						virtual void CLOAK_CALL_THIS SetOpenAngle(In float angle) override;
						virtual void CLOAK_CALL_THIS SetOpenAngle(In float angle, In float range) override;

						virtual API::Components::Light_v1::SPOT_LIGHT_INFO CLOAK_CALL_THIS GetInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum) const override;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out API::Components::Light_v1::SPOT_LIGHT_INFO* res) const override;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out API::Components::Light_v1::LIGHT_INFO* res) const override;

						virtual float CLOAK_CALL_THIS GetBoundingRadius() const override;
						virtual void CLOAK_CALL_THIS Update(In API::Global::Time time, In Impl::Components::Position* position, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager) override;
					protected:
						API::Global::Math::Vector m_pos;
						API::Global::Math::Vector m_dir;
						bool m_updBounding;
						float m_range;
						float m_angle;
						API::Rendering::WorldID m_worldID;
						Impl::Files::BoundingBox* m_bounding;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif