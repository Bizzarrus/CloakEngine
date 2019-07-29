#pragma once
#ifndef CE_IMPL_COMPONENTS_CAMERA_H
#define CE_IMPL_COMPONENTS_CAMERA_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Components/Camera.h"
#include "CloakEngine/Components/Position.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Rendering/RenderPass.h"

#include "Implementation/Files/Scene.h"
#include "Implementation/Components/Position.h"

#include "Engine/WorldStreamer.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			inline namespace Camera_v1 {
				struct CamSettings {
					float Near;
					float Mid;
					float Far;
					float LoadRange;
					float BloomThreshold;
					float BloomBoost;
				};
				class CLOAK_UUID("{CD89DB07-C18E-45B7-B212-4CFB24DBAD93}") Camera : public virtual API::Components::Camera_v1::ICamera {
					public:
						typedef API::Components::Camera_v1::ICamera Super;

						CLOAK_CALL Camera(In API::World::IEntity* parent, In type_id typeID);
						CLOAK_CALL Camera(In API::World::IEntity* parent, In API::World::Component* src);
						virtual CLOAK_CALL ~Camera();

						virtual void CLOAK_CALL_THIS SetViewDistance(In float maxNearDist, In_opt float nearDist = 0.1f, In_opt float farOffset = 0, In_opt float loadOffset = 64) override;
						virtual void CLOAK_CALL_THIS SetBloom(In float threshold, In float boost) override;
						virtual void CLOAK_CALL_THIS SetFOV(In float fov) override;
						virtual void CLOAK_CALL_THIS ResetFOV() override;
						virtual void CLOAK_CALL_THIS SetProjection(In API::Components::Camera_v1::ProjectionType type) override;
						virtual void CLOAK_CALL_THIS SetRenderTarget(In API::Components::Camera_v1::RenderTargetType type, In_opt API::Rendering::IColorBuffer* target = nullptr) override;
						virtual void CLOAK_CALL_THIS SetRenderTarget(In API::Components::Camera_v1::RenderTargetType type, In float left, In float right, In float top, In float bottom, In_opt API::Rendering::IColorBuffer* target = nullptr) override;
						virtual void CLOAK_CALL_THIS SetRenderTarget(In API::Components::Camera_v1::RenderTargetType type, In API::Rendering::IColorBuffer* target, In float left, In float right, In float top, In float bottom) override;
						
						virtual void CLOAK_CALL_THIS Update(In API::Global::Time time, In Impl::Components::Position* pos, In unsigned int W, In unsigned int H, In float fov, In bool updated, Out API::Rendering::CameraData* data);

						virtual const API::Global::Math::Matrix& CLOAK_CALL_THIS GetWorldToView() const;
						virtual const API::Global::Math::Matrix& CLOAK_CALL_THIS GetViewToProjection() const;
						virtual const CamSettings& CLOAK_CALL_THIS GetSettings() const;
						virtual API::Rendering::RenderTargetData CLOAK_CALL_THIS GetRenderTarget(In API::Rendering::IColorBuffer* screen) const;
						virtual const API::Global::Math::Vector& CLOAK_CALL_THIS GetViewPosition() const;

						virtual void CLOAK_CALL_THIS OnNodeChange(In API::Files::IScene* scene, In API::Files::IWorldNode* node) override;
						virtual void CLOAK_CALL_THIS OnEnable() override;
						virtual void CLOAK_CALL_THIS OnDisable() override;
					protected:
						Engine::WorldStreamer::streamAdr m_streamAdr;
						float m_fov;
						bool m_ownFov;
						API::Global::Math::Vector m_pos;
						API::Global::Math::Matrix m_WTV;
						API::Global::Math::Matrix m_VTP;
						API::Components::Camera_v1::ProjectionType m_type;
						Impl::Files::WorldNode* m_node;
						API::Components::Camera_v1::RenderTargetType m_targetType;
						API::Rendering::IColorBuffer* m_target;
						struct {
							float Left;
							float Right;
							float Top;
							float Bottom;
						} m_targetSize;
						CamSettings m_settings;
						bool m_updatedClip;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif