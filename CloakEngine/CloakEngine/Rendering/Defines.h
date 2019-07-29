#pragma once
#ifndef CE_API_RENDERING_DEFINES_H
#define CE_API_RENDERING_DEFINES_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/Color.h"
#include "CloakEngine/Files/Shader.h"
#define GPU_ADDRESS_UNKNOWN (~static_cast<CloakEngine::API::Rendering::GPU_ADDRESS>(0))
#define GPU_ADDRESS_NULL static_cast<CloakEngine::API::Rendering::GPU_ADDRESS>(0)
#define CPU_ADDRESS_NULL static_cast<SIZE_T>(0)

#define FORMAT_1(W) API::Rendering::Format::R##W
#define FORMAT_2(W) FORMAT_1(W)##G##W
#define FORMAT_3(W) FORMAT_2(W)##B##W
#define FORMAT_4(W) FORMAT_3(W)##A##W
#define FORMAT(Count,Width,Type) FORMAT_##Count##(Width)##_##Type

#define VIEWTYPE_TEXTURE_DEFAULT CloakEngine::API::Rendering::VIEW_TYPE::SRV

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			typedef UINT64 GPU_ADDRESS;
			enum class Format { 
				UNKNOWN, 
				R32G32B32A32_TYPELESS, 
				R32G32B32A32_FLOAT, 
				R32G32B32A32_UINT, 
				R32G32B32A32_SINT, 
				R32G32B32_TYPELESS, 
				R32G32B32_FLOAT, 
				R32G32B32_UINT, 
				R32G32B32_SINT, 
				R16G16B16A16_TYPELESS, 
				R16G16B16A16_FLOAT, 
				R16G16B16A16_UNORM, 
				R16G16B16A16_UINT, 
				R16G16B16A16_SNORM, 
				R16G16B16A16_SINT, 
				R32G32_TYPELESS, 
				R32G32_FLOAT, 
				R32G32_UINT, 
				R32G32_SINT, 
				R32G8X24_TYPELESS, 
				D32_FLOAT_S8X24_UINT, 
				R32_FLOAT_X8X24_TYPELESS, 
				X32_TYPELESS_G8X24_UINT, 
				R10G10B10A2_TYPELESS, 
				R10G10B10A2_UNORM, 
				R10G10B10A2_UINT, 
				R11G11B10_FLOAT, 
				R8G8B8A8_TYPELESS, 
				R8G8B8A8_UNORM, 
				R8G8B8A8_UNORM_SRGB, 
				R8G8B8A8_UINT, 
				R8G8B8A8_SNORM, 
				R8G8B8A8_SINT, 
				R16G16_TYPELESS, 
				R16G16_FLOAT, 
				R16G16_UNORM, 
				R16G16_UINT, 
				R16G16_SNORM, 
				R16G16_SINT, 
				R32_TYPELESS, 
				D32_FLOAT, 
				R32_FLOAT, 
				R32_UINT, 
				R32_SINT, 
				R24G8_TYPELESS, 
				D24_UNORM_S8_UINT, 
				R24_UNORM_X8_TYPELESS, 
				X24_TYPELESS_G8_UINT, 
				R8G8_TYPELESS, 
				R8G8_UNORM, 
				R8G8_UINT, 
				R8G8_SNORM, 
				R8G8_SINT, 
				R16_TYPELESS, 
				R16_FLOAT, 
				D16_UNORM, 
				R16_UNORM, 
				R16_UINT, 
				R16_SNORM, 
				R16_SINT, 
				R8_TYPELESS, 
				R8_UNORM, 
				R8_UINT, 
				R8_SNORM, 
				R8_SINT, 
				A8_UNORM, 
				R1_UNORM, 
				R9G9B9E5_SHAREDEXP, 
				R8G8_B8G8_UNORM, 
				G8R8_G8B8_UNORM, 
				BC1_TYPELESS, 
				BC1_UNORM, 
				BC1_UNORM_SRGB, 
				BC2_TYPELESS, 
				BC2_UNORM, 
				BC2_UNORM_SRGB, 
				BC3_TYPELESS, 
				BC3_UNORM, 
				BC3_UNORM_SRGB, 
				BC4_TYPELESS, 
				BC4_UNORM, 
				BC4_SNORM, 
				BC5_TYPELESS, 
				BC5_UNORM, 
				BC5_SNORM, 
				B5G6R5_UNORM, 
				B5G5R5A1_UNORM, 
				B8G8R8A8_UNORM, 
				B8G8R8X8_UNORM, 
				R10G10B10_XR_BIAS_A2_UNORM, 
				B8G8R8A8_TYPELESS, 
				B8G8R8A8_UNORM_SRGB, 
				B8G8R8X8_TYPELESS, 
				B8G8R8X8_UNORM_SRGB, 
				BC6H_TYPELESS, 
				BC6H_UF16, 
				BC6H_SF16, 
				BC7_TYPELESS, 
				BC7_UNORM, 
				BC7_UNORM_SRGB, 
				AYUV, 
				Y410, 
				Y416, 
				NV12, 
				P010, 
				P016, 
				YUY2, 
				Y210, 
				Y216, 
				NV11, 
				AI44, 
				IA44, 
				P8, 
				A8P8, 
				B4G4R4A4_UNORM, 
				P208, 
				V208, 
				V408, 
				FORCE_UINT 
			};
			enum PRIMITIVE_TOPOLOGY {
				TOPOLOGY_UNDEFINED = 0,
				TOPOLOGY_POINTLIST = 1,
				TOPOLOGY_LINELIST = 2,
				TOPOLOGY_LINESTRIP = 3,
				TOPOLOGY_TRIANGLELIST = 4,
				TOPOLOGY_TRIANGLESTRIP = 5,
				TOPOLOGY_LINELIST_ADJ = 10,
				TOPOLOGY_LINESTRIP_ADJ = 11,
				TOPOLOGY_TRIANGLELIST_ADJ = 12,
				TOPOLOGY_TRIANGLESTRIP_ADJ = 13,
				TOPOLOGY_1_CONTROL_POINT_PATCHLIST = 33,
				TOPOLOGY_2_CONTROL_POINT_PATCHLIST = 34,
				TOPOLOGY_3_CONTROL_POINT_PATCHLIST = 35,
				TOPOLOGY_4_CONTROL_POINT_PATCHLIST = 36,
				TOPOLOGY_5_CONTROL_POINT_PATCHLIST = 37,
				TOPOLOGY_6_CONTROL_POINT_PATCHLIST = 38,
				TOPOLOGY_7_CONTROL_POINT_PATCHLIST = 39,
				TOPOLOGY_8_CONTROL_POINT_PATCHLIST = 40,
				TOPOLOGY_9_CONTROL_POINT_PATCHLIST = 41,
				TOPOLOGY_10_CONTROL_POINT_PATCHLIST = 42,
				TOPOLOGY_11_CONTROL_POINT_PATCHLIST = 43,
				TOPOLOGY_12_CONTROL_POINT_PATCHLIST = 44,
				TOPOLOGY_13_CONTROL_POINT_PATCHLIST = 45,
				TOPOLOGY_14_CONTROL_POINT_PATCHLIST = 46,
				TOPOLOGY_15_CONTROL_POINT_PATCHLIST = 47,
				TOPOLOGY_16_CONTROL_POINT_PATCHLIST = 48,
				TOPOLOGY_17_CONTROL_POINT_PATCHLIST = 49,
				TOPOLOGY_18_CONTROL_POINT_PATCHLIST = 50,
				TOPOLOGY_19_CONTROL_POINT_PATCHLIST = 51,
				TOPOLOGY_20_CONTROL_POINT_PATCHLIST = 52,
				TOPOLOGY_21_CONTROL_POINT_PATCHLIST = 53,
				TOPOLOGY_22_CONTROL_POINT_PATCHLIST = 54,
				TOPOLOGY_23_CONTROL_POINT_PATCHLIST = 55,
				TOPOLOGY_24_CONTROL_POINT_PATCHLIST = 56,
				TOPOLOGY_25_CONTROL_POINT_PATCHLIST = 57,
				TOPOLOGY_26_CONTROL_POINT_PATCHLIST = 58,
				TOPOLOGY_27_CONTROL_POINT_PATCHLIST = 59,
				TOPOLOGY_28_CONTROL_POINT_PATCHLIST = 60,
				TOPOLOGY_29_CONTROL_POINT_PATCHLIST = 61,
				TOPOLOGY_30_CONTROL_POINT_PATCHLIST = 62,
				TOPOLOGY_31_CONTROL_POINT_PATCHLIST = 63,
				TOPOLOGY_32_CONTROL_POINT_PATCHLIST = 64,
			};
			enum QUERY_TYPE {
				//If nothing was actually rendered, query value will be zero, otherwise non-zero.
				//This query might be faster then QUERY_TYPE_OCCLUSION_COUNT.
				QUERY_TYPE_OCCLUSION = 0,
				//If nothing was actually rendered, query value will be zero, otherwise it will be 
				//the exact count of rendered pixels. Might be slower then QUERY_TYPE_OCCLUSION.
				QUERY_TYPE_OCCLUSION_COUNT = 1,
				QUERY_TYPE_TIMESTAMP = 2,
				QUERY_TYPE_PIPELINE_STATISTICS = 3,
				QUERY_TYPE_STREAM_OUTPUT_ALL = 4,
				QUERY_TYPE_STREAM_OUTPUT_0 = 5,
				QUERY_TYPE_STREAM_OUTPUT_1 = 6,
				QUERY_TYPE_STREAM_OUTPUT_2 = 7,
				QUERY_TYPE_STREAM_OUTPUT_3 = 8,
			};
			enum RESOURCE_STATE {
				STATE_COMMON							= 0x0000000,
				STATE_VERTEX_AND_CONSTANT_BUFFER		= 0x0000001,
				STATE_INDEX_BUFFER						= 0x0000002,
				STATE_RENDER_TARGET						= 0x0000004,
				STATE_UNORDERED_ACCESS					= 0x0000008,
				STATE_DEPTH_WRITE						= 0x0000010,
				STATE_DEPTH_READ						= 0x0000020,
				STATE_NON_PIXEL_SHADER_RESOURCE			= 0x0000040,
				STATE_PIXEL_SHADER_RESOURCE				= 0x0000080,
				STATE_STREAM_OUT						= 0x0000100,
				STATE_INDIRECT_ARGUMENT					= 0x0000200,
				STATE_COPY_DEST							= 0x0000400,
				STATE_COPY_SOURCE						= 0x0000800,
				STATE_RESOLVE_DEST						= 0x0001000,
				STATE_RESOLVE_SOURCE					= 0x0002000,
				STATE_RAYTRACING_ACCELERATION_STRUCTURE	= 0x0400000,
				STATE_SHADING_RATE_SOURCE				= 0x1000000,

				STATE_PRESENT							= 0x0000000,
				STATE_PREDICATION						= 0x0000200,
				STATE_VIDEO_DECODE_READ					= 0x0010000,
				STATE_VIDEO_DECODE_WRITE				= 0x0020000,
				STATE_VIDEO_PROCESS_READ				= 0x0040000,
				STATE_VIDEO_PROCESS_WRITE				= 0x0080000,
				STATE_VIDEO_ENCODE_READ					= 0x0200000,
				STATE_VIDEO_ENCODE_WRITE				= 0x0800000,

				STATE_GENERIC_READ = (STATE_VERTEX_AND_CONSTANT_BUFFER | STATE_INDEX_BUFFER | STATE_NON_PIXEL_SHADER_RESOURCE | STATE_PIXEL_SHADER_RESOURCE | STATE_INDIRECT_ARGUMENT | STATE_COPY_SOURCE),
			};
			DEFINE_ENUM_FLAG_OPERATORS(RESOURCE_STATE);
			enum CONTEXT_TYPE {
				CONTEXT_TYPE_GRAPHIC,
				CONTEXT_TYPE_GRAPHIC_COPY,
				CONTEXT_TYPE_PHYSICS,
				CONTEXT_TYPE_PHYSICS_COPY,
			};
			enum class DRAW_TAG {
				DRAW_TAG_OPAQUE = 0x01,
				DRAW_TAG_ALPHA = 0x02,
				DRAW_TAG_TRANSPARENT = 0x04,
				DRAW_TAG_RAIN = 0x08,
			};
			DEFINE_ENUM_FLAG_OPERATORS(DRAW_TAG);
			enum class DRAW_TYPE {
				DRAW_TYPE_ALL,
				DRAW_TYPE_GEOMETRY,
				DRAW_TYPE_GEOMETRY_TESSELLATION,
			};
			enum class VIEW_TYPE { 
				NONE = 0x0000, 
				SRV = 0x0001, 
				UAV = 0x0002, 
				RTV = 0x0004, 
				DSV = 0x0008, 
				CUBE = 0x0010, 
				CBV = 0x0020,
				CBV_DYNAMIC = 0x0040,
				SRV_DYNAMIC = 0x0080,
				UAV_DYNAMIC = 0x0100,
				CUBE_DYNAMIC = 0x200,
				ALL = 0xffff 
			};
			DEFINE_ENUM_FLAG_OPERATORS(VIEW_TYPE);
			enum class DSV_ACCESS {
				NONE = 0x0,
				READ_ONLY_DEPTH = 0x1,
				READ_ONLY_STENCIL = 0x2,
				READ_ONLY = READ_ONLY_DEPTH | READ_ONLY_STENCIL,
			};
			DEFINE_ENUM_FLAG_OPERATORS(DSV_ACCESS);
			enum SHADING_RATE {
				SHADING_RATE_X1 = 0,
				SHADING_RATE_X2 = 1,
				SHADING_RATE_X4 = 2,
			};
			enum RENDER_PASS_ACCESS {
				RENDER_PASS_ACCESS_DISCARD = 0,
				RENDER_PASS_ACCESS_PRESERVE = 1,
				RENDER_PASS_ACCESS_CLEAR = 2,
				RENDER_PASS_ACCESS_NO_ACCESS = 3,
			};
			enum RENDER_PASS_FLAG {
				RENDER_PASS_FLAG_NONE = 0,
				RENDER_PASS_FLAG_UAV_WRITES = 0x1,
				RENDER_PASS_FLAG_SUSPEND = 0x2,
				RENDER_PASS_FLAG_RESUME = 0x4,
			};
			DEFINE_ENUM_FLAG_OPERATORS(RENDER_PASS_FLAG);
			enum SHADER_LANGUAGE {
				SHADER_DXIL, //HLSL 6.0+
				SHADER_SPIRV, //SPIR-V (Vulkan shading language)
				SHADER_HLSL, // < HLSL 6.0
				SHADER_GLSL, //OpenGL Shading Language
				SHADER_ESSL, //OpenGL ES Shading Language
				SHADER_MSL_MacOS, //Metal Shading Language (Mac OS)
				SHADER_MSL_iOS, //Metal Shading Language (iOS)
			};
			struct SHADER_MODEL {
				SHADER_LANGUAGE Language;
				uint32_t Major : 16;
				uint32_t Minor : 8;
				uint32_t Patch : 8;

				constexpr CLOAK_CALL SHADER_MODEL() : Language(SHADER_DXIL), Major(0), Minor(0), Patch(0) {}
				constexpr CLOAK_CALL SHADER_MODEL(In SHADER_LANGUAGE l, In uint16_t major, In uint8_t minor, In_opt uint8_t patch = 0) : Language(l), Major(major), Minor(minor), Patch(patch) {}
				constexpr uint32_t CLOAK_CALL_THIS FullVersion() const { return (static_cast<uint32_t>(Major) << 16) | (static_cast<uint32_t>(Minor) << 8) | Patch; }
				constexpr bool CLOAK_CALL_THIS operator==(In const SHADER_MODEL& o) const { return Language == o.Language && FullVersion() == o.FullVersion(); }
				constexpr bool CLOAK_CALL_THIS operator!=(In const SHADER_MODEL& o) const { return Language != o.Language || FullVersion() != o.FullVersion(); }
				constexpr bool CLOAK_CALL_THIS operator<=(In const SHADER_MODEL& o) const { return Language == o.Language && FullVersion() <= o.FullVersion(); }
				constexpr bool CLOAK_CALL_THIS operator<(In const SHADER_MODEL& o) const { return Language == o.Language && FullVersion() < o.FullVersion(); }
				constexpr bool CLOAK_CALL_THIS operator>=(In const SHADER_MODEL& o) const { return Language == o.Language && FullVersion() >= o.FullVersion(); }
				constexpr bool CLOAK_CALL_THIS operator>(In const SHADER_MODEL& o) const { return Language == o.Language && FullVersion() > o.FullVersion(); }
			};

			constexpr SHADER_MODEL MODEL_VULKAN_1_0(SHADER_SPIRV, 1, 0);
			constexpr SHADER_MODEL MODEL_VULKAN_1_1(SHADER_SPIRV, 1, 3);

			constexpr SHADER_MODEL MODEL_DIRECTX_11_0(SHADER_HLSL, 5, 0);
			constexpr SHADER_MODEL MODEL_DIRECTX_11_1(SHADER_HLSL, 5, 0);
			constexpr SHADER_MODEL MODEL_DIRECTX_11_2(SHADER_HLSL, 5, 0);
			constexpr SHADER_MODEL MODEL_DIRECTX_11_3(SHADER_HLSL, 5, 1);
			constexpr SHADER_MODEL MODEL_DIRECTX_12_0(SHADER_DXIL, 6, 0);

			constexpr SHADER_MODEL MODEL_HLSL_5_0(SHADER_HLSL, 5, 0);
			constexpr SHADER_MODEL MODEL_HLSL_5_1(SHADER_HLSL, 5, 1);
			constexpr SHADER_MODEL MODEL_HLSL_6_0(SHADER_DXIL, 6, 0);
			constexpr SHADER_MODEL MODEL_HLSL_6_1(SHADER_DXIL, 6, 1);
			constexpr SHADER_MODEL MODEL_HLSL_6_2(SHADER_DXIL, 6, 2);
			constexpr SHADER_MODEL MODEL_HLSL_6_3(SHADER_DXIL, 6, 3);
			constexpr SHADER_MODEL MODEL_HLSL_6_4(SHADER_DXIL, 6, 4);
			constexpr SHADER_MODEL MODEL_HLSL_6_5(SHADER_DXIL, 6, 5);

			//Generally supported shader versions:
			constexpr SHADER_MODEL SHADER_VERSIONS[] = {
				MODEL_HLSL_5_0,
				MODEL_HLSL_5_1,
				MODEL_HLSL_6_0,
				MODEL_HLSL_6_1,
				MODEL_HLSL_6_2,
				MODEL_HLSL_6_3,
				MODEL_HLSL_6_4,
				MODEL_HLSL_6_5,
			};

			constexpr size_t NUM_SHADER_MODELS = array_size(SHADER_VERSIONS);
			struct VIEWPORT {
				FLOAT TopLeftX;
				FLOAT TopLeftY;
				FLOAT Width;
				FLOAT Height;
				FLOAT MinDepth;
				FLOAT MaxDepth;
			};
			struct SCISSOR_RECT {
				LONG TopLeftX;
				LONG TopLeftY;
				LONG Width;
				LONG Height;
			};
			struct SCALED_SCISSOR_RECT {
				FLOAT TopLeftX;
				FLOAT TopLeftY;
				FLOAT Width;
				FLOAT Height;
			};
			struct VERTEX_BUFFER_VIEW {
				GPU_ADDRESS Address;
				UINT SizeInBytes;
				UINT StrideInBytes;
			};
			struct INDEX_BUFFER_VIEW {
				GPU_ADDRESS Address;
				UINT SizeInBytes;
				Format Format;
			};
			struct QUERY_DATA {
				QUERY_TYPE Type;
				union {
					bool Occlusion;
					uint64_t OcclusionCount;
					uint64_t Timestamp;
					struct {
						struct {
							uint64_t VertexCount;
							uint64_t PrimitiveCount;
							uint64_t Invocations;
						} VS;
						struct {
							uint64_t PrimitiveCount;
							uint64_t Invocations;
						} GS;
						struct {
							uint64_t PrimitiveCount;
							uint64_t Invocations;
						} Rasterizer;
						struct {
							uint64_t Invocations;
						} PS;
						struct {
							uint64_t Invocations;
						} HS;
						struct {
							uint64_t Invocations;
						} DS;
						struct {
							uint64_t Invocations;
						} CS;
					} PipelineStatistics;
					struct {
						uint64_t WrittenPrimitives;
						uint64_t OverflowSize;
					} StreamOutput[4];
				};
			};

			//-------------------------------------------SAMPLER---------------------------------------------
			enum FILTER_TYPE {
				FILTER_MIN_MAG_MIP_POINT = 0,
				FILTER_MIN_MAG_POINT_MIP_LINEAR = 0x1,
				FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x4,
				FILTER_MIN_POINT_MAG_MIP_LINEAR = 0x5,
				FILTER_MIN_LINEAR_MAG_MIP_POINT = 0x10,
				FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
				FILTER_MIN_MAG_LINEAR_MIP_POINT = 0x14,
				FILTER_MIN_MAG_MIP_LINEAR = 0x15,
				FILTER_ANISOTROPIC = 0x55,
				FILTER_COMPARISON_MIN_MAG_MIP_POINT = 0x80,
				FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR = 0x81,
				FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x84,
				FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR = 0x85,
				FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT = 0x90,
				FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
				FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT = 0x94,
				FILTER_COMPARISON_MIN_MAG_MIP_LINEAR = 0x95,
				FILTER_COMPARISON_ANISOTROPIC = 0xd5,
				FILTER_MINIMUM_MIN_MAG_MIP_POINT = 0x100,
				FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x101,
				FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x104,
				FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x105,
				FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x110,
				FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x111,
				FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x114,
				FILTER_MINIMUM_MIN_MAG_MIP_LINEAR = 0x115,
				FILTER_MINIMUM_ANISOTROPIC = 0x155,
				FILTER_MAXIMUM_MIN_MAG_MIP_POINT = 0x180,
				FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x181,
				FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x184,
				FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x185,
				FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x190,
				FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x191,
				FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x194,
				FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR = 0x195,
				FILTER_MAXIMUM_ANISOTROPIC = 0x1d5
			};
			enum TEXTURE_ADDRESS_MODE {
				MODE_WRAP = 1,
				MODE_MIRROR = 2,
				MODE_CLAMP = 3,
				MODE_BORDER = 4,
				MODE_MIRROR_ONCE = 5
			};
			enum COMPARE_FUNC {
				CMP_FUNC_NEVER = 1,
				CMP_FUNC_LESS = 2,
				CMP_FUNC_EQUAL = 3,
				CMP_FUNC_LESS_EQUAL = 4,
				CMP_FUNC_GREATER = 5,
				CMP_FUNC_NOT_EQUAL = 6,
				CMP_FUNC_GREATER_EQUAL = 7,
				CMP_FUNC_ALWAYS = 8
			};
			struct SAMPLER_DESC {
				FILTER_TYPE Filter;
				TEXTURE_ADDRESS_MODE AddressU;
				TEXTURE_ADDRESS_MODE AddressV;
				TEXTURE_ADDRESS_MODE AddressW;
				FLOAT MipLODBias;
				UINT MaxAnisotropy;
				COMPARE_FUNC CompareFunction;
				FLOAT BorderColor[4];
				FLOAT MinLOD;
				FLOAT MaxLOD;
			};

			//----------------------------------------------ROOT DESCRIPTION--------------------------------------------
			enum SHADER_VISIBILITY {
				VISIBILITY_ALL		= 0xFF,
				VISIBILITY_VERTEX	= 1 << 0,
				VISIBILITY_HULL		= 1 << 1,
				VISIBILITY_DOMAIN	= 1 << 2,
				VISIBILITY_GEOMETRY = 1 << 3,
				VISIBILITY_PIXEL	= 1 << 4,
				VISIBILITY_COMPUTE	= 1 << 5,
			};
			DEFINE_ENUM_FLAG_OPERATORS(SHADER_VISIBILITY);

			enum DESCRIPTOR_RANGE_TYPE {
				RANGE_TYPE_SRV = 0,
				RANGE_TYPE_UAV = (RANGE_TYPE_SRV + 1),
				RANGE_TYPE_CBV = (RANGE_TYPE_UAV + 1),
				RANGE_TYPE_SAMPLER = (RANGE_TYPE_CBV + 1)
			};
			enum ROOT_PARAMETER_TYPE {
				ROOT_PARAMETER_TYPE_UNKNOWN = 0,
				ROOT_PARAMETER_TYPE_TABLE = (ROOT_PARAMETER_TYPE_UNKNOWN + 1),
				ROOT_PARAMETER_TYPE_CONSTANTS = (ROOT_PARAMETER_TYPE_TABLE + 1),
				ROOT_PARAMETER_TYPE_CBV = (ROOT_PARAMETER_TYPE_CONSTANTS + 1),
				ROOT_PARAMETER_TYPE_SRV = (ROOT_PARAMETER_TYPE_CBV + 1),
				ROOT_PARAMETER_TYPE_UAV = (ROOT_PARAMETER_TYPE_SRV + 1),
				ROOT_PARAMETER_TYPE_SAMPLER = (ROOT_PARAMETER_TYPE_UAV + 1),
				ROOT_PARAMETER_TYPE_TEXTURE_ATLAS = (ROOT_PARAMETER_TYPE_SAMPLER + 1),
			};
			enum ROOT_SIGNATURE_FLAG {
				ROOT_SIGNATURE_FLAG_NONE = 0,
				ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT = 0x1,
				ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS = 0x2,
				ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS = 0x4,
				ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS = 0x8,
				ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS = 0x10,
				ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS = 0x20,
				ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT = 0x40
			};
			enum STATIC_BORDER_COLOR {
				STATIC_BORDER_COLOR_TRANSPARENT_BLACK = 0,
				STATIC_BORDER_COLOR_OPAQUE_BLACK = (STATIC_BORDER_COLOR_TRANSPARENT_BLACK + 1),
				STATIC_BORDER_COLOR_OPAQUE_WHITE = (STATIC_BORDER_COLOR_OPAQUE_BLACK + 1),
			};
			enum ROOT_PARAMETER_FLAG {
				//Default, driver chooses most suitable
				ROOT_PARAMETER_FLAG_NONE = 0,
				//Data can be changed at any time
				ROOT_PARAMETER_FLAG_VOLATILE = (1 << 0),
				//Data can be changed as long as the resource is not bound to the context
				ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION = (1 << 1),
				//All data changes must be complete before the resource can be bound to the context
				ROOT_PARAMETER_FLAG_STATIC = (1 << 2),
				//Must be used when using SetDynamicDescriptorTable to set parameter
				ROOT_PARAMETER_FLAG_DYNAMIC_DESCRIPTOR = (1 << 3),
			};
			DEFINE_ENUM_FLAG_OPERATORS(ROOT_PARAMETER_FLAG);
			DEFINE_ENUM_FLAG_OPERATORS(ROOT_SIGNATURE_FLAG);
			struct STATIC_SAMPLER_DESC {
				FILTER_TYPE Filter;
				TEXTURE_ADDRESS_MODE AddressU;
				TEXTURE_ADDRESS_MODE AddressV;
				TEXTURE_ADDRESS_MODE AddressW;
				FLOAT MipLODBias;
				UINT MaxAnisotropy;
				COMPARE_FUNC CompareFunction;
				STATIC_BORDER_COLOR BorderColor;
				FLOAT MinLOD;
				FLOAT MaxLOD;
				UINT RegisterID;
				UINT Space;
				SHADER_VISIBILITY Visible;
			};
			struct DESCRIPTOR_RANGE_ENTRY {
				DESCRIPTOR_RANGE_TYPE Type;
				UINT RegisterID;
				UINT Space;
				UINT Count;
				ROOT_PARAMETER_FLAG Flags;
			};
			struct ROOT_PARAMETER {
				ROOT_PARAMETER_TYPE Type;
				SHADER_VISIBILITY Visible;
				union {
					struct {
						UINT Size;
						const DESCRIPTOR_RANGE_ENTRY* Range;
					} Table;
					struct {
						UINT RegisterID;
						UINT Space;
						UINT NumDWords;
					} Constants;
					struct {
						UINT RegisterID;
						UINT Space;
						ROOT_PARAMETER_FLAG Flags;
					} CBV;
					struct {
						UINT RegisterID;
						UINT Space;
						ROOT_PARAMETER_FLAG Flags;
					} SRV;
					struct {
						UINT RegisterID;
						UINT Space;
						ROOT_PARAMETER_FLAG Flags;
					} UAV;
					struct {
						UINT RegisterID;
						UINT Space;
						SAMPLER_DESC Desc;
					} Sampler;
					struct {
						UINT RegisterID;
						UINT Space;
						ROOT_PARAMETER_FLAG Flags;
					} TextureAtlas;
				};
			};

			//--------------------------------------------------PIPELINE STATE-----------------------------------------
			constexpr size_t MAX_RENDER_TARGET = 8;
			constexpr FLOAT MAX_LOD = 3.402823466e+38f;
			constexpr UINT DEFAULT_SAMPLE_MASK = 0xffffffff;
			constexpr UINT MAX_VIEW_INSTANCES = 4;
			enum BLEND {
				BLEND_ZERO = 1,
				BLEND_ONE = 2,
				BLEND_SRC_COLOR = 3,
				BLEND_INV_SRC_COLOR = 4,
				BLEND_SRC_ALPHA = 5,
				BLEND_INV_SRC_ALPHA = 6,
				BLEND_DEST_ALPHA = 7,
				BLEND_INV_DEST_ALPHA = 8,
				BLEND_DEST_COLOR = 9,
				BLEND_INV_DEST_COLOR = 10,
				BLEND_SRC_ALPHA_SAT = 11,
				BLEND_FACTOR = 14,
				BLEND_INV_BLEND_FACTOR = 15,
				BLEND_SRC1_COLOR = 16,
				BLEND_INV_SRC1_COLOR = 17,
				BLEND_SRC1_ALPHA = 18,
				BLEND_INV_SRC1_ALPHA = 19
			};
			enum BLEND_OP {
				BLEND_OP_ADD = 1,
				BLEND_OP_SUBTRACT = 2,
				BLEND_OP_REV_SUBTRACT = 3,
				BLEND_OP_MIN = 4,
				BLEND_OP_MAX = 5
			};
			enum LOGIC_OP {
				LOGIC_OP_CLEAR = 0,
				LOGIC_OP_SET = (LOGIC_OP_CLEAR + 1),
				LOGIC_OP_COPY = (LOGIC_OP_SET + 1),
				LOGIC_OP_COPY_INVERTED = (LOGIC_OP_COPY + 1),
				LOGIC_OP_NOOP = (LOGIC_OP_COPY_INVERTED + 1),
				LOGIC_OP_INVERT = (LOGIC_OP_NOOP + 1),
				LOGIC_OP_AND = (LOGIC_OP_INVERT + 1),
				LOGIC_OP_NAND = (LOGIC_OP_AND + 1),
				LOGIC_OP_OR = (LOGIC_OP_NAND + 1),
				LOGIC_OP_NOR = (LOGIC_OP_OR + 1),
				LOGIC_OP_XOR = (LOGIC_OP_NOR + 1),
				LOGIC_OP_EQUIV = (LOGIC_OP_XOR + 1),
				LOGIC_OP_AND_REVERSE = (LOGIC_OP_EQUIV + 1),
				LOGIC_OP_AND_INVERTED = (LOGIC_OP_AND_REVERSE + 1),
				LOGIC_OP_OR_REVERSE = (LOGIC_OP_AND_INVERTED + 1),
				LOGIC_OP_OR_INVERTED = (LOGIC_OP_OR_REVERSE + 1)
			};
			enum COLOR_MASK {
				COLOR_RED = 1,
				COLOR_GREEN = 2,
				COLOR_BLUE = 4,
				COLOR_ALPHA = 8,
				COLOR_RGB = (COLOR_RED | COLOR_GREEN | COLOR_BLUE),
				COLOR_ALL = (COLOR_RGB | COLOR_ALPHA)
			};
			enum FILL_MODE {
				FILL_WIREFRAME = 2,
				FILL_SOLID = 3
			};
			enum CULL_MODE {
				CULL_NONE = 1,
				CULL_FRONT = 2,
				CULL_BACK = 3
			};
			enum CONSERVATIVE_RASTERIZATION_MODE {
				CONSERVATIVE_RASTERIZATION_OFF = 0,
				CONSERVATIVE_RASTERIZATION_ON = 1
			};
			enum WRITE_MASK {
				WRITE_MASK_ZERO = 0,
				WRITE_MASK_ALL = 1
			};
			enum STENCIL_OP {
				STENCIL_OP_KEEP = 1,
				STENCIL_OP_ZERO = 2,
				STENCIL_OP_REPLACE = 3,
				STENCIL_OP_INCR_SAT = 4,
				STENCIL_OP_DECR_SAT = 5,
				STENCIL_OP_INVERT = 6,
				STENCIL_OP_INCR = 7,
				STENCIL_OP_DECR = 8
			};
			enum INPUT_LAYOUT_TYPE {
				INPUT_LAYOUT_TYPE_NONE = 0,
				INPUT_LAYOUT_TYPE_GUI = 1,
				INPUT_LAYOUT_TYPE_MESH = 2,
				INPUT_LAYOUT_TYPE_MESH_POS = 3,
				INPUT_LAYOUT_TYPE_TERRAIN = 4,
				INPUT_LAYOUT_TYPE_CUSTOM = 5,
			};
			enum INPUT_CLASSIFICATION {
				PER_VERTEX_DATA = 0,
				PER_INSTANCE_DATA = 1
			};
			enum INDEX_BUFFER_STRIP_CUT {
				INDEX_BUFFER_STRIP_CUT_DISABLED = 0,
				INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF = 1,
				INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF = 2
			};
			enum PRIMITIVE_TOPOLOGY_TYPE {
				TOPOLOGY_TYPE_UNDEFINED = 0,
				TOPOLOGY_TYPE_POINT = 1,
				TOPOLOGY_TYPE_LINE = 2,
				TOPOLOGY_TYPE_TRIANGLE = 3,
				TOPOLOGY_TYPE_PATCH = 4
			};
			enum VIEW_INSTANCING_FLAGS {
				VIEW_INSTANCING_FLAGS_NONE = 0, //The given numer of instances will allways be executed
				VIEW_INSTANCING_FLAGS_ALLOW_MASKING = 0x1, //Instances can be masked out during command list recording
			};
			enum PSO_SUBTYPE {
				PSO_SUBTYPE_CS = 0,
				PSO_SUBTYPE_PS = PSO_SUBTYPE_CS + 1,
				PSO_SUBTYPE_DS = PSO_SUBTYPE_PS + 1,
				PSO_SUBTYPE_HS = PSO_SUBTYPE_DS + 1,
				PSO_SUBTYPE_GS = PSO_SUBTYPE_HS + 1,
				PSO_SUBTYPE_VS = PSO_SUBTYPE_GS + 1,
				PSO_SUBTYPE_STREAM_OUTPUT = PSO_SUBTYPE_VS + 1,
				PSO_SUBTYPE_BLEND = PSO_SUBTYPE_STREAM_OUTPUT + 1,
				PSO_SUBTYPE_SAMPLE_MASK = PSO_SUBTYPE_BLEND + 1,
				PSO_SUBTYPE_RASTERIZER = PSO_SUBTYPE_SAMPLE_MASK + 1,
				PSO_SUBTYPE_DEPTH_STENCIL = PSO_SUBTYPE_RASTERIZER + 1,
				PSO_SUBTYPE_INPUT_LAYOUT = PSO_SUBTYPE_DEPTH_STENCIL + 1,
				PSO_SUBTYPE_INDEX_BUFFER_STRIP_CUT = PSO_SUBTYPE_INPUT_LAYOUT + 1,
				PSO_SUBTYPE_PRIMITIVE_TOPOLOGY = PSO_SUBTYPE_INDEX_BUFFER_STRIP_CUT + 1,
				PSO_SUBTYPE_RENDER_TARGETS = PSO_SUBTYPE_PRIMITIVE_TOPOLOGY + 1,
				PSO_SUBTYPE_SAMPLE_DESC = PSO_SUBTYPE_RENDER_TARGETS + 1,
				PSO_SUBTYPE_VIEW_INSTANCING = PSO_SUBTYPE_SAMPLE_DESC + 1,
			};

			struct SHADER_CODE {
				Filed_size_bytes(Length) const void* Code;
				size_t Length;
			};
			struct STREAM_OUTPUT_ENTRY_DESC {
				UINT Stream;
				LPCSTR SemanticName;
				UINT SemanticIndex;
				BYTE StartComponent;
				BYTE ComponentCount;
				BYTE OutputSlot;
			};
			struct STREAM_OUTPUT_DESC {
				Field_size(NumEntries) const STREAM_OUTPUT_ENTRY_DESC* Entries;
				UINT NumEntries;
				Field_size(NumStrides) const UINT* BufferStrides;
				UINT NumStrides;
				UINT RasterizedStreamIndex;
			};
			struct BLEND_STATE_TARGET_DESC {
				bool BlendEnable;
				bool LogicOperationEnable;
				BLEND SrcBlend;
				BLEND DstBlend;
				BLEND_OP BlendOperation;
				BLEND SrcBlendAlpha;
				BLEND DstBlendAlpha;
				BLEND_OP BlendOperationAlpha;
				LOGIC_OP LogicOperation;
				COLOR_MASK WriteMask;
			};
			struct BLEND_STATE_DESC {
				bool AlphaToCoverage;
				bool IndependentBlend;
				BLEND_STATE_TARGET_DESC RenderTarget[MAX_RENDER_TARGET];
			};
			struct RASTERIZER_STATE_DESC {
				FILL_MODE FillMode;
				CULL_MODE CullMode;
				bool FrontCounterClockwise;
				INT DepthBias;
				FLOAT DepthBiasClamp;
				FLOAT SlopeScaledDepthBias;
				bool DepthClip;
				bool Multisample;
				bool AntialiasedLine;
				UINT ForcedSampleCount;
				CONSERVATIVE_RASTERIZATION_MODE ConservativeRasterization;
			};
			struct STENCIL_OP_DESC {
				STENCIL_OP StencilFailOperation;
				STENCIL_OP DepthFailOperation;
				STENCIL_OP PassOperation;
				COMPARE_FUNC StencilFunction;
			};
			struct DEPTH_STENCIL_STATE_DESC {
				bool DepthEnable;
				bool StencilEnable;
				WRITE_MASK DepthWriteMask;
				COMPARE_FUNC DepthFunction;
				uint8_t StencilReadMask;
				uint8_t StencilWriteMask;
				STENCIL_OP_DESC FrontFace;
				STENCIL_OP_DESC BackFace;
				bool DepthBoundsTests;
				Format Format;
			};
			struct INPUT_ELEMENT_DESC {
				LPCSTR SemanticName;
				UINT SemanticIndex;
				Format Format;
				UINT Offset;
				In_range(0, 15) UINT InputSlot;
				INPUT_CLASSIFICATION InputSlotClass;
				UINT InstanceDataStepRate;
			};
			struct INPUT_LAYOUT_DESC {
				INPUT_LAYOUT_TYPE Type;
				struct {
					UINT NumElements;
					Field_size(NumElements) const INPUT_ELEMENT_DESC* Elements;
				} Custom;
			};
			struct SAMPLE_DESC {
				UINT Count;
				UINT Quality;
			};
			struct SHADER_SET {
				SHADER_CODE CS;
				SHADER_CODE VS;
				SHADER_CODE GS;
				SHADER_CODE HS;
				SHADER_CODE DS;
				SHADER_CODE PS;
			};
			struct RENDER_TARGETS_DESC {
				In_range(0, MAX_RENDER_TARGET) UINT NumRenderTargets;
				Format RTVFormat[MAX_RENDER_TARGET];
			};
			struct VIEW_INSTANCE_LOCATION {
				UINT ViewportIndex;
				UINT RenderTargetIndex;
			};
			struct VIEW_INSTANCING_DESC {
				VIEW_INSTANCING_FLAGS Flags;
				In_range(0, MAX_VIEW_INSTANCES) UINT NumInstances;
				VIEW_INSTANCE_LOCATION Location[MAX_VIEW_INSTANCES];
			};
			struct alignas(void*) PIPELINE_STATE_DESC {
				private:
					template<PSO_SUBTYPE> struct ST : public std::false_type {};
					template<> struct ST<PSO_SUBTYPE_CS> : public std::true_type { typedef SHADER_CODE type; };
					template<> struct ST<PSO_SUBTYPE_PS> : public std::true_type { typedef SHADER_CODE type; };
					template<> struct ST<PSO_SUBTYPE_DS> : public std::true_type { typedef SHADER_CODE type; };
					template<> struct ST<PSO_SUBTYPE_HS> : public std::true_type { typedef SHADER_CODE type; };
					template<> struct ST<PSO_SUBTYPE_GS> : public std::true_type { typedef SHADER_CODE type; };
					template<> struct ST<PSO_SUBTYPE_VS> : public std::true_type { typedef SHADER_CODE type; };
					template<> struct ST<PSO_SUBTYPE_STREAM_OUTPUT> : public std::true_type  { typedef STREAM_OUTPUT_DESC type; };
					template<> struct ST<PSO_SUBTYPE_BLEND> : public std::true_type { typedef BLEND_STATE_DESC type; };
					template<> struct ST<PSO_SUBTYPE_SAMPLE_MASK> : public std::true_type { typedef UINT type; };
					template<> struct ST<PSO_SUBTYPE_RASTERIZER> : public std::true_type { typedef RASTERIZER_STATE_DESC type; };
					template<> struct ST<PSO_SUBTYPE_DEPTH_STENCIL> : public std::true_type { typedef DEPTH_STENCIL_STATE_DESC type; };
					template<> struct ST<PSO_SUBTYPE_INPUT_LAYOUT> : public std::true_type { typedef INPUT_LAYOUT_DESC type; };
					template<> struct ST<PSO_SUBTYPE_INDEX_BUFFER_STRIP_CUT> : public std::true_type { typedef INDEX_BUFFER_STRIP_CUT type; };
					template<> struct ST<PSO_SUBTYPE_PRIMITIVE_TOPOLOGY> : public std::true_type { typedef PRIMITIVE_TOPOLOGY_TYPE type; };
					template<> struct ST<PSO_SUBTYPE_RENDER_TARGETS> : public std::true_type { typedef RENDER_TARGETS_DESC type; };
					template<> struct ST<PSO_SUBTYPE_SAMPLE_DESC> : public std::true_type { typedef SAMPLE_DESC type; };
					template<> struct ST<PSO_SUBTYPE_VIEW_INSTANCING> : public std::true_type { typedef VIEW_INSTANCING_DESC type; };

					template<PSO_SUBTYPE T> struct alignas(void*) PST {
						public:
							typename ST<T>::type Data;
					};

					template<PSO_SUBTYPE T = static_cast<PSO_SUBTYPE>(0)> struct STS {
						private:
							template<PSO_SUBTYPE A, bool> struct S 
							{ 
								static constexpr size_t value = sizeof(PST<A>) + STS<static_cast<PSO_SUBTYPE>(T + 1)>::value;
								static constexpr size_t count = 1 + STS<static_cast<PSO_SUBTYPE>(T + 1)>::count;
							};
							template<PSO_SUBTYPE A> struct S<A, false> { static constexpr size_t value = 0; static constexpr size_t count = 0; };
						public:
							static constexpr size_t value = S<T, ST<T>::value>::value;
							static constexpr size_t count = S<T, ST<T>::value>::count;
					};
					template<typename T> struct CTH {
						static constexpr bool CLOAK_CALL Contains(In const T& t) { return true; }
					};
					template<> struct CTH<SHADER_CODE> {
						static bool CLOAK_CALL Contains(In const SHADER_CODE& t) { return t.Length > 0; }
					};
					

					static constexpr size_t MAX_BUFFER_SIZE = STS<>::value;
					static constexpr size_t TYPE_COUNT = STS<>::count;

					uint8_t m_buffer[MAX_BUFFER_SIZE];
					size_t m_offset[TYPE_COUNT];
					size_t m_size;
				public:
					CLOAK_CALL PIPELINE_STATE_DESC() : m_size(0) { for (size_t a = 0; a < TYPE_COUNT; a++) { m_offset[a] = MAX_BUFFER_SIZE; } memset(m_buffer, 0, MAX_BUFFER_SIZE); }

					template<PSO_SUBTYPE T> CLOAK_FORCEINLINE typename ST<T>::type& CLOAK_CALL_THIS Get()
					{
						static_assert(T < TYPE_COUNT, "Illegal Parameter");
						static_assert(sizeof(typename ST<T>::type) > 0, "Non-existing type");
						if (m_offset[T] == MAX_BUFFER_SIZE)
						{
							m_offset[T] = m_size;
							m_size += sizeof(PST<T>);
							CLOAK_ASSUME(m_size <= MAX_BUFFER_SIZE);
							::new(reinterpret_cast<PST<T>*>(&m_buffer[m_offset[T]]))PST<T>();
						}
						return reinterpret_cast<PST<T>*>(&m_buffer[m_offset[T]])->Data;
					}
					template<PSO_SUBTYPE T> CLOAK_FORCEINLINE const typename ST<T>::type& CLOAK_CALL_THIS Get() const
					{
						CLOAK_ASSUME(Contains<T>() == true);
						return reinterpret_cast<const PST<T>*>(&m_buffer[m_offset[T]])->Data;
					}
					template<PSO_SUBTYPE T> CLOAK_FORCEINLINE bool CLOAK_CALL_THIS Contains() const
					{
						static_assert(T < TYPE_COUNT, "Illegal Parameter");
						static_assert(sizeof(typename ST<T>::type) > 0, "Non-existing type");
						if (m_offset[T] >= MAX_BUFFER_SIZE) { return false; }
						return CTH<typename ST<T>::type>::Contains(reinterpret_cast<const PST<T>*>(&m_buffer[m_offset[T]])->Data);
					}
					CLOAK_FORCEINLINE void CLOAK_CALL_THIS Reset() 
					{
						for (size_t a = 0; a < array_size(m_offset); a++) { m_offset[a] = MAX_BUFFER_SIZE; }
						m_size = 0;
					}
			};

			//--------------------------------------------------HARDWARE SETTINGS----------------------------------------
			enum RESOURCE_BINDING_TIER {
				RESOURCE_BINDING_TIER_1 = 1,
				RESOURCE_BINDING_TIER_2 = 2,
				RESOURCE_BINDING_TIER_3 = 3,
			};
			enum RESOURCE_HEAP_TIER {
				RESOURCE_HEAP_TIER_1 = 1, // Buffers, Non-Render-Non-DepthStencil textures and Render/DepthStencil textures must not be on the same heap
				RESOURCE_HEAP_TIER_2 = 2, // Any type of resource may be on the same heap
			};
			enum ROOT_SIGNATURE_VERSION {
				ROOT_SIGNATURE_VERSION_1_0 = 0x1,
				ROOT_SIGNATURE_VERSION_1_1 = 0x2,
			};
			enum CROSS_ADAPTER_SUPPORT {
				CROSS_ADAPTER_NOT_SUPPORTED = 0,
				CROSS_ADAPTER_SUPPORT_TIER_1_EMULATED = 1,
				CROSS_ADAPTER_SUPPORT_TIER_1 = 2,
				CROSS_ADAPTER_SUPPORT_TIER_2 = 3,
			};
			enum VIEW_INSTANCE_SUPPORT {
				VIEW_INSTANCE_NOT_SUPPORTED = 0,
				VIEW_INSTANCE_SUPPORT_TIER_1 = 1, //Instancing is only implemented with Draw-Call Looping
				VIEW_INSTANCE_SUPPORT_TIER_2 = 2, //Same as Tier 1, but Hardware may optimize it further
				VIEW_INSTANCE_SUPPORT_TIER_3 = 3, //Instancing is fully supported, all redudant work is eliminated
			};
			enum TILED_RESOURCES_SUPPORT {
				TILED_RESOURCES_NOT_SUPPORTED = 0,
				TILED_RESOURCES_SUPPORT_TIER_1 = 1, //Support 2D Texture. GPU Reads/Writes to unmapped tiles are undefined
				TILED_RESOURCES_SUPPORT_TIER_2 = 2, //GPU reads to unmapped tiles return 0, GPU Writes to unmapped tiles are discarded
				TILED_RESOURCES_SUPPORT_TIER_3 = 3, //Support 3D Texture
				TILED_RESOURCES_SUPPORT_TIER_4 = 4,
			};
			enum RAYTRACING_SUPPORT {
				RAYTRACING_NOT_SUPPORTED = 0,
				RAYTRACING_SUPPORT_TIER_1 = 1,
			};
			enum VARIABLE_SHADING_RATE_SUPPORT {
				VARIABLE_SHADING_RATE_NOT_SUPPORTED = 0,
				VARIABLE_SHADING_RATE_TIER_1 = 1,
				VARIABLE_SHADING_RATE_TIER_2 = 2,
			};
			enum CONSERVATIVE_RASTERIZATION_SUPPORT {
				CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED = 0,
				CONSERVATIVE_RASTERIZATION_SUPPORT_TIER_1 = 1, // Maximum 1/2 pixel uncertainty
				CONSERVATIVE_RASTERIZATION_SUPPORT_TIER_2 = 2, // Maximum 1/256 pixel uncertainty
				CONSERVATIVE_RASTERIZATION_SUPPORT_TIER_3 = 3, // Maximum 1/256 pixel uncertainty and SV_InnerCoverage in HLSL
			};
			struct Hardware {
				SHADER_MODEL ShaderModel;
				RESOURCE_BINDING_TIER ResourceBindingTier;
				RESOURCE_HEAP_TIER ResourceHeapTier;
				ROOT_SIGNATURE_VERSION RootVersion;
				CROSS_ADAPTER_SUPPORT CrossAdapterSupport;
				VIEW_INSTANCE_SUPPORT ViewInstanceSupport;
				TILED_RESOURCES_SUPPORT TiledResourcesSupport;
				CONSERVATIVE_RASTERIZATION_SUPPORT ConservativeRasterizationSupport;
				uint32_t LaneCount;
				bool TypedUAV;
				bool AllowTearing;
				bool CopyContextAllowTimestampQueries;
				bool AllowDepthBoundsTests;
				bool StandardSwizzle64KBSupported;
				struct {
					RAYTRACING_SUPPORT Supported;
					bool TiledSRVTier3; //Support Tier 3 Tiled Resources within raytracing (only SRV)
				} Raytracing;
				struct {
					VARIABLE_SHADING_RATE_SUPPORT Supported;
					SHADING_RATE MaxShadingRate;
					UINT ImageTileSize;
				} VariableShadingRate;
			};

			//--------------------------------------------------RESSOURCE CREATION----------------------------------------
			enum class TEXTURE_TYPE { TEXTURE, TEXTURE_ARRAY, TEXTURE_CUBE };
			struct TEXTURE_DESC {
				uint32_t Width;
				uint32_t Height;
				Format Format;
				TEXTURE_TYPE Type;
				API::Helper::Color::RGBA clearValue;
				API::Rendering::VIEW_TYPE AccessType = VIEWTYPE_TEXTURE_DEFAULT;
				union {
					struct {
						uint32_t SampleCount;
						uint32_t MipMaps;
					} Texture;
					struct {
						uint32_t MipMaps;
						uint32_t Images;
					} TextureArray;
					struct {
						uint32_t MipMaps;
					} TextureCube;
				};
			};
			struct DEPTH_DESC {
				uint32_t Width;
				uint32_t Height;
				Format Format;
				struct {
					bool UseSRV;
					float Clear;
				} Depth;
				struct {
					bool UseSRV;
					bool UseRTV;
					uint8_t Clear;
				} Stencil;
				uint32_t SampleCount;
			};
			struct SUBRESOURCE_DATA {
				const void *pData;
				LONG_PTR RowPitch;
				LONG_PTR SlicePitch;
			};
			struct GPU_DESCRIPTOR_HANDLE {
				constexpr CLOAKENGINE_API CLOAK_CALL GPU_DESCRIPTOR_HANDLE(In GPU_ADDRESS Adress = GPU_ADDRESS_NULL) : ptr(Adress) {}
				constexpr CLOAKENGINE_API CLOAK_CALL GPU_DESCRIPTOR_HANDLE(In const GPU_DESCRIPTOR_HANDLE& h) : ptr(h.ptr) {}
				constexpr CLOAKENGINE_API GPU_DESCRIPTOR_HANDLE& operator=(In const GPU_DESCRIPTOR_HANDLE& h) { ptr = h.ptr; return *this; }
				GPU_ADDRESS ptr = GPU_ADDRESS_NULL;
			};
			struct CPU_DESCRIPTOR_HANDLE {
				constexpr CLOAKENGINE_API CLOAK_CALL CPU_DESCRIPTOR_HANDLE(In SIZE_T Adress = CPU_ADDRESS_NULL) : ptr(Adress) {}
				constexpr CLOAKENGINE_API CLOAK_CALL CPU_DESCRIPTOR_HANDLE(In const CPU_DESCRIPTOR_HANDLE& h) : ptr(h.ptr) {}
				constexpr CLOAKENGINE_API CPU_DESCRIPTOR_HANDLE& operator=(In const CPU_DESCRIPTOR_HANDLE& h) { ptr = h.ptr; return *this; }
				SIZE_T ptr = CPU_ADDRESS_NULL;
			};
			struct DWParam {
				CLOAKENGINE_API CLOAK_CALL DWParam();
				CLOAKENGINE_API CLOAK_CALL DWParam(In FLOAT f);
				CLOAKENGINE_API CLOAK_CALL DWParam(In UINT f);
				CLOAKENGINE_API CLOAK_CALL DWParam(In INT f);

				CLOAKENGINE_API void CLOAK_CALL_THIS operator=(In FLOAT f);
				CLOAKENGINE_API void CLOAK_CALL_THIS operator=(In UINT f);
				CLOAKENGINE_API void CLOAK_CALL_THIS operator=(In INT f);

				union {
					FLOAT FloatVal;
					UINT UIntVal;
					INT IntVal;
				};
			};
			static_assert(sizeof(DWParam) == 4, "DWParam struct must have a size of exactly 32 bit!");
			constexpr CPU_DESCRIPTOR_HANDLE CPU_HANDLE_INVALID = CPU_DESCRIPTOR_HANDLE(CPU_ADDRESS_NULL);
			constexpr GPU_DESCRIPTOR_HANDLE GPU_HANDLE_INVALID = GPU_DESCRIPTOR_HANDLE(GPU_ADDRESS_UNKNOWN);
			constexpr uint32_t HANDLEINFO_INVALID_HEAP = 0;

			struct ResourceView {
				CPU_DESCRIPTOR_HANDLE CPU = CPU_HANDLE_INVALID;
				GPU_DESCRIPTOR_HANDLE GPU = GPU_HANDLE_INVALID;
				uint32_t HeapID = HANDLEINFO_INVALID_HEAP;
				uint32_t NodeMask = 0;
				uint32_t Position = 0;
				CLOAKENGINE_API bool CLOAK_CALL operator==(In const ResourceView& v) const { return CPU.ptr == v.CPU.ptr && GPU.ptr == v.GPU.ptr && HeapID == v.HeapID && NodeMask == v.NodeMask; }
				CLOAKENGINE_API bool CLOAK_CALL operator!=(In const ResourceView& v) const { return CPU.ptr != v.CPU.ptr || GPU.ptr != v.GPU.ptr || HeapID != v.HeapID || NodeMask != v.NodeMask; }
			};
			constexpr ResourceView InvalidResourceView = { CPU_HANDLE_INVALID, GPU_HANDLE_INVALID, HANDLEINFO_INVALID_HEAP, 0, 0 };

			//--------------------------------------------------EXECUTE INDIRECT------------------------------------------
			enum INDIRECT_ARGUMENT_TYPE {
				INDIRECT_ARGUMENT_TYPE_DRAW = 0,
				INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED = INDIRECT_ARGUMENT_TYPE_DRAW + 1,
				INDIRECT_ARGUMENT_TYPE_DISPATCH = INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED + 1,
				INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER = INDIRECT_ARGUMENT_TYPE_DISPATCH + 1,
				INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER = INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER + 1,
				INDIRECT_ARGUMENT_TYPE_CONSTANT = INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER + 1,
				INDIRECT_ARGUMENT_TYPE_CBV = INDIRECT_ARGUMENT_TYPE_CONSTANT + 1,
				INDIRECT_ARGUMENT_TYPE_SRV = INDIRECT_ARGUMENT_TYPE_CBV + 1,
				INDIRECT_ARGUMENT_TYPE_UAV = INDIRECT_ARGUMENT_TYPE_SRV + 1,
			};
			struct INDIRECT_ARGUMENT_DESC {
				INDIRECT_ARGUMENT_TYPE Type;
				union {
					struct {

					} Draw;
					struct {

					} DrawIndexed;
					struct {

					} Dispatch;
					struct {
						UINT Slot;
					} VertexBuffer;
					struct {

					} IndexBuffer;
					struct {
						UINT RootIndex;
						UINT Offset;
						UINT Num32BitConstants;
					} Constant;
					struct {
						UINT RootIndex;
					} CBV;
					struct {
						UINT RootIndex;
					} SRV;
					struct {
						UINT RootIndex;
					} UAV;
				};
			};
			struct INDIRECT_ARGUMENT_DRAW {
				UINT VertexCountPerInstance;
				UINT InstanceCount;
				UINT StartVertexOffset;
				UINT StartInstanceOffset;
			};
			struct INDIRECT_ARGUMENT_DRAW_INDEXED {
				UINT IndexCountPerInstance;
				UINT InstanceCount;
				UINT StartIndexOffset;
				INT StartVertexOffset;
				UINT StartInstanceOffset;
			};
			struct INDIRECT_ARGUMENT_DISPATCH {
				UINT GroupCountX;
				UINT GroupCountY;
				UINT GroupCountZ;
			};
			struct INDIRECT_ARGUMENT_VERTEX_BUFFER {
				GPU_ADDRESS Address;
				UINT SizeInBytes;
				UINT StrideInBytes;
			};
			struct INDIRECT_ARGUMENT_INDEX_BUFFER {
				GPU_ADDRESS Address;
				UINT SizeInBytes;
				Format Format;
			};
			typedef GPU_ADDRESS INDIRECT_ARGUMENT_CBV;
			typedef GPU_ADDRESS INDIRECT_ARGUMENT_SRV;
			typedef GPU_ADDRESS INDIRECT_ARGUMENT_UAV;
			typedef DWParam INDIRECT_ARGUMENT_CONSTANT;
		}
	}
}

namespace std {
	template<> struct hash<CE::Rendering::SHADER_MODEL> {
		inline size_t CLOAK_CALL operator()(In const CE::Rendering::SHADER_MODEL& o) const { return hash<uint32_t>()(static_cast<uint32_t>(o.Language)) ^ hash<uint32_t>()(o.FullVersion()); }
	};
}

#endif