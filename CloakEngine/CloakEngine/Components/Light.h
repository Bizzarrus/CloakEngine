#pragma once
#ifndef CE_API_COMPONENTS_LIGHT_H
#define CE_API_COMPONENTS_LIGHT_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/World/Entity.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Helper/Color.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Components {
			inline namespace Light_v1 {
				enum class LightType {Directional, Point, Spot};
				struct LIGHT_INFO_BASE {
					bool Visible = false;
					API::Helper::Color::RGBA Color;
				};
				struct DIRECTIONAL_LIGHT_INFO : public LIGHT_INFO_BASE {
					API::Global::Math::Vector Direction;
				};
				struct POINT_LIGHT_INFO : public LIGHT_INFO_BASE {
					API::Global::Math::Vector Position;
					float Range = 0;
				};
				struct SPOT_LIGHT_INFO : public LIGHT_INFO_BASE {
					API::Global::Math::Vector Position;
					API::Global::Math::Vector Direction;
					float Angle = 0;
					float Range = 0;
				};
				struct LIGHT_INFO {
					LightType Type;
					union {
						LIGHT_INFO_BASE Info;
						DIRECTIONAL_LIGHT_INFO Directional;
						POINT_LIGHT_INFO Point;
						SPOT_LIGHT_INFO Spot;
					};
				};

				CLOAK_COMPONENT("{DF96CEA6-E2CC-402D-8B9F-DEC98DE0D858}", ILight) {
					public:
						CLOAKENGINE_API CLOAK_CALL ILight(In API::World::IEntity* e, In type_id typeID) : Component(e, typeID) {}

						virtual void CLOAK_CALL_THIS SetColor(In const API::Helper::Color::RGBA& color) = 0;
						virtual void CLOAK_CALL_THIS SetColor(In const API::Helper::Color::RGBA& color, In float intensity = 1.0f) = 0;
						virtual void CLOAK_CALL_THIS SetColor(In const API::Helper::Color::RGBX& color, In float intensity = 1.0f) = 0;
						virtual void CLOAK_CALL_THIS SetColor(In float R, In float G, In float B, In float intensity = 1.0f) = 0;
				};
				class CLOAK_UUID("{1EEA57E9-3B0B-40C1-AEC5-D6148F085D2E}") IDirectionalLight : public virtual ILight {
					public:
						typedef ILight Super;

						CLOAKENGINE_API CLOAK_CALL IDirectionalLight(In API::World::IEntity* e, In type_id typeID) : ILight(e, typeID), Component(e, typeID) {}

						virtual void CLOAK_CALL_THIS SetDirection(In const API::Global::Math::Vector& dir) = 0;
						virtual void CLOAK_CALL_THIS SetDirection(In float X, In float Y, In float Z) = 0;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In const API::Global::Math::Vector& av) = 0;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In const API::Global::Math::Quaternion& av) = 0;
						virtual void CLOAK_CALL_THIS SetAngularVelocity(In float roll, In float pitch, In float yaw, In API::Global::Math::RotationOrder order) = 0;

						virtual DIRECTIONAL_LIGHT_INFO CLOAK_CALL_THIS GetInfo(In API::Rendering::WorldID worldID) const = 0;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, Out DIRECTIONAL_LIGHT_INFO* res) const = 0;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, Out LIGHT_INFO* res) const = 0;
				};
				class CLOAK_UUID("{637C62AE-3BE1-453E-9D90-9470DD2C30F5}") IPointLight : public virtual ILight {
					public:
						typedef ILight Super;

						CLOAKENGINE_API CLOAK_CALL IPointLight(In API::World::IEntity* e, In type_id typeID) : ILight(e, typeID), Component(e, typeID) {}

						virtual void CLOAK_CALL_THIS SetRange(In float range) = 0;

						virtual POINT_LIGHT_INFO CLOAK_CALL_THIS GetInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum) const = 0;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out POINT_LIGHT_INFO* res) const = 0;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out LIGHT_INFO* res) const = 0;
				};
				class CLOAK_UUID("{C893FFDF-5653-4E3E-9D6E-E1E5C451DEBA}") ISpotLight : public virtual ILight {
					public:
						typedef ILight Super;

						CLOAKENGINE_API CLOAK_CALL ISpotLight(In API::World::IEntity* e, In type_id typeID) : ILight(e, typeID), Component(e, typeID) {}

						virtual void CLOAK_CALL_THIS SetRange(In float range) = 0;
						virtual void CLOAK_CALL_THIS SetRange(In float range, In float angle) = 0;
						virtual void CLOAK_CALL_THIS SetOpenAngle(In float angle) = 0;
						virtual void CLOAK_CALL_THIS SetOpenAngle(In float angle, In float range) = 0;

						virtual SPOT_LIGHT_INFO CLOAK_CALL_THIS GetInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum) const = 0;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out SPOT_LIGHT_INFO* res) const = 0;
						virtual void CLOAK_CALL_THIS FillInfo(In API::Rendering::WorldID worldID, In const API::Global::Math::OctantFrustum& frustum, Out LIGHT_INFO* res) const = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif