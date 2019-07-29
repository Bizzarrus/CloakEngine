#pragma once
#ifndef CE_IMPL_RENDERING_DEFINES_H
#define CE_IMPL_RENDERING_DEFINES_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Rendering/Defines.h"
#include "CloakEngine/Rendering/Allocator.h"

#define RENDERING_INTERFACE CLOAK_INTERFACE_BASIC
#define FLOAT32_MAX (3.402823466e+38f)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			template<typename T> struct Align {
				static CLOAK_FORCEINLINE T CLOAK_CALL AlignUpWithMask(In T val, In uint64_t mask) { return static_cast<T>((val + mask) & ~mask); }
				static CLOAK_FORCEINLINE T CLOAK_CALL AlignUp(In T val, In uint64_t align) { return AlignUpWithMask(val, align - 1); }
				static CLOAK_FORCEINLINE bool CLOAK_CALL IsAligned(In T val, In uint64_t align) { return (val & (align - 1)) == 0; }
				static CLOAK_FORCEINLINE T CLOAK_CALL DivideByMultiple(In T val, In uint64_t align) { return static_cast<T>((val + align - 1) / align); }
			};
			template<typename T> struct Align<T*> {
				static CLOAK_FORCEINLINE T* CLOAK_CALL AlignUpWithMask(In T* val, In uint64_t mask) { return reinterpret_cast<T*>(static_cast<uintptr_t>((reinterpret_cast<uintptr_t>(val) + mask) & ~mask)); }
				static CLOAK_FORCEINLINE T* CLOAK_CALL AlignUp(In T* val, In uint64_t align) { return AlignUpWithMask(val, align - 1); }
				static CLOAK_FORCEINLINE bool CLOAK_CALL IsAligned(In T* val, In uint64_t align) { return (reinterpret_cast<uintptr_t>(val) & (align - 1)) == 0; }
				static CLOAK_FORCEINLINE T* CLOAK_CALL DivideByMultiple(In T* val, In uint64_t align) { return reinterpret_cast<T*>(static_cast<uintptr_t>((reinterpret_cast<uintptr_t>(val) + align - 1) / align)); }
			};
			template<typename T, typename = enable_if_satisfied<decltype(is_pointer || is_integral), T>> CLOAK_FORCEINLINE T CLOAK_CALL AlignUpWithMask(In T val, In uint64_t mask) { return Align<T>::AlignUpWithMask(val, mask); }
			template<typename T, typename = enable_if_satisfied<decltype(is_pointer || is_integral), T>> CLOAK_FORCEINLINE T CLOAK_CALL AlignUp(In T val, In uint64_t align) { return Align<T>::AlignUp(val, align); }
			template<typename T, typename = enable_if_satisfied<decltype(is_pointer || is_integral), T>> CLOAK_FORCEINLINE bool CLOAK_CALL IsAligned(In T val, In uint64_t align) { return Align<T>::IsAligned(val, align); }
			template<typename T, typename = enable_if_satisfied<decltype(is_pointer || is_integral), T>> CLOAK_FORCEINLINE T CLOAK_CALL DivideByMultiple(In T val, In size_t align) { return Align<T>::DivideByMultiple(val, align); }

			constexpr CLOAK_FORCEINLINE size_t CLOAK_CALL GetFormatSize(In API::Rendering::Format f)
			{
				switch (f)
				{
					case API::Rendering::Format::R32G32B32A32_TYPELESS: return (32 + 32 + 32 + 32) >> 3;
					case API::Rendering::Format::R32G32B32A32_FLOAT: return (32 + 32 + 32 + 32) >> 3;
					case API::Rendering::Format::R32G32B32A32_UINT: return (32 + 32 + 32 + 32) >> 3;
					case API::Rendering::Format::R32G32B32A32_SINT: return (32 + 32 + 32 + 32) >> 3;
					case API::Rendering::Format::R32G32B32_TYPELESS: return (32 + 32 + 32) >> 3;
					case API::Rendering::Format::R32G32B32_FLOAT: return (32 + 32 + 32) >> 3;
					case API::Rendering::Format::R32G32B32_UINT: return (32 + 32 + 32) >> 3;
					case API::Rendering::Format::R32G32B32_SINT: return (32 + 32 + 32) >> 3;
					case API::Rendering::Format::R16G16B16A16_TYPELESS: return (16 + 16 + 16 + 16) >> 3;
					case API::Rendering::Format::R16G16B16A16_FLOAT: return (16 + 16 + 16 + 16) >> 3;
					case API::Rendering::Format::R16G16B16A16_UNORM: return (16 + 16 + 16 + 16) >> 3;
					case API::Rendering::Format::R16G16B16A16_UINT: return (16 + 16 + 16 + 16) >> 3;
					case API::Rendering::Format::R16G16B16A16_SNORM: return (16 + 16 + 16 + 16) >> 3;
					case API::Rendering::Format::R16G16B16A16_SINT: return (16 + 16 + 16 + 16) >> 3;
					case API::Rendering::Format::R32G32_TYPELESS: return (32 + 32) >> 3;
					case API::Rendering::Format::R32G32_FLOAT: return (32 + 32) >> 3;
					case API::Rendering::Format::R32G32_UINT: return (32 + 32) >> 3;
					case API::Rendering::Format::R32G32_SINT: return (32 + 32) >> 3;
					case API::Rendering::Format::R32G8X24_TYPELESS: return (32 + 8 + 24) >> 3;
					case API::Rendering::Format::D32_FLOAT_S8X24_UINT: return 32 >> 3;
					case API::Rendering::Format::R32_FLOAT_X8X24_TYPELESS: return 32 >> 3;
					case API::Rendering::Format::X32_TYPELESS_G8X24_UINT: return 32 >> 3;
					case API::Rendering::Format::R10G10B10A2_TYPELESS: return (10 + 10 + 10 + 2) >> 3;
					case API::Rendering::Format::R10G10B10A2_UNORM: return (10 + 10 + 10 + 2) >> 3;
					case API::Rendering::Format::R10G10B10A2_UINT: return (10 + 10 + 10 + 2) >> 3;
					case API::Rendering::Format::R11G11B10_FLOAT: return (11 + 11 + 10) >> 3;
					case API::Rendering::Format::R8G8B8A8_TYPELESS: return (8 + 8 + 8 + 8) >> 3;
					case API::Rendering::Format::R8G8B8A8_UNORM: return (8 + 8 + 8 + 8) >> 3;
					case API::Rendering::Format::R8G8B8A8_UNORM_SRGB: return (8 + 8 + 8 + 8) >> 3;
					case API::Rendering::Format::R8G8B8A8_UINT: return (8 + 8 + 8 + 8) >> 3;
					case API::Rendering::Format::R8G8B8A8_SNORM: return (8 + 8 + 8 + 8) >> 3;
					case API::Rendering::Format::R8G8B8A8_SINT: return (8 + 8 + 8 + 8) >> 3;
					case API::Rendering::Format::R16G16_TYPELESS: return (16 + 16) >> 3;
					case API::Rendering::Format::R16G16_FLOAT: return (16 + 16) >> 3;
					case API::Rendering::Format::R16G16_UNORM: return (16 + 16) >> 3;
					case API::Rendering::Format::R16G16_UINT: return (16 + 16) >> 3;
					case API::Rendering::Format::R16G16_SNORM: return (16 + 16) >> 3;
					case API::Rendering::Format::R16G16_SINT: return (16 + 16) >> 3;
					case API::Rendering::Format::R32_TYPELESS: return 32 >> 3;
					case API::Rendering::Format::D32_FLOAT: return 32 >> 3;
					case API::Rendering::Format::R32_FLOAT: return 32 >> 3;
					case API::Rendering::Format::R32_UINT: return 32 >> 3;
					case API::Rendering::Format::R32_SINT: return 32 >> 3;
					case API::Rendering::Format::R24G8_TYPELESS: return (24 + 8) >> 3;
					case API::Rendering::Format::D24_UNORM_S8_UINT: return 24 >> 3;
					case API::Rendering::Format::R24_UNORM_X8_TYPELESS: return 24 >> 3;
					case API::Rendering::Format::X24_TYPELESS_G8_UINT: return 24 >> 3;
					case API::Rendering::Format::R8G8_TYPELESS: return (8 + 8) >> 3;
					case API::Rendering::Format::R8G8_UNORM: return (8 + 8) >> 3;
					case API::Rendering::Format::R8G8_UINT: return (8 + 8) >> 3;
					case API::Rendering::Format::R8G8_SNORM: return (8 + 8) >> 3;
					case API::Rendering::Format::R8G8_SINT: return (8 + 8) >> 3;
					case API::Rendering::Format::R16_TYPELESS: return 16 >> 3;
					case API::Rendering::Format::R16_FLOAT: return 16 >> 3;
					case API::Rendering::Format::D16_UNORM: return 16 >> 3;
					case API::Rendering::Format::R16_UNORM: return 16 >> 3;
					case API::Rendering::Format::R16_UINT: return 16 >> 3;
					case API::Rendering::Format::R16_SNORM: return 16 >> 3;
					case API::Rendering::Format::R16_SINT: return 16 >> 3;
					case API::Rendering::Format::R8_TYPELESS: return 8 >> 3;
					case API::Rendering::Format::R8_UNORM: return 8 >> 3;
					case API::Rendering::Format::R8_UINT: return 8 >> 3;
					case API::Rendering::Format::R8_SNORM: return 8 >> 3;
					case API::Rendering::Format::R8_SINT: return 8 >> 3;
					case API::Rendering::Format::A8_UNORM: return 8 >> 3;
					case API::Rendering::Format::R1_UNORM: return 1 >> 3;
					case API::Rendering::Format::R9G9B9E5_SHAREDEXP: return (9 + 9 + 9 + 5) >> 3;
					case API::Rendering::Format::R8G8_B8G8_UNORM: return (8 + 8) >> 3;
					case API::Rendering::Format::G8R8_G8B8_UNORM: return (8 + 8) >> 3;
					case API::Rendering::Format::B5G6R5_UNORM: return (5 + 6 + 5) >> 3;
					case API::Rendering::Format::B5G5R5A1_UNORM: return (5 + 5 + 5 + 1) >> 3;
					case API::Rendering::Format::B8G8R8A8_UNORM: return (8 + 8 + 8 + 8) >> 3;
					case API::Rendering::Format::B8G8R8X8_UNORM: return (8 + 8 + 8 + 8) >> 3;
					case API::Rendering::Format::R10G10B10_XR_BIAS_A2_UNORM: return (10 + 10 + 10) >> 3;
					case API::Rendering::Format::B8G8R8A8_TYPELESS: return (8 + 8 + 8 + 8) >> 3;
					case API::Rendering::Format::B8G8R8A8_UNORM_SRGB: return (8 + 8 + 8 + 8) >> 3;
					case API::Rendering::Format::B8G8R8X8_TYPELESS: return (8 + 8 + 8 + 8) >> 3;
					case API::Rendering::Format::B8G8R8X8_UNORM_SRGB: return (8 + 8 + 8 + 8) >> 3;
					default: return 0;
				}
			}

			enum PIPELINE_STATE_TYPE {
				PIPELINE_STATE_TYPE_NONE,
				PIPELINE_STATE_TYPE_COMPUTE,
				PIPELINE_STATE_TYPE_GRAPHIC,
				PIPELINE_STATE_TYPE_RAY,
			};
			enum HEAP_TYPE {
				HEAP_CBV_SRV_UAV = 0,
				HEAP_SAMPLER = (HEAP_CBV_SRV_UAV+1),
				HEAP_RTV = (HEAP_SAMPLER+1),
				HEAP_DSV = (HEAP_RTV+1),
				HEAP_CBV_SRV_UAV_DYNAMIC = (HEAP_DSV+1),
				HEAP_SAMPLER_DYNAMIC = (HEAP_CBV_SRV_UAV_DYNAMIC+1),

				HEAP_NUM_TYPES = (HEAP_SAMPLER_DYNAMIC+1)
			};
			enum SRV_DIMENSION {
				SRV_DIMENSION_UNKNOWN = 0,
				SRV_DIMENSION_BUFFER = 1,
				SRV_DIMENSION_TEXTURE1D = 2,
				SRV_DIMENSION_TEXTURE1DARRAY = 3,
				SRV_DIMENSION_TEXTURE2D = 4,
				SRV_DIMENSION_TEXTURE2DARRAY = 5,
				SRV_DIMENSION_TEXTURE2DMS = 6,
				SRV_DIMENSION_TEXTURE2DMSARRAY = 7,
				SRV_DIMENSION_TEXTURE3D = 8,
				SRV_DIMENSION_TEXTURECUBE = 9,
				SRV_DIMENSION_TEXTURECUBEARRAY = 10
			};
			enum RTV_DIMENSION {
				RTV_DIMENSION_UNKNOWN = 0,
				RTV_DIMENSION_BUFFER = 1,
				RTV_DIMENSION_TEXTURE1D = 2,
				RTV_DIMENSION_TEXTURE1DARRAY = 3,
				RTV_DIMENSION_TEXTURE2D = 4,
				RTV_DIMENSION_TEXTURE2DARRAY = 5,
				RTV_DIMENSION_TEXTURE2DMS = 6,
				RTV_DIMENSION_TEXTURE2DMSARRAY = 7,
				RTV_DIMENSION_TEXTURE3D = 8
			};
			enum UAV_DIMENSION {
				UAV_DIMENSION_UNKNOWN = 0,
				UAV_DIMENSION_BUFFER = 1,
				UAV_DIMENSION_TEXTURE1D = 2,
				UAV_DIMENSION_TEXTURE1DARRAY = 3,
				UAV_DIMENSION_TEXTURE2D = 4,
				UAV_DIMENSION_TEXTURE2DARRAY = 5,
				UAV_DIMENSION_TEXTURE3D = 8
			};
			enum DSV_DIMENSION {
				DSV_DIMENSION_UNKNOWN = 0,
				DSV_DIMENSION_TEXTURE1D = 1,
				DSV_DIMENSION_TEXTURE1DARRAY = 2,
				DSV_DIMENSION_TEXTURE2D = 3,
				DSV_DIMENSION_TEXTURE2DARRAY = 4,
				DSV_DIMENSION_TEXTURE2DMS = 5,
				DSV_DIMENSION_TEXTURE2DMSARRAY = 6
			};
			enum SRV_FLAG {
				SRV_FLAG_NONE = 0,
				SRV_FLAG_RAW = 0x1
			};
			DEFINE_ENUM_FLAG_OPERATORS(SRV_FLAG);
			enum UAV_FLAG {
				UAV_FLAG_NONE = 0,
				UAV_FLAG_RAW = 0x1
			};
			DEFINE_ENUM_FLAG_OPERATORS(UAV_FLAG);
			enum DSV_FLAG {
				DSV_FLAG_NONE = 0,
				DSV_FLAG_READ_ONLY_DEPTH = 0x1,
				DSV_FLAG_READ_ONLY_STENCIL = 0x2
			};
			DEFINE_ENUM_FLAG_OPERATORS(DSV_FLAG);
			enum PRESENT_FLAG {
				PRESENT_WAIT = 0,
				PRESENT_NO_WAIT = 0x1,
				PRESENT_TEST = 0x2
			};
			DEFINE_ENUM_FLAG_OPERATORS(PRESENT_FLAG);
			enum class SWAP_CHAIN_RESULT {
				OK,
				FAILED,
				NEED_RETRY,
				OCCLUDED,
			};
			
			enum SCANLINE_ORDER {
				ORDER_UNSPECIFIED = 0,
				ORDER_PROGRESSIVE = 1,
				ORDER_UPPER_FIELD_FIRST = 2,
				ORDER_LOWER_FIELD_FIRST = 3
			};
			enum SCALING {
				SCALING_UNSPECIFIED = 0,
				SCALING_CENTERED = 1,
				SCALING_STRETCHED = 2
			};
			enum QUEUE_TYPE {
				QUEUE_TYPE_DIRECT = 0,
				QUEUE_TYPE_COPY = 1,
			};
			enum class TextureAllocType { MEMORY, ALLOCATOR };
			enum class TextureType { TEXTURE, TEXTURE_ARRAY, TEXTURE_CUBEMAP, SWAPCHAIN };

			struct DYN_SAMPLER_INFO {
				API::Rendering::SAMPLER_DESC Desc;
				UINT rootIndex;
			};
			struct CBV_DESC {
				API::Rendering::GPU_ADDRESS BufferLocation;
				UINT SizeInBytes;
			};
			struct SRV_DESC {
				API::Rendering::Format Format;
				SRV_DIMENSION ViewDimension;
				UINT Shader4ComponentMapping;
				union {
					struct {
						UINT64 FirstElement;
						UINT NumElements;
						UINT StructureByteStride;
						SRV_FLAG Flags;
					} Buffer;
					struct {
						UINT MostDetailedMip;
						UINT MipLevels;
						FLOAT ResourceMinLODClamp;
					} Texture1D;
					struct {
						UINT MostDetailedMip;
						UINT MipLevels;
						UINT FirstArraySlice;
						UINT ArraySize;
						FLOAT ResourceMinLODClamp;
					} Texture1DArray;
					struct {
						UINT MostDetailedMip;
						UINT MipLevels;
						UINT PlaneSlice;
						FLOAT ResourceMinLODClamp;
					} Texture2D;
					struct {
						UINT MostDetailedMip;
						UINT MipLevels;
						UINT FirstArraySlice;
						UINT ArraySize;
						UINT PlaneSlice;
						FLOAT ResourceMinLODClamp;
					} Texture2DArray;
					struct {

					} Texture2DMS;
					struct {
						UINT FirstArraySlice;
						UINT ArraySize;
					} Texture2DMSArray;
					struct {
						UINT MostDetailedMip;
						UINT MipLevels;
						FLOAT ResourceMinLODClamp;
					} Texture3D;
					struct {
						UINT MostDetailedMip;
						UINT MipLevels;
						FLOAT ResourceMinLODClamp;
					} TextureCube;
					struct {
						UINT MostDetailedMip;
						UINT MipLevels;
						UINT First2DArrayFace;
						UINT NumCubes;
						FLOAT ResourceMinLODClamp;
					} TextureCubeArray;
				};
			};
			struct RTV_DESC {
				API::Rendering::Format Format;
				RTV_DIMENSION ViewDimension;
				union {
					struct {
						UINT64 FirstElement;
						UINT NumElements;
					} Buffer;
					struct {
						UINT MipSlice;
					} Texture1D;
					struct {
						UINT MipSlice;
						UINT FirstArraySlice;
						UINT ArraySize;
					} Texture1DArray;
					struct {
						UINT MipSlice;
						UINT PlaneSlice;
					} Texture2D;
					struct {
						UINT MipSlice;
						UINT FirstArraySlice;
						UINT ArraySize;
						UINT PlaneSlice;
					} Texture2DArray;
					struct {

					} Texture2DMS;
					struct {
						UINT FirstArraySlice;
						UINT ArraySize;
					} Texture2DMSArray;
					struct {
						UINT MipSlice;
						UINT FirstWSlice;
						UINT WSize;
					} Texture3D;
				};
			};
			struct UAV_DESC {
				API::Rendering::Format Format;
				UAV_DIMENSION ViewDimension;
				union {
					struct {
						UINT64 FirstElement;
						UINT NumElements;
						UINT StructureByteStride;
						UINT64 CounterOffsetInBytes;
						UAV_FLAG Flags;
					} Buffer;
					struct {
						UINT MipSlice;
					} Texture1D;
					struct {
						UINT MipSlice;
						UINT FirstArraySlice;
						UINT ArraySize;
					} Texture1DArray;
					struct {
						UINT MipSlice;
						UINT PlaneSlice;
					} Texture2D;
					struct {
						UINT MipSlice;
						UINT FirstArraySlice;
						UINT ArraySize;
						UINT PlaneSlice;
					} Texture2DArray;
					struct {
						UINT MipSlice;
						UINT FirstWSlice;
						UINT WSize;
					} Texture3D;
				};
			};
			struct DSV_DESC {
				API::Rendering::Format Format;
				DSV_DIMENSION ViewDimension;
				DSV_FLAG Flags;
				union {
					struct {
						UINT MipSlice;
					} Texture1D;
					struct {
						UINT MipSlice;
						UINT FirstArraySlice;
						UINT ArraySize;
					} Texture1DArray;
					struct {
						UINT MipSlice;
					} Texture2D;
					struct {
						UINT MipSlice;
						UINT FirstArraySlice;
						UINT ArraySize;
					} Texture2DArray;
					struct {

					} Texture2DMS;
					struct {
						UINT FirstArraySlice;
						UINT ArraySize;
					} Texture2DMSArray;
				};
			};
			struct RESOURCE_FLAGS {
				UINT BindFlags;
				UINT MiscFlags;
				UINT CPUAccessFlags;
				UINT StructureByteStride;
			};
			struct GPU_MEMORY {
				TextureAllocType AllocType = TextureAllocType::MEMORY;
				union {
					API::Rendering::GPU_ADDRESS VideoMemory = GPU_ADDRESS_UNKNOWN;
					API::Rendering::IAllocator* Alloc;
				};
			};
			struct SWAPCHAIN_MODE {
				UINT Width;
				UINT Height;
				API::Rendering::Format Format;
				struct {
					UINT Numerator;
					UINT Denominator;
				} RefreshRate;
				SCANLINE_ORDER ScanlineOrdering;
				SCALING Scaling;
			};
			
			struct DRAW3D_COMMAND {
				float AABB[4];
				float Depth;
				API::Rendering::INDIRECT_ARGUMENT_CONSTANT UseTessellation;
				API::Rendering::INDIRECT_ARGUMENT_SRV Material;
				API::Rendering::INDIRECT_ARGUMENT_SRV Displacement;
				API::Rendering::INDIRECT_ARGUMENT_CBV LocalBuffer;
				API::Rendering::INDIRECT_ARGUMENT_CBV VertexBufferPossition;
				API::Rendering::INDIRECT_ARGUMENT_INDEX_BUFFER IndexBuffer;
				API::Rendering::INDIRECT_ARGUMENT_VERTEX_BUFFER VertexBuffer[2];
				API::Rendering::INDIRECT_ARGUMENT_DRAW Draw;
			};
			struct DRAW3D_INDEXED_COMMAND {
				float AABB[4];
				float Depth;
				API::Rendering::INDIRECT_ARGUMENT_CONSTANT UseTessellation;
				API::Rendering::INDIRECT_ARGUMENT_SRV Material;
				API::Rendering::INDIRECT_ARGUMENT_SRV Displacement;
				API::Rendering::INDIRECT_ARGUMENT_CBV LocalBuffer;
				API::Rendering::INDIRECT_ARGUMENT_CBV VertexBufferPossition;
				API::Rendering::INDIRECT_ARGUMENT_INDEX_BUFFER IndexBuffer;
				API::Rendering::INDIRECT_ARGUMENT_VERTEX_BUFFER VertexBuffer[2];
				API::Rendering::INDIRECT_ARGUMENT_DRAW_INDEXED Draw;
			};

			namespace InputElementDescription {
				constexpr API::Rendering::INPUT_ELEMENT_DESC MESH[] = {
					{ "POSITION",	0, FORMAT(4,32,FLOAT),	0,	0, API::Rendering::PER_VERTEX_DATA,	0 },
					{ "NORMAL",		0, FORMAT(4,32,FLOAT),	0,	1, API::Rendering::PER_VERTEX_DATA,	0 },
					{ "BINORMAL",	0, FORMAT(4,32,FLOAT),	0,	1, API::Rendering::PER_VERTEX_DATA,	0 },
					{ "TANGENT",	0, FORMAT(4,32,FLOAT),	0,	1, API::Rendering::PER_VERTEX_DATA,	0 },
					{ "BONEWEIGHT", 0, FORMAT(4,32,FLOAT),	0,	1, API::Rendering::PER_VERTEX_DATA,	0 },
					{ "BONEID",		0, FORMAT(4,32,UINT),	0,	1, API::Rendering::PER_VERTEX_DATA,	0 },
					{ "TEXCOORD",	0, FORMAT(2,32,FLOAT),	0,	1, API::Rendering::PER_VERTEX_DATA,	0 },
				};
				constexpr API::Rendering::INPUT_ELEMENT_DESC MESH_POS[] = {
					{ "POSITION",	0, FORMAT(4,32,FLOAT),	0,	0, API::Rendering::PER_VERTEX_DATA,	0 },
				};
				constexpr API::Rendering::INPUT_ELEMENT_DESC GUI[] = {
					{ "POSITION",	0, FORMAT(2,32,FLOAT),	0,	0, API::Rendering::PER_INSTANCE_DATA,	1 },
					{ "SIZE",		0, FORMAT(2,32,FLOAT),	0,	0, API::Rendering::PER_INSTANCE_DATA,	1 },
					{ "TEXTL",		0, FORMAT(2,32,FLOAT),	0,	0, API::Rendering::PER_INSTANCE_DATA,	1 },
					{ "TEXBR",		0, FORMAT(2,32,FLOAT),	0,	0, API::Rendering::PER_INSTANCE_DATA,	1 },
					{ "COLORR",		0, FORMAT(4,32,FLOAT),	0,	0, API::Rendering::PER_INSTANCE_DATA,	1 },
					{ "COLORG",		0, FORMAT(4,32,FLOAT),	0,	0, API::Rendering::PER_INSTANCE_DATA,	1 },
					{ "COLORB",		0, FORMAT(4,32,FLOAT),	0,	0, API::Rendering::PER_INSTANCE_DATA,	1 },
					{ "SCISSOR",	0, FORMAT(4,32,FLOAT),	0,	0, API::Rendering::PER_INSTANCE_DATA,	1 },
					{ "ROTATE",		0, FORMAT(2,32,FLOAT),	0,	0, API::Rendering::PER_INSTANCE_DATA,	1 },
					{ "FLAGS",		0, FORMAT(1,32,UINT),	0,	0, API::Rendering::PER_INSTANCE_DATA,	1 },
				};
				/*
				Final Idea for terrain rendering:
				Each tile is seperated in patches (8x8 / 4x4 / 2x2 / 1x1), depending on distance. These patches are created in the
				vertex shader, displaced and culled by the hull shader (frustrum culling and height culling to remove patches outside 
				a tiles 32x32x32 unit box). Adjacency flags in the vertex data are used to identify, if a neightbor tile uses a 
				different (either half or double) amount of patches per side, in which case the smaller patches (the patches of the 
				tile with more patches) are placed as if they were larger, so that the edges corelate.
					At this point, we have a crackless un-tessellated terrain, that also support multiple layers of hight maps,
					wholes and caves. The steepness of terrain is limited however, since the displacement only happens in one 
					direction. Also, the capability of tight overhangs is limited to tile borders at best, or not possible at all.
					For these kind of terrain, other algorithmes, such as vector field or specificly created meshes, are still
					needed.
				Tesslation factors are calculated according to the distance to the camera. The edges use their mid-point (or
				one of the end points if it's adjacent to a larger tile) to calculate the distance, the inside the mid-point of
				the patch. The tesslation factors are then multiplied 8 divided by the number of patches per tile *side* (wich is
				either 8, 4, 2, or 1), so that the screen size of each triangle should stay pretty constant - moreover, the tesslation
				factors of an edge that's shared between patches of different sizes should now be in such a way that each triangle
				edge after tesslation should be at the exact same position. The tesslated triangles are then displaced and rendered
				acordingly, without any further culling.
					This might result in sub-optimal culling and the exact way of how to decide what patches are culled by the hull
					shader is a bit difficult, but not culling the tesslated geometry prevents cracks in the terrain and allows
					diver optimizations, such as early depth culling. 

				Texturing of tiles:
				The height map of the overall terrain is saved as tiled resource, with only the relevant (acutally rendered) tiles
				set (Need to think of an implementation without tiled texture support!!!). Tiles with fewer patches might use mip
				maps with lower resolution. 
				Actual texturing requires full PBR materials. Multiple materials are blended together, based on artistic given blend 
				factors and slope. The used texture indices are given on a per-tile basis and point into synchronous mip-mapped textur 
				atlases (One atlas for albedo, one for  surface normals, etc. All atlases have the coresponding information of the same 
				material at the same index). 5 Blend factors are given with another tiled texture (4 color channel + 1 implicit factor,
				which is 1 minus the sum of the 4 channels). Slope blend factors (calculated during rendering based on actual geometry
				slope) might be used as well, to blend to yet another material. (Slope calculation requires calculated surface normals 
				per vertex based on adjacent geometry, to allow smooth slope transitions instead of constant slope values per triangle)
				*/
				constexpr API::Rendering::INPUT_ELEMENT_DESC TERRAIN[] = {
					{ "POSITION",	0, FORMAT(4,32,FLOAT),	0,	0, API::Rendering::PER_INSTANCE_DATA, 1 },
					{ "UV",			0, FORMAT(4,32,FLOAT),	0,	0, API::Rendering::PER_INSTANCE_DATA, 1 },
					{ "ADJACENCY",	0, FORMAT(1,32,UINT),	0,	0, API::Rendering::PER_INSTANCE_DATA, 1 },
					{ "TEXTUREID",	0, FORMAT(1,32,UINT),	0,	0, API::Rendering::PER_INSTANCE_DATA, 1 },
				};
			}
		}
	}
}

#endif