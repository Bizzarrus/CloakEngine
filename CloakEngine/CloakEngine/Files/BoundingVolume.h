#pragma once
#ifndef CE_API_FILES_BOUNDINGVOLUME_H
#define CE_API_FILES_BOUNDINGVOLUME_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Helper/SavePtr.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			inline namespace BoundingVolume_v1 {
				enum class BoundingType { NONE, SPHERE, BOX };
				struct CLOAK_ALIGN BoundingDesc {
					BoundingType Type;
					struct {
						Global::Math::Vector Center;
						float Radius;
					} Sphere;
					struct {
						Global::Math::Vector Center;
						Global::Math::Vector ScaledAxis[3];
					} Box;
					struct {
					} None;
				};
				CLOAK_INTERFACE_ID("{F37000E6-B267-43A0-AFEE-828FBCD50231}") IBoundingVolume : public virtual Helper::SavePtr_v1::ISavePtr{
					public:
						virtual bool CLOAK_CALL_THIS Check(In const Global::Math::OctantFrustum& frst, In size_t passID, In_opt bool checkNearPlane = true) = 0;
						virtual void CLOAK_CALL_THIS SetLocalToWorld(In const API::Global::Math::Matrix& localToWorld) = 0;
						virtual BoundingDesc CLOAK_CALL_THIS GetDescription() const = 0;
						virtual void CLOAK_CALL_THIS Update() = 0;
						virtual float CLOAK_CALL_THIS GetRadius() const = 0;
						virtual API::Global::Math::AABB CLOAK_CALL_THIS ToAABB() const = 0;
						virtual API::Global::Math::AABB CLOAK_CALL_THIS ToAABB(In const API::Global::Math::Matrix& WorldToProjection) const = 0;
				};
				CLOAKENGINE_API IBoundingVolume* CLOAK_CALL CreateBoundingVolume(In const BoundingDesc& desc);
				CLOAKENGINE_API IBoundingVolume* CLOAK_CALL UpdateBoundingVolume(In IBoundingVolume* volume, In const BoundingDesc& desc);
			}
		}
	}
}

#endif