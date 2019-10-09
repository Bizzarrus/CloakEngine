#include "stdafx.h"
#include "Engine/Graphic/DefaultRenderPass.h"
#include "Engine/Graphic/DefaultShaderLib.h"

#include "CloakEngine/Rendering/Context.h"
#include "CloakEngine/Global/Video.h"
#include "CloakEngine/World/ComponentManager.h"
#include "CloakEngine/Components/Render.h"
#include "CloakEngine/Components/Light.h"

#include <atomic>

//#define PRINT_PERFORMANCE_TIMER

#ifdef _DEBUG
#define SET_DEBUG_RESOURCE_NAME(rsc, name) (rsc)->SetDebugResourceName(name)
#else
#define SET_DEBUG_RESOURCE_NAME(rsc, name)
#endif

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {
			constexpr unsigned int TIMER_PRINTING_TIME = 1024;

			constexpr unsigned int RENDERING_POST_PROCESS_BUFFER_COUNT = 3;
			constexpr unsigned int RENDERING_BLOOM_BUFFER_COUNT = 4;
			constexpr unsigned int RENDERING_DEFERRED_BUFFER_COUNT = 5;
			constexpr unsigned int RENDERING_OIB_BUFFER_COUNT = 2;
			constexpr unsigned int RENDERING_SHADOW_MAP_COUNT = 2;

			constexpr float STANDARD_NITS = 100.0f;
			constexpr size_t SHADOW_CASCADE_COUNT = 4;
			constexpr size_t FRUSTRUM_CORNERS_CLIP_SIZE = 8;
			constexpr size_t FRUSTRUM_CORNERS_CLIP_HALF = FRUSTRUM_CORNERS_CLIP_SIZE >> 1;
			constexpr UINT SHADOW_MAP_WIDTH = static_cast<UINT>(SHADOW_CASCADE_COUNT >> 1);
			constexpr UINT SHADOW_MAP_HEIGHT = static_cast<UINT>(SHADOW_CASCADE_COUNT / SHADOW_MAP_WIDTH);
			constexpr float SHADOW_MAP_TEXCOORD_X = 1.0f / SHADOW_MAP_WIDTH;
			constexpr float SHADOW_MAP_TEXCOORD_Y = 1.0f / SHADOW_MAP_HEIGHT;
			constexpr float SHADOW_MAP_TEXCOORD_X_HALF = SHADOW_MAP_TEXCOORD_X / 2;
			constexpr float SHADOW_MAP_TEXCOORD_Y_HALF = SHADOW_MAP_TEXCOORD_Y / 2;
			constexpr float SHADOW_MAP_OVERLAP_OFFSET = 0.08f;
			constexpr float SHADOW_MAP_LAMBDA = 0.8f;
			constexpr double LUMINANCE_BLEND_FACTOR = 0.0065;
			constexpr double LUMINANCE_BLEND_INV = 1 - LUMINANCE_BLEND_FACTOR;
			constexpr float LUMINANCE_FREQUENCY_NORMAL = 1000.0f / 60.0f;
			constexpr size_t BLUR_KERNEL_SIZE = 15;
			constexpr size_t BLUR_KERNEL_HALF = BLUR_KERNEL_SIZE >> 1;
			constexpr size_t BLUR_CHUNK_SIZE = 128 - (BLUR_KERNEL_HALF << 1);
			constexpr size_t SHADOW_CASCADE_BLUR_SIZE[SHADOW_CASCADE_COUNT] = {7, 5, 4, 2};
			constexpr size_t SHADOW_CASCADE_NORM_SIZE = 1024;
			constexpr size_t LUMINANCE_BUFFER_COUNT = 2;
			constexpr size_t LUMINANCE_DISPATCH_X = 16;
			constexpr size_t LUMINANCE_DISPATCH_Y = 16;
			constexpr size_t LUMINANCE_REDUCTION_SIZE = 128;
			constexpr size_t BLOOM_ITERATIONS = 2;
			constexpr size_t BLOOM_BORDER_OFFSET = 1 << (BLOOM_ITERATIONS - 1);

			const API::Global::Math::Vector FRUSTRUM_CORNERS_CLIP[FRUSTRUM_CORNERS_CLIP_SIZE] = { { -1,-1,1 },{ 1,-1,1 },{ -1,1,1 },{ 1,1,1 },{ -1,-1,0 },{ 1,-1,0 },{ -1,1,0 },{ 1,1,0 } };

#define PSO_CALL_INIT_true(Name) API::Rendering::IPipelineState* Name##Simple; API::Rendering::IPipelineState* Name##Complex
#define PSO_CALL_INIT_false(Name) API::Rendering::IPipelineState* Name
#define PSO_CALL_INIT(Name, Complex,...) PSO_CALL_INIT_##Complex(Name)

#define PSO_CALL_CREATE_false(Name) Name=man->CreatePipelineState()
#define PSO_CALL_CREATE_true(Name) Name##Simple=man->CreatePipelineState(); Name##Complex=man->CreatePipelineState()
#define PSO_CALL_CREATE(Name, Complex,...) PSO_CALL_CREATE_##Complex(Name)

#ifdef _DEBUG
#define PSO_CALL_RESET_BASE(Name) if(DefaultShaderLib::ResetPSO(Name,DefaultShaderLib::ShaderType::Name,gset,hardware)==false){CloakDebugLog(std::string("Could not reset ")+#Name);}
#else
#define PSO_CALL_RESET_BASE(Name) DefaultShaderLib::ResetPSO(Name,DefaultShaderLib::ShaderType::Name,gset,hardware)
#endif

#define PSO_CALL_RESET_true(Name) PSO_CALL_RESET_BASE(Name##Simple); PSO_CALL_RESET_BASE(Name##Complex)
#define PSO_CALL_RESET_false(Name) PSO_CALL_RESET_BASE(Name)
#define PSO_CALL_RESET(Name, Complex,...) PSO_CALL_RESET_##Complex(Name)

#define PSO_CALL_RELEASE_true(Name) SAVE_RELEASE(Name##Simple); SAVE_RELEASE(Name##Complex)
#define PSO_CALL_RELEASE_false(Name) SAVE_RELEASE(Name)
#define PSO_CALL_RELEASE(Name, Complex,...) PSO_CALL_RELEASE_##Complex(Name)
			struct {
				CE_PER_SHADER(PSO_CALL_INIT,CE_SEP_SIMECOLON)
				void CLOAK_CALL Create(In API::Rendering::IManager* man) { CE_PER_SHADER(PSO_CALL_CREATE, CE_SEP_SIMECOLON) }
				void CLOAK_CALL Reset(In const API::Global::Graphic::Settings& gset, In const API::Rendering::Hardware& hardware) { CE_PER_SHADER(PSO_CALL_RESET, CE_SEP_SIMECOLON); }
				void CLOAK_CALL Release() { CE_PER_SHADER(PSO_CALL_RELEASE, CE_SEP_SIMECOLON) }
			} g_pso;

			struct CLOAK_ALIGN ProjBuffer {
				API::Rendering::MATRIX ViewToProject;
				API::Rendering::MATRIX ProjectToView;
			};
			struct CLOAK_ALIGN ViewBuffer {
				API::Rendering::MATRIX WorldToView;
				API::Rendering::MATRIX ViewToWorld;
				API::Rendering::MATRIX WorldToProjection;
				API::Rendering::MATRIX ProjectionToWorld;
				API::Rendering::FLOAT3 ViewPos;
			};
			struct CLOAK_ALIGN FluidBuffer {
				API::Rendering::FLOAT1 GlobalWetness;
				API::Rendering::FLOAT1 Specular; //F0 (Fluid to Air)
				API::Rendering::FLOAT1 RefractionFactor; //IOR-Air / IOR-Fluid
			};
			struct CLOAK_ALIGN CSMData {
				API::Rendering::FLOAT1 NCS[SHADOW_CASCADE_COUNT];
				API::Rendering::FLOAT1 LCS[SHADOW_CASCADE_COUNT];
				API::Rendering::MATRIX WorldToLight[SHADOW_CASCADE_COUNT];
			};
			struct CLOAK_ALIGN DirLightBuffer {
				API::Rendering::FLOAT4 Direction;
				API::Rendering::FLOAT4 Color;
				CSMData CSM;
			};
			struct CLOAK_ALIGN DirViewBuffer {
				API::Rendering::MATRIX WorldToLightClip;
			};

			API::Rendering::IConstantBuffer* g_projBuffer = nullptr;
			API::Rendering::IConstantBuffer* g_viewBuffer = nullptr;
			API::Rendering::IConstantBuffer* g_fluidBuffer = nullptr;
			API::Rendering::IConstantBuffer* g_diLiBuffer = nullptr;
			API::Rendering::IConstantBuffer* g_diViewBuffer[SHADOW_CASCADE_COUNT] = { nullptr, nullptr, nullptr, nullptr };
			API::Rendering::IStructuredBuffer* g_luminanceBuffer[LUMINANCE_BUFFER_COUNT] = { nullptr, nullptr };
			/*
			Deffered Buffer:
			[ R G B X ] R, G, B = Albedo (Red, Green, Blue), X = ?
			[ N N N O ] N = Normal (X, Y, Z), O = Ambient Occlusion (?)
			[ S S S R ] S = Specular (Red, Green, Blue), R = Roughness
			[ S P W R ] S = Specular Water, P = porosity, W = Wet Level, R = Liquid Refraction coefficient (IOR Air / IOR Liquid)
			[ M ]		M = MSAA buffer
			*/
			API::Rendering::IColorBuffer* g_renderDeferredBuffer[RENDERING_DEFERRED_BUFFER_COUNT] = { nullptr,nullptr,nullptr,nullptr,nullptr };
			/*
			Post Process Buffer:
			Swaping buffers for post process (blur, etc)
			*/
			API::Rendering::IColorBuffer* g_renderPostBuffer[RENDERING_POST_PROCESS_BUFFER_COUNT] = { nullptr,nullptr,nullptr };
			/*
			Interface/load screen/video buffer
			*/
			//API::Rendering::IColorBuffer* g_renderInterface = nullptr;
			/*
			Weighted Order Independend Transparency buffers:
			[ R G B A ] R, G, B = Weighted pre-multiplied color, A = weighted alpha
			[ R ] R = Reveal
			*/
			API::Rendering::IColorBuffer* g_renderOITBuffer[RENDERING_OIB_BUFFER_COUNT] = { nullptr,nullptr };
			/*
			Shadow map buffer
			*/
			API::Rendering::IColorBuffer* g_renderShadowMapBuffer[RENDERING_SHADOW_MAP_COUNT] = { nullptr,nullptr };
			/*
			Bloom buffer, to downscale, blur and sum back
			*/
			API::Rendering::IColorBuffer* g_renderBloomBuffer[RENDERING_BLOOM_BUFFER_COUNT] = { nullptr, nullptr, nullptr, nullptr };
			/*
			Post Process depth buffer, mainly used for MSAA handling (stencil buffer)
			*/
			API::Rendering::IDepthBuffer* g_renderScreenDepthBuffer = nullptr;
			/*
			Depth buffer for deffered rendering + weighted order inepended transparency
			*/
			API::Rendering::IDepthBuffer* g_renderDefferedDepthBuffer = nullptr;
			/*
			Shadow map depth buffer
			*/
			API::Rendering::IDepthBuffer* g_renderShadowDepthBuffer = nullptr;

			std::atomic<UINT> g_screenW = 0;
			std::atomic<UINT> g_screenH = 0;
			std::atomic<bool> g_createdRessources = false;
			std::atomic<uint32_t> g_shadowMapRes = 1024;
			std::atomic<bool> g_lerpLuminance = false;
			std::atomic<size_t> g_curLuminanceBuf = 0;
			std::atomic<API::Components::ICamera*> g_lastCam = nullptr;

#ifdef PRINT_PERFORMANCE_TIMER
			CloakCreateTimer(g_timerStartup);
			CloakCreateTimer(g_timerMSAAPrePass);
			CloakCreateTimer(g_timerSkybox);
			CloakCreateTimer(g_timerAmbientLight);
			CloakCreateTimer(g_timerIBL);
			CloakCreateTimer(g_timerDirectionalLight);
			CloakCreateTimer(g_timerWorldPhase1);
			CloakCreateTimer(g_timerWorldPhase2);
			CloakCreateTimer(g_timerWorldPhase3);
			CloakCreateTimer(g_timerWorldPhase4);
			CloakCreateTimer(g_timerOpaqueResolve);
			CloakCreateTimer(g_timerLuminanceReduction);
			CloakCreateTimer(g_timerBloom);
			CloakCreateTimer(g_timerToneMapping);
			CloakCreateTimer(g_timerInterface);

			CloakCreateTimer(g_timerDL_Cascades);
			CloakCreateTimer(g_timerDL_Blur);
			CloakCreateTimer(g_timerDL_Light);
#endif

			namespace Utility {
				inline void CLOAK_CALL SetScreenRect(In API::Rendering::IContext* context, In UINT W, In UINT H, In_opt UINT X = 0, In_opt UINT Y = 0)
				{
					context->SetViewportAndScissor(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(W), static_cast<float>(H));
				}

				//lambda: value between 0 and 1 to determine sharpness of curve (0 creates linear curve, therefore equaly spaced split ranges)
				//offset: Overlap-offset to allow blending between cascades
				inline float CLOAK_CALL GetCascadePoint(In size_t ID, In float nearPlane, In float farPlane, In float lambda, In float offset, Out_opt float* pointClip = nullptr)
				{
					float A = (static_cast<float>(ID) + offset) / SHADOW_CASCADE_COUNT;
					A = min(1, max(0, A));
					float B = farPlane / nearPlane;
					float C = farPlane - nearPlane;
					const float d = (lambda*(((nearPlane*(powf(B, A) - 1)) / C) - A)) + A;
					if (pointClip != nullptr) { *pointClip = (farPlane*d) / ((C*d) + nearPlane); }
					return d;
				}
			}

			namespace RenderPasses {
				struct PassData {
					API::Rendering::IColorBuffer* RTV[RENDERING_DEFERRED_BUFFER_COUNT];
					API::Rendering::IDepthBuffer* DSV;
					UINT W;
					UINT H;
					API::Global::Math::Matrix WorldToProjection;
				};
				struct ShadowData {
					uint32_t X;
					uint32_t Y;
					uint32_t W;
					uint32_t H;
					size_t bufferID;
					API::Global::Math::Matrix WorldToLightClip;
				};

				API::Rendering::IContext3D* CLOAK_CALL DrawInitCommand(In API::Rendering::IContext* context, In size_t passID, In void* data)
				{
					PassData* d = reinterpret_cast<PassData*>(data);
					context->SetRenderTargets(RENDERING_DEFERRED_BUFFER_COUNT, d->RTV, d->DSV);
					context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);

					Utility::SetScreenRect(context, d->W, d->H);
					context->SetConstantBuffer(0, g_projBuffer);
					context->SetConstantBuffer(1, g_viewBuffer);
					context->SetConstantBuffer(3, g_fluidBuffer);

					return context->Enable3D(d->WorldToProjection, true, false, 4, 2, 7, 6);
				}
				API::Rendering::IContext3D* CLOAK_CALL DrawShadowCommand(In API::Rendering::IContext* context, In size_t passID, In void* data)
				{
					ShadowData* d = reinterpret_cast<ShadowData*>(data);

					CLOAK_ASSUME(d->bufferID != 0 || (d->X == 0 && d->Y == 0));
					CLOAK_ASSUME(d->bufferID != 1 || (d->X == d->W && d->Y == 0));
					CLOAK_ASSUME(d->bufferID != 2 || (d->X == 0 && d->Y == d->H));
					CLOAK_ASSUME(d->bufferID != 3 || (d->X == d->W && d->Y == d->H));
					CLOAK_ASSUME(d->W == d->H);
					CLOAK_ASSUME((reinterpret_cast<uintptr_t>(d) & 0xF) == 0);
					CLOAK_ASSUME((reinterpret_cast<uintptr_t>(&d->WorldToLightClip) & 0xF) == 0);

					context->SetRenderTargets(1, &g_renderShadowMapBuffer[0], g_renderShadowDepthBuffer);
					context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);

					Utility::SetScreenRect(context, d->W, d->H, d->X, d->Y);
					context->SetConstantBuffer(0, g_projBuffer);
					context->SetConstantBuffer(1, g_viewBuffer);
					context->SetConstantBuffer(3, g_diViewBuffer[d->bufferID]);

					return context->Enable3D(d->WorldToLightClip, false, false, INVALID_ROOT_INDEX, 2, 5, 4);
				}


				/*
				GPU based culling pipeline overview:
				1. Calculate HTILE structure from reprojected last depth buffer (compute shader, 8x8 thread group size)
					1.1 Get reprojected min/max depth (see below) of each pixel
					1.2 Get min/max depth across all threads in group (use WaveActiveMin/WaveActiveMax)
					1.3 Thread 0 of each group: Compress min/max depth into 32 bit integer (16 bit per value) and write into buffer
					1.4 Create Octree structure with each parent element have min/max depth of 2x2 chunks
				2. Batch all objects in Buffer
					- Bounding Volume (AABB)
					- IBV/VBV/CBV/SRV (Material)
					- Vertex Count & Offset
				3. Coarse Culling ("Occluders Pass", compute shader)
					3.1 Occlusion Culling (Bounding Volume)
						3.1.1 Use reprojected depth buffer from last frame
							- Use HTILE structure
							- Use max difference of floor(vertex pos * screen size / 8) to determine octree level
							- Cull if maxBoundingDepth < minHTILEDepth (see below)
						3.1.2 Perform based on fail/success:
							3.1.2.1 On Success: Write in occluders argument Buffer
								- Use de-interleaved possition vertex buffer also as CBV
								- Issue dispatch calls for 4.
							3.1.2.2 On Fail: Copy in occludee argument buffer
								- Same layout as current argument buffer
				4. Per Triangle Culling ("Occluders Pass", compute shader using ExecuteIndirect from 3.1.2.1) | Requires implementation for Triangle Strips and Triangle Lists
					4.1 Read vertex possitions and transform to screen space
					4.2 Orientation and Zero Area Culling
						4.2.1 Cull if determinant(float4x4(pos[0].xyw, pos[2].xyw, pos[3].xyw)) <= 0
					4.3 Small Primitive Culling
						4.3.1 Cull if AABB of primitive does not occlude at least one pixel center
							- Cull if any(round(min.xy) == round(max.xy))
								=> Round to pixel borders
								=> Cull if rounded to same border in x or y direction
					4.4 Frustrum Culling
						4.4.1 Cull if max.xy < 0 or min.xy > 2
					4.5 Depth Culling
						4.5.1 Cull if max.z < minDepth using reprojected depth buffer from last frame (see 3.1.1)
							4.5.1.1 If Culled by depth
								- Write culled indices into culled index buffer, write dispatch calles into culled argument buffer
									- Also write transformed vertex possitions into argument buffer (no reason to compute them again)
					4.6 Write remaining triangle indices in draw index buffer, write draw calls into draw argument buffer
				5. Draw Occluders using ExecuteIndirect from 4.6
				6. Calculate HTILE structure from depth buffer
					6.1 Get depth at pixel pos
					6.2 Get min/max depth across all threads in group (use WaveActiveMin/WaveActiveMax)
					6.3 Thread 0 of each group: Compress min/max depth into 32 bit integer (16 bit per value) and write into buffer
					6.4 Create Octree structure with each parent element have min/max depth of 2x2 chunks
				7. Per Triangle Culling ("False Negatives", compute shader using ExecuteIndirect from 4.5.1.1)
					7.1 Depth Culling
						7.1.1 Cull if max.z < minDepth using depth buffer from 5.
							- Use HTILE structure
							- Use max difference of floor(vertex pos * screen size / 8) to determine octree level
							- Cull if maxBoundingDepth < minHTILEDepth (see below)
					7.2 Write remaining triangle indices in draw index buffer, write draw calls into draw argument buffer
				8. Coarse Culling ("Occludees Pass", compute shader)
					8.1 Occlusion Culling (Bounding Volume)
						8.1.1 Use depth buffer from 5. (See 7.1.1)
					8.2 Write remaining arguments in occludees argument Buffer
						- Use de-interleaved possition vertex buffer also as CBV
								- Issue dispatch calls for 9.
				9. Per Triangle Culling ("Occludees Pass", compute shader using ExecuteIndirect from 8.2) | Requires implementation for Triangle Strips and Triangle Lists
					9.1 Read vertex possitions and transform to screen space
					9.2 Orientation and Zero Area Culling
						9.2.1 Cull if determinant(float4x4(pos[0].xyw, pos[2].xyw, pos[3].xyw)) <= 0
					9.3 Small Primitive Culling
						9.3.1 Cull if AABB of primitive does not occlude at least one pixel center
							- Cull if any(round(min.xy) == round(max.xy))
								=> Round to pixel borders
								=> Cull if rounded to same border in x or y direction
					9.4 Frustrum Culling
						9.4.1 Cull if max.xy < 0 or min.xy > 2
					9.5 Depth Culling
						9.5.1 Cull if max.z < minDepth using depth buffer from 5. (See 7.1.1)
					9.6 Write remaining triangle indices in draw index buffer, write draw calls into draw argument Buffer
				10. Draw Occludees using ExecuteIndirect from 9.6 and 7.2



				Depth Buffer Reprojection:
				float2 GetRerpojectedDepth(float2 texPos, Texture2D<float> depth, Matrix4x4 ProjectionToWorld, Matrix4x4 lastWorldToProjection, SamplerState ClampPointSampler)
				{
					//ProjectionToWorld: Current Frame's Projection to World Matrix
					//lastWorldToProjection: Last Frame's World to Projection Matrix
					//ClampPointSampler: Sampler with clamp and point sampling
					float4 pos=float4((texPos.x * 2) - 1, 1 - (texPos.y * 2),0,1); //Remember: Z = 0 -> Far Plane
					pos = mul(mul(pos, ProjectionToWorld), lastWorldToProjection);
					pos /=pos.w;
					texPos = float2(pos.x + 1, 1 - pos.y) / 2;
					float4 d = depth.Gather(ClampPointSampler, texPos);
					d.xy = d.x < d.y ? d.xy : d.yx;
					d.zw = d.z < d.w ? d.zw : d.wz;
					d.xz = d.x < d.z ? d.xz : d.zx;
					d.yw = d.y < d.w ? d.yw : d.wy;
					return d.xw;
				}

				Bounding Volumes:
				- Allways upload Bounding Box AND Bounding Sphere
					- Format: [Center, Radius] + 3x[Half Side]
					- For Bounding Spheres: Use Radius in X,Y,Z as half Sides (Box around Sphere)
					- For Bounding Boxes: use max Distance(Center, Corner) as Radius (Sphere around Box)
				- Test against both bounding volumes
					- Frustrum culling: Cull if test against either one bounding volume failes
					- Depth culling: cull if min(Depth of Center + Radius, max(Depth of all Corners)) < min depth of depth buffer
				*/

				inline void CLOAK_CALL Startup(In API::Rendering::IContext* con, In API::Rendering::IManager* manager, In const API::Rendering::CameraData& data, In bool updateClip)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerStartup);
#endif
					//TODO: set fluid and wetness
					const float iorf = 1.325f;
					const float wetness = 0;

					ViewBuffer view;
					ProjBuffer projection;
					FluidBuffer fluid;
					view.WorldToView = data.WorldToView;
					view.ViewToWorld = data.ViewToWorld;
					view.WorldToProjection = data.WorldToClip;
					view.ProjectionToWorld = data.ClipToWorld;
					view.ViewPos = data.ViewPosition;
					projection.ProjectToView = data.ClipToView;
					projection.ViewToProject = data.ViewToClip;
					const float ns = iorf - 1;
					const float nk = iorf + 1;
					fluid.GlobalWetness = wetness;
					fluid.Specular = (ns*ns) / (nk*nk);
					fluid.RefractionFactor = 1 / iorf;

					if (updateClip) { con->WriteBuffer(g_projBuffer, 0, &projection, sizeof(ProjBuffer)); }
					con->WriteBuffer(g_viewBuffer, 0, &view, sizeof(ViewBuffer));
					con->WriteBuffer(g_fluidBuffer, 0, &fluid, sizeof(FluidBuffer));
					
					for (unsigned int a = 0; a < RENDERING_DEFERRED_BUFFER_COUNT; a++) { con->TransitionResource(g_renderDeferredBuffer[a], API::Rendering::STATE_RENDER_TARGET); }
					for (unsigned int a = 0; a < RENDERING_POST_PROCESS_BUFFER_COUNT; a++) { con->TransitionResource(g_renderPostBuffer[a], API::Rendering::STATE_RENDER_TARGET); }
					for (unsigned int a = 0; a < RENDERING_OIB_BUFFER_COUNT; a++) { con->TransitionResource(g_renderOITBuffer[a], API::Rendering::STATE_RENDER_TARGET); }
					con->TransitionResource(g_renderScreenDepthBuffer, API::Rendering::STATE_DEPTH_WRITE);
					con->TransitionResource(g_renderDefferedDepthBuffer, API::Rendering::STATE_DEPTH_WRITE);
					con->TransitionResource(g_projBuffer, API::Rendering::STATE_VERTEX_AND_CONSTANT_BUFFER);
					con->TransitionResource(g_fluidBuffer, API::Rendering::STATE_VERTEX_AND_CONSTANT_BUFFER);
					con->TransitionResource(g_viewBuffer, API::Rendering::STATE_VERTEX_AND_CONSTANT_BUFFER, true);
					
					for (unsigned int a = 0; a < RENDERING_DEFERRED_BUFFER_COUNT; a++) { con->ClearColor(g_renderDeferredBuffer[a]); }
					for (unsigned int a = 0; a < RENDERING_POST_PROCESS_BUFFER_COUNT; a++) { con->ClearColor(g_renderPostBuffer[a]); }
					for (unsigned int a = 0; a < RENDERING_OIB_BUFFER_COUNT; a++) { con->ClearColor(g_renderOITBuffer[a]); }
					con->ClearDepthAndStencil(g_renderScreenDepthBuffer);
					con->ClearDepthAndStencil(g_renderDefferedDepthBuffer);
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerStartup);
					CloakPrintTimer(g_timerStartup, "Rendering Startup Time: ", TIMER_PRINTING_TIME);
#endif
				}
				inline void CLOAK_CALL MSAAPrePass(In API::Rendering::IContext* context, In UINT W, In UINT H, In bool MSAA)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerMSAAPrePass);
#endif
					if (MSAA)
					{
						context->SetPipelineState(g_pso.PreMSAA);
						context->SetRenderTargets(0, nullptr, g_renderScreenDepthBuffer, API::Rendering::DSV_ACCESS::READ_ONLY_DEPTH);
						context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);
						context->SetSRV(0, g_renderDeferredBuffer[RENDERING_DEFERRED_BUFFER_COUNT - 1]->GetSRV());
						Utility::SetScreenRect(context, W, H);
						context->Draw(3);
					}
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerMSAAPrePass);
					CloakPrintTimer(g_timerMSAAPrePass, "Rendering MSAAPrePass Time: ", TIMER_PRINTING_TIME);
#endif
				}
				inline void CLOAK_CALL Skybox(In API::Files::ICubeMap* skybox, In float intensity, In API::Rendering::IContext* context, In size_t targetFrame, In UINT W, In UINT H)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerSkybox);
#endif
					if (skybox != nullptr && skybox->isLoaded(API::Files::Priority::NORMAL))
					{
						context->SetPipelineState(g_pso.Skybox);
						context->SetRenderTargets(1, &g_renderPostBuffer[targetFrame], g_renderScreenDepthBuffer, API::Rendering::DSV_ACCESS::READ_ONLY);
						context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);
						context->SetSRV(2, skybox->GetBuffer()->GetSRV());
						context->SetConstants(3, intensity);
						Utility::SetScreenRect(context, W, H);
						context->SetConstantBuffer(0, g_projBuffer);
						context->SetConstantBuffer(1, g_viewBuffer);
					}
					else
					{
						context->SetPipelineState(g_pso.FadeOut);
						context->SetRenderTargets(1, &g_renderPostBuffer[targetFrame], g_renderScreenDepthBuffer, API::Rendering::DSV_ACCESS::READ_ONLY);
						context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);
						Utility::SetScreenRect(context, W, H);
						context->SetConstants(0, 0.0f, 0.0f, 0.0f, 1.0f);
					}
					context->Draw(3);
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerSkybox);
					CloakPrintTimer(g_timerSkybox, "Rendering Skybox Time: ", TIMER_PRINTING_TIME);
#endif
				}
				inline void CLOAK_CALL AmbientLightPass(In const API::Helper::Color::RGBA& color, In API::Rendering::IContext* context, In size_t targetFrame, In bool MSAA, In UINT W, In UINT H)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerAmbientLight);
#endif
					for (size_t a = 0; a < RENDERING_DEFERRED_BUFFER_COUNT; a++) { context->TransitionResource(g_renderDeferredBuffer[a], API::Rendering::STATE_PIXEL_SHADER_RESOURCE, a + 1 == RENDERING_DEFERRED_BUFFER_COUNT); }
					context->SetPipelineState(g_pso.AmbientOpaqueSimple);
					context->SetRenderTargets(1, &g_renderPostBuffer[targetFrame], g_renderScreenDepthBuffer);
					context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);
					context->SetSRV(0, g_renderDeferredBuffer[0]->GetSRV());
					context->SetConstants(1, color.R, color.G, color.B, color.A);
					context->SetDepthStencilRefValue(0);
					Utility::SetScreenRect(context, W, H);
					context->Draw(3);
					if (MSAA)
					{
						context->SetPipelineState(g_pso.AmbientOpaqueComplex);
						context->Draw(3);
					}
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerAmbientLight);
					CloakPrintTimer(g_timerAmbientLight, "Rendering Ambient Light: ", TIMER_PRINTING_TIME);
#endif
				}
				inline void CLOAK_CALL IBL(In API::Rendering::IContext* context, In API::Files::ICubeMap* skybox, In size_t diffuseFrame, In size_t specularFrame, In bool MSAA, In UINT W, In UINT H)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerIBL);
#endif
					if (skybox != nullptr && skybox->isLoaded(API::Files::Priority::NORMAL))
					{
						API::Files::ISkybox* sky = nullptr;
						if (SUCCEEDED(skybox->QueryInterface(CE_QUERY_ARGS(&sky))))
						{
							API::Rendering::IColorBuffer* RTV[] = { g_renderPostBuffer[diffuseFrame], g_renderPostBuffer[specularFrame] };

							for (size_t c = 0; c < ARRAYSIZE(RTV); c++) { context->TransitionResource(RTV[c], API::Rendering::STATE_RENDER_TARGET); }
							context->TransitionResource(g_renderDefferedDepthBuffer, API::Rendering::STATE_PIXEL_SHADER_RESOURCE);
							context->TransitionResource(g_renderScreenDepthBuffer, API::Rendering::STATE_DEPTH_WRITE, true);

							context->SetPipelineState(g_pso.IBLSimple);
							context->SetRenderTargets(ARRAYSIZE(RTV), RTV, g_renderScreenDepthBuffer, API::Rendering::DSV_ACCESS::READ_ONLY_STENCIL);
							context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);
							context->SetDepthStencilRefValue(0);
							Utility::SetScreenRect(context, W, H);
							context->SetConstantBuffer(0, g_projBuffer);
							context->SetConstantBuffer(1, g_viewBuffer);
							context->SetConstantBuffer(2, sky->GetIrradiance());

							API::Rendering::ResourceView views[] = { 
								g_renderDeferredBuffer[0]->GetDynamicSRV(),
								g_renderDeferredBuffer[1]->GetDynamicSRV(),
								g_renderDeferredBuffer[2]->GetDynamicSRV(),
								g_renderDeferredBuffer[3]->GetDynamicSRV(),
								g_renderDefferedDepthBuffer->GetDynamicDepthSRV(),
								sky->GetLUT()->GetDynamicSRV(),
								sky->GetRadianceMap()->GetDynamicSRV(),
							};
							
							context->SetConstants(3, 1.0f, 1.0f); //TODO: Should be loaded from skybox
							context->SetDescriptorTable(4, 0, &views[0], 4);
							context->SetDescriptorTable(5, 0, &views[4], 1);
							context->SetDescriptorTable(6, 0, &views[5], 2);
							context->Draw(3);
							if (MSAA)
							{
								context->SetPipelineState(g_pso.IBLComplex);
								context->Draw(3);
							}
						}
						SAVE_RELEASE(sky);
					}
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerIBL);
					CloakPrintTimer(g_timerIBL, "Rendering IBL Time: ", TIMER_PRINTING_TIME);
#endif
				}
				inline void CLOAK_CALL DirectionalLight(Inout API::Rendering::IContext** context, In API::Rendering::IRenderWorker* worker, In API::Rendering::IManager* manager, In const API::Rendering::CameraData& camData, In float lambda, In UINT W, In UINT H, In const API::World::Group<API::Components::IRender>& toDraw, In size_t diffuseFrame, In size_t specularFrame, In bool MSAA)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerDirectionalLight);
#endif
					API::Global::Math::Vector diaPos[4];
					diaPos[0] = FRUSTRUM_CORNERS_CLIP[0] * camData.ClipToView;
					diaPos[1] = FRUSTRUM_CORNERS_CLIP[4] * camData.ClipToView;
					diaPos[2] = FRUSTRUM_CORNERS_CLIP[3] * camData.ClipToView;
					diaPos[3] = FRUSTRUM_CORNERS_CLIP[7] * camData.ClipToView;
					API::Rendering::IColorBuffer* RTV[] = { g_renderPostBuffer[diffuseFrame], g_renderPostBuffer[specularFrame] };
					API::Rendering::IContext* con = *context;

					API::World::Group<API::Components::IDirectionalLight> lights;
					API::Components::DIRECTIONAL_LIGHT_INFO info;
					for (const auto& a : lights)
					{
						API::Components::IDirectionalLight* light = a.Get<API::Components::IDirectionalLight>();
						light->FillInfo(camData.WorldID, &info);
						if (info.Visible)
						{
							con->TransitionResource(g_renderShadowDepthBuffer, API::Rendering::STATE_DEPTH_WRITE);
							con->TransitionResource(g_renderShadowMapBuffer[0], API::Rendering::STATE_RENDER_TARGET, true);
							con->ClearDepthAndStencil(g_renderShadowDepthBuffer);
							con->ClearColor(g_renderShadowMapBuffer[0]);
							//Draw Cascaded Shadows:
#ifdef PRINT_PERFORMANCE_TIMER
							CloakStartTimer(g_timerDL_Cascades);
#endif
							API::Global::Math::Vector Up = API::Global::Math::Vector(0, 1, 0);
							if (fabsf(Up.Dot(info.Direction)) > 1.0f - 1e-3f) { Up = API::Global::Math::Vector(0, 0, 1); }
							API::Global::Math::Matrix WorldToLightView = API::Global::Math::Matrix::LookTo(API::Global::Math::Vector(0, 0, 0), info.Direction, Up);
							API::Global::Math::Matrix ClipToLightView = camData.ClipToWorld * WorldToLightView;
							API::Global::Math::Vector FC[FRUSTRUM_CORNERS_CLIP_SIZE];
							float lcsc[SHADOW_CASCADE_COUNT];
							float ncsc[SHADOW_CASCADE_COUNT];
							lcsc[SHADOW_CASCADE_COUNT - 1] = 1;
							API::Global::Math::Matrix WorldToCascade[SHADOW_CASCADE_COUNT];
							API::Global::Math::Matrix WorldToLightClip[SHADOW_CASCADE_COUNT];
							const uint32_t ShadowMapSize = g_shadowMapRes;
							for (uint32_t b = 0; b < FRUSTRUM_CORNERS_CLIP_SIZE; b++) { FC[b] = FRUSTRUM_CORNERS_CLIP[b] * ClipToLightView; }
							for (uint32_t b = 0; b < SHADOW_CASCADE_COUNT; b++)
							{
								float lcs = Utility::GetCascadePoint(b, camData.NearPlane, camData.FarPlane, lambda, -SHADOW_MAP_OVERLAP_OFFSET, b > 0 ? &lcsc[b - 1] : nullptr);
								float ncs = Utility::GetCascadePoint(b + 1, camData.NearPlane, camData.FarPlane, lambda, SHADOW_MAP_OVERLAP_OFFSET, &ncsc[b]);
								API::Global::Math::Vector diaC[2];
								diaC[0] = API::Global::Math::Vector::Lerp(diaPos[0], diaPos[1], lcs);
								diaC[1] = API::Global::Math::Vector::Lerp(diaPos[2], diaPos[3], ncs);
								API::Global::Math::Vector minV;
								API::Global::Math::Vector maxV;
								for (uint32_t c = 0; c < FRUSTRUM_CORNERS_CLIP_HALF; c++)
								{
									API::Global::Math::Vector corner = API::Global::Math::Vector::Lerp(FC[c], FC[c + FRUSTRUM_CORNERS_CLIP_HALF], lcs);
									if (c == 0) { minV = maxV = corner; }
									else
									{
										minV = API::Global::Math::Vector::Min(minV, corner);
										maxV = API::Global::Math::Vector::Max(maxV, corner);
									}
								}
								for (uint32_t c = FRUSTRUM_CORNERS_CLIP_HALF; c < FRUSTRUM_CORNERS_CLIP_SIZE; c++)
								{
									API::Global::Math::Vector corner = API::Global::Math::Vector::Lerp(FC[c - FRUSTRUM_CORNERS_CLIP_HALF], FC[c], ncs);
									minV = API::Global::Math::Vector::Min(minV, corner);
									maxV = API::Global::Math::Vector::Max(maxV, corner);
								}

								const float diav = std::ceil((diaC[0] - diaC[1]).Length());

								API::Global::Math::Vector dia(diav, diav, diav);
								API::Global::Math::Vector off = 0.5f*(dia - (maxV - minV));
								float upt = diav / ShadowMapSize;
								API::Global::Math::Point minP = static_cast<API::Global::Math::Point>(upt*API::Global::Math::Vector::Floor((minV - off) / upt));
								API::Global::Math::Point maxP = static_cast<API::Global::Math::Point>(upt*API::Global::Math::Vector::Floor((maxV + off) / upt));

								API::Global::Math::Point minD = static_cast<API::Global::Math::Point>(minV);
								API::Global::Math::Point maxD = static_cast<API::Global::Math::Point>(maxV);

								API::Global::Math::Matrix LightViewToLightClip = API::Global::Math::Matrix::OrthographicProjection(minP[0], maxP[0], minP[1], maxP[1], minD[2] - 1, maxD[2] + 1);
								WorldToLightClip[b] = WorldToLightView * LightViewToLightClip;

								const uint32_t x = b % SHADOW_MAP_WIDTH;
								const uint32_t y = b / SHADOW_MAP_WIDTH;
								const float xi = SHADOW_MAP_TEXCOORD_X_HALF + (SHADOW_MAP_TEXCOORD_X * x);
								const float yi = SHADOW_MAP_TEXCOORD_Y_HALF + (SHADOW_MAP_TEXCOORD_Y * y);
								WorldToCascade[b] = WorldToLightClip[b] * API::Global::Math::Matrix(SHADOW_MAP_TEXCOORD_X_HALF, 0, 0, 0, 0, -SHADOW_MAP_TEXCOORD_Y_HALF, 0, 0, 0, 0, 1, 0, xi, yi, 0, 1);

								DirViewBuffer dvBuffer;
								dvBuffer.WorldToLightClip = WorldToLightClip[b];
								con->WriteBuffer(g_diViewBuffer[b], 0, &dvBuffer, sizeof(DirViewBuffer));
								con->TransitionResource(g_diViewBuffer[b], API::Rendering::RESOURCE_STATE::STATE_VERTEX_AND_CONSTANT_BUFFER);
							}
								
							ShadowData* pdata = reinterpret_cast<ShadowData*>(worker->StartDraw(manager, con, sizeof(ShadowData) * SHADOW_CASCADE_COUNT));
							for (uint32_t b = 0; b < SHADOW_CASCADE_COUNT; b++)
							{
								const uint32_t x = b % SHADOW_MAP_WIDTH;
								const uint32_t y = b / SHADOW_MAP_WIDTH;

								pdata[b].WorldToLightClip = WorldToLightClip[b];
								pdata[b].X = ShadowMapSize * x;
								pdata[b].Y = ShadowMapSize * y;
								pdata[b].W = ShadowMapSize;
								pdata[b].H = ShadowMapSize;
								pdata[b].bufferID = b;
								API::Files::LOD maxLOD = API::Files::LOD::ULTRA;
								if (b > 2) { maxLOD = API::Files::LOD::LOW; }
								else if (b > 1) { maxLOD = API::Files::LOD::NORMAL; }
								else if (b > 0) { maxLOD = API::Files::LOD::HIGH; }
								worker->Setup(DrawShadowCommand, b * sizeof(ShadowData), API::Rendering::DRAW_TYPE::DRAW_TYPE_GEOMETRY_TESSELLATION, API::Rendering::DRAW_TAG::DRAW_TAG_OPAQUE, g_pso.DirectionalShadowOpaque, camData.ViewPosition, camData.ViewDistance, maxLOD);
								for (const auto& c : toDraw) 
								{
									API::Components::IRender* render = c.Get<API::Components::IRender*>();
									worker->Draw(render);
									render->Release();
								}
								//worker->Setup(DrawShadowCommand, API::Rendering::DRAW_TYPE::DRAW_TYPE_ALL, API::Rendering::DRAW_TAG::DRAW_TAG_ALPHA, 0, 0, g_pso.DirectionalShadowAlpha, maxLOD);
								//for (size_t c = 0; c < toDraw.size(); c++) { worker->Draw(toDraw[c]); }
								
								//TODO: See Shadow_Caster_Culling_for_Efficient_Shadow_Mapping.pdf for more efficient object culling mechanics
							}

							worker->FinishDraw(&con, true);

#ifdef PRINT_PERFORMANCE_TIMER
							CloakStopTimer(g_timerDL_Cascades);
							CloakStartTimer(g_timerDL_Blur);
#endif

							API::Rendering::DWParam blurParams[6];
							for (uint32_t b = 0; b < SHADOW_CASCADE_COUNT; b++)
							{
								con->SetPipelineState(g_pso.BlurX);
								con->TransitionResource(g_renderShadowMapBuffer[1], API::Rendering::STATE_UNORDERED_ACCESS);
								con->TransitionResource(g_renderShadowMapBuffer[0], API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE, true);
								con->SetSRV(0, g_renderShadowMapBuffer[0]->GetSRV());
								con->SetUAV(1, g_renderShadowMapBuffer[1]->GetUAV());
								const uint32_t x = b % SHADOW_MAP_WIDTH;
								const uint32_t y = b / SHADOW_MAP_WIDTH;
								const uint32_t left = x * ShadowMapSize;
								const uint32_t top = y * ShadowMapSize;
								blurParams[0] = static_cast<API::Rendering::INT1>(ShadowMapSize);
								blurParams[1] = static_cast<API::Rendering::INT1>(ShadowMapSize);
								blurParams[2] = left;
								blurParams[3] = top;
								blurParams[4] = 0;
								blurParams[5] = 0;
								con->SetConstants(2, ARRAYSIZE(blurParams), blurParams);
								con->Dispatch2D(ShadowMapSize, ShadowMapSize, BLUR_CHUNK_SIZE, 1);

								con->SetPipelineState(g_pso.BlurY);
								blurParams[2] = 0;
								blurParams[3] = 0;
								blurParams[4] = left;
								blurParams[5] = top;
								con->SetConstants(2, ARRAYSIZE(blurParams), blurParams);
								con->TransitionResource(g_renderShadowMapBuffer[0], API::Rendering::STATE_UNORDERED_ACCESS);
								con->TransitionResource(g_renderShadowMapBuffer[1], API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE, true);
								con->SetSRV(0, g_renderShadowMapBuffer[1]->GetSRV());
								con->SetUAV(1, g_renderShadowMapBuffer[0]->GetUAV());
								con->Dispatch2D(ShadowMapSize, ShadowMapSize, 1, BLUR_CHUNK_SIZE);
							}
#ifdef PRINT_PERFORMANCE_TIMER
							CloakStopTimer(g_timerDL_Blur);
							CloakStartTimer(g_timerDL_Light);
#endif

							//Draw Light:
							con->SetPipelineState(g_pso.DirectionalOpaqueSimple);

							DirLightBuffer dlb;
							dlb.Color = info.Color;
							dlb.Direction = info.Direction;
							for (size_t b = 0; b < SHADOW_CASCADE_COUNT; b++)
							{
								dlb.CSM.LCS[b] = lcsc[b];
								dlb.CSM.NCS[b] = ncsc[b];
								dlb.CSM.WorldToLight[b] = WorldToCascade[b];
							}
							con->WriteBuffer(g_diLiBuffer, 0, &dlb, sizeof(DirLightBuffer));
							for (size_t c = 0; c < RENDERING_DEFERRED_BUFFER_COUNT; c++) { con->TransitionResource(g_renderDeferredBuffer[c], API::Rendering::STATE_PIXEL_SHADER_RESOURCE); }
							con->TransitionResource(g_renderDefferedDepthBuffer, API::Rendering::STATE_PIXEL_SHADER_RESOURCE);
							for (size_t c = 0; c < ARRAYSIZE(RTV); c++) { con->TransitionResource(RTV[c], API::Rendering::STATE_RENDER_TARGET); }
							con->TransitionResource(g_renderScreenDepthBuffer, API::Rendering::STATE_DEPTH_WRITE);
							con->TransitionResource(g_renderShadowMapBuffer[0], API::Rendering::STATE_PIXEL_SHADER_RESOURCE, true);

							con->SetRenderTargets(ARRAYSIZE(RTV), RTV, g_renderScreenDepthBuffer, API::Rendering::DSV_ACCESS::READ_ONLY_STENCIL);
							con->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);
							con->SetDepthStencilRefValue(0);
							Utility::SetScreenRect(con, W, H);
							con->SetConstantBuffer(0, g_projBuffer);
							con->SetConstantBuffer(1, g_viewBuffer);
							con->SetConstantBuffer(2, g_diLiBuffer);
							con->SetSRV(3, g_renderDeferredBuffer[0]->GetSRV());
							con->SetSRV(4, g_renderDefferedDepthBuffer->GetDepthSRV());
							con->SetSRV(5, g_renderShadowMapBuffer[0]->GetSRV());
							con->SetDepthStencilRefValue(0);
							con->Draw(3);
							if (MSAA)
							{
								con->SetPipelineState(g_pso.DirectionalOpaqueComplex);
								con->Draw(3);
							}
#ifdef PRINT_PERFORMANCE_TIMER
							CloakStopTimer(g_timerDL_Light);
							CloakPrintTimer(g_timerDL_Cascades, "Rendering Direct Light Cascades: ", TIMER_PRINTING_TIME);
							CloakPrintTimer(g_timerDL_Blur, "Rendering Direct Light Blur: ", TIMER_PRINTING_TIME);
							CloakPrintTimer(g_timerDL_Light, "Rendering Direct Light ScreenPass: ", TIMER_PRINTING_TIME);
#endif
						}
						light->Release();
					}
					*context = con;
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerDirectionalLight);
					CloakPrintTimer(g_timerDirectionalLight, "Rendering Directional Light Time: ", TIMER_PRINTING_TIME);
#endif
				}
				inline void CLOAK_CALL WorldPhase1(In API::Rendering::IRenderWorker* worker, In API::Rendering::IManager* manager, In UINT W, In UINT H, In const API::Rendering::CameraData& camData, In const API::World::Group<API::Components::IRender>& toDraw, Inout API::Rendering::IContext** context)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerWorldPhase1);
#endif
					API::Rendering::IContext* con = *context;
					for (size_t a = 0; a < RENDERING_DEFERRED_BUFFER_COUNT; a++) { con->TransitionResource(g_renderDeferredBuffer[a], API::Rendering::STATE_RENDER_TARGET); }
					con->TransitionResource(g_renderDefferedDepthBuffer, API::Rendering::STATE_DEPTH_WRITE, true);

					PassData* pdata = reinterpret_cast<PassData*>(worker->StartDraw(manager, con, sizeof(PassData)));
					pdata[0].W = W;
					pdata[0].H = H;
					for (size_t a = 0; a < RENDERING_DEFERRED_BUFFER_COUNT; a++) { pdata[0].RTV[a] = g_renderDeferredBuffer[a]; }
					pdata[0].DSV = g_renderDefferedDepthBuffer;
					pdata[0].WorldToProjection = camData.WorldToClip;
					worker->Setup(DrawInitCommand, 0, API::Rendering::DRAW_TYPE::DRAW_TYPE_ALL, API::Rendering::DRAW_TAG::DRAW_TAG_OPAQUE, g_pso.Phase1Opaque, camData.ViewPosition, camData.ViewDistance);
					for (const auto& a : toDraw) 
					{
						API::Components::IRender* render = a.Get<API::Components::IRender*>();
						worker->Draw(render);
						render->Release();
					}
					//TODO: Draw alpha-mapped & transparent
					//worker->Setup(DrawInitCommand, API::Rendering::DRAW_TYPE::DRAW_TYPE_ALL, API::Rendering::DRAW_TAG::DRAW_TAG_ALPHA, 0, 0, g_pso.Phase1Alpha);
					//for (size_t a = 0; a < toDraw.size(); a++) { worker->Draw(toDraw[a]); }
					//worker->Setup(DrawInitCommand, API::Rendering::DRAW_TYPE::DRAW_TYPE_ALL, API::Rendering::DRAW_TAG::DRAW_TAG_TRANSPARENT, 0, 0, g_pso.Phase1Transparent);
					//for (size_t a = 0; a < toDraw.size(); a++) { worker->Draw(toDraw[a]); }
					worker->FinishDraw(&con, true);
					
					for (size_t a = 0; a < RENDERING_DEFERRED_BUFFER_COUNT; a++) { con->TransitionResource(g_renderDeferredBuffer[a], API::Rendering::STATE_PIXEL_SHADER_RESOURCE); }
					con->TransitionResource(g_renderDefferedDepthBuffer, API::Rendering::STATE_DEPTH_READ, true);
					*context = con;

					/*

					//Draw Opaque:

					if(g_worker.Size()>0)
					{
					con->CloseAndExecute();
					SAVE_RELEASE(con);
					g_worker.StartDraw(...);
					foreach (object in scene)
					{
					g_worker.PushToDraw(object);
					}
					g_worker.ExecuteDraws();
					g_cmdListManager->FlushExecutions();
					g_cmdListManager->BeginContext(CE_QUERY_ARGS(&con));
					}
					else
					{
					init con (similar to StartDraw(...))
					foreach (object in scene)
					{
					con->Draw(object);
					}
					}

					//Draw alphaMapped:

					if(g_worker.Size()>0)
					{
					g_worker.StartDraw(...);
					foreach (object in scene)
					{
					g_worker.PushToDraw(object);
					}
					g_worker.ExecuteDraws();
					g_cmdListManager->FlushExecutions();
					}
					else
					{
					init con (similar to StartDraw(...))
					foreach (object in scene)
					{
					con->Draw(object);
					}
					}

					//Draw transparent:

					//TODO: Flag clusteres occupied by transparent objects direct

					con->TransitionResource(g_renderScreenDepthBuffer, Impl::Rendering::STATE_DEPTH_READ);
					if(g_worker.Size()>0)
					{
					con->CloseAndExecute();
					SAVE_RELEASE(con);
					g_worker.StartDraw(...);
					foreach (object in scene)
					{
					g_worker.PushToDraw(object);
					}
					g_worker.ExecuteDraws();
					g_cmdListManager->FlushExecutions();
					g_cmdListManager->BeginContext(CE_QUERY_ARGS(&con));
					}
					else
					{
					init con (similar to StartDraw(...))
					foreach (object in scene)
					{
					con->Draw(object);
					}
					}

					//TODO: Read depth buffer -> flag clusters with pixels in them

					//TODO: Render ambient light onto HDR buffer

					*context=con;
					*/
					//Notice: Use SV_Coverage as ps input to mask what samples are used
					//eg: SV_Coverage = 01001101 => 1,3,4 and 7 are drawn to (Save in 4th render target)
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerWorldPhase1);
					CloakPrintTimer(g_timerWorldPhase1, "Rendering World Phase 1 Time: ", TIMER_PRINTING_TIME);
#endif
				}
				inline bool CLOAK_CALL WorldPhase2(In API::Rendering::IContext* context, In size_t callID)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerWorldPhase2);
#endif
					/*
					TODO:
					- Load directional/point lights from callID * 1024 to (callID + 1) * 1024, load spot lights from callID * 512 to (callID + 1) * 512
					-> Use different buffers for directional/point light (1024 lights) and spot lights (512 lights?)
					- Assign lights from lightBuffer to occupied clusters
					-> ClusterBuffer with format [ Offset, #dir/point lights, #spot lights ]
					-> Offset points to a possition within assignment buffer
					-> Assignment buffer as linear buffer, saving indices into light buffer
					- return wether scene has more than (callID + 1)*1024 directional/point lights or more than (callID + 1)*512 spot lights
					*/
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerWorldPhase2);
					CloakPrintTimer(g_timerWorldPhase2, "Rendering World Phase 2 Time: ", TIMER_PRINTING_TIME);
#endif
					return false;
				}
				inline void CLOAK_CALL WorldPhase3(In const API::World::Group<API::Components::IRender>& toDraw, Inout API::Rendering::IContext** context)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerWorldPhase3);
#endif
					API::Rendering::IContext* con = *context;
					/*
					TODO:
					- Render opaque as screen-space squad (similar to deffered)
					- Render transparent objects onto OIT buffer (similar to how they were rendered in WorldPhase1)
					-> This may close current context, in such cases create new one
					*/
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerWorldPhase3);
					CloakPrintTimer(g_timerWorldPhase3, "Rendering World Phase 3 Time: ", TIMER_PRINTING_TIME);
#endif
				}
				inline void CLOAK_CALL WorldPhase4(In API::Rendering::IContext* context)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerWorldPhase4);
#endif
					/*
					TODO: Render OIT buffers as screen size squad
					*/
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerWorldPhase4);
					CloakPrintTimer(g_timerWorldPhase4, "Rendering World Phase 4 Time: ", TIMER_PRINTING_TIME);
#endif
				}
				inline size_t CLOAK_CALL OpaqueResolve(In API::Rendering::IContext* context, In UINT W, In UINT H, In size_t diffuseFrame, In size_t specularFrame)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerOpaqueResolve);
#endif
					context->TransitionResource(g_renderPostBuffer[diffuseFrame], API::Rendering::STATE_RENDER_TARGET);
					context->TransitionResource(g_renderPostBuffer[specularFrame], API::Rendering::STATE_PIXEL_SHADER_RESOURCE, true);

					context->SetPipelineState(g_pso.OpaqueResolve);
					context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);
					context->SetSRV(0, g_renderPostBuffer[specularFrame]->GetSRV());
					Utility::SetScreenRect(context, W, H);
					context->Draw(3);
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerOpaqueResolve);
					CloakPrintTimer(g_timerOpaqueResolve, "Rendering Opaque Resolve Time: ", TIMER_PRINTING_TIME);
#endif
					return diffuseFrame;
				}

				inline void CLOAK_CALL LuminanceReducton(Inout API::Rendering::IContext* context, In API::Global::Time etime, In UINT W, In UINT H, In size_t targetFrameID)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerLuminanceReduction);
#endif
					float alpha[2] = {0, 1};
					if (g_lerpLuminance.exchange(true) == true)
					{
						const double f = (etime / LUMINANCE_FREQUENCY_NORMAL);
						const double p = std::pow(LUMINANCE_BLEND_INV, f);
						alpha[0] = static_cast<float>(p);
						alpha[1] = static_cast<float>(1 - p);
					}
					const UINT dx = (W + LUMINANCE_DISPATCH_X - 1) / LUMINANCE_DISPATCH_X;
					const UINT dy = (H + LUMINANCE_DISPATCH_Y - 1) / LUMINANCE_DISPATCH_Y;
					size_t lclb = g_curLuminanceBuf;
					size_t nclb = (lclb + 1) % LUMINANCE_BUFFER_COUNT;
					context->TransitionResource(g_renderPostBuffer[targetFrameID], API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE);
					context->TransitionResource(g_luminanceBuffer[lclb], API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE);
					context->TransitionResource(g_luminanceBuffer[nclb], API::Rendering::STATE_UNORDERED_ACCESS, true);
					context->SetPipelineState(g_pso.LuminancePass1);
					context->SetSRV(0, g_renderPostBuffer[targetFrameID]->GetSRV());
					context->SetSRV(1, g_luminanceBuffer[lclb]->GetSRV());
					context->SetUAV(2, g_luminanceBuffer[nclb]->GetUAV());
					API::Rendering::DWParam params[5];
					params[0] = W;
					params[1] = H;
					params[2] = alpha[0];
					params[3] = alpha[1];
					params[4] = dx;
					context->SetConstants(3, ARRAYSIZE(params), params);
					context->Dispatch(dx, dy);
					UINT numToReduce = dx*dy;
					if (numToReduce > 1)
					{
						context->SetPipelineState(g_pso.LuminancePass2);
						do {
							lclb = nclb;
							nclb = (lclb + 1) % LUMINANCE_BUFFER_COUNT;
							context->TransitionResource(g_luminanceBuffer[lclb], API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE);
							context->TransitionResource(g_luminanceBuffer[nclb], API::Rendering::STATE_UNORDERED_ACCESS, true);
							context->SetSRV(0, g_luminanceBuffer[lclb]->GetSRV());
							context->SetUAV(1, g_luminanceBuffer[nclb]->GetUAV());
							context->SetConstants(2, numToReduce);
							numToReduce = (numToReduce + LUMINANCE_REDUCTION_SIZE - 1) / LUMINANCE_REDUCTION_SIZE;
							context->Dispatch(numToReduce);
						} while (numToReduce > 1);
					}
					g_curLuminanceBuf = nclb;
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerLuminanceReduction);
					CloakPrintTimer(g_timerLuminanceReduction, "Rendering Luminance Reduction Time: ", TIMER_PRINTING_TIME);
#endif
				}
				inline void CLOAK_CALL Bloom(In API::Rendering::IContext* context, In size_t frameID, In UINT W, In UINT H, In API::Global::Graphic::BloomMode mode, In float threshold)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerBloom);
#endif
					if (mode == API::Global::Graphic::BloomMode::DISABLED || (W >> BLOOM_ITERATIONS) == 0 || (H >> BLOOM_ITERATIONS) == 0)
					{
						context->TransitionResource(g_renderBloomBuffer[0], API::Rendering::STATE_RENDER_TARGET, true);
						context->ClearColor(g_renderBloomBuffer[0]);
					}
					else
					{
						UINT bw = W;
						UINT bh = H;
						size_t si = 0;
						if (mode == API::Global::Graphic::BloomMode::NORMAL)
						{
							si = 1;
							bw >>= 1;
							bh >>= 1;
						}
						UINT w = bw;
						UINT h = bh;
						CLOAK_ASSUME(w > 0 && h > 0);
						CLOAK_ASSUME(si <= BLOOM_ITERATIONS);

						const float bloomNormalize = 1.0f / (1 + BLOOM_ITERATIONS - si);

						context->TransitionResource(g_renderPostBuffer[frameID], API::Rendering::STATE_PIXEL_SHADER_RESOURCE);
						context->TransitionResource(g_luminanceBuffer[g_curLuminanceBuf], API::Rendering::STATE_PIXEL_SHADER_RESOURCE);
						context->TransitionResource(g_renderBloomBuffer[0], API::Rendering::STATE_RENDER_TARGET);
						context->TransitionResource(g_renderBloomBuffer[1], API::Rendering::STATE_RENDER_TARGET);
						context->TransitionResource(g_renderBloomBuffer[2], API::Rendering::STATE_UNORDERED_ACCESS);
						context->TransitionResource(g_renderBloomBuffer[3], API::Rendering::STATE_UNORDERED_ACCESS, true);
						context->ClearColor(g_renderBloomBuffer[0]);

						context->SetPipelineState(g_pso.BloomBright);
						context->SetSRV(0, g_renderPostBuffer[frameID]->GetSRV());
						context->SetSRV(1, g_luminanceBuffer[g_curLuminanceBuf]->GetSRV());
						context->SetConstants(2, threshold);
						context->SetRenderTargets(1, &g_renderBloomBuffer[1]);
						Utility::SetScreenRect(context, bw, bh);
						context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);
						context->Draw(3);

						API::Rendering::DWParam blurParams[6];
						blurParams[0] = static_cast<API::Rendering::INT1>(w);
						blurParams[1] = static_cast<API::Rendering::INT1>(h);
						blurParams[2] = 0;
						blurParams[3] = 0;
						blurParams[4] = 0;
						blurParams[5] = 0;
						context->TransitionResource(g_renderBloomBuffer[1], API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE, true);
						context->SetPipelineState(g_pso.BlurX);
						context->SetSRV(0, g_renderBloomBuffer[1]->GetSRV());
						context->SetUAV(1, g_renderBloomBuffer[2]->GetUAV());
						context->SetConstants(2, ARRAYSIZE(blurParams), blurParams);
						context->Dispatch2D(w, h, BLUR_CHUNK_SIZE, 1);
						context->SetPipelineState(g_pso.BlurY);
						context->TransitionResource(g_renderBloomBuffer[2], API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE, true);
						context->SetSRV(0, g_renderBloomBuffer[2]->GetSRV());
						context->SetUAV(1, g_renderBloomBuffer[3]->GetUAV());
						context->Dispatch2D(w, h, 1, BLUR_CHUNK_SIZE);
						context->TransitionResource(g_renderBloomBuffer[3], API::Rendering::STATE_PIXEL_SHADER_RESOURCE, true);

						context->SetPipelineState(g_pso.BloomFinish);
						context->SetRenderTargets(1, &g_renderBloomBuffer[0]);
						Utility::SetScreenRect(context, bw, bh);
						context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);
						context->SetSRV(0, g_renderBloomBuffer[3]->GetSRV());
						context->SetConstants(1, 1.0f, 1.0f, bloomNormalize);
						context->Draw(3);

						size_t lastResize = 1;
						for (size_t a = si; a < BLOOM_ITERATIONS; a++)
						{
							UINT lw = w;
							UINT lh = h;
							w = w >> 1;
							h = h >> 1;
							const UINT blW = w + BLOOM_BORDER_OFFSET;
							const UINT blH = h + BLOOM_BORDER_OFFSET;
							const float scaleX = static_cast<float>(blW) / w;
							const float scaleY = static_cast<float>(blH) / h;
							CLOAK_ASSUME(w > 0 && h > 0);
							size_t nRT = (lastResize % 3) + 1;

							context->SetPipelineState(g_pso.Resize);
							context->TransitionResource(g_renderBloomBuffer[lastResize], API::Rendering::STATE_PIXEL_SHADER_RESOURCE);
							context->TransitionResource(g_renderBloomBuffer[nRT], API::Rendering::STATE_RENDER_TARGET, true);
							context->SetRenderTargets(1, &g_renderBloomBuffer[nRT]);
							Utility::SetScreenRect(context, blW, blH);
							context->SetSRV(0, g_renderBloomBuffer[lastResize]->GetSRV());
							context->SetConstants(1, scaleX * static_cast<float>(lw) / bw, scaleY * static_cast<float>(lh) / bh, 1.0f);
							context->Draw(3);
							lastResize = nRT;

							const UINT sW = w + 1;
							const UINT sH = h + 1;

							size_t bl1 = (nRT % 3) + 1;
							size_t bl2 = (bl1 % 3) + 1;
							blurParams[0] = static_cast<API::Rendering::INT1>(sW);
							blurParams[1] = static_cast<API::Rendering::INT1>(sH);
							context->SetPipelineState(g_pso.BlurX);
							context->TransitionResource(g_renderBloomBuffer[bl1], API::Rendering::STATE_UNORDERED_ACCESS);
							context->TransitionResource(g_renderBloomBuffer[bl2], API::Rendering::STATE_UNORDERED_ACCESS);
							context->TransitionResource(g_renderBloomBuffer[nRT], API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE, true);
							context->SetSRV(0, g_renderBloomBuffer[nRT]->GetSRV());
							context->SetUAV(1, g_renderBloomBuffer[bl1]->GetUAV());
							context->SetConstants(2, ARRAYSIZE(blurParams), blurParams);
							context->Dispatch2D(sW, sH, BLUR_CHUNK_SIZE, 1);
							context->TransitionResource(g_renderBloomBuffer[bl1], API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE, true);
							context->SetPipelineState(g_pso.BlurY);
							context->SetSRV(0, g_renderBloomBuffer[bl1]->GetSRV());
							context->SetUAV(1, g_renderBloomBuffer[bl2]->GetUAV());
							context->Dispatch2D(sW, sH, 1, BLUR_CHUNK_SIZE);
							context->TransitionResource(g_renderBloomBuffer[bl2], API::Rendering::STATE_PIXEL_SHADER_RESOURCE, true);

							context->SetPipelineState(g_pso.BloomFinish);
							context->SetRenderTargets(1, &g_renderBloomBuffer[0]);
							Utility::SetScreenRect(context, bw, bh);
							context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);
							context->SetSRV(0, g_renderBloomBuffer[bl2]->GetSRV());
							context->SetConstants(1, static_cast<float>(w) / bw, static_cast<float>(h) / bh, bloomNormalize);
							context->Draw(3);
						}
						if (mode == API::Global::Graphic::BloomMode::HIGH_QUALITY)
						{
							//Post-Blur:
							blurParams[0] = static_cast<API::Rendering::INT1>(bw);
							blurParams[1] = static_cast<API::Rendering::INT1>(bh);
							context->SetPipelineState(g_pso.BlurX);
							context->TransitionResource(g_renderBloomBuffer[1], API::Rendering::STATE_UNORDERED_ACCESS);
							context->TransitionResource(g_renderBloomBuffer[0], API::Rendering::STATE_GENERIC_READ, true);
							context->SetSRV(0, g_renderBloomBuffer[0]->GetSRV());
							context->SetUAV(1, g_renderBloomBuffer[1]->GetUAV());
							context->SetConstants(2, ARRAYSIZE(blurParams), blurParams);
							context->Dispatch2D(bw, bh, BLUR_CHUNK_SIZE, 1);

							context->TransitionResource(g_renderBloomBuffer[0], API::Rendering::STATE_UNORDERED_ACCESS);
							context->TransitionResource(g_renderBloomBuffer[1], API::Rendering::STATE_GENERIC_READ, true);
							context->SetPipelineState(g_pso.BlurY);
							context->SetSRV(0, g_renderBloomBuffer[1]->GetSRV());
							context->SetUAV(1, g_renderBloomBuffer[0]->GetUAV());
							context->SetConstants(2, ARRAYSIZE(blurParams), blurParams);
							context->Dispatch2D(bw, bh, 1, BLUR_CHUNK_SIZE);
						}
					}
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerBloom);
					CloakPrintTimer(g_timerBloom, "Rendering Bloom Time: ", TIMER_PRINTING_TIME);
#endif
				}
				inline void CLOAK_CALL ToneMappingAndFinal(In API::Rendering::IContext* context, In size_t postProcessID, In const API::Rendering::RenderTargetData& target, In float gamma, In bool hdr, In float bloomBoost)
				{
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStartTimer(g_timerToneMapping);
#endif
					API::Rendering::IColorBuffer* rtv = target.Target;
					context->SetPipelineState(g_pso.ToneMapping);
					context->SetRenderTargets(1, &rtv);
					Utility::SetScreenRect(context, target.Pixel.W, target.Pixel.H, target.Pixel.X, target.Pixel.Y);
					context->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST);
					context->TransitionResource(g_renderPostBuffer[postProcessID], API::Rendering::STATE_PIXEL_SHADER_RESOURCE);
					context->TransitionResource(g_renderBloomBuffer[0], API::Rendering::STATE_PIXEL_SHADER_RESOURCE);
					context->TransitionResource(g_luminanceBuffer[g_curLuminanceBuf], API::Rendering::STATE_PIXEL_SHADER_RESOURCE, true);
					context->SetSRV(0, g_renderPostBuffer[postProcessID]->GetSRV());
					context->SetSRV(1, g_luminanceBuffer[g_curLuminanceBuf]->GetSRV());
					context->SetSRV(2, g_renderBloomBuffer[0]->GetSRV());
					context->SetConstants(3, gamma, hdr ? 1 : 0, STANDARD_NITS, bloomBoost);
					context->Draw(3);
#ifdef PRINT_PERFORMANCE_TIMER
					CloakStopTimer(g_timerToneMapping);
					CloakPrintTimer(g_timerToneMapping, "Rendering Tone Mapping: ", TIMER_PRINTING_TIME);
#endif
				}
			}

			CLOAK_CALL DefaultRenderPass::DefaultRenderPass()
			{

			}
			CLOAK_CALL DefaultRenderPass::~DefaultRenderPass()
			{
				SAVE_RELEASE(g_projBuffer);
				SAVE_RELEASE(g_viewBuffer);
				SAVE_RELEASE(g_fluidBuffer);
				SAVE_RELEASE(g_diLiBuffer);
				for (unsigned int a = 0; a < SHADOW_CASCADE_COUNT; a++) { SAVE_RELEASE(g_diViewBuffer[a]); }
				for (unsigned int a = 0; a < RENDERING_DEFERRED_BUFFER_COUNT; a++) { SAVE_RELEASE(g_renderDeferredBuffer[a]); }
				for (unsigned int a = 0; a < RENDERING_POST_PROCESS_BUFFER_COUNT; a++) { SAVE_RELEASE(g_renderPostBuffer[a]); }
				for (unsigned int a = 0; a < RENDERING_OIB_BUFFER_COUNT; a++) { SAVE_RELEASE(g_renderOITBuffer[a]); }
				for (unsigned int a = 0; a < RENDERING_SHADOW_MAP_COUNT; a++) { SAVE_RELEASE(g_renderShadowMapBuffer[a]); }
				for (unsigned int a = 0; a < RENDERING_BLOOM_BUFFER_COUNT; a++) { SAVE_RELEASE(g_renderBloomBuffer[a]); }
				for (unsigned int a = 0; a < LUMINANCE_BUFFER_COUNT; a++) { SAVE_RELEASE(g_luminanceBuffer[a]); }
				//SAVE_RELEASE(g_renderInterface);
				SAVE_RELEASE(g_renderScreenDepthBuffer);
				SAVE_RELEASE(g_renderDefferedDepthBuffer);
				SAVE_RELEASE(g_renderShadowDepthBuffer);
				g_pso.Release();
			}
			void CLOAK_CALL_THIS DefaultRenderPass::OnInit(In const CE::RefPointer<API::Rendering::IManager>& manager)
			{
				g_projBuffer = manager->CreateConstantBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, sizeof(ProjBuffer));
				g_viewBuffer = manager->CreateConstantBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, sizeof(ViewBuffer));
				g_fluidBuffer = manager->CreateConstantBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, sizeof(FluidBuffer));
				g_diLiBuffer = manager->CreateConstantBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, sizeof(DirLightBuffer));
				for (unsigned int a = 0; a < SHADOW_CASCADE_COUNT; a++) { g_diViewBuffer[a] = manager->CreateConstantBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, sizeof(DirViewBuffer)); }
				API::Rendering::IGraphicBuffer* rndBuff[4+SHADOW_CASCADE_COUNT] = { g_projBuffer,g_viewBuffer,g_fluidBuffer,g_diLiBuffer };
				for (unsigned int a = 0; a < SHADOW_CASCADE_COUNT; a++) { rndBuff[a + 4] = g_diViewBuffer[a]; }
				manager->GenerateViews(rndBuff, ARRAYSIZE(rndBuff), API::Rendering::DescriptorPageType::UTILITY);
				g_pso.Create(manager);
			}
			void CLOAK_CALL_THIS DefaultRenderPass::OnResize(In const CE::RefPointer<API::Rendering::IManager>& manager, In const API::Global::Graphic::Settings& newSet, In const API::Global::Graphic::Settings& oldSet, In bool updateShaders)
			{
				const bool first = !g_createdRessources.exchange(true);
				bool updateFrameBuffer = false;
				bool updateBloom = false;
				bool updateShadowMap = false;
				if (newSet.Resolution.Height != oldSet.Resolution.Height || newSet.Resolution.Width != oldSet.Resolution.Width)
				{
					updateFrameBuffer = true;
					updateBloom = true;
				}
				if (newSet.MSAA != oldSet.MSAA) { updateFrameBuffer = true; }
				if (newSet.ShadowMapResolution != oldSet.ShadowMapResolution) { updateShadowMap = true; }
				if (newSet.Bloom != oldSet.Bloom) { updateBloom = true; }

				if (first || updateFrameBuffer)
				{
					g_screenW = newSet.Resolution.Width;
					g_screenH = newSet.Resolution.Height;

					const UINT luminanceSize = ((newSet.Resolution.Width + LUMINANCE_DISPATCH_X - 1) / LUMINANCE_DISPATCH_X)*((newSet.Resolution.Height + LUMINANCE_DISPATCH_Y - 1) / LUMINANCE_DISPATCH_Y);
					for (size_t a = 0; a < LUMINANCE_BUFFER_COUNT; a++)
					{
						if (first)
						{
							g_luminanceBuffer[a] = manager->CreateStructuredBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, luminanceSize, sizeof(float), false);
						}
						else
						{
							g_luminanceBuffer[a]->Recreate(luminanceSize, sizeof(float));
						}
					}
					g_lerpLuminance = false;

					API::Rendering::DEPTH_DESC ddesc;
					ddesc.Depth.Clear = 0;
					ddesc.Depth.UseSRV = true;
					ddesc.Stencil.Clear = 1;
					ddesc.Stencil.UseSRV = false;
					ddesc.Format = API::Rendering::Format::D32_FLOAT;
					ddesc.Width = newSet.Resolution.Width;
					ddesc.Height = newSet.Resolution.Height;
					ddesc.SampleCount = 1;

					API::Rendering::TEXTURE_DESC tdesc;
					tdesc.Width = newSet.Resolution.Width;
					tdesc.Height = newSet.Resolution.Height;
					tdesc.Format = FORMAT(4, 16, UNORM);
					tdesc.clearValue = API::Helper::Color::RGBA(0, 0, 0, 0);
					tdesc.AccessType = API::Rendering::VIEW_TYPE::SRV | API::Rendering::VIEW_TYPE::RTV | API::Rendering::VIEW_TYPE::SRV_DYNAMIC;
					tdesc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE;
					tdesc.Texture.MipMaps = 1;

					tdesc.Texture.SampleCount = static_cast<uint32_t>(newSet.MSAA);
					ddesc.SampleCount = static_cast<uint32_t>(newSet.MSAA);
					for (unsigned int a = 0; a < RENDERING_DEFERRED_BUFFER_COUNT; a++)
					{
						if (a == RENDERING_DEFERRED_BUFFER_COUNT - 1)
						{
							tdesc.Format = FORMAT(1, 8, UINT);
						}
						if (first == true) { g_renderDeferredBuffer[a] = manager->CreateColorBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, tdesc); }
						else { g_renderDeferredBuffer[a]->Recreate(tdesc); }
					}
					tdesc.Format = FORMAT(4, 16, FLOAT);
					for (unsigned int a = 0; a < RENDERING_OIB_BUFFER_COUNT; a++)
					{
						if (a == RENDERING_OIB_BUFFER_COUNT - 1)
						{
							tdesc.Format = FORMAT(1, 16, FLOAT);
							tdesc.clearValue = API::Helper::Color::RGBA(1, 1, 1, 1);
						}
						if (first == true) { g_renderOITBuffer[a] = manager->CreateColorBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, tdesc); }
						else { g_renderOITBuffer[a]->Recreate(tdesc); }
					}
					tdesc.Format = FORMAT(4, 16, FLOAT);
					tdesc.Texture.SampleCount = 1;
					tdesc.clearValue = API::Helper::Color::RGBA(0, 0, 0, 0);
					//Use RGBA-16 format for numeric stability (gamma correction)
					//if (first == true) { g_renderInterface = manager->CreateColorBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, tdesc); }
					//else { g_renderInterface->Recreate(tdesc); }

					//Use RGBA-16 float format to preserve > 1 values
					for (unsigned int a = 0; a < RENDERING_POST_PROCESS_BUFFER_COUNT; a++)
					{
						if (first == true) { g_renderPostBuffer[a] = manager->CreateColorBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, tdesc); }
						else { g_renderPostBuffer[a]->Recreate(tdesc); }
					}

					if (first)
					{
						g_renderDefferedDepthBuffer = manager->CreateDepthBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, ddesc);
						ddesc.Format = API::Rendering::Format::D24_UNORM_S8_UINT;
						ddesc.SampleCount = 1;
						ddesc.Depth.UseSRV = false;
						g_renderScreenDepthBuffer = manager->CreateDepthBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, ddesc);
					}
					else
					{
						g_renderDefferedDepthBuffer->Recreate(ddesc);
						ddesc.Format = API::Rendering::Format::D24_UNORM_S8_UINT;
						ddesc.SampleCount = 1;
						ddesc.Depth.UseSRV = false;
						g_renderScreenDepthBuffer->Recreate(ddesc);
					}
				}
				if (first || updateBloom)
				{
					API::Rendering::TEXTURE_DESC tdesc;
					switch (newSet.Bloom)
					{
						case API::Global::Graphic::BloomMode::HIGH_QUALITY:
							tdesc.Width = newSet.Resolution.Width;
							tdesc.Height = newSet.Resolution.Height;
							break;
						case API::Global::Graphic::BloomMode::NORMAL:
							tdesc.Width = newSet.Resolution.Width >> 1;
							tdesc.Height = newSet.Resolution.Height >> 1;
							break;
						case API::Global::Graphic::BloomMode::DISABLED:
							tdesc.Width = 1;
							tdesc.Height = 1;
							break;
					}

					tdesc.Texture.SampleCount = 1;
					tdesc.Format = FORMAT(4, 16, FLOAT);
					tdesc.clearValue = API::Helper::Color::RGBA(0, 0, 0, 0);
					tdesc.AccessType = API::Rendering::VIEW_TYPE::SRV | API::Rendering::VIEW_TYPE::UAV | API::Rendering::VIEW_TYPE::RTV;
					tdesc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE;
					tdesc.Texture.MipMaps = 1;

					for (unsigned int a = 0; a < RENDERING_BLOOM_BUFFER_COUNT; a++)
					{
						if (first == true) { g_renderBloomBuffer[a] = manager->CreateColorBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, tdesc); }
						else { g_renderBloomBuffer[a]->Recreate(tdesc); }
					}
				}
				if (first || updateShadowMap)
				{
					const uint32_t smW = static_cast<uint32_t>(newSet.ShadowMapResolution*SHADOW_MAP_WIDTH);
					const uint32_t smH = static_cast<uint32_t>(newSet.ShadowMapResolution *SHADOW_MAP_HEIGHT);
					g_shadowMapRes = newSet.ShadowMapResolution;
					API::Rendering::DEPTH_DESC ddesc;
					ddesc.Width = smW;
					ddesc.Height = smH;
					ddesc.Depth.Clear = 0;
					ddesc.Depth.UseSRV = false;
					ddesc.Stencil.Clear = 0;
					ddesc.Stencil.UseSRV = false;
					ddesc.Format = API::Rendering::Format::D32_FLOAT;
					ddesc.SampleCount = 1;

					if (first == true) { g_renderShadowDepthBuffer = manager->CreateDepthBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, ddesc); }
					else { g_renderShadowDepthBuffer->Recreate(ddesc); }

					API::Rendering::TEXTURE_DESC tdesc;
					tdesc.Width = smW;
					tdesc.Height = smH;
					tdesc.Format = FORMAT(4, 16, UNORM);
					tdesc.clearValue = API::Helper::Color::RGBA(1, 1, 1, 1);
					tdesc.AccessType = API::Rendering::VIEW_TYPE::SRV | API::Rendering::VIEW_TYPE::RTV | API::Rendering::VIEW_TYPE::UAV;
					tdesc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE;
					tdesc.Texture.MipMaps = 1;
					tdesc.Texture.SampleCount = 1;
					for (unsigned int a = 0; a < RENDERING_SHADOW_MAP_COUNT; a++)
					{
						if (a > 0)
						{
							tdesc.Width = newSet.ShadowMapResolution;
							tdesc.Height = newSet.ShadowMapResolution;
						}
						if (first == true) { g_renderShadowMapBuffer[a] = manager->CreateColorBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, tdesc); }
						else { g_renderShadowMapBuffer[a]->Recreate(tdesc); }
					}
				}
				if (first)
				{
					constexpr size_t POST_BUFFER_POS = 3;
					constexpr size_t DEF_BUFFER_POS = POST_BUFFER_POS + RENDERING_POST_PROCESS_BUFFER_COUNT;
					constexpr size_t OIB_BUFFER_POS = DEF_BUFFER_POS + RENDERING_DEFERRED_BUFFER_COUNT;
					constexpr size_t SHADOW_BUFFER_POS = OIB_BUFFER_POS + RENDERING_OIB_BUFFER_COUNT;
					constexpr size_t BLOOM_BUFFER_POS = SHADOW_BUFFER_POS + RENDERING_SHADOW_MAP_COUNT;
					constexpr size_t LUM_BUFFER_POS = BLOOM_BUFFER_POS + RENDERING_BLOOM_BUFFER_COUNT;
					constexpr size_t BUFFER_COUNT = LUM_BUFFER_POS + LUMINANCE_BUFFER_COUNT;
					API::Rendering::IResource* cd[BUFFER_COUNT];
					cd[0] = g_renderDefferedDepthBuffer;
					cd[1] = g_renderScreenDepthBuffer;
					//cd[2] = g_renderInterface;
					cd[2] = g_renderShadowDepthBuffer;
					for (size_t a = 0; a < RENDERING_POST_PROCESS_BUFFER_COUNT; a++) { cd[a + POST_BUFFER_POS] = g_renderPostBuffer[a]; }
					for (size_t a = 0; a < RENDERING_DEFERRED_BUFFER_COUNT; a++) { cd[a + DEF_BUFFER_POS] = g_renderDeferredBuffer[a]; }
					for (size_t a = 0; a < RENDERING_OIB_BUFFER_COUNT; a++) { cd[a + OIB_BUFFER_POS] = g_renderOITBuffer[a]; }
					for (size_t a = 0; a < RENDERING_SHADOW_MAP_COUNT; a++) { cd[a + SHADOW_BUFFER_POS] = g_renderShadowMapBuffer[a]; }
					for (size_t a = 0; a < RENDERING_BLOOM_BUFFER_COUNT; a++) { cd[a + BLOOM_BUFFER_POS] = g_renderBloomBuffer[a]; }
					for (size_t a = 0; a < LUMINANCE_BUFFER_COUNT; a++) { cd[a + LUM_BUFFER_POS] = g_luminanceBuffer[a]; }
					manager->GenerateViews(cd, BUFFER_COUNT, API::Rendering::VIEW_TYPE::ALL, API::Rendering::DescriptorPageType::UTILITY);

					//API::Rendering::IGraphicBuffer* rndBuff[LUMINANCE_BUFFER_COUNT];
					//for (size_t a = 0; a < LUMINANCE_BUFFER_COUNT; a++) { rndBuff[a] = g_luminanceBuffer[a]; }
					//manager->GenerateViews(rndBuff, ARRAYSIZE(rndBuff));
				}
				if (first || updateShaders) { g_pso.Reset(newSet, manager->GetHardwareSetting()); }
			}
			void CLOAK_CALL_THIS DefaultRenderPass::OnRenderCamera(In const CE::RefPointer<API::Rendering::IManager>& manager, Inout API::Rendering::IContext** context, In API::Rendering::IRenderWorker* worker, In API::Components::ICamera* camera, In const API::Rendering::RenderTargetData& target, In const API::Rendering::CameraData& camData, In const API::Rendering::PassData& pass, In API::Global::Time etime)
			{
				const UINT W = g_screenW;
				const UINT H = g_screenH;
				const bool MSAA = pass.MSAA != CE::Global::Graphic::MSAA::DISABLED;
				API::Rendering::IContext* con = *context;
				RenderPasses::Startup(con, manager, camData, camData.UpdatedClip || g_lastCam.exchange(camera) != camera);
				API::Global::Graphic::BloomMode bloom = API::Global::Graphic::BloomMode::DISABLED;
				size_t ppF = 0;
				if (pass.ResetFrame == true)
				{
					g_lerpLuminance = false;
				}
				bloom = pass.Bloom;

				API::World::Group<API::Components::IRender> objects;
				RenderPasses::WorldPhase1(worker, manager, W, H, camData, objects, &con);
				RenderPasses::MSAAPrePass(con, W, H, MSAA);
				RenderPasses::Skybox(camData.Skybox, camData.SkyIntensity, con, ppF, W, H);
				RenderPasses::AmbientLightPass(camData.AmbientColor, con, ppF, MSAA, W, H);
				const size_t dpF = ppF;
				const size_t spF = (ppF + 1) % RENDERING_POST_PROCESS_BUFFER_COUNT;
				RenderPasses::DirectionalLight(&con, worker, manager, camData, SHADOW_MAP_LAMBDA, W, H, objects, dpF, spF, MSAA);
				//for (size_t a = 0; RenderPasses::WorldPhase2(lights, context, a); a++)
				//{
				//	RenderPasses::WorldPhase3(toDraw, &context);
				//}
				RenderPasses::IBL(con, camData.Skybox, dpF, spF, MSAA, W, H);
				//TODO: Separable Subsurface Scattering
				ppF = RenderPasses::OpaqueResolve(con, W, H, dpF, spF);
				//RenderPasses::WorldPhase4(context);
				//TODO: Post processing
				RenderPasses::LuminanceReducton(con, etime, W, H, ppF);
				RenderPasses::Bloom(con, ppF, W, H, bloom, camData.Bloom.Threshold);
				RenderPasses::ToneMappingAndFinal(con, ppF, target, pass.Gamma, pass.HDR, camData.Bloom.Boost);
				*context = con;
			}
			void CLOAK_CALL_THIS DefaultRenderPass::OnRenderInterface(In const CE::RefPointer<API::Rendering::IManager>& manager, Inout API::Rendering::IContext** context, In API::Rendering::IRenderWorker* worker, In API::Rendering::IColorBuffer* screen, In size_t numGuis, In API::Interface::IBasicGUI** guis, In const API::Rendering::PassData& pass)
			{
#ifdef PRINT_PERFORMANCE_TIMER
				CloakStartTimer(g_timerInterface);
#endif
				const UINT W = screen->GetWidth();
				const UINT H = screen->GetHeight();
				API::Rendering::IContext* con = *context;
				con->SetPipelineState(g_pso.Interface);
				con->SetRenderTargets(1, &screen);
				con->SetPrimitiveTopology(API::Rendering::TOPOLOGY_POINTLIST);
				con->SetConstants(1, pass.Gamma);
				Utility::SetScreenRect(con, W, H);
				API::Rendering::IContext2D* c2d = con->Enable2D(0);
				CLOAK_ASSUME(c2d != nullptr);
				for (size_t a = 0; a < numGuis; a++) { guis[a]->Draw(c2d); }
				c2d->Execute();
				*context = con;
#ifdef PRINT_PERFORMANCE_TIMER
				CloakStopTimer(g_timerInterface);
				CloakPrintTimer(g_timerInterface, "Interface rendering time: ", TIMER_PRINTING_TIME);
#endif
			}

			Success(return == true) bool CLOAK_CALL_THIS DefaultRenderPass::iQueryInterface(In REFIID riid, Outptr void** ptr)
			{
				if (riid == __uuidof(API::Rendering::IRenderPass)) { *ptr = (API::Rendering::IRenderPass*)this; return true; }
				return SavePtr::iQueryInterface(riid, ptr);
			}
		}
	}
}