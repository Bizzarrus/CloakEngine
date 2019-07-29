#include "stdafx.h"
#include "Engine/Graphic/DefaultShaderLib.h"

#include "Implementation/Rendering/PipelineState.h"
#include "Implementation/Files/Shader.h"

#include "Includes/DefaultShader.h"

#define CE_CALL_FILL_CODE(Name, Compl, P, HW, GrS, DSC) DSC->Get<API::Rendering::PSO_SUBTYPE_##P>() = DefaultShader::Name::Get##P(HW, GrS, Compl);
#define CE_CALL_FILL_PASSES(Name, Compl, HW, GrS, DSC) CE_CALL_FILL_CODE(Name, Compl, VS, HW, GrS, DSC) CE_CALL_FILL_CODE(Name, Compl, PS, HW, GrS, DSC) CE_CALL_FILL_CODE(Name, Compl, GS, HW, GrS, DSC) CE_CALL_FILL_CODE(Name, Compl, HS, HW, GrS, DSC) CE_CALL_FILL_CODE(Name, Compl, DS, HW, GrS, DSC) CE_CALL_FILL_CODE(Name, Compl, CS, HW, GrS, DSC)
#define CE_CALL_FILL_SWITCH(Name, NameOrig, Compl, HW, GrS, DSC) case ShaderType::Name: CE_CALL_FILL_PASSES(NameOrig, Compl, HW, GrS, DSC) break;
#define CE_CALL_FILL_false(Name, Compl, HW, GrS, DSC) CE_CALL_FILL_SWITCH(Name, Name, Compl, HW, GrS, DSC)
#define CE_CALL_FILL_true(Name, Compl, HW, GrS, DSC) CE_CALL_FILL_SWITCH(Name##Simple, Name, false, HW, GrS, DSC) CE_CALL_FILL_SWITCH(Name##Complex, Name, true, HW, GrS, DSC)
#define CE_CALL_FILL(Name, Compl, HW, GrS, DSC) CE_CALL_FILL_##Compl(Name, Compl, HW, GrS, DSC)

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {
			namespace DefaultShaderLib {
				namespace PSO {
					namespace Sampler {
						inline API::Rendering::SAMPLER_DESC CLOAK_CALL CreateSampler(In API::Rendering::FILTER_TYPE filter, In API::Rendering::TEXTURE_ADDRESS_MODE mode, In UINT anisotropy, In_opt const API::Helper::Color::RGBA& border = API::Helper::Color::Black)
						{
							API::Rendering::SAMPLER_DESC desc;
							desc.Filter = filter;
							desc.AddressU = mode;
							desc.AddressV = mode;
							desc.AddressW = mode;
							desc.MipLODBias = 0.0f;
							desc.MaxAnisotropy = anisotropy;
							desc.CompareFunction = API::Rendering::CMP_FUNC_NEVER;
							desc.BorderColor[0] = border.R;
							desc.BorderColor[1] = border.G;
							desc.BorderColor[2] = border.B;
							desc.BorderColor[3] = border.A;
							desc.MinLOD = 0;
							desc.MaxLOD = API::Rendering::MAX_LOD;
							return desc;
						}
						inline API::Rendering::SAMPLER_DESC CLOAK_CALL CreateSampler(In API::Rendering::FILTER_TYPE filter, In API::Rendering::TEXTURE_ADDRESS_MODE mode, In_opt const API::Helper::Color::RGBA& border = API::Helper::Color::Black)
						{
							return CreateSampler(filter, mode, 0, border);
						}
						inline API::Rendering::SAMPLER_DESC CLOAK_CALL CreateSampler(In const API::Global::Graphic::Settings& set, In API::Rendering::TEXTURE_ADDRESS_MODE mode, In_opt const API::Helper::Color::RGBA& border = API::Helper::Color::Black)
						{
							API::Rendering::FILTER_TYPE filter = API::Rendering::FILTER_MIN_MAG_MIP_POINT;
							UINT anisotropy = 0;
							switch (set.TextureFilter)
							{
								case API::Global::Graphic::TextureFilter::BILINEAR:
									filter = API::Rendering::FILTER_MIN_MAG_LINEAR_MIP_POINT;
									anisotropy = 0;
									break;
								case API::Global::Graphic::TextureFilter::TRILINIEAR:
									filter = API::Rendering::FILTER_MIN_MAG_MIP_LINEAR;
									anisotropy = 0;
									break;
								case API::Global::Graphic::TextureFilter::ANISOTROPx2:
									filter = API::Rendering::FILTER_ANISOTROPIC;
									anisotropy = 2;
									break;
								case API::Global::Graphic::TextureFilter::ANISOTROPx4:
									filter = API::Rendering::FILTER_ANISOTROPIC;
									anisotropy = 4;
									break;
								case API::Global::Graphic::TextureFilter::ANISOTROPx8:
									filter = API::Rendering::FILTER_ANISOTROPIC;
									anisotropy = 8;
									break;
								case API::Global::Graphic::TextureFilter::ANISOTROPx16:
									filter = API::Rendering::FILTER_ANISOTROPIC;
									anisotropy = 16;
									break;
								default:
									break;
							}
							return CreateSampler(filter, mode, anisotropy, border);
						}
					}
					inline void CLOAK_CALL OutputStream(Out API::Rendering::STREAM_OUTPUT_DESC* desc, In ShaderType type)
					{
						desc->NumEntries = 0;
						desc->NumStrides = 0;
						desc->BufferStrides = nullptr;
						desc->Entries = nullptr;
						desc->RasterizedStreamIndex = 0;
					}
					inline void CLOAK_CALL BlendState(Out API::Rendering::BLEND_STATE_DESC* desc, In ShaderType type)
					{
						desc->IndependentBlend = false;
						switch (type)
						{
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Interface:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::FadeOut:
								desc->AlphaToCoverage = false;
								desc->RenderTarget[0].BlendEnable = true;
								desc->RenderTarget[0].BlendOperation = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].BlendOperationAlpha = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].DstBlend = API::Rendering::BLEND_INV_SRC_ALPHA;
								desc->RenderTarget[0].DstBlendAlpha = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].LogicOperation = API::Rendering::LOGIC_OP_NOOP;
								desc->RenderTarget[0].LogicOperationEnable = false;
								desc->RenderTarget[0].WriteMask = API::Rendering::COLOR_ALL;
								desc->RenderTarget[0].SrcBlend = API::Rendering::BLEND_SRC_ALPHA;
								desc->RenderTarget[0].SrcBlendAlpha = API::Rendering::BLEND_INV_DEST_ALPHA;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Skybox:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::ToneMapping:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterCompression:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::PreMSAA:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass1:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass2:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowOpaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Resize:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomBright:
								desc->AlphaToCoverage = false;
								desc->RenderTarget[0].BlendEnable = false;
								desc->RenderTarget[0].BlendOperation = API::Rendering::BLEND_OP_MIN;
								desc->RenderTarget[0].BlendOperationAlpha = API::Rendering::BLEND_OP_MIN;
								desc->RenderTarget[0].DstBlend = API::Rendering::BLEND_ZERO;
								desc->RenderTarget[0].DstBlendAlpha = API::Rendering::BLEND_ZERO;
								desc->RenderTarget[0].LogicOperation = API::Rendering::LOGIC_OP_NOOP;
								desc->RenderTarget[0].LogicOperationEnable = false;
								desc->RenderTarget[0].WriteMask = API::Rendering::COLOR_ALL;
								desc->RenderTarget[0].SrcBlend = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].SrcBlendAlpha = API::Rendering::BLEND_ONE;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomFinish:
								desc->AlphaToCoverage = false;
								desc->RenderTarget[0].BlendEnable = true;
								desc->RenderTarget[0].BlendOperation = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].BlendOperationAlpha = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].DstBlend = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].DstBlendAlpha = API::Rendering::BLEND_ZERO;
								desc->RenderTarget[0].LogicOperation = API::Rendering::LOGIC_OP_NOOP;
								desc->RenderTarget[0].LogicOperationEnable = false;
								desc->RenderTarget[0].WriteMask = API::Rendering::COLOR_ALL;
								desc->RenderTarget[0].SrcBlend = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].SrcBlendAlpha = API::Rendering::BLEND_ONE;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Opaque:
								desc->AlphaToCoverage = false;
								for (size_t a = 0; a < 5; a++)
								{
									desc->RenderTarget[a].BlendEnable = false;
									desc->RenderTarget[a].BlendOperation = API::Rendering::BLEND_OP_ADD;
									desc->RenderTarget[a].BlendOperationAlpha = API::Rendering::BLEND_OP_ADD;
									desc->RenderTarget[a].DstBlend = API::Rendering::BLEND_ONE;
									desc->RenderTarget[a].DstBlendAlpha = API::Rendering::BLEND_ONE;
									desc->RenderTarget[a].LogicOperation = API::Rendering::LOGIC_OP_NOOP;
									desc->RenderTarget[a].LogicOperationEnable = false;
									desc->RenderTarget[a].WriteMask = API::Rendering::COLOR_ALL;
									desc->RenderTarget[a].SrcBlend = API::Rendering::BLEND_ONE;
									desc->RenderTarget[a].SrcBlendAlpha = API::Rendering::BLEND_ONE;
								}
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientTransparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalTransparent:
								desc->AlphaToCoverage = false;
								desc->RenderTarget[0].BlendEnable = true;
								desc->RenderTarget[0].BlendOperation = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].BlendOperationAlpha = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].DstBlend = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].DstBlendAlpha = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].LogicOperation = API::Rendering::LOGIC_OP_NOOP;
								desc->RenderTarget[0].LogicOperationEnable = false;
								desc->RenderTarget[0].WriteMask = API::Rendering::COLOR_ALL;
								desc->RenderTarget[0].SrcBlend = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].SrcBlendAlpha = API::Rendering::BLEND_ONE;

								desc->RenderTarget[1].BlendEnable = true;
								desc->RenderTarget[1].BlendOperation = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[1].BlendOperationAlpha = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[1].DstBlend = API::Rendering::BLEND_INV_SRC_ALPHA;
								desc->RenderTarget[1].DstBlendAlpha = API::Rendering::BLEND_INV_SRC_ALPHA;
								desc->RenderTarget[1].LogicOperation = API::Rendering::LOGIC_OP_NOOP;
								desc->RenderTarget[1].LogicOperationEnable = false;
								desc->RenderTarget[1].WriteMask = API::Rendering::COLOR_ALL;
								desc->RenderTarget[1].SrcBlend = API::Rendering::BLEND_ZERO;
								desc->RenderTarget[1].SrcBlendAlpha = API::Rendering::BLEND_ZERO;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveComplex:
								desc->AlphaToCoverage = false;
								desc->RenderTarget[0].BlendEnable = true;
								desc->RenderTarget[0].BlendOperation = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].BlendOperationAlpha = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].DstBlend = API::Rendering::BLEND_SRC_ALPHA;
								desc->RenderTarget[0].DstBlendAlpha = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].LogicOperation = API::Rendering::LOGIC_OP_NOOP;
								desc->RenderTarget[0].LogicOperationEnable = false;
								desc->RenderTarget[0].WriteMask = API::Rendering::COLOR_ALL;
								desc->RenderTarget[0].SrcBlend = API::Rendering::BLEND_INV_SRC_ALPHA;
								desc->RenderTarget[0].SrcBlendAlpha = API::Rendering::BLEND_ONE;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueComplex:
								desc->AlphaToCoverage = false;
								desc->RenderTarget[0].BlendEnable = true;
								desc->RenderTarget[0].BlendOperation = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].BlendOperationAlpha = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].DstBlend = API::Rendering::BLEND_INV_SRC_ALPHA;
								desc->RenderTarget[0].DstBlendAlpha = API::Rendering::BLEND_ZERO;
								desc->RenderTarget[0].LogicOperation = API::Rendering::LOGIC_OP_NOOP;
								desc->RenderTarget[0].LogicOperationEnable = false;
								desc->RenderTarget[0].WriteMask = API::Rendering::COLOR_ALL;
								desc->RenderTarget[0].SrcBlend = API::Rendering::BLEND_SRC_ALPHA;
								desc->RenderTarget[0].SrcBlendAlpha = API::Rendering::BLEND_ONE;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLComplex:
								desc->AlphaToCoverage = false;
								desc->RenderTarget[0].BlendEnable = true;
								desc->RenderTarget[0].BlendOperation = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].BlendOperationAlpha = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].DstBlend = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].DstBlendAlpha = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].LogicOperation = API::Rendering::LOGIC_OP_NOOP;
								desc->RenderTarget[0].LogicOperationEnable = false;
								desc->RenderTarget[0].WriteMask = API::Rendering::COLOR_ALL;
								desc->RenderTarget[0].SrcBlend = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].SrcBlendAlpha = API::Rendering::BLEND_ONE;

								desc->RenderTarget[1].BlendEnable = true;
								desc->RenderTarget[1].BlendOperation = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[1].BlendOperationAlpha = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[1].DstBlend = API::Rendering::BLEND_ONE;
								desc->RenderTarget[1].DstBlendAlpha = API::Rendering::BLEND_ONE;
								desc->RenderTarget[1].LogicOperation = API::Rendering::LOGIC_OP_NOOP;
								desc->RenderTarget[1].LogicOperationEnable = false;
								desc->RenderTarget[1].WriteMask = API::Rendering::COLOR_ALL;
								desc->RenderTarget[1].SrcBlend = API::Rendering::BLEND_ONE;
								desc->RenderTarget[1].SrcBlendAlpha = API::Rendering::BLEND_ONE;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::OpaqueResolve:
								desc->AlphaToCoverage = false;
								desc->RenderTarget[0].BlendEnable = true;
								desc->RenderTarget[0].BlendOperation = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].BlendOperationAlpha = API::Rendering::BLEND_OP_ADD;
								desc->RenderTarget[0].DstBlend = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].DstBlendAlpha = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].LogicOperation = API::Rendering::LOGIC_OP_NOOP;
								desc->RenderTarget[0].LogicOperationEnable = false;
								desc->RenderTarget[0].WriteMask = API::Rendering::COLOR_ALL;
								desc->RenderTarget[0].SrcBlend = API::Rendering::BLEND_ONE;
								desc->RenderTarget[0].SrcBlendAlpha = API::Rendering::BLEND_ONE;
								break;
							default:
								break;
						}
					}
					inline void CLOAK_CALL Rasterizer(Out API::Rendering::RASTERIZER_STATE_DESC* desc, In ShaderType type, In bool useMSAA)
					{
						switch (type)
						{
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Skybox:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Interface:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::FadeOut:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::ToneMapping:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterCompression:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::PreMSAA:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass1:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass2:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Resize:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomFinish:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomBright:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::OpaqueResolve:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLComplex:
								desc->AntialiasedLine = false;
								desc->ConservativeRasterization = API::Rendering::CONSERVATIVE_RASTERIZATION_OFF;
								desc->CullMode = API::Rendering::CULL_BACK;
								desc->DepthBias = 0;
								desc->DepthBiasClamp = 0;
								desc->DepthClip = true;
								desc->FillMode = API::Rendering::FILL_SOLID;
								desc->ForcedSampleCount = 0;
								desc->FrontCounterClockwise = false;
								desc->Multisample = false;
								desc->SlopeScaledDepthBias = 0;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Opaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalTransparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientTransparent:
								desc->AntialiasedLine = true;
								desc->ConservativeRasterization = API::Rendering::CONSERVATIVE_RASTERIZATION_OFF;
								desc->CullMode = API::Rendering::CULL_BACK;
								desc->DepthBias = 0;
								desc->DepthBiasClamp = 0;
								desc->DepthClip = true;
								desc->FillMode = API::Rendering::FILL_SOLID;
								desc->ForcedSampleCount = 0;
								desc->FrontCounterClockwise = false;
								desc->Multisample = useMSAA;
								desc->SlopeScaledDepthBias = 0;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowOpaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowAlpha:
								desc->AntialiasedLine = false;
								desc->ConservativeRasterization = API::Rendering::CONSERVATIVE_RASTERIZATION_OFF;
								desc->CullMode = API::Rendering::CULL_BACK;
								desc->DepthBias = 0;
								desc->DepthBiasClamp = 0;
								desc->DepthClip = false;
								desc->FillMode = API::Rendering::FILL_SOLID;
								desc->ForcedSampleCount = 0;
								desc->FrontCounterClockwise = false;
								desc->Multisample = false;
								desc->SlopeScaledDepthBias = 0;
								break;
							default:
								break;
						}
					}
					inline void CLOAK_CALL DepthStencil(Out API::Rendering::DEPTH_STENCIL_STATE_DESC* desc, In ShaderType type, In bool useMSAA)
					{
						desc->DepthBoundsTests = false;
						switch (type)
						{
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Interface:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::FadeOut:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::ToneMapping:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterCompression:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass1:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass2:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Resize:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomFinish:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomBright:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::OpaqueResolve:
								desc->DepthEnable = false;
								desc->DepthFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->DepthWriteMask = API::Rendering::WRITE_MASK_ALL;
								desc->StencilEnable = false;
								desc->StencilReadMask = 0;
								desc->StencilWriteMask = 0;

								desc->FrontFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->FrontFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->BackFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->BackFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->Format = API::Rendering::Format::D24_UNORM_S8_UINT;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Opaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowOpaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowAlpha:
								desc->DepthEnable = true;
								desc->DepthFunction = API::Rendering::CMP_FUNC_GREATER;
								desc->DepthWriteMask = API::Rendering::WRITE_MASK_ALL;
								desc->StencilEnable = false;
								desc->StencilReadMask = 0;
								desc->StencilWriteMask = 0;

								desc->FrontFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->FrontFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->BackFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->BackFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->Format = API::Rendering::Format::D32_FLOAT;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Skybox:
								desc->DepthEnable = true;
								desc->DepthFunction = API::Rendering::CMP_FUNC_GREATER_EQUAL;
								desc->DepthWriteMask = API::Rendering::WRITE_MASK_ZERO;
								desc->StencilEnable = false;
								desc->StencilReadMask = 0;
								desc->StencilWriteMask = 0;

								desc->FrontFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->FrontFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->BackFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->BackFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->Format = API::Rendering::Format::D24_UNORM_S8_UINT;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Transparent:
								desc->DepthEnable = true;
								desc->DepthFunction = API::Rendering::CMP_FUNC_GREATER_EQUAL;
								desc->DepthWriteMask = API::Rendering::WRITE_MASK_ZERO;
								desc->StencilEnable = false;
								desc->StencilReadMask = 0;
								desc->StencilWriteMask = 0;

								desc->FrontFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->FrontFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->BackFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->BackFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->Format = API::Rendering::Format::D32_FLOAT;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientTransparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalTransparent:
								desc->DepthEnable = true;
								desc->DepthFunction = API::Rendering::CMP_FUNC_GREATER;
								desc->DepthWriteMask = API::Rendering::WRITE_MASK_ZERO;
								desc->StencilEnable = useMSAA;
								desc->StencilReadMask = 0xFF;
								desc->StencilWriteMask = 0xFF;

								desc->FrontFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->FrontFace.PassOperation = API::Rendering::STENCIL_OP_REPLACE;

								desc->BackFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFunction = API::Rendering::CMP_FUNC_NEVER;
								desc->BackFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->Format = API::Rendering::Format::D32_FLOAT;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::PreMSAA:
								desc->DepthEnable = false;
								desc->DepthFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->DepthWriteMask = API::Rendering::WRITE_MASK_ALL;
								desc->StencilEnable = true;
								desc->StencilReadMask = 0;
								desc->StencilWriteMask = 0xFF;

								desc->FrontFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->FrontFace.PassOperation = API::Rendering::STENCIL_OP_REPLACE;

								desc->BackFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->BackFace.PassOperation = API::Rendering::STENCIL_OP_REPLACE;

								desc->Format = API::Rendering::Format::D24_UNORM_S8_UINT;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLSimple:
								desc->DepthEnable = false;
								desc->DepthFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->DepthWriteMask = API::Rendering::WRITE_MASK_ZERO;
								desc->StencilEnable = useMSAA;
								desc->StencilReadMask = 0xFF;
								desc->StencilWriteMask = 0;

								desc->FrontFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFunction = API::Rendering::CMP_FUNC_EQUAL;
								desc->FrontFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->BackFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFunction = API::Rendering::CMP_FUNC_EQUAL;
								desc->BackFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->Format = API::Rendering::Format::D24_UNORM_S8_UINT;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLComplex:
								desc->DepthEnable = false;
								desc->DepthFunction = API::Rendering::CMP_FUNC_ALWAYS;
								desc->DepthWriteMask = API::Rendering::WRITE_MASK_ZERO;
								desc->StencilEnable = useMSAA;
								desc->StencilReadMask = 0xFF;
								desc->StencilWriteMask = 0;

								desc->FrontFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->FrontFace.StencilFunction = API::Rendering::CMP_FUNC_NOT_EQUAL;
								desc->FrontFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->BackFace.DepthFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFailOperation = API::Rendering::STENCIL_OP_KEEP;
								desc->BackFace.StencilFunction = API::Rendering::CMP_FUNC_NOT_EQUAL;
								desc->BackFace.PassOperation = API::Rendering::STENCIL_OP_KEEP;

								desc->Format = API::Rendering::Format::D24_UNORM_S8_UINT;
								break;
							default:
								break;
						}
					}
					inline void CLOAK_CALL InputLayout(Out API::Rendering::INPUT_LAYOUT_DESC* desc, In ShaderType type, In bool useTess)
					{
						desc->Custom.Elements = nullptr;
						desc->Custom.NumElements = 0;
						switch (type)
						{
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Skybox:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::FadeOut:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::ToneMapping:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterCompression:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::PreMSAA:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass1:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass2:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Resize:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomFinish:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomBright:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::OpaqueResolve:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLComplex:
								desc->Type = API::Rendering::INPUT_LAYOUT_TYPE_NONE;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Interface:
								desc->Type = API::Rendering::INPUT_LAYOUT_TYPE_GUI;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Opaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientTransparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalTransparent:
								desc->Type = API::Rendering::INPUT_LAYOUT_TYPE_MESH;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowOpaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowAlpha:
								if (useTess) { desc->Type = API::Rendering::INPUT_LAYOUT_TYPE_MESH; }
								else { desc->Type = API::Rendering::INPUT_LAYOUT_TYPE_MESH_POS; }
								break;
							default:
								break;
						}
					}
					inline void CLOAK_CALL RenderTargets(Out API::Rendering::RENDER_TARGETS_DESC* desc, In ShaderType type)
					{
						switch (type)
						{
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Interface:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::ToneMapping:
								desc->NumRenderTargets = 1;
								desc->RTVFormat[0] = API::Rendering::Format::R10G10B10A2_UNORM;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Skybox:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::FadeOut:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::OpaqueResolve:
								desc->NumRenderTargets = 1;
								desc->RTVFormat[0] = FORMAT(4, 16, FLOAT);
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLComplex:
								desc->NumRenderTargets = 2;
								desc->RTVFormat[0] = FORMAT(4, 16, FLOAT);
								desc->RTVFormat[1] = FORMAT(4, 16, FLOAT);
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Opaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Alpha:
								desc->NumRenderTargets = 5;
								desc->RTVFormat[0] = FORMAT(4, 16, UNORM);
								desc->RTVFormat[1] = FORMAT(4, 16, UNORM);
								desc->RTVFormat[2] = FORMAT(4, 16, UNORM);
								desc->RTVFormat[3] = FORMAT(4, 16, UNORM);
								desc->RTVFormat[4] = FORMAT(1, 8, UINT);
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterCompression:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::PreMSAA:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass1:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass2:
								desc->NumRenderTargets = 0;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientTransparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalTransparent:
								desc->NumRenderTargets = 2;
								desc->RTVFormat[0] = FORMAT(4, 16, FLOAT);
								desc->RTVFormat[1] = FORMAT(1, 16, FLOAT);
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowOpaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowAlpha:
								desc->NumRenderTargets = 1;
								desc->RTVFormat[0] = FORMAT(4, 16, UNORM);
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Resize:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomFinish:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomBright:
								desc->NumRenderTargets = 1;
								desc->RTVFormat[0] = FORMAT(4, 16, FLOAT);
								break;
							default:
								break;
						}
					}
					inline void CLOAK_CALL SampleState(Out API::Rendering::SAMPLE_DESC* desc, In ShaderType type, In bool useMSAA, In UINT MSAACount)
					{
						switch (type)
						{
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Skybox:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Interface:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::FadeOut:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::ToneMapping:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass1:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass2:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterCompression:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::PreMSAA:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowOpaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Resize:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomFinish:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomBright:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::OpaqueResolve:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLComplex:
								desc->Count = 1;
								desc->Quality = 0;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Opaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientTransparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalTransparent:
								desc->Count = useMSAA ? MSAACount : 1;
								desc->Quality = 0;
								break;
							default:
								break;
						}
					}
					inline void CLOAK_CALL GPSO(Out API::Rendering::PIPELINE_STATE_DESC* desc, In ShaderType type, In bool useTess)
					{
						desc->Get<API::Rendering::PSO_SUBTYPE_SAMPLE_MASK>() = API::Rendering::DEFAULT_SAMPLE_MASK;
						desc->Get<API::Rendering::PSO_SUBTYPE_INDEX_BUFFER_STRIP_CUT>() = API::Rendering::INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
						switch (type)
						{
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Interface:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Skybox:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Resize:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::OpaqueResolve:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::PreMSAA:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::FadeOut:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::ToneMapping:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomBright:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomFinish:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterCompression:
								desc->Get<API::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY>() = API::Rendering::TOPOLOGY_TYPE_TRIANGLE;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass1:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass2:
								desc->Get<API::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY>() = API::Rendering::TOPOLOGY_TYPE_POINT;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Opaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientTransparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalTransparent:
								if (useTess) { desc->Get<API::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY>() = API::Rendering::TOPOLOGY_TYPE_PATCH; }
								else { desc->Get<API::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY>() = API::Rendering::TOPOLOGY_TYPE_TRIANGLE; }
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowOpaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowAlpha:
								if (useTess) { desc->Get<API::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY>() = API::Rendering::TOPOLOGY_TYPE_PATCH; }
								else { desc->Get<API::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY>() = API::Rendering::TOPOLOGY_TYPE_TRIANGLE; }
								break;
							default:
								break;
						}
					}

					inline void CLOAK_CALL FillDescription(Out API::Rendering::PIPELINE_STATE_DESC* desc, In ShaderType type, In const API::Global::Graphic::Settings& gset, In const API::Rendering::Hardware& hardware, In bool isCompute)
					{
						if (isCompute)
						{

						}
						else
						{
							bool useMSAA = gset.MSAA != CE::Global::Graphic::MSAA::DISABLED;
							UINT MSAACount = static_cast<UINT>(gset.MSAA);
							GPSO(desc, type, gset.Tessellation);
							SampleState(&desc->Get<API::Rendering::PSO_SUBTYPE_SAMPLE_DESC>(), type, useMSAA, MSAACount);
							RenderTargets(&desc->Get<API::Rendering::PSO_SUBTYPE_RENDER_TARGETS>(), type);
							InputLayout(&desc->Get<API::Rendering::PSO_SUBTYPE_INPUT_LAYOUT>(), type, gset.Tessellation);
							DepthStencil(&desc->Get<API::Rendering::PSO_SUBTYPE_DEPTH_STENCIL>(), type, useMSAA);
							Rasterizer(&desc->Get<API::Rendering::PSO_SUBTYPE_RASTERIZER>(), type, useMSAA);
							BlendState(&desc->Get<API::Rendering::PSO_SUBTYPE_BLEND>(), type);
							OutputStream(&desc->Get<API::Rendering::PSO_SUBTYPE_STREAM_OUTPUT>(), type);
						}
						desc->Get<API::Rendering::PSO_SUBTYPE_VS>() = DefaultShader::Interface::GetVS(hardware, gset, false);
						switch (type)
						{
							CE_PER_SHADER(CE_CALL_FILL, CE_SEP_EMPTY, hardware, gset, desc)
							default:
								break;
						}
					}
					inline void CLOAK_CALL FillRoot(Out API::Rendering::RootDescription* root, In ShaderType type, In const API::Global::Graphic::Settings& gset, Out bool* isCompute)
					{
						CLOAK_ASSUME(isCompute != nullptr);
						*isCompute = false;
						UINT numRootParams = 0;
						UINT numRootSamplers = 0;
						switch (type)
						{
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Skybox:
								numRootParams = 5;
								numRootSamplers = 0;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Interface:
								numRootParams = 3;
								numRootSamplers = 0;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::FadeOut:
								numRootParams = 1;
								numRootSamplers = 0;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::ToneMapping:
								numRootParams = 4;
								numRootSamplers = 2;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Opaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Transparent:
								numRootParams = 6;//TODO
								numRootSamplers = 0;
								if (gset.Tessellation) { numRootParams += 2; }
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterCompression:
								numRootParams = 3;
								numRootSamplers = 0;
								*isCompute = true;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentSimple:
								numRootParams = 5;
								numRootSamplers = 1;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass1:
								numRootParams = 4;
								numRootSamplers = 0;
								*isCompute = true;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass2:
								numRootParams = 3;
								numRootSamplers = 0;
								*isCompute = true;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BlurX:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BlurY:
								numRootParams = 3;
								numRootSamplers = 0;
								*isCompute = true;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Resize:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomFinish:
								numRootParams = 2;
								numRootSamplers = 1;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomBright:
								numRootParams = 3;
								numRootSamplers = 1;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueComplex:
								numRootParams = 6;
								numRootSamplers = 2;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowOpaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowAlpha:
								if (gset.Tessellation)
								{
									numRootParams = 7;
									numRootSamplers = 0;
								}
								else
								{
									numRootParams = 4;
									numRootSamplers = 0;
								}
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueComplex:
								numRootParams = 2;
								numRootSamplers = 1;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::OpaqueResolve:
								numRootParams = 1;
								numRootSamplers = 1;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLComplex:
								numRootParams = 7;
								numRootSamplers = 2;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::PreMSAA:
								numRootParams = 1;
								numRootSamplers = 0;
								break;
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientTransparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalTransparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveComplex:
								numRootParams = 0;//TODO
								numRootSamplers = 0;
								break;
							default:
								break;
						}
						root->Resize(numRootParams, numRootSamplers);
						root->SetFlags(API::Rendering::ROOT_SIGNATURE_FLAG_NONE);
						switch (type)
						{
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Skybox:
							{
								root->At(0).SetAsRootCBV(0, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(1).SetAsRootCBV(1, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(2).SetAsSRV(0, 1, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC);
								root->At(3).SetAsConstants(2, 1, API::Rendering::VISIBILITY_PIXEL);
								root->At(4).SetAsAutomaticSampler(0, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_LINEAR, API::Rendering::MODE_CLAMP), API::Rendering::VISIBILITY_PIXEL, 0);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Interface:
							{
								root->At(0).SetAsSRV(0, 1, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC);
								root->At(1).SetAsConstants(0, 1, API::Rendering::VISIBILITY_PIXEL);
								root->At(2).SetAsAutomaticSampler(0, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_LINEAR, API::Rendering::MODE_BORDER, API::Helper::Color::RGBA(0, 0, 0, 0)), API::Rendering::VISIBILITY_PIXEL, 0);

								root->SetFlags(API::Rendering::ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::FadeOut:
							{
								root->At(0).SetAsConstants(0, 4, API::Rendering::VISIBILITY_PIXEL);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::ToneMapping:
							{
								root->At(0).SetAsSRV(0, 1, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(1).SetAsSRV(1, 1, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(2).SetAsSRV(2, 1, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(3).SetAsConstants(0, 4, API::Rendering::VISIBILITY_PIXEL);

								root->SetStaticSampler(0, 0, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_POINT, API::Rendering::MODE_BORDER), API::Rendering::VISIBILITY_PIXEL);
								root->SetStaticSampler(1, 1, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_LINEAR, API::Rendering::MODE_CLAMP), API::Rendering::VISIBILITY_PIXEL);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Opaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase1Transparent:
							{
								root->SetFlags(API::Rendering::ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
								if (gset.Tessellation)
								{
									root->At(0).SetAsRootCBV(0, API::Rendering::VISIBILITY_DOMAIN, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(1).SetAsRootCBV(1, API::Rendering::VISIBILITY_ALL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(2).SetAsRootCBV(2, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(3).SetAsRootCBV(3, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(4).SetAsSRV(0, 3, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(5).SetAsAutomaticSampler(0, Sampler::CreateSampler(gset, API::Rendering::MODE_CLAMP), API::Rendering::VISIBILITY_ALL);
									root->At(6).SetAsConstants(4, 1, API::Rendering::VISIBILITY_DOMAIN);
									root->At(7).SetAsSRV(3, 1, API::Rendering::VISIBILITY_DOMAIN, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								}
								else
								{
									root->At(0).SetAsRootCBV(0, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(1).SetAsRootCBV(1, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(2).SetAsRootCBV(2, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(3).SetAsRootCBV(3, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(4).SetAsSRV(0, 3, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(5).SetAsAutomaticSampler(0, Sampler::CreateSampler(gset, API::Rendering::MODE_CLAMP), API::Rendering::VISIBILITY_PIXEL);
								}
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowOpaque:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalShadowAlpha:
							{
								root->SetFlags(API::Rendering::ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
								if (gset.Tessellation)
								{
									root->At(0).SetAsRootCBV(0, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(1).SetAsRootCBV(1, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(2).SetAsRootCBV(2, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(3).SetAsRootCBV(3, API::Rendering::VISIBILITY_DOMAIN, API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE);
									root->At(4).SetAsConstants(4, 1, API::Rendering::VISIBILITY_HULL);
									root->At(5).SetAsSRV(0, 1, API::Rendering::VISIBILITY_DOMAIN, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(6).SetAsAutomaticSampler(0, Sampler::CreateSampler(gset, API::Rendering::MODE_CLAMP), API::Rendering::VISIBILITY_DOMAIN);
								}
								else
								{
									root->At(0).SetAsRootCBV(0, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(1).SetAsRootCBV(1, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(2).SetAsRootCBV(2, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
									root->At(3).SetAsRootCBV(3, API::Rendering::VISIBILITY_VERTEX, API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE);
								}
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalOpaqueComplex:
							{
								root->At(0).SetAsRootCBV(0, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(1).SetAsRootCBV(1, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(2).SetAsRootCBV(2, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(3).SetAsSRV(0, 4, API::Rendering::VISIBILITY_PIXEL, 0, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(4).SetAsSRV(0, 1, API::Rendering::VISIBILITY_PIXEL, 1, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(5).SetAsSRV(1, 1, API::Rendering::VISIBILITY_PIXEL, 1, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);

								root->SetStaticSampler(0, 0, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_POINT, API::Rendering::MODE_CLAMP), API::Rendering::VISIBILITY_PIXEL);
								root->SetStaticSampler(1, 1, Sampler::CreateSampler(gset, API::Rendering::MODE_BORDER, API::Helper::Color::RGBA(0, 0, 0, 0)), API::Rendering::VISIBILITY_PIXEL);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientOpaqueComplex:
							{
								root->At(0).SetAsSRV(0, 4, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(1).SetAsConstants(0, 4, API::Rendering::VISIBILITY_PIXEL);

								root->SetStaticSampler(0, 0, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_POINT, API::Rendering::MODE_CLAMP), API::Rendering::VISIBILITY_PIXEL);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterCompression:
							{
								root->At(0).SetAsUAV(0, 1, API::Rendering::VISIBILITY_COMPUTE, API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE);
								root->At(1).SetAsUAV(1, 1, API::Rendering::VISIBILITY_COMPUTE, API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE);
								root->At(2).SetAsSRV(0, 1, API::Rendering::VISIBILITY_COMPUTE, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase2ClusterAssignmentSimple:
							{
								root->At(0).SetAsRootCBV(0, API::Rendering::VISIBILITY_PIXEL, 0, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(1).SetAsRootCBV(1, API::Rendering::VISIBILITY_PIXEL, 0, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(2).SetAsRootCBV(0, API::Rendering::VISIBILITY_PIXEL, 1, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(3).SetAsSRV(0, 1, API::Rendering::VISIBILITY_PIXEL, 1, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(4).SetAsUAV(0, 1, API::Rendering::VISIBILITY_PIXEL, 0, API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE);

								root->SetStaticSampler(0, 0, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_POINT, API::Rendering::MODE_CLAMP), API::Rendering::VISIBILITY_PIXEL);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass1:
							{
								root->At(0).SetAsSRV(0, 1, API::Rendering::VISIBILITY_COMPUTE, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(1).SetAsSRV(1, 1, API::Rendering::VISIBILITY_COMPUTE, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(2).SetAsUAV(0, 1, API::Rendering::VISIBILITY_COMPUTE, API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE);
								root->At(3).SetAsConstants(0, 5, API::Rendering::VISIBILITY_COMPUTE);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::LuminancePass2:
							{
								root->At(0).SetAsSRV(0, 1, API::Rendering::VISIBILITY_COMPUTE, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(1).SetAsUAV(0, 1, API::Rendering::VISIBILITY_COMPUTE, API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE);
								root->At(2).SetAsConstants(0, 1, API::Rendering::VISIBILITY_COMPUTE);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BlurX:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BlurY:
							{
								root->At(0).SetAsSRV(0, 1, API::Rendering::VISIBILITY_COMPUTE, API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE);
								root->At(1).SetAsUAV(0, 1, API::Rendering::VISIBILITY_COMPUTE, API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE);
								root->At(2).SetAsConstants(0, 6, API::Rendering::VISIBILITY_COMPUTE);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Resize:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomFinish:
							{
								root->At(0).SetAsSRV(0, 1, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE);
								root->At(1).SetAsConstants(0, 3, API::Rendering::VISIBILITY_PIXEL);

								root->SetStaticSampler(0, 0, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_LINEAR, API::Rendering::MODE_CLAMP), API::Rendering::VISIBILITY_PIXEL);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::BloomBright:
							{
								root->At(0).SetAsSRV(0, 1, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(1).SetAsSRV(1, 1, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
								root->At(2).SetAsConstants(0, 1, API::Rendering::VISIBILITY_PIXEL);

								root->SetStaticSampler(0, 0, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_LINEAR, API::Rendering::MODE_CLAMP), API::Rendering::VISIBILITY_PIXEL);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::OpaqueResolve:
							{
								root->At(0).SetAsSRV(0, 1, API::Rendering::VISIBILITY_PIXEL, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);

								root->SetStaticSampler(0, 0, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_POINT, API::Rendering::MODE_CLAMP), API::Rendering::VISIBILITY_PIXEL);
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::IBLComplex:
							{
								root->At(0).SetAsRootCBV(0, API::Rendering::VISIBILITY_PIXEL, 0, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION); //Projection
								root->At(1).SetAsRootCBV(1, API::Rendering::VISIBILITY_PIXEL, 0, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION); //View
								root->At(2).SetAsRootCBV(0, API::Rendering::VISIBILITY_PIXEL, 1, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION); //Irradiance
								root->At(3).SetAsConstants(1, API::Rendering::VISIBILITY_PIXEL, 1); //Settings
								root->At(4).SetAsSRV(0, 4, API::Rendering::VISIBILITY_PIXEL, 0, API::Rendering::ROOT_PARAMETER_FLAG_DYNAMIC_DESCRIPTOR); //GBuffer
								root->At(5).SetAsSRV(0, 1, API::Rendering::VISIBILITY_PIXEL, 1, API::Rendering::ROOT_PARAMETER_FLAG_DYNAMIC_DESCRIPTOR); //Depth
								root->At(6).SetAsSRV(0, 2, API::Rendering::VISIBILITY_PIXEL, 2, API::Rendering::ROOT_PARAMETER_FLAG_DYNAMIC_DESCRIPTOR); //LUT + FirstSum

								root->SetStaticSampler(0, 0, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_POINT, API::Rendering::MODE_CLAMP));
								root->SetStaticSampler(1, 1, Sampler::CreateSampler(API::Rendering::FILTER_MIN_MAG_MIP_LINEAR, API::Rendering::MODE_CLAMP));
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::PreMSAA:
							{
								root->At(0).SetAsSRV(0, 1, API::Rendering::VISIBILITY_PIXEL, 0, API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION); //Coverage Texture
								break;
							}
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3OpaqueComplex:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Alpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::Phase3Transparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::AmbientTransparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalAlpha:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::DirectionalTransparent:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveSimple:
							case CloakEngine::Engine::Graphic::DefaultShaderLib::ShaderType::TransparentResolveComplex:
							{
								//TODO
								break;
							}
							default:
								break;
						}
					}
				}

				bool CLOAK_CALL ResetPSO(In API::Rendering::IPipelineState* pso, In ShaderType type, In const API::Global::Graphic::Settings& gset, In const API::Rendering::Hardware& hardware)
				{
					CLOAK_ASSUME(pso != nullptr);
					API::Rendering::PIPELINE_STATE_DESC desc;
					API::Rendering::RootDescription root;
					bool isCompute = false;
					PSO::FillRoot(&root, type, gset, &isCompute);
					PSO::FillDescription(&desc, type, gset, hardware, isCompute);
					return pso->Reset(desc, root);
				}
			}
		}
	}
}