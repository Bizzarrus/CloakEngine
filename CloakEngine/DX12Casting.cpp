#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/Casting.h" 

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace Casting {
					struct FormatCast {
						API::Rendering::Format Left;
						DXGI_FORMAT Right;
					};
					constexpr FormatCast FormatCasting[] = {
						{ API::Rendering::Format::UNKNOWN,DXGI_FORMAT_UNKNOWN },
						{ API::Rendering::Format::R32G32B32A32_TYPELESS,DXGI_FORMAT_R32G32B32A32_TYPELESS },
						{ API::Rendering::Format::R32G32B32A32_FLOAT,DXGI_FORMAT_R32G32B32A32_FLOAT },
						{ API::Rendering::Format::R32G32B32A32_UINT,DXGI_FORMAT_R32G32B32A32_UINT },
						{ API::Rendering::Format::R32G32B32A32_SINT,DXGI_FORMAT_R32G32B32A32_SINT },
						{ API::Rendering::Format::R32G32B32_TYPELESS,DXGI_FORMAT_R32G32B32_TYPELESS },
						{ API::Rendering::Format::R32G32B32_FLOAT,DXGI_FORMAT_R32G32B32_FLOAT },
						{ API::Rendering::Format::R32G32B32_UINT,DXGI_FORMAT_R32G32B32_UINT },
						{ API::Rendering::Format::R32G32B32_SINT,DXGI_FORMAT_R32G32B32_SINT },
						{ API::Rendering::Format::R16G16B16A16_TYPELESS,DXGI_FORMAT_R16G16B16A16_TYPELESS },
						{ API::Rendering::Format::R16G16B16A16_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT },
						{ API::Rendering::Format::R16G16B16A16_UNORM,DXGI_FORMAT_R16G16B16A16_UNORM },
						{ API::Rendering::Format::R16G16B16A16_UINT,DXGI_FORMAT_R16G16B16A16_UINT },
						{ API::Rendering::Format::R16G16B16A16_SNORM,DXGI_FORMAT_R16G16B16A16_SNORM },
						{ API::Rendering::Format::R16G16B16A16_SINT,DXGI_FORMAT_R16G16B16A16_SINT },
						{ API::Rendering::Format::R32G32_TYPELESS,DXGI_FORMAT_R32G32_TYPELESS },
						{ API::Rendering::Format::R32G32_FLOAT,DXGI_FORMAT_R32G32_FLOAT },
						{ API::Rendering::Format::R32G32_UINT,DXGI_FORMAT_R32G32_UINT },
						{ API::Rendering::Format::R32G32_SINT,DXGI_FORMAT_R32G32_SINT },
						{ API::Rendering::Format::R32G8X24_TYPELESS,DXGI_FORMAT_R32G8X24_TYPELESS },
						{ API::Rendering::Format::D32_FLOAT_S8X24_UINT,DXGI_FORMAT_D32_FLOAT_S8X24_UINT },
						{ API::Rendering::Format::R32_FLOAT_X8X24_TYPELESS,DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS },
						{ API::Rendering::Format::X32_TYPELESS_G8X24_UINT,DXGI_FORMAT_X32_TYPELESS_G8X24_UINT },
						{ API::Rendering::Format::R10G10B10A2_TYPELESS,DXGI_FORMAT_R10G10B10A2_TYPELESS },
						{ API::Rendering::Format::R10G10B10A2_UNORM,DXGI_FORMAT_R10G10B10A2_UNORM },
						{ API::Rendering::Format::R10G10B10A2_UINT,DXGI_FORMAT_R10G10B10A2_UINT },
						{ API::Rendering::Format::R11G11B10_FLOAT,DXGI_FORMAT_R11G11B10_FLOAT },
						{ API::Rendering::Format::R8G8B8A8_TYPELESS,DXGI_FORMAT_R8G8B8A8_TYPELESS },
						{ API::Rendering::Format::R8G8B8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM },
						{ API::Rendering::Format::R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },
						{ API::Rendering::Format::R8G8B8A8_UINT,DXGI_FORMAT_R8G8B8A8_UINT },
						{ API::Rendering::Format::R8G8B8A8_SNORM,DXGI_FORMAT_R8G8B8A8_SNORM },
						{ API::Rendering::Format::R8G8B8A8_SINT,DXGI_FORMAT_R8G8B8A8_SINT },
						{ API::Rendering::Format::R16G16_TYPELESS,DXGI_FORMAT_R16G16_TYPELESS },
						{ API::Rendering::Format::R16G16_FLOAT,DXGI_FORMAT_R16G16_FLOAT },
						{ API::Rendering::Format::R16G16_UNORM,DXGI_FORMAT_R16G16_UNORM },
						{ API::Rendering::Format::R16G16_UINT,DXGI_FORMAT_R16G16_UINT },
						{ API::Rendering::Format::R16G16_SNORM,DXGI_FORMAT_R16G16_SNORM },
						{ API::Rendering::Format::R16G16_SINT,DXGI_FORMAT_R16G16_SINT },
						{ API::Rendering::Format::R32_TYPELESS,DXGI_FORMAT_R32_TYPELESS },
						{ API::Rendering::Format::D32_FLOAT,DXGI_FORMAT_D32_FLOAT },
						{ API::Rendering::Format::R32_FLOAT,DXGI_FORMAT_R32_FLOAT },
						{ API::Rendering::Format::R32_UINT,DXGI_FORMAT_R32_UINT },
						{ API::Rendering::Format::R32_SINT,DXGI_FORMAT_R32_SINT },
						{ API::Rendering::Format::R24G8_TYPELESS,DXGI_FORMAT_R24G8_TYPELESS },
						{ API::Rendering::Format::D24_UNORM_S8_UINT,DXGI_FORMAT_D24_UNORM_S8_UINT },
						{ API::Rendering::Format::R24_UNORM_X8_TYPELESS,DXGI_FORMAT_R24_UNORM_X8_TYPELESS },
						{ API::Rendering::Format::X24_TYPELESS_G8_UINT,DXGI_FORMAT_X24_TYPELESS_G8_UINT },
						{ API::Rendering::Format::R8G8_TYPELESS,DXGI_FORMAT_R8G8_TYPELESS },
						{ API::Rendering::Format::R8G8_UNORM,DXGI_FORMAT_R8G8_UNORM },
						{ API::Rendering::Format::R8G8_UINT,DXGI_FORMAT_R8G8_UINT },
						{ API::Rendering::Format::R8G8_SNORM,DXGI_FORMAT_R8G8_SNORM },
						{ API::Rendering::Format::R8G8_SINT,DXGI_FORMAT_R8G8_SINT },
						{ API::Rendering::Format::R16_TYPELESS,DXGI_FORMAT_R16_TYPELESS },
						{ API::Rendering::Format::R16_FLOAT,DXGI_FORMAT_R16_FLOAT },
						{ API::Rendering::Format::D16_UNORM,DXGI_FORMAT_D16_UNORM },
						{ API::Rendering::Format::R16_UNORM,DXGI_FORMAT_R16_UNORM },
						{ API::Rendering::Format::R16_UINT,DXGI_FORMAT_R16_UINT },
						{ API::Rendering::Format::R16_SNORM,DXGI_FORMAT_R16_SNORM },
						{ API::Rendering::Format::R16_SINT,DXGI_FORMAT_R16_SINT },
						{ API::Rendering::Format::R8_TYPELESS,DXGI_FORMAT_R8_TYPELESS },
						{ API::Rendering::Format::R8_UNORM,DXGI_FORMAT_R8_UNORM },
						{ API::Rendering::Format::R8_UINT,DXGI_FORMAT_R8_UINT },
						{ API::Rendering::Format::R8_SNORM,DXGI_FORMAT_R8_SNORM },
						{ API::Rendering::Format::R8_SINT,DXGI_FORMAT_R8_SINT },
						{ API::Rendering::Format::A8_UNORM,DXGI_FORMAT_A8_UNORM },
						{ API::Rendering::Format::R1_UNORM,DXGI_FORMAT_R1_UNORM },
						{ API::Rendering::Format::R9G9B9E5_SHAREDEXP,DXGI_FORMAT_R9G9B9E5_SHAREDEXP },
						{ API::Rendering::Format::R8G8_B8G8_UNORM,DXGI_FORMAT_R8G8_B8G8_UNORM },
						{ API::Rendering::Format::G8R8_G8B8_UNORM,DXGI_FORMAT_G8R8_G8B8_UNORM },
						{ API::Rendering::Format::BC1_TYPELESS,DXGI_FORMAT_BC1_TYPELESS },
						{ API::Rendering::Format::BC1_UNORM,DXGI_FORMAT_BC1_UNORM },
						{ API::Rendering::Format::BC1_UNORM_SRGB,DXGI_FORMAT_BC1_UNORM_SRGB },
						{ API::Rendering::Format::BC2_TYPELESS,DXGI_FORMAT_BC2_TYPELESS },
						{ API::Rendering::Format::BC2_UNORM,DXGI_FORMAT_BC2_UNORM },
						{ API::Rendering::Format::BC2_UNORM_SRGB,DXGI_FORMAT_BC2_UNORM_SRGB },
						{ API::Rendering::Format::BC3_TYPELESS,DXGI_FORMAT_BC3_TYPELESS },
						{ API::Rendering::Format::BC3_UNORM,DXGI_FORMAT_BC3_UNORM },
						{ API::Rendering::Format::BC3_UNORM_SRGB,DXGI_FORMAT_BC3_UNORM_SRGB },
						{ API::Rendering::Format::BC4_TYPELESS,DXGI_FORMAT_BC4_TYPELESS },
						{ API::Rendering::Format::BC4_UNORM,DXGI_FORMAT_BC4_UNORM },
						{ API::Rendering::Format::BC4_SNORM,DXGI_FORMAT_BC4_SNORM },
						{ API::Rendering::Format::BC5_TYPELESS,DXGI_FORMAT_BC5_TYPELESS },
						{ API::Rendering::Format::BC5_UNORM,DXGI_FORMAT_BC5_UNORM },
						{ API::Rendering::Format::BC5_SNORM,DXGI_FORMAT_BC5_SNORM },
						{ API::Rendering::Format::B5G6R5_UNORM,DXGI_FORMAT_B5G6R5_UNORM },
						{ API::Rendering::Format::B5G5R5A1_UNORM,DXGI_FORMAT_B5G5R5A1_UNORM },
						{ API::Rendering::Format::B8G8R8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM },
						{ API::Rendering::Format::B8G8R8X8_UNORM,DXGI_FORMAT_B8G8R8X8_UNORM },
						{ API::Rendering::Format::R10G10B10_XR_BIAS_A2_UNORM,DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM },
						{ API::Rendering::Format::B8G8R8A8_TYPELESS,DXGI_FORMAT_B8G8R8A8_TYPELESS },
						{ API::Rendering::Format::B8G8R8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB },
						{ API::Rendering::Format::B8G8R8X8_TYPELESS,DXGI_FORMAT_B8G8R8X8_TYPELESS },
						{ API::Rendering::Format::B8G8R8X8_UNORM_SRGB,DXGI_FORMAT_B8G8R8X8_UNORM_SRGB },
						{ API::Rendering::Format::BC6H_TYPELESS,DXGI_FORMAT_BC6H_TYPELESS },
						{ API::Rendering::Format::BC6H_UF16,DXGI_FORMAT_BC6H_UF16 },
						{ API::Rendering::Format::BC6H_SF16,DXGI_FORMAT_BC6H_SF16 },
						{ API::Rendering::Format::BC7_TYPELESS,DXGI_FORMAT_BC7_TYPELESS },
						{ API::Rendering::Format::BC7_UNORM,DXGI_FORMAT_BC7_UNORM },
						{ API::Rendering::Format::BC7_UNORM_SRGB,DXGI_FORMAT_BC7_UNORM_SRGB },
						{ API::Rendering::Format::AYUV,DXGI_FORMAT_AYUV },
						{ API::Rendering::Format::Y410,DXGI_FORMAT_Y410 },
						{ API::Rendering::Format::Y416,DXGI_FORMAT_Y416 },
						{ API::Rendering::Format::NV12,DXGI_FORMAT_NV12 },
						{ API::Rendering::Format::P010,DXGI_FORMAT_P010 },
						{ API::Rendering::Format::P016,DXGI_FORMAT_P016 },
						{ API::Rendering::Format::YUY2,DXGI_FORMAT_YUY2 },
						{ API::Rendering::Format::Y210,DXGI_FORMAT_Y210 },
						{ API::Rendering::Format::Y216,DXGI_FORMAT_Y216 },
						{ API::Rendering::Format::NV11,DXGI_FORMAT_NV11 },
						{ API::Rendering::Format::AI44,DXGI_FORMAT_AI44 },
						{ API::Rendering::Format::IA44,DXGI_FORMAT_IA44 },
						{ API::Rendering::Format::P8,DXGI_FORMAT_P8 },
						{ API::Rendering::Format::A8P8,DXGI_FORMAT_A8P8 },
						{ API::Rendering::Format::B4G4R4A4_UNORM,DXGI_FORMAT_B4G4R4A4_UNORM },

						{ API::Rendering::Format::P208,DXGI_FORMAT_P208 },
						{ API::Rendering::Format::V208,DXGI_FORMAT_V208 },
						{ API::Rendering::Format::V408,DXGI_FORMAT_V408 },
						{API::Rendering::Format::FORCE_UINT,DXGI_FORMAT_FORCE_UINT},
					};

					template<typename I, typename O, typename S> O CLOAK_CALL CastForward(In I input, In O defVar, In_reads(arrSize) S arr[], In size_t arrSize)
					{
						for (size_t a = 0; a < arrSize; a++)
						{
							if (arr[a].Left == input) { return arr[a].Right; }
						}
						return defVar;
					}
					template<typename I, typename O, typename S> O CLOAK_CALL CastBackward(In I input, In O defVar, In_reads(arrSize) S arr[], In size_t arrSize)
					{
						for (size_t a = 0; a < arrSize && f == false; a++)
						{
							if (arr[a].Right == input) { return arr[a].Left; }
						}
						return defVar;
					}

					D3D12_CPU_DESCRIPTOR_HANDLE CLOAK_CALL CastForward(In const API::Rendering::CPU_DESCRIPTOR_HANDLE& api)
					{
						D3D12_CPU_DESCRIPTOR_HANDLE res;
						res.ptr = api.ptr;
						return res;
					}
					API::Rendering::CPU_DESCRIPTOR_HANDLE CLOAK_CALL CastBackward(In const D3D12_CPU_DESCRIPTOR_HANDLE& dx)
					{
						API::Rendering::CPU_DESCRIPTOR_HANDLE res(dx.ptr);
						return res;
					}

					D3D12_GPU_DESCRIPTOR_HANDLE CLOAK_CALL CastForward(In const API::Rendering::GPU_DESCRIPTOR_HANDLE& api)
					{
						D3D12_GPU_DESCRIPTOR_HANDLE res;
						res.ptr = api.ptr;
						return res;
					}
					API::Rendering::GPU_DESCRIPTOR_HANDLE CLOAK_CALL CastBackward(In const D3D12_GPU_DESCRIPTOR_HANDLE& dx)
					{
						API::Rendering::GPU_DESCRIPTOR_HANDLE res(dx.ptr);
						return res;
					}

					D3D12_VIEWPORT CLOAK_CALL CastForward(In const API::Rendering::VIEWPORT& api)
					{
						D3D12_VIEWPORT v;
						v.Height = api.Height;
						v.MaxDepth = api.MaxDepth;
						v.MinDepth = api.MinDepth;
						v.TopLeftX = api.TopLeftX;
						v.TopLeftY = api.TopLeftY;
						v.Width = api.Width;
						return v;
					}
					API::Rendering::VIEWPORT CLOAK_CALL CastBackward(In const D3D12_VIEWPORT& api)
					{
						API::Rendering::VIEWPORT v;
						v.Height = api.Height;
						v.MaxDepth = api.MaxDepth;
						v.MinDepth = api.MinDepth;
						v.TopLeftX = api.TopLeftX;
						v.TopLeftY = api.TopLeftY;
						v.Width = api.Width;
						return v;
					}
					D3D12_CONSTANT_BUFFER_VIEW_DESC CLOAK_CALL CastForward(In const Impl::Rendering::CBV_DESC& api)
					{
						D3D12_CONSTANT_BUFFER_VIEW_DESC res;
						res.BufferLocation = api.BufferLocation;
						res.SizeInBytes = api.SizeInBytes;
						return res;
					}
					D3D12_RENDER_TARGET_VIEW_DESC CLOAK_CALL CastForward(In const Impl::Rendering::RTV_DESC& api)
					{
						D3D12_RENDER_TARGET_VIEW_DESC res;
						res.Format = CastForward(api.Format);
						res.ViewDimension = CastForward(api.ViewDimension);
						switch (api.ViewDimension)
						{
							case RTV_DIMENSION_BUFFER:
								res.Buffer.FirstElement = api.Buffer.FirstElement;
								res.Buffer.NumElements = api.Buffer.NumElements;
								break;
							case RTV_DIMENSION_TEXTURE1D:
								res.Texture1D.MipSlice = api.Texture1D.MipSlice;
								break;
							case RTV_DIMENSION_TEXTURE1DARRAY:
								res.Texture1DArray.ArraySize = api.Texture1DArray.ArraySize;
								res.Texture1DArray.FirstArraySlice = api.Texture1DArray.FirstArraySlice;
								res.Texture1DArray.MipSlice = api.Texture1DArray.MipSlice;
								break;
							case RTV_DIMENSION_TEXTURE2D:
								res.Texture2D.MipSlice = api.Texture2D.MipSlice;
								res.Texture2D.PlaneSlice = api.Texture2D.PlaneSlice;
								break;
							case RTV_DIMENSION_TEXTURE2DARRAY:
								res.Texture2DArray.ArraySize = api.Texture2DArray.ArraySize;
								res.Texture2DArray.FirstArraySlice = api.Texture2DArray.FirstArraySlice;
								res.Texture2DArray.MipSlice = api.Texture2DArray.MipSlice;
								res.Texture2DArray.PlaneSlice = api.Texture2DArray.PlaneSlice;
								break;
							case RTV_DIMENSION_TEXTURE2DMS:
								break;
							case RTV_DIMENSION_TEXTURE2DMSARRAY:
								res.Texture2DMSArray.ArraySize = api.Texture2DMSArray.ArraySize;
								res.Texture2DMSArray.FirstArraySlice = api.Texture2DMSArray.FirstArraySlice;
								break;
							case RTV_DIMENSION_TEXTURE3D:
								res.Texture3D.FirstWSlice = api.Texture3D.FirstWSlice;
								res.Texture3D.MipSlice = api.Texture3D.MipSlice;
								res.Texture3D.WSize = api.Texture3D.WSize;
								break;
							case RTV_DIMENSION_UNKNOWN:
							default:
								break;
						}
						return res;
					}
					D3D12_SHADER_RESOURCE_VIEW_DESC CLOAK_CALL CastForward(In const Impl::Rendering::SRV_DESC& api)
					{
						D3D12_SHADER_RESOURCE_VIEW_DESC res;
						res.Format = CastForward(api.Format);
						res.ViewDimension = CastForward(api.ViewDimension);
						res.Shader4ComponentMapping = api.Shader4ComponentMapping;
						switch (api.ViewDimension)
						{
							case SRV_DIMENSION_BUFFER:
								res.Buffer.FirstElement = api.Buffer.FirstElement;
								res.Buffer.Flags = CastForward(api.Buffer.Flags);
								res.Buffer.NumElements = api.Buffer.NumElements;
								res.Buffer.StructureByteStride = api.Buffer.StructureByteStride;
								break;
							case SRV_DIMENSION_TEXTURE1D:
								res.Texture1D.MipLevels = api.Texture1D.MipLevels;
								res.Texture1D.MostDetailedMip = api.Texture1D.MostDetailedMip;
								res.Texture1D.ResourceMinLODClamp = api.Texture1D.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURE1DARRAY:
								res.Texture1DArray.ArraySize = api.Texture1DArray.ArraySize;
								res.Texture1DArray.FirstArraySlice = api.Texture1DArray.FirstArraySlice;
								res.Texture1DArray.MipLevels = api.Texture1DArray.MipLevels;
								res.Texture1DArray.MostDetailedMip = api.Texture1DArray.MostDetailedMip;
								res.Texture1DArray.ResourceMinLODClamp = api.Texture1DArray.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURE2D:
								res.Texture2D.MipLevels = api.Texture2D.MipLevels;
								res.Texture2D.MostDetailedMip = api.Texture2D.MostDetailedMip;
								res.Texture2D.PlaneSlice = api.Texture2D.PlaneSlice;
								res.Texture2D.ResourceMinLODClamp = api.Texture2D.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURE2DARRAY:
								res.Texture2DArray.ArraySize = api.Texture2DArray.ArraySize;
								res.Texture2DArray.FirstArraySlice = api.Texture2DArray.FirstArraySlice;
								res.Texture2DArray.MipLevels = api.Texture2DArray.MipLevels;
								res.Texture2DArray.MostDetailedMip = api.Texture2DArray.MostDetailedMip;
								res.Texture2DArray.PlaneSlice = api.Texture2DArray.PlaneSlice;
								res.Texture2DArray.ResourceMinLODClamp = api.Texture2DArray.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURE2DMS:
								break;
							case SRV_DIMENSION_TEXTURE2DMSARRAY:
								res.Texture2DMSArray.ArraySize = api.Texture2DMSArray.ArraySize;
								res.Texture2DMSArray.FirstArraySlice = api.Texture2DMSArray.FirstArraySlice;
								break;
							case SRV_DIMENSION_TEXTURE3D:
								res.Texture3D.MipLevels = api.Texture3D.MipLevels;
								res.Texture3D.MostDetailedMip = api.Texture3D.MostDetailedMip;
								res.Texture3D.ResourceMinLODClamp = api.Texture3D.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURECUBE:
								res.TextureCube.MipLevels = api.TextureCube.MipLevels;
								res.TextureCube.MostDetailedMip = api.TextureCube.MostDetailedMip;
								res.TextureCube.ResourceMinLODClamp = api.TextureCube.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURECUBEARRAY:
								res.TextureCubeArray.First2DArrayFace = api.TextureCubeArray.First2DArrayFace;
								res.TextureCubeArray.MipLevels = api.TextureCubeArray.MipLevels;
								res.TextureCubeArray.MostDetailedMip = api.TextureCubeArray.MostDetailedMip;
								res.TextureCubeArray.NumCubes = api.TextureCubeArray.NumCubes;
								res.TextureCubeArray.ResourceMinLODClamp = api.TextureCubeArray.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_UNKNOWN:
							default:
								break;
						}
						return res;
					}
					D3D12_DEPTH_STENCIL_VIEW_DESC CLOAK_CALL CastForward(In const Impl::Rendering::DSV_DESC& api)
					{
						D3D12_DEPTH_STENCIL_VIEW_DESC res;
						res.Flags = CastForward(api.Flags);
						res.Format = CastForward(api.Format);
						res.ViewDimension = CastForward(api.ViewDimension);
						switch (api.ViewDimension)
						{						
							case DSV_DIMENSION_TEXTURE1D:
								res.Texture1D.MipSlice = api.Texture1D.MipSlice;
								break;
							case DSV_DIMENSION_TEXTURE1DARRAY:
								res.Texture1DArray.ArraySize = api.Texture1DArray.ArraySize;
								res.Texture1DArray.FirstArraySlice = api.Texture1DArray.FirstArraySlice;
								res.Texture1DArray.MipSlice = api.Texture1DArray.MipSlice;
								break;
							case DSV_DIMENSION_TEXTURE2D:
								res.Texture2D.MipSlice = api.Texture2D.MipSlice;
								break;
							case DSV_DIMENSION_TEXTURE2DARRAY:
								res.Texture2DArray.ArraySize = api.Texture2DArray.ArraySize;
								res.Texture2DArray.FirstArraySlice = api.Texture2DArray.FirstArraySlice;
								res.Texture2DArray.MipSlice = api.Texture2DArray.MipSlice;
								break;
							case DSV_DIMENSION_TEXTURE2DMS:
								break;
							case DSV_DIMENSION_TEXTURE2DMSARRAY:
								res.Texture2DMSArray.ArraySize = api.Texture2DMSArray.ArraySize;
								res.Texture2DMSArray.FirstArraySlice = api.Texture2DMSArray.FirstArraySlice;
								break;
							case DSV_DIMENSION_UNKNOWN:
							default:
								break;
						}
						return res;
					}
					D3D12_UNORDERED_ACCESS_VIEW_DESC CLOAK_CALL CastForward(In const Impl::Rendering::UAV_DESC& api)
					{
						D3D12_UNORDERED_ACCESS_VIEW_DESC res;
						res.Format = CastForward(api.Format);
						res.ViewDimension = CastForward(api.ViewDimension);
						switch (api.ViewDimension)
						{
							case UAV_DIMENSION_BUFFER:
								res.Buffer.CounterOffsetInBytes = api.Buffer.CounterOffsetInBytes;
								res.Buffer.FirstElement = api.Buffer.FirstElement;
								res.Buffer.Flags = CastForward(api.Buffer.Flags);
								res.Buffer.NumElements = api.Buffer.NumElements;
								res.Buffer.StructureByteStride = api.Buffer.StructureByteStride;
								break;
							case UAV_DIMENSION_TEXTURE1D:
								res.Texture1D.MipSlice = api.Texture1D.MipSlice;
								break;
							case UAV_DIMENSION_TEXTURE1DARRAY:
								res.Texture1DArray.ArraySize = api.Texture1DArray.ArraySize;
								res.Texture1DArray.FirstArraySlice = api.Texture1DArray.FirstArraySlice;
								res.Texture1DArray.MipSlice = api.Texture1DArray.MipSlice;
								break;
							case UAV_DIMENSION_TEXTURE2D:
								res.Texture2D.MipSlice = api.Texture2D.MipSlice;
								res.Texture2D.PlaneSlice = api.Texture2D.PlaneSlice;
								break;
							case UAV_DIMENSION_TEXTURE2DARRAY:
								res.Texture2DArray.ArraySize = api.Texture2DArray.ArraySize;
								res.Texture2DArray.FirstArraySlice = api.Texture2DArray.FirstArraySlice;
								res.Texture2DArray.MipSlice = api.Texture2DArray.MipSlice;
								res.Texture2DArray.PlaneSlice = api.Texture2DArray.PlaneSlice;
								break;
							case UAV_DIMENSION_TEXTURE3D:
								res.Texture3D.FirstWSlice = api.Texture3D.FirstWSlice;
								res.Texture3D.MipSlice = api.Texture3D.MipSlice;
								res.Texture3D.WSize = api.Texture3D.WSize;
								break;
							case UAV_DIMENSION_UNKNOWN:
							default:
								break;
						}
						return res;
					}
					D3D11_RESOURCE_FLAGS CLOAK_CALL CastForward(In const Impl::Rendering::RESOURCE_FLAGS& api)
					{
						D3D11_RESOURCE_FLAGS res;
						res.BindFlags = api.BindFlags;
						res.CPUAccessFlags = api.CPUAccessFlags;
						res.MiscFlags = api.MiscFlags;
						res.StructureByteStride = api.StructureByteStride;
						return res;
					}

					Impl::Rendering::CBV_DESC CLOAK_CALL CastBackward(In const D3D12_CONSTANT_BUFFER_VIEW_DESC& api)
					{
						Impl::Rendering::CBV_DESC res;
						res.BufferLocation = api.BufferLocation;
						res.SizeInBytes = api.SizeInBytes;
						return res;
					}
					Impl::Rendering::RTV_DESC CLOAK_CALL CastBackward(In const D3D12_RENDER_TARGET_VIEW_DESC& api)
					{
						Impl::Rendering::RTV_DESC res;
						res.Format = CastBackward(api.Format);
						res.ViewDimension = CastBackward(api.ViewDimension);
						switch (res.ViewDimension)
						{
							case RTV_DIMENSION_BUFFER:
								res.Buffer.FirstElement = api.Buffer.FirstElement;
								res.Buffer.NumElements = api.Buffer.NumElements;
								break;
							case RTV_DIMENSION_TEXTURE1D:
								res.Texture1D.MipSlice = api.Texture1D.MipSlice;
								break;
							case RTV_DIMENSION_TEXTURE1DARRAY:
								res.Texture1DArray.ArraySize = api.Texture1DArray.ArraySize;
								res.Texture1DArray.FirstArraySlice = api.Texture1DArray.FirstArraySlice;
								res.Texture1DArray.MipSlice = api.Texture1DArray.MipSlice;
								break;
							case RTV_DIMENSION_TEXTURE2D:
								res.Texture2D.MipSlice = api.Texture2D.MipSlice;
								res.Texture2D.PlaneSlice = api.Texture2D.PlaneSlice;
								break;
							case RTV_DIMENSION_TEXTURE2DARRAY:
								res.Texture2DArray.ArraySize = api.Texture2DArray.ArraySize;
								res.Texture2DArray.FirstArraySlice = api.Texture2DArray.FirstArraySlice;
								res.Texture2DArray.MipSlice = api.Texture2DArray.MipSlice;
								res.Texture2DArray.PlaneSlice = api.Texture2DArray.PlaneSlice;
								break;
							case RTV_DIMENSION_TEXTURE2DMS:
								break;
							case RTV_DIMENSION_TEXTURE2DMSARRAY:
								res.Texture2DMSArray.ArraySize = api.Texture2DMSArray.ArraySize;
								res.Texture2DMSArray.FirstArraySlice = api.Texture2DMSArray.FirstArraySlice;
								break;
							case RTV_DIMENSION_TEXTURE3D:
								res.Texture3D.FirstWSlice = api.Texture3D.FirstWSlice;
								res.Texture3D.MipSlice = api.Texture3D.MipSlice;
								res.Texture3D.WSize = api.Texture3D.WSize;
								break;
							case RTV_DIMENSION_UNKNOWN:
							default:
								break;
						}
						return res;
					}
					Impl::Rendering::SRV_DESC CLOAK_CALL CastBackward(In const D3D12_SHADER_RESOURCE_VIEW_DESC& api)
					{
						Impl::Rendering::SRV_DESC res;
						res.Format = CastBackward(api.Format);
						res.ViewDimension = CastBackward(api.ViewDimension);
						res.Shader4ComponentMapping = api.Shader4ComponentMapping;
						switch (res.ViewDimension)
						{
							case SRV_DIMENSION_BUFFER:
								res.Buffer.FirstElement = api.Buffer.FirstElement;
								res.Buffer.Flags = CastBackward(api.Buffer.Flags);
								res.Buffer.NumElements = api.Buffer.NumElements;
								res.Buffer.StructureByteStride = api.Buffer.StructureByteStride;
								break;
							case SRV_DIMENSION_TEXTURE1D:
								res.Texture1D.MipLevels = api.Texture1D.MipLevels;
								res.Texture1D.MostDetailedMip = api.Texture1D.MostDetailedMip;
								res.Texture1D.ResourceMinLODClamp = api.Texture1D.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURE1DARRAY:
								res.Texture1DArray.ArraySize = api.Texture1DArray.ArraySize;
								res.Texture1DArray.FirstArraySlice = api.Texture1DArray.FirstArraySlice;
								res.Texture1DArray.MipLevels = api.Texture1DArray.MipLevels;
								res.Texture1DArray.MostDetailedMip = api.Texture1DArray.MostDetailedMip;
								res.Texture1DArray.ResourceMinLODClamp = api.Texture1DArray.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURE2D:
								res.Texture2D.MipLevels = api.Texture2D.MipLevels;
								res.Texture2D.MostDetailedMip = api.Texture2D.MostDetailedMip;
								res.Texture2D.PlaneSlice = api.Texture2D.PlaneSlice;
								res.Texture2D.ResourceMinLODClamp = api.Texture2D.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURE2DARRAY:
								res.Texture2DArray.ArraySize = api.Texture2DArray.ArraySize;
								res.Texture2DArray.FirstArraySlice = api.Texture2DArray.FirstArraySlice;
								res.Texture2DArray.MipLevels = api.Texture2DArray.MipLevels;
								res.Texture2DArray.MostDetailedMip = api.Texture2DArray.MostDetailedMip;
								res.Texture2DArray.PlaneSlice = api.Texture2DArray.PlaneSlice;
								res.Texture2DArray.ResourceMinLODClamp = api.Texture2DArray.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURE2DMS:
								break;
							case SRV_DIMENSION_TEXTURE2DMSARRAY:
								res.Texture2DMSArray.ArraySize = api.Texture2DMSArray.ArraySize;
								res.Texture2DMSArray.FirstArraySlice = api.Texture2DMSArray.FirstArraySlice;
								break;
							case SRV_DIMENSION_TEXTURE3D:
								res.Texture3D.MipLevels = api.Texture3D.MipLevels;
								res.Texture3D.MostDetailedMip = api.Texture3D.MostDetailedMip;
								res.Texture3D.ResourceMinLODClamp = api.Texture3D.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURECUBE:
								res.TextureCube.MipLevels = api.TextureCube.MipLevels;
								res.TextureCube.MostDetailedMip = api.TextureCube.MostDetailedMip;
								res.TextureCube.ResourceMinLODClamp = api.TextureCube.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_TEXTURECUBEARRAY:
								res.TextureCubeArray.First2DArrayFace = api.TextureCubeArray.First2DArrayFace;
								res.TextureCubeArray.MipLevels = api.TextureCubeArray.MipLevels;
								res.TextureCubeArray.MostDetailedMip = api.TextureCubeArray.MostDetailedMip;
								res.TextureCubeArray.NumCubes = api.TextureCubeArray.NumCubes;
								res.TextureCubeArray.ResourceMinLODClamp = api.TextureCubeArray.ResourceMinLODClamp;
								break;
							case SRV_DIMENSION_UNKNOWN:
							default:
								break;
						}
						return res;
					}
					Impl::Rendering::DSV_DESC CLOAK_CALL CastBackward(In const D3D12_DEPTH_STENCIL_VIEW_DESC& api)
					{
						Impl::Rendering::DSV_DESC res;
						res.Flags = CastBackward(api.Flags);
						res.Format = CastBackward(api.Format);
						res.ViewDimension = CastBackward(api.ViewDimension);
						switch (res.ViewDimension)
						{
							case DSV_DIMENSION_TEXTURE1D:
								res.Texture1D.MipSlice = api.Texture1D.MipSlice;
								break;
							case DSV_DIMENSION_TEXTURE1DARRAY:
								res.Texture1DArray.ArraySize = api.Texture1DArray.ArraySize;
								res.Texture1DArray.FirstArraySlice = api.Texture1DArray.FirstArraySlice;
								res.Texture1DArray.MipSlice = api.Texture1DArray.MipSlice;
								break;
							case DSV_DIMENSION_TEXTURE2D:
								res.Texture2D.MipSlice = api.Texture2D.MipSlice;
								break;
							case DSV_DIMENSION_TEXTURE2DARRAY:
								res.Texture2DArray.ArraySize = api.Texture2DArray.ArraySize;
								res.Texture2DArray.FirstArraySlice = api.Texture2DArray.FirstArraySlice;
								res.Texture2DArray.MipSlice = api.Texture2DArray.MipSlice;
								break;
							case DSV_DIMENSION_TEXTURE2DMS:
								break;
							case DSV_DIMENSION_TEXTURE2DMSARRAY:
								res.Texture2DMSArray.ArraySize = api.Texture2DMSArray.ArraySize;
								res.Texture2DMSArray.FirstArraySlice = api.Texture2DMSArray.FirstArraySlice;
								break;
							case DSV_DIMENSION_UNKNOWN:
							default:
								break;
						}
						return res;
					}
					Impl::Rendering::UAV_DESC CLOAK_CALL CastBackward(In const D3D12_UNORDERED_ACCESS_VIEW_DESC& api)
					{
						Impl::Rendering::UAV_DESC res;
						res.Format = CastBackward(api.Format);
						res.ViewDimension = CastBackward(api.ViewDimension);
						switch (api.ViewDimension)
						{
							case UAV_DIMENSION_BUFFER:
								res.Buffer.CounterOffsetInBytes = api.Buffer.CounterOffsetInBytes;
								res.Buffer.FirstElement = api.Buffer.FirstElement;
								res.Buffer.Flags = CastBackward(api.Buffer.Flags);
								res.Buffer.NumElements = api.Buffer.NumElements;
								res.Buffer.StructureByteStride = api.Buffer.StructureByteStride;
								break;
							case UAV_DIMENSION_TEXTURE1D:
								res.Texture1D.MipSlice = api.Texture1D.MipSlice;
								break;
							case UAV_DIMENSION_TEXTURE1DARRAY:
								res.Texture1DArray.ArraySize = api.Texture1DArray.ArraySize;
								res.Texture1DArray.FirstArraySlice = api.Texture1DArray.FirstArraySlice;
								res.Texture1DArray.MipSlice = api.Texture1DArray.MipSlice;
								break;
							case UAV_DIMENSION_TEXTURE2D:
								res.Texture2D.MipSlice = api.Texture2D.MipSlice;
								res.Texture2D.PlaneSlice = api.Texture2D.PlaneSlice;
								break;
							case UAV_DIMENSION_TEXTURE2DARRAY:
								res.Texture2DArray.ArraySize = api.Texture2DArray.ArraySize;
								res.Texture2DArray.FirstArraySlice = api.Texture2DArray.FirstArraySlice;
								res.Texture2DArray.MipSlice = api.Texture2DArray.MipSlice;
								res.Texture2DArray.PlaneSlice = api.Texture2DArray.PlaneSlice;
								break;
							case UAV_DIMENSION_TEXTURE3D:
								res.Texture3D.FirstWSlice = api.Texture3D.FirstWSlice;
								res.Texture3D.MipSlice = api.Texture3D.MipSlice;
								res.Texture3D.WSize = api.Texture3D.WSize;
								break;
							case UAV_DIMENSION_UNKNOWN:
							default:
								break;
						}
						return res;
					}
					Impl::Rendering::RESOURCE_FLAGS CLOAK_CALL CastBackward(In const D3D11_RESOURCE_FLAGS& api)
					{
						Impl::Rendering::RESOURCE_FLAGS res;
						res.BindFlags = api.BindFlags;
						res.CPUAccessFlags = api.CPUAccessFlags;
						res.MiscFlags = api.MiscFlags;
						res.StructureByteStride = api.StructureByteStride;
						return res;
					}
					D3D12_SUBRESOURCE_DATA CLOAK_CALL CastForward(In const API::Rendering::SUBRESOURCE_DATA& api)
					{
						D3D12_SUBRESOURCE_DATA res;
						res.pData = api.pData;
						res.RowPitch = api.RowPitch;
						res.SlicePitch = api.SlicePitch;
						return res;
					}
					API::Rendering::SUBRESOURCE_DATA CLOAK_CALL CastBackward(In const D3D12_SUBRESOURCE_DATA& api)
					{
						API::Rendering::SUBRESOURCE_DATA res;
						res.pData = api.pData;
						res.RowPitch = api.RowPitch;
						res.SlicePitch = api.SlicePitch;
						return res;
					}
					DXGI_MODE_DESC CLOAK_CALL CastForward(In const Impl::Rendering::SWAPCHAIN_MODE& api)
					{
						DXGI_MODE_DESC res;
						res.Format = CastForward(api.Format);
						res.Height = api.Height;
						res.RefreshRate.Denominator = api.RefreshRate.Denominator;
						res.RefreshRate.Numerator = api.RefreshRate.Numerator;
						res.Width = api.Width;
						res.ScanlineOrdering = CastForward(api.ScanlineOrdering);
						res.Scaling = CastForward(api.Scaling);
						return res;
					}
					Impl::Rendering::SWAPCHAIN_MODE CLOAK_CALL CastBackward(In const DXGI_MODE_DESC& api)
					{
						Impl::Rendering::SWAPCHAIN_MODE res;
						res.Format = CastBackward(api.Format);
						res.Height = api.Height;
						res.RefreshRate.Denominator = api.RefreshRate.Denominator;
						res.RefreshRate.Numerator = api.RefreshRate.Numerator;
						res.Width = api.Width;
						res.ScanlineOrdering = CastBackward(api.ScanlineOrdering);
						res.Scaling = CastBackward(api.Scaling);
						return res;
					}
					D3D12_SHADER_VISIBILITY CLOAK_CALL CastForward(In const API::Rendering::SHADER_VISIBILITY& api)
					{
						switch (api)
						{
							case API::Rendering::VISIBILITY_VERTEX: return D3D12_SHADER_VISIBILITY_VERTEX;
							case API::Rendering::VISIBILITY_DOMAIN: return D3D12_SHADER_VISIBILITY_DOMAIN;
							case API::Rendering::VISIBILITY_HULL: return D3D12_SHADER_VISIBILITY_HULL;
							case API::Rendering::VISIBILITY_GEOMETRY: return D3D12_SHADER_VISIBILITY_GEOMETRY;
							case API::Rendering::VISIBILITY_PIXEL: return D3D12_SHADER_VISIBILITY_PIXEL;
							default: return D3D12_SHADER_VISIBILITY_ALL;
						}
					}
					API::Rendering::SHADER_VISIBILITY CLOAK_CALL CastBackward(In const D3D12_SHADER_VISIBILITY& api)
					{
						switch (api)
						{
							case D3D12_SHADER_VISIBILITY_VERTEX: return API::Rendering::VISIBILITY_VERTEX;
							case D3D12_SHADER_VISIBILITY_DOMAIN: return API::Rendering::VISIBILITY_DOMAIN;
							case D3D12_SHADER_VISIBILITY_HULL: return API::Rendering::VISIBILITY_HULL;
							case D3D12_SHADER_VISIBILITY_GEOMETRY: return API::Rendering::VISIBILITY_GEOMETRY;
							case D3D12_SHADER_VISIBILITY_PIXEL: return API::Rendering::VISIBILITY_PIXEL;
							default: return API::Rendering::VISIBILITY_ALL;
						}
					}
					D3D12_VERTEX_BUFFER_VIEW CLOAK_CALL CastForward(In const API::Rendering::VERTEX_BUFFER_VIEW& api)
					{
						D3D12_VERTEX_BUFFER_VIEW res;
						res.BufferLocation = CastForward(api.Address);
						res.SizeInBytes = api.SizeInBytes;
						res.StrideInBytes = api.StrideInBytes;
						return res;
					}
					API::Rendering::VERTEX_BUFFER_VIEW CLOAK_CALL CastBackward(In const D3D12_VERTEX_BUFFER_VIEW& api)
					{
						API::Rendering::VERTEX_BUFFER_VIEW res;
						res.Address = CastBackward(api.BufferLocation);
						res.SizeInBytes = api.SizeInBytes;
						res.StrideInBytes = api.StrideInBytes;
						return res;
					}
					D3D12_INDEX_BUFFER_VIEW CLOAK_CALL CastForward(In const API::Rendering::INDEX_BUFFER_VIEW& api)
					{
						D3D12_INDEX_BUFFER_VIEW res;
						res.BufferLocation = CastForward(api.Address);
						res.Format = CastForward(api.Format);
						res.SizeInBytes = api.SizeInBytes;
						return res;
					}
					API::Rendering::INDEX_BUFFER_VIEW CLOAK_CALL CastBackward(In const D3D12_INDEX_BUFFER_VIEW& api)
					{
						API::Rendering::INDEX_BUFFER_VIEW res;
						res.Address = CastBackward(api.BufferLocation);
						res.Format = CastBackward(api.Format);
						res.SizeInBytes = api.SizeInBytes;
						return res;
					}
					D3D12_DESCRIPTOR_HEAP_TYPE CLOAK_CALL CastForward(In const Impl::Rendering::HEAP_TYPE& api)
					{
						switch (api)
						{
							case Impl::Rendering::HEAP_CBV_SRV_UAV_DYNAMIC:
							case Impl::Rendering::HEAP_CBV_SRV_UAV:
								return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
							case Impl::Rendering::HEAP_DSV:
								return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
							case Impl::Rendering::HEAP_RTV:
								return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
							case Impl::Rendering::HEAP_SAMPLER:
							case Impl::Rendering::HEAP_SAMPLER_DYNAMIC:
								return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
							default:
								break;
						}
						CLOAK_ASSUME(false);
						return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
					}
					Impl::Rendering::HEAP_TYPE CLOAK_CALL CastBackward(In const D3D12_DESCRIPTOR_HEAP_TYPE& api)
					{
						switch (api)
						{
							case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
								return Impl::Rendering::HEAP_CBV_SRV_UAV;
							case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
								return Impl::Rendering::HEAP_DSV;
							case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
								return Impl::Rendering::HEAP_RTV;
							case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
								return Impl::Rendering::HEAP_SAMPLER;
							default:
								break;
						}
						CLOAK_ASSUME(false);
						return Impl::Rendering::HEAP_NUM_TYPES;
					}

					D3D12_QUERY_TYPE CLOAK_CALL CastForward(In const API::Rendering::QUERY_TYPE& api)
					{
						switch (api)
						{
							case API::Rendering::QUERY_TYPE_OCCLUSION:
								return D3D12_QUERY_TYPE_BINARY_OCCLUSION;
							case API::Rendering::QUERY_TYPE_OCCLUSION_COUNT:
								return D3D12_QUERY_TYPE_OCCLUSION;
							case API::Rendering::QUERY_TYPE_PIPELINE_STATISTICS:
								return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
							case API::Rendering::QUERY_TYPE_STREAM_OUTPUT_ALL:
							case API::Rendering::QUERY_TYPE_STREAM_OUTPUT_0:
								return D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0;
							case API::Rendering::QUERY_TYPE_STREAM_OUTPUT_1:
								return D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1;
							case API::Rendering::QUERY_TYPE_STREAM_OUTPUT_2:
								return D3D12_QUERY_TYPE_SO_STATISTICS_STREAM2;
							case API::Rendering::QUERY_TYPE_STREAM_OUTPUT_3:
								return D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3;
							default:
								break;
						}
						CLOAK_ASSUME(false);
						return D3D12_QUERY_TYPE_BINARY_OCCLUSION;
					}
					API::Rendering::QUERY_TYPE CLOAK_CALL CastBackward(In const D3D12_QUERY_TYPE& api)
					{
						switch (api)
						{
							case D3D12_QUERY_TYPE_OCCLUSION:
								return API::Rendering::QUERY_TYPE_OCCLUSION_COUNT;
							case D3D12_QUERY_TYPE_BINARY_OCCLUSION:
								return API::Rendering::QUERY_TYPE_OCCLUSION;
							case D3D12_QUERY_TYPE_TIMESTAMP:
								return API::Rendering::QUERY_TYPE_TIMESTAMP;
							case D3D12_QUERY_TYPE_PIPELINE_STATISTICS:
								return API::Rendering::QUERY_TYPE_PIPELINE_STATISTICS;
							case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0:
								return API::Rendering::QUERY_TYPE_STREAM_OUTPUT_0;
							case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1:
								return API::Rendering::QUERY_TYPE_STREAM_OUTPUT_1;
							case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM2:
								return API::Rendering::QUERY_TYPE_STREAM_OUTPUT_2;
							case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3:
								return API::Rendering::QUERY_TYPE_STREAM_OUTPUT_3;
							case D3D12_QUERY_TYPE_VIDEO_DECODE_STATISTICS:
							default:
								break;
						}
						CLOAK_ASSUME(false);
						return API::Rendering::QUERY_TYPE_OCCLUSION;
					}
				}
			}
		}
	}
}
#endif