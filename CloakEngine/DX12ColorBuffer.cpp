#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/ColorBuffer.h"
#include "Implementation/Rendering/DX12/Casting.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/DX12/SwapChain.h"
#include "Implementation/Rendering/PipelineState.h"
#include "Implementation/Rendering/Manager.h"

#include "Engine/Graphic/Core.h"

#include <dxgi1_5.h>

#define TEXTURE_VIEW_TYPES (API::Rendering::VIEW_TYPE::SRV | API::Rendering::VIEW_TYPE::CUBE | API::Rendering::VIEW_TYPE::RTV | API::Rendering::VIEW_TYPE::UAV | API::Rendering::VIEW_TYPE::SRV_DYNAMIC | API::Rendering::VIEW_TYPE::UAV_DYNAMIC | API::Rendering::VIEW_TYPE::CUBE_DYNAMIC)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace ColorBuffer_v1 {
					inline DXGI_FORMAT CLOAK_CALL GetBaseFormat(In DXGI_FORMAT format)
					{
						switch (format)
						{
							case DXGI_FORMAT_R8G8B8A8_TYPELESS:
							case DXGI_FORMAT_R8G8B8A8_UNORM:
							case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
							case DXGI_FORMAT_R8G8B8A8_UINT:
							case DXGI_FORMAT_R8G8B8A8_SNORM:
							case DXGI_FORMAT_R8G8B8A8_SINT:
								return DXGI_FORMAT_R8G8B8A8_TYPELESS;

							case DXGI_FORMAT_B8G8R8A8_TYPELESS:
							case DXGI_FORMAT_B8G8R8A8_UNORM:
							case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
								return DXGI_FORMAT_B8G8R8A8_TYPELESS;

							case DXGI_FORMAT_B8G8R8X8_TYPELESS:
							case DXGI_FORMAT_B8G8R8X8_UNORM:
							case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
								return DXGI_FORMAT_B8G8R8X8_TYPELESS;

							case DXGI_FORMAT_R32G8X24_TYPELESS:
							case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
							case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
							case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
								return DXGI_FORMAT_R32G8X24_TYPELESS;

							case DXGI_FORMAT_R32_TYPELESS:
							case DXGI_FORMAT_R32_FLOAT:
							case DXGI_FORMAT_D32_FLOAT:
								return DXGI_FORMAT_R32_TYPELESS;

							case DXGI_FORMAT_R24G8_TYPELESS:
							case DXGI_FORMAT_D24_UNORM_S8_UINT:
							case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
							case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
								return DXGI_FORMAT_R24G8_TYPELESS;

							case DXGI_FORMAT_R16_TYPELESS:
							case DXGI_FORMAT_R16_UNORM:
							case DXGI_FORMAT_D16_UNORM:
								return DXGI_FORMAT_R16_TYPELESS;

							case DXGI_FORMAT_R16G16_TYPELESS:
							case DXGI_FORMAT_R16G16_FLOAT:
							case DXGI_FORMAT_R16G16_UNORM:
							case DXGI_FORMAT_R16G16_UINT:
							case DXGI_FORMAT_R16G16_SNORM:
							case DXGI_FORMAT_R16G16_SINT:
								return DXGI_FORMAT_R16G16_TYPELESS;

							case DXGI_FORMAT_R16G16B16A16_TYPELESS:
							case DXGI_FORMAT_R16G16B16A16_FLOAT:
							case DXGI_FORMAT_R16G16B16A16_SINT:
							case DXGI_FORMAT_R16G16B16A16_SNORM:
							case DXGI_FORMAT_R16G16B16A16_UINT:
							case DXGI_FORMAT_R16G16B16A16_UNORM:
								return DXGI_FORMAT_R16G16B16A16_TYPELESS;

							case DXGI_FORMAT_R32G32_TYPELESS:
							case DXGI_FORMAT_R32G32_FLOAT:
							case DXGI_FORMAT_R32G32_UINT:
							case DXGI_FORMAT_R32G32_SINT:
								return DXGI_FORMAT_R32G32_TYPELESS;

							case DXGI_FORMAT_R32G32B32A32_TYPELESS:
							case DXGI_FORMAT_R32G32B32A32_FLOAT:
							case DXGI_FORMAT_R32G32B32A32_SINT:
							case DXGI_FORMAT_R32G32B32A32_UINT:
								return DXGI_FORMAT_R32G32B32A32_TYPELESS;

							default:
								return format;
						}
					}
					inline DXGI_FORMAT CLOAK_CALL GetClearFormat(In DXGI_FORMAT format)
					{
						switch (format)
						{
							case DXGI_FORMAT_R8G8B8A8_TYPELESS:
								return DXGI_FORMAT_R8G8B8A8_UNORM;

							case DXGI_FORMAT_B8G8R8A8_TYPELESS:
								return DXGI_FORMAT_B8G8R8A8_UNORM;

							case DXGI_FORMAT_B8G8R8X8_TYPELESS:
								return DXGI_FORMAT_B8G8R8X8_UNORM;

							case DXGI_FORMAT_R32G8X24_TYPELESS:
							case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
							case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
								return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

							case DXGI_FORMAT_R32_TYPELESS:
								return DXGI_FORMAT_R32_FLOAT;

							case DXGI_FORMAT_R24G8_TYPELESS:
							case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
							case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
								return DXGI_FORMAT_D24_UNORM_S8_UINT;

							case DXGI_FORMAT_R16_TYPELESS:
								return DXGI_FORMAT_R16_UNORM;

							case DXGI_FORMAT_R16G16_TYPELESS:
								return DXGI_FORMAT_R16G16_UNORM;

							default:
								return format;
						}
					}
					inline DXGI_FORMAT CLOAK_CALL GetUAVFormat(In DXGI_FORMAT format)
					{
						switch (format)
						{
							case DXGI_FORMAT_R8G8B8A8_TYPELESS:
							case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
								return DXGI_FORMAT_R8G8B8A8_UNORM;

							case DXGI_FORMAT_B8G8R8A8_TYPELESS:
							case DXGI_FORMAT_B8G8R8A8_UNORM:
							case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:

							case DXGI_FORMAT_B8G8R8X8_TYPELESS:
							case DXGI_FORMAT_B8G8R8X8_UNORM:
							case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:

							case DXGI_FORMAT_R32_TYPELESS:
							case DXGI_FORMAT_D32_FLOAT:

							case DXGI_FORMAT_R24G8_TYPELESS:
							case DXGI_FORMAT_D24_UNORM_S8_UINT:
							case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
							case DXGI_FORMAT_X24_TYPELESS_G8_UINT:

							case DXGI_FORMAT_R16G16_TYPELESS:
							case DXGI_FORMAT_R16G16_FLOAT:
							case DXGI_FORMAT_R16G16_UNORM:
							case DXGI_FORMAT_R16G16_UINT:
							case DXGI_FORMAT_R16G16_SNORM:
							case DXGI_FORMAT_R16G16_SINT:
								return DXGI_FORMAT_R32_UINT;

							default:
								return format;
						}
					}
					inline D3D12_RESOURCE_DESC CLOAK_CALL CreateResourceDesc(In uint32_t W, In uint32_t H, In uint32_t DepthOrArraySize, In uint32_t numMips, In UINT sampleCount, In DXGI_FORMAT format, In D3D12_RESOURCE_FLAGS flags)
					{
						D3D12_RESOURCE_DESC desc;
						ZeroMemory(&desc, sizeof(desc));
						desc.Alignment = 0;
						desc.DepthOrArraySize = max(1, DepthOrArraySize);
						desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
						desc.Flags = flags;
						desc.Format = GetBaseFormat(format);
						desc.Height = static_cast<UINT>(H);
						desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
						desc.MipLevels = max(1, numMips);
						desc.SampleDesc.Count = sampleCount;
						desc.SampleDesc.Quality = 0;
						desc.Width = static_cast<UINT64>(W);
						return desc;
					}

					size_t CLOAK_CALL_THIS ColorBuffer::GetResourceViewCount(In API::Rendering::VIEW_TYPE view)
					{
						API::Helper::Lock lock(m_sync);
						if ((m_views & view) == API::Rendering::VIEW_TYPE::NONE) { return 0; }
						switch (view)
						{
							case CloakEngine::API::Rendering::VIEW_TYPE::SRV:
							case CloakEngine::API::Rendering::VIEW_TYPE::SRV_DYNAMIC:
								return 1;
							case CloakEngine::API::Rendering::VIEW_TYPE::RTV:
								return m_size;
							case CloakEngine::API::Rendering::VIEW_TYPE::CUBE:
							case CloakEngine::API::Rendering::VIEW_TYPE::CUBE_DYNAMIC:
								return 1;
							case CloakEngine::API::Rendering::VIEW_TYPE::UAV:
							case CloakEngine::API::Rendering::VIEW_TYPE::UAV_DYNAMIC:
								return m_numMipMaps;
							default:
								break;
						}
						return 0;
					}
					void CLOAK_CALL_THIS ColorBuffer::CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE type)
					{
						API::Helper::Lock lock(m_sync);
						CLOAK_ASSUME(count > 0);
						CLOAK_ASSUME(count >= GetResourceViewCount(type));
						IManager* clm = Engine::Graphic::Core::GetCommandListManager();
						IDevice* dev = clm->GetDevice();
						switch (type)
						{
							case CloakEngine::API::Rendering::VIEW_TYPE::SRV:
							case CloakEngine::API::Rendering::VIEW_TYPE::SRV_DYNAMIC:
							{
								SRV_DESC desc;
								ZeroMemory(&desc, sizeof(desc));
								desc.Format = Casting::CastBackward(m_realFormat);
								desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
								switch (m_type)
								{
									case TextureType::SWAPCHAIN:
									case TextureType::TEXTURE:
										if (m_sampleCount > 1)
										{
											desc.ViewDimension = SRV_DIMENSION_TEXTURE2DMS;
										}
										else
										{
											desc.ViewDimension = SRV_DIMENSION_TEXTURE2D;
											desc.Texture2D.MipLevels = m_numMipMaps;
											desc.Texture2D.MostDetailedMip = 0;
											desc.Texture2D.PlaneSlice = 0;
											desc.Texture2D.ResourceMinLODClamp = 0;
										}
										break;
									case TextureType::TEXTURE_ARRAY:
										desc.ViewDimension = SRV_DIMENSION_TEXTURE2DARRAY;
										desc.Texture2DArray.ArraySize = m_size;
										desc.Texture2DArray.FirstArraySlice = 0;
										desc.Texture2DArray.MipLevels = m_numMipMaps;
										desc.Texture2DArray.MostDetailedMip = 0;
										desc.Texture2DArray.PlaneSlice = 0;
										desc.Texture2DArray.ResourceMinLODClamp = 0;
										break;
									case TextureType::TEXTURE_CUBEMAP:
										desc.ViewDimension = SRV_DIMENSION_TEXTURECUBE;
										desc.TextureCube.MipLevels = m_numMipMaps;
										desc.TextureCube.MostDetailedMip = 0;
										desc.TextureCube.ResourceMinLODClamp = 0;
										break;
									default:
										break;
								}
								
								if (handles[0].HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP) { dev->CreateShaderResourceView(this, desc, handles[0].CPU); }
								m_SRV[type == API::Rendering::VIEW_TYPE::SRV ? 0 : 2] = handles[0];
								break;
							}
							case CloakEngine::API::Rendering::VIEW_TYPE::RTV:
							{
								RTV_DESC desc;
								ZeroMemory(&desc, sizeof(desc));
								desc.Format = Casting::CastBackward(m_realFormat);
								switch (m_type)
								{
									case TextureType::SWAPCHAIN:
									case TextureType::TEXTURE:
										if (m_sampleCount > 1)
										{
											desc.ViewDimension = RTV_DIMENSION_TEXTURE2DMS;
										}
										else
										{
											desc.ViewDimension = RTV_DIMENSION_TEXTURE2D;
											desc.Texture2D.MipSlice = 0;
											desc.Texture2D.PlaneSlice = 0;
										}
										break;
									case TextureType::TEXTURE_CUBEMAP:
									case TextureType::TEXTURE_ARRAY:
										desc.ViewDimension = RTV_DIMENSION_TEXTURE2DARRAY;
										desc.Texture2DArray.ArraySize = 1;
										desc.Texture2DArray.MipSlice = 0;
										desc.Texture2DArray.FirstArraySlice = 0;
										desc.Texture2DArray.PlaneSlice = 0;
										break;
									default:
										break;
								}

								CLOAK_ASSUME(m_size <= count);
								for (UINT a = 0; a < m_size; a++)
								{
									if (desc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2DARRAY) { desc.Texture2DArray.FirstArraySlice = a; }
									if (handles[a].HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP) { dev->CreateRenderTargetView(this, desc, handles[a].CPU); }
									m_RTV[a] = handles[a];
								}
								break;
							}
							case CloakEngine::API::Rendering::VIEW_TYPE::CUBE:
							case CloakEngine::API::Rendering::VIEW_TYPE::CUBE_DYNAMIC:
							{
								SRV_DESC desc;
								ZeroMemory(&desc, sizeof(desc));
								desc.Format = Casting::CastBackward(m_realFormat);
								desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
								desc.ViewDimension = SRV_DIMENSION_TEXTURE2DARRAY;
								desc.Texture2DArray.ArraySize = m_size;
								desc.Texture2DArray.FirstArraySlice = 0;
								desc.Texture2DArray.MipLevels = m_numMipMaps;
								desc.Texture2DArray.MostDetailedMip = 0;
								desc.Texture2DArray.PlaneSlice = 0;
								desc.Texture2DArray.ResourceMinLODClamp = 0;

								if (handles[0].HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP) { dev->CreateShaderResourceView(this, desc, handles[0].CPU); }
								m_SRV[type == API::Rendering::VIEW_TYPE::CUBE ? 1 : 3] = handles[0];
								break;
							}
							case CloakEngine::API::Rendering::VIEW_TYPE::UAV:
							case CloakEngine::API::Rendering::VIEW_TYPE::UAV_DYNAMIC:
							{
								UAV_DESC desc;
								ZeroMemory(&desc, sizeof(desc));
								desc.Format = Casting::CastBackward(GetUAVFormat(m_realFormat));
								switch (m_type)
								{
									case CloakEngine::Impl::Rendering::TextureType::SWAPCHAIN:
									case CloakEngine::Impl::Rendering::TextureType::TEXTURE:
										desc.ViewDimension = UAV_DIMENSION_TEXTURE2D;
										desc.Texture2D.MipSlice = 0;
										desc.Texture2D.PlaneSlice = 0;
										break;
									case CloakEngine::Impl::Rendering::TextureType::TEXTURE_ARRAY:
									case CloakEngine::Impl::Rendering::TextureType::TEXTURE_CUBEMAP:
										desc.ViewDimension = UAV_DIMENSION_TEXTURE2DARRAY;
										desc.Texture2DArray.ArraySize = m_size;
										desc.Texture2DArray.FirstArraySlice = 0;
										desc.Texture2DArray.MipSlice = 0;
										desc.Texture2DArray.PlaneSlice = 0;
										break;
									default:
										break;
								}

								const size_t offset = (type == API::Rendering::VIEW_TYPE::UAV) ? 0 : m_numMipMaps;
								for (UINT a = 0; a < m_numMipMaps; a++)
								{
									if (desc.ViewDimension == UAV_DIMENSION_TEXTURE2D) { desc.Texture2D.MipSlice = a; }
									else { desc.Texture2DArray.MipSlice = a; }
									if (handles[a].HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP) 
									{ 
										dev->CreateUnorderedAccessView(this, nullptr, desc, handles[a].CPU); 
									}
									m_UAV[offset + a] = handles[a];
								}
								break;
							}
							default:
								CLOAK_ASSUME(count == 0);
								break;
						}
						dev->Release();
					}

					When(base != nullptr, Ret_notnull) ColorBuffer* CLOAK_CALL ColorBuffer::CreateFromSwapChain(In ISwapChain* base, In uint32_t textureNum)
					{
						if (CloakDebugCheckOK(base != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							SwapChain* sc = nullptr;
							if (SUCCEEDED(base->QueryInterface(CE_QUERY_ARGS(&sc))))
							{
								ColorBuffer* res = new ColorBuffer();
								res->m_views = API::Rendering::VIEW_TYPE::RTV;
								res->m_sampleCount = 1;
								res->m_type = TextureType::SWAPCHAIN;
								res->m_nodeID = 1;
								res->UseTexture(sc->GetBuffer(textureNum), D3D12_RESOURCE_STATE_PRESENT);
								res->m_RTV = NewArray(API::Rendering::ResourceView, res->m_size);
								res->m_UAV = NewArray(API::Rendering::ResourceView, 2 * res->m_numMipMaps);
								sc->Release();
								return res;
							}
						}
						return nullptr;
					}
					Ret_notnull ColorBuffer* CLOAK_CALL ColorBuffer::Create(In const API::Rendering::TEXTURE_DESC& desc, In size_t nodeID, In UINT nodeMask)
					{
						const API::Rendering::VIEW_TYPE access = desc.AccessType & (TEXTURE_VIEW_TYPES | (desc.Type == API::Rendering::TEXTURE_TYPE::TEXTURE_CUBE ? API::Rendering::VIEW_TYPE::CUBE : API::Rendering::VIEW_TYPE::NONE));
						D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
						if ((access & API::Rendering::VIEW_TYPE::UAV) != API::Rendering::VIEW_TYPE::NONE) { flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; }
						if ((access & API::Rendering::VIEW_TYPE::RTV) != API::Rendering::VIEW_TYPE::NONE) { flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; }

						const DXGI_FORMAT format = Casting::CastForward(desc.Format);

						D3D12_CLEAR_VALUE clear;
						ZeroMemory(&clear, sizeof(clear));
						clear.Format = GetClearFormat(format);
						clear.Color[0] = desc.clearValue.R;
						clear.Color[1] = desc.clearValue.G;
						clear.Color[2] = desc.clearValue.B;
						clear.Color[3] = desc.clearValue.A;

						ColorBuffer* res = new ColorBuffer();
						IManager* clm = Engine::Graphic::Core::GetCommandListManager();
						IDevice* dev = clm->GetDevice();

						switch (desc.Type)
						{
							case API::Rendering::TEXTURE_TYPE::TEXTURE:
							{
								res->m_type = TextureType::TEXTURE;
								uint32_t mipMaps = max(1, desc.Texture.MipMaps);
								if (CloakCheckError(mipMaps == 1 || desc.Texture.SampleCount == 1, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false)) { res->Release(); res = nullptr; }
								else
								{
									res->m_color = desc.clearValue;
									res->m_views = access;

									D3D12_RESOURCE_DESC resDesc = CreateResourceDesc(desc.Width, desc.Height, 1, mipMaps, desc.Texture.SampleCount, format, flags);
									res->CreateTexture(dev, nodeMask, nodeID, format, resDesc, clear);
								}
								break;
							}
							case API::Rendering::TEXTURE_TYPE::TEXTURE_ARRAY:
							{
								res->m_type = TextureType::TEXTURE_ARRAY;
								if (CloakDebugCheckOK(desc.TextureArray.Images > 0, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
								{
									res->m_color = desc.clearValue;
									res->m_views = access;
									D3D12_RESOURCE_DESC resDesc = CreateResourceDesc(desc.Width, desc.Height, desc.TextureArray.Images, desc.TextureArray.MipMaps, 1, format, flags);
									res->CreateTexture(dev, nodeMask, nodeID, format, resDesc, clear);
								}
								else
								{
									res->Release();
									res = nullptr;
								}
								break;
							}
							case API::Rendering::TEXTURE_TYPE::TEXTURE_CUBE:
							{
								res->m_type = TextureType::TEXTURE_CUBEMAP;
								res->m_color = desc.clearValue;
								res->m_views = access;
								D3D12_RESOURCE_DESC resDesc = CreateResourceDesc(desc.Width, desc.Height, 6, desc.TextureCube.MipMaps, 1, format, flags);
								res->CreateTexture(dev, nodeMask, nodeID, format, resDesc, clear);
								break;
							}

							default:
								res->Release();
								res = nullptr;
								break;
						}
						res->m_RTV = NewArray(API::Rendering::ResourceView, res->m_size);
						res->m_UAV = NewArray(API::Rendering::ResourceView, 2 * res->m_numMipMaps);
						dev->Release();
						return res;
					}
					CLOAK_CALL ColorBuffer::ColorBuffer()
					{
						m_dlvl = 0;
						m_UAV = nullptr;
						m_RTV = nullptr;
						DEBUG_NAME(ColorBuffer);
					}
					CLOAK_CALL ColorBuffer::~ColorBuffer()
					{
						if (Impl::Global::Game::canThreadRun())
						{
							IDescriptorHeap* heap = Engine::Graphic::Core::GetCommandListManager()->GetQueueByNodeID(m_nodeID - 1)->GetDescriptorHeap(HEAP_CBV_SRV_UAV);
							heap->Free(&m_SRV[0], 2);
							heap->Free(&m_UAV[0], m_numMipMaps);
							heap = Engine::Graphic::Core::GetCommandListManager()->GetQueueByNodeID(m_nodeID - 1)->GetDescriptorHeap(HEAP_CBV_SRV_UAV_DYNAMIC);
							heap->Free(&m_SRV[2], 2);
							heap->Free(&m_UAV[m_numMipMaps], m_numMipMaps);
							Engine::Graphic::Core::GetCommandListManager()->GetQueueByNodeID(m_nodeID - 1)->GetDescriptorHeap(HEAP_RTV)->Free(m_RTV, m_size);
						}
						DeleteArray(m_UAV);
						DeleteArray(m_RTV);
					}
					API::Rendering::ResourceView CLOAK_CALL_THIS ColorBuffer::GetSRV(In_opt bool cube) const
					{
						API::Helper::Lock lock(m_sync);
						if (cube && m_type == TextureType::TEXTURE_CUBEMAP) { return m_SRV[1]; }
						return m_SRV[0];
					}
					API::Rendering::ResourceView CLOAK_CALL_THIS ColorBuffer::GetUAV(In_opt UINT num) const { API::Helper::Lock lock(m_sync); return num < m_numMipMaps ? m_UAV[num] : m_UAV[0]; }
					API::Rendering::ResourceView CLOAK_CALL_THIS ColorBuffer::GetDynamicSRV(In_opt bool cube) const
					{
						API::Helper::Lock lock(m_sync);
						if (cube && m_type == TextureType::TEXTURE_CUBEMAP) { return m_SRV[3]; }
						return m_SRV[2];
					}
					API::Rendering::ResourceView CLOAK_CALL_THIS ColorBuffer::GetDynamicUAV(In_opt UINT num) const { API::Helper::Lock lock(m_sync); return num < m_numMipMaps ? m_UAV[m_numMipMaps + num] : m_UAV[m_numMipMaps]; }
					API::Rendering::ResourceView CLOAK_CALL_THIS ColorBuffer::GetRTV(In_opt UINT num) const { API::Helper::Lock lock(m_sync); return num < m_size ? m_RTV[num] : m_RTV[0]; }
					const API::Helper::Color::RGBA& CLOAK_CALL_THIS ColorBuffer::GetClearColor() const { return m_color; }
					void CLOAK_CALL_THIS ColorBuffer::Recreate(In const API::Rendering::TEXTURE_DESC& desc)
					{
						const API::Rendering::VIEW_TYPE access = desc.AccessType & TEXTURE_VIEW_TYPES;
						API::Helper::Lock lock(m_sync);
						API::Helper::Lock ulock = LockUsage();
						Impl::Rendering::TextureType nt;
						switch (desc.Type)
						{
							case API::Rendering::TEXTURE_TYPE::TEXTURE: nt = Impl::Rendering::TextureType::TEXTURE; break;
							case API::Rendering::TEXTURE_TYPE::TEXTURE_ARRAY: nt = Impl::Rendering::TextureType::TEXTURE_ARRAY; break;
							case API::Rendering::TEXTURE_TYPE::TEXTURE_CUBE: nt = Impl::Rendering::TextureType::TEXTURE_CUBEMAP; break;
							default:
								break;
						}

						if (CloakDebugCheckOK(m_type == nt && m_views == access && ((m_type == TextureType::TEXTURE_ARRAY && m_size == desc.TextureArray.Images && m_numMipMaps == desc.TextureArray.MipMaps) || (m_type == TextureType::TEXTURE && m_numMipMaps == desc.Texture.MipMaps)), API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
							if ((access & API::Rendering::VIEW_TYPE::UAV) != API::Rendering::VIEW_TYPE::NONE) { flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; }
							if ((access & API::Rendering::VIEW_TYPE::RTV) != API::Rendering::VIEW_TYPE::NONE) { flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; }

							DXGI_FORMAT format = Casting::CastForward(desc.Format);

							D3D12_CLEAR_VALUE clear;
							ZeroMemory(&clear, sizeof(clear));
							clear.Format = GetClearFormat(format);
							clear.Color[0] = desc.clearValue.R;
							clear.Color[1] = desc.clearValue.G;
							clear.Color[2] = desc.clearValue.B;
							clear.Color[3] = desc.clearValue.A;

							IManager* clm = Engine::Graphic::Core::GetCommandListManager();
							IDevice* dev = clm->GetDevice();
							const size_t nodeID = m_nodeID - 1;
							const UINT nodeMask = clm->GetNodeCount() == 1 ? 0 : static_cast<UINT>(1 << nodeID);

							const UINT lSize = m_size;
							const UINT lMips = m_numMipMaps;

							switch (m_type)
							{
								case TextureType::TEXTURE:
								{
									uint32_t mipMaps = min(1, desc.Texture.MipMaps);
									if (CloakDebugCheckOK(mipMaps == 1 || desc.Texture.SampleCount == 1, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
									{
										m_color = desc.clearValue;
										m_views = access;
										D3D12_RESOURCE_DESC resDesc = CreateResourceDesc(desc.Width, desc.Height, 1, mipMaps, desc.Texture.SampleCount, format, flags);
										CreateTexture(dev, nodeMask, nodeID, format, resDesc, clear);
										break;
									}
								}
								case TextureType::TEXTURE_ARRAY:
								{
									m_color = desc.clearValue;
									m_views = access;
									D3D12_RESOURCE_DESC resDesc = CreateResourceDesc(desc.Width, desc.Height, desc.TextureArray.Images, desc.TextureArray.MipMaps, 1, format, flags);
									CreateTexture(dev, nodeMask, nodeID, format, resDesc, clear);
									break;
								}
								case TextureType::TEXTURE_CUBEMAP:
								{
									m_color = desc.clearValue;
									m_views = access;
									D3D12_RESOURCE_DESC resDesc = CreateResourceDesc(desc.Width, desc.Height, 6, desc.TextureCube.MipMaps, 1, format, flags);
									CreateTexture(dev, nodeMask, nodeID, format, resDesc, clear); 
									break;
								}
								default:
									break;
							}

							if ((m_views & API::Rendering::VIEW_TYPE::SRV) != API::Rendering::VIEW_TYPE::NONE) { CreateResourceViews(1, &m_SRV[0], API::Rendering::VIEW_TYPE::SRV); }
							if ((m_views & API::Rendering::VIEW_TYPE::CUBE) != API::Rendering::VIEW_TYPE::NONE) { CreateResourceViews(1, &m_SRV[1], API::Rendering::VIEW_TYPE::CUBE); }
							if ((m_views & API::Rendering::VIEW_TYPE::RTV) != API::Rendering::VIEW_TYPE::NONE) { CLOAK_ASSUME(lSize == m_size); CreateResourceViews(m_size, m_RTV, API::Rendering::VIEW_TYPE::RTV); }
							if ((m_views & API::Rendering::VIEW_TYPE::UAV) != API::Rendering::VIEW_TYPE::NONE) { CLOAK_ASSUME(lMips == m_numMipMaps); CreateResourceViews(m_numMipMaps, &m_UAV[0], API::Rendering::VIEW_TYPE::UAV); }
							if ((m_views & API::Rendering::VIEW_TYPE::SRV_DYNAMIC) != API::Rendering::VIEW_TYPE::NONE) { CreateResourceViews(1, &m_SRV[2], API::Rendering::VIEW_TYPE::SRV_DYNAMIC); }
							if ((m_views & API::Rendering::VIEW_TYPE::CUBE_DYNAMIC) != API::Rendering::VIEW_TYPE::NONE) { CreateResourceViews(1, &m_SRV[3], API::Rendering::VIEW_TYPE::CUBE_DYNAMIC); }
							if ((m_views & API::Rendering::VIEW_TYPE::UAV_DYNAMIC) != API::Rendering::VIEW_TYPE::NONE) { CLOAK_ASSUME(lMips == m_numMipMaps); CreateResourceViews(m_numMipMaps, &m_UAV[m_numMipMaps], API::Rendering::VIEW_TYPE::UAV_DYNAMIC); }

							dev->Release();
						}
					}
					void CLOAK_CALL_THIS ColorBuffer::FreeSwapChain()
					{
						if (m_type == TextureType::SWAPCHAIN)
						{
							UseTexture(nullptr, D3D12_RESOURCE_STATE_PRESENT);
						}
					}
					void CLOAK_CALL_THIS ColorBuffer::RecreateFromSwapChain(In ISwapChain* base, In uint32_t textureNum)
					{
						if (m_type == TextureType::SWAPCHAIN)
						{
							SwapChain* sc = nullptr;
							if (SUCCEEDED(base->QueryInterface(CE_QUERY_ARGS(&sc))))
							{
								const UINT lSize = m_size;
								const UINT lMips = m_numMipMaps;
								UseTexture(sc->GetBuffer(textureNum), D3D12_RESOURCE_STATE_PRESENT);
								if ((m_views & API::Rendering::VIEW_TYPE::SRV) != API::Rendering::VIEW_TYPE::NONE) { CreateResourceViews(1, &m_SRV[0], API::Rendering::VIEW_TYPE::SRV); }
								if ((m_views & API::Rendering::VIEW_TYPE::CUBE) != API::Rendering::VIEW_TYPE::NONE) { CreateResourceViews(1, &m_SRV[1], API::Rendering::VIEW_TYPE::CUBE); }
								if ((m_views & API::Rendering::VIEW_TYPE::RTV) != API::Rendering::VIEW_TYPE::NONE) { CLOAK_ASSUME(lSize == m_size); CreateResourceViews(m_size, m_RTV, API::Rendering::VIEW_TYPE::RTV); }
								if ((m_views & API::Rendering::VIEW_TYPE::UAV) != API::Rendering::VIEW_TYPE::NONE) { CLOAK_ASSUME(lMips == m_numMipMaps); CreateResourceViews(m_numMipMaps, &m_UAV[0], API::Rendering::VIEW_TYPE::UAV); }
								if ((m_views & API::Rendering::VIEW_TYPE::SRV_DYNAMIC) != API::Rendering::VIEW_TYPE::NONE) { CreateResourceViews(1, &m_SRV[2], API::Rendering::VIEW_TYPE::SRV_DYNAMIC); }
								if ((m_views & API::Rendering::VIEW_TYPE::CUBE_DYNAMIC) != API::Rendering::VIEW_TYPE::NONE) { CreateResourceViews(1, &m_SRV[3], API::Rendering::VIEW_TYPE::CUBE_DYNAMIC); }
								if ((m_views & API::Rendering::VIEW_TYPE::UAV_DYNAMIC) != API::Rendering::VIEW_TYPE::NONE) { CLOAK_ASSUME(lMips == m_numMipMaps); CreateResourceViews(m_numMipMaps, &m_UAV[m_numMipMaps], API::Rendering::VIEW_TYPE::UAV_DYNAMIC); }
								sc->Release();
							}
						}
					}
					uint8_t CLOAK_CALL_THIS ColorBuffer::GetDetailLevel() const { return m_dlvl; }
					void CLOAK_CALL_THIS ColorBuffer::SetDetailLevel(In uint32_t lvl) { m_dlvl = static_cast<uint8_t>(min(lvl, SAMPLER_MAX_MIP_CLAMP - 1)); }

					Success(return == true) bool CLOAK_CALL_THIS ColorBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY(ptr, riid, ColorBuffer, ColorBuffer_v1, PixelBuffer);
					}
				}
			}
		}
	}
}
#endif