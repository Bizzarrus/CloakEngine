#pragma once
#ifndef CE_API_COMPONENTS_CAMERA_H
#define CE_API_COMPONENTS_CAMERA_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/World/Entity.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Files/Scene.h"
#include "CloakEngine/Rendering/ColorBuffer.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Components {
			inline namespace Camera_v1 {
				enum class ProjectionType {
					Perspective,
					Orthographic,
				};
				enum class RenderTargetType {
					None,
					Screen,
					Texture,
				};
				CLOAK_COMPONENT("{14CE66C3-FF7C-4B58-904F-67BA53F38818}", ICamera) {
					public:
						CLOAKENGINE_API CLOAK_CALL ICamera(In API::World::IEntity* e, In type_id typeID) : Component(e, typeID) {}
						virtual void CLOAK_CALL_THIS SetViewDistance(In float maxNearDist, In_opt float nearDist = 0.1f, In_opt float farOffset = 0, In_opt float loadOffset = 64) = 0;
						virtual void CLOAK_CALL_THIS SetBloom(In float threshold, In float boost) = 0;
						virtual void CLOAK_CALL_THIS SetFOV(In float fov) = 0;
						virtual void CLOAK_CALL_THIS ResetFOV() = 0;
						virtual void CLOAK_CALL_THIS SetProjection(In ProjectionType type) = 0;
						//target must be nullptr if and only if type is not Texture
						virtual void CLOAK_CALL_THIS SetRenderTarget(In RenderTargetType type, In_opt API::Rendering::IColorBuffer* target = nullptr) = 0;
						virtual void CLOAK_CALL_THIS SetRenderTarget(In RenderTargetType type, In float left, In float right, In float top, In float bottom, In_opt API::Rendering::IColorBuffer* target = nullptr) = 0;
						virtual void CLOAK_CALL_THIS SetRenderTarget(In RenderTargetType type, In API::Rendering::IColorBuffer* target, In float left, In float right, In float top, In float bottom) = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif