#pragma once
#ifndef CE_API_RENDERING_RENDERPASS_H
#define CE_API_RENDERING_RENDERPASS_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Global/Graphic.h"
#include "CloakEngine/Files/Image.h"
#include "CloakEngine/Rendering/Manager.h"
#include "CloakEngine/Rendering/DepthBuffer.h"
#include "CloakEngine/Rendering/ColorBuffer.h"
#include "CloakEngine/Rendering/PipelineState.h"
#include "CloakEngine/Rendering/Context.h"
#include "CloakEngine/Interface/BasicGUI.h"
#include "CloakEngine/Components/Camera.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace RenderPass_v1 {
				typedef IContext3D*(CLOAK_CALL *WorkerSetup)(IContext* context, size_t passID, void* data);

				struct RenderTargetData {
					API::Rendering::IColorBuffer* Target;
					struct {
						float X;
						float Y;
						float W;
						float H;
					} UV;
					struct {
						UINT X;
						UINT Y;
						UINT W;
						UINT H;
					} Pixel;
					struct {
						UINT W;
						UINT H;
					} BaseSize;
				};

				struct PassData {
					bool ResetFrame;
					bool HDR;
					API::Global::Graphic::BloomMode Bloom;
					API::Global::Graphic::MSAA MSAA;
					float Gamma;
				};
				struct CameraData {
					struct {
						float Boost;
						float Threshold;
					} Bloom;
					size_t WorldID; //This ID is required for visibility tests!
					float SkyIntensity;
					float NearPlane;
					float FarPlane;
					float ViewDistance; //ViewDistance is used to determine LOD. This value is independend of Near/Far Plane!
					bool UpdatedClip;
					Files::ICubeMap* Skybox;
					Helper::Color::RGBA AmbientColor;
					API::Global::Math::Matrix WorldToView;
					API::Global::Math::Matrix ViewToWorld;
					API::Global::Math::Matrix WorldToClip;
					API::Global::Math::Matrix ClipToWorld;
					API::Global::Math::Matrix ViewToClip;
					API::Global::Math::Matrix ClipToView;
					API::Global::Math::Vector ViewPosition;
				};
				CLOAK_INTERFACE_ID("{2C325CE4-CF60-446B-822E-D6617E8EEBCE}") IRenderWorker : public virtual Helper::SavePtr_v1::ISavePtr {
					public:
						virtual void* CLOAK_CALL_THIS StartDraw(In IManager* manager, In Rendering::IContext* context, In size_t bufferSize) = 0;
						virtual void* CLOAK_CALL_THIS StartDraw(In IManager* manager, In Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In size_t bufferSize) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In bool increasePassID) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In bool increasePassID) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In bool increasePassID) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In bool increasePassID) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In bool increasePassID) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In const API::Global::Math::Vector& camPos, In float viewDistance, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In const API::Global::Math::Vector& camPos, In float viewDistance, In bool increasePassID) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) = 0;
						virtual void CLOAK_CALL_THIS Setup(In WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In bool increasePassID) = 0;
						virtual void CLOAK_CALL_THIS Draw(In IDrawable3D* toDraw) = 0;
						virtual void CLOAK_CALL_THIS FinishDraw(Inout Rendering::IContext** context, In_opt bool increasePassID = true) = 0;
						virtual void CLOAK_CALL_THIS WaitForExecution() = 0;
				};
				CLOAK_INTERFACE_ID("{886CF8A8-A6D3-475D-AB4D-957F71DA1CAB}") IRenderPass : public virtual Helper::SavePtr_v1::ISavePtr {
					public:
						virtual void CLOAK_CALL_THIS OnInit(In IManager* manager) = 0;
						virtual void CLOAK_CALL_THIS OnResize(In IManager* manager, In const Global::Graphic::Settings& newSet, In const Global::Graphic::Settings& oldSet, In bool updateShaders) = 0;
						virtual void CLOAK_CALL_THIS OnRenderCamera(In IManager* manager, Inout Rendering::IContext** context, In IRenderWorker* worker, In Components::ICamera* camera, In const RenderTargetData& target, In const CameraData& camData, In const PassData& pass, In API::Global::Time etime) = 0;
						virtual void CLOAK_CALL_THIS OnRenderInterface(In IManager* manager, Inout Rendering::IContext** context, In IRenderWorker* worker, In IColorBuffer* screen, In size_t numGuis, In API::Interface::IBasicGUI** guis, In const PassData& pass, In API::Global::Time etime) = 0;
				};
			}
		}
		namespace Global {
			namespace Graphic {
				CLOAKENGINE_API void CLOAK_CALL SetRenderPass(In Rendering::IRenderPass* pass);
			}
		}
	}
}

#endif