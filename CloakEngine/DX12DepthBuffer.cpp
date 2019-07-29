#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/DepthBuffer.h"
#include "Implementation/Rendering/DX12/Casting.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/Manager.h"

#include "Engine/Graphic/Core.h"

#define DEPTH_VIEW_TYPES (API::Rendering::VIEW_TYPE::DSV | API::Rendering::VIEW_TYPE::SRV | API::Rendering::VIEW_TYPE::SRV_DYNAMIC)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace DepthBuffer_v1 {
					inline DXGI_FORMAT CLOAK_CALL GetBaseFormat(In DXGI_FORMAT format)
					{
						switch (format)
						{
							case DXGI_FORMAT_R8G8B8A8_TYPELESS:
							case DXGI_FORMAT_R8G8B8A8_UNORM:
							case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
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

							default:
								return format;
						}
					}
					inline DXGI_FORMAT CLOAK_CALL GetClearFormat(In DXGI_FORMAT format)
					{
						switch (format)
						{
							case DXGI_FORMAT_R32G8X24_TYPELESS:
							case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
							case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
							case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
								return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

							case DXGI_FORMAT_R32_TYPELESS:
							case DXGI_FORMAT_R32_FLOAT:
							case DXGI_FORMAT_D32_FLOAT:
								return DXGI_FORMAT_D32_FLOAT;

							case DXGI_FORMAT_R24G8_TYPELESS:
							case DXGI_FORMAT_D24_UNORM_S8_UINT:
							case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
							case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
								return DXGI_FORMAT_D24_UNORM_S8_UINT;

							case DXGI_FORMAT_R16_TYPELESS:
							case DXGI_FORMAT_R16_UNORM:
							case DXGI_FORMAT_D16_UNORM:
								return DXGI_FORMAT_D16_UNORM;

							default:
								return format;
						}
					}
					inline DXGI_FORMAT CLOAK_CALL GetDSVFormat(In DXGI_FORMAT format)
					{
						switch (format)
						{
							case DXGI_FORMAT_R32G8X24_TYPELESS:
							case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
							case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
							case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
								return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

							case DXGI_FORMAT_R32_TYPELESS:
							case DXGI_FORMAT_R32_FLOAT:
							case DXGI_FORMAT_D32_FLOAT:
								return DXGI_FORMAT_D32_FLOAT;

							case DXGI_FORMAT_R24G8_TYPELESS:
							case DXGI_FORMAT_D24_UNORM_S8_UINT:
							case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
							case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
								return DXGI_FORMAT_D24_UNORM_S8_UINT;

							case DXGI_FORMAT_R16_TYPELESS:
							case DXGI_FORMAT_R16_UNORM:
							case DXGI_FORMAT_D16_UNORM:
								return DXGI_FORMAT_D16_UNORM;

							default:
								return DXGI_FORMAT_UNKNOWN;
						}
					}
					inline DXGI_FORMAT CLOAK_CALL GetDepthFormat(In DXGI_FORMAT format)
					{
						switch (format)
						{
							case DXGI_FORMAT_R32G8X24_TYPELESS:
							case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
							case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
							case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
								return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

							case DXGI_FORMAT_R32_TYPELESS:
							case DXGI_FORMAT_R32_FLOAT:
							case DXGI_FORMAT_D32_FLOAT:
								return DXGI_FORMAT_R32_FLOAT;

							case DXGI_FORMAT_R24G8_TYPELESS:
							case DXGI_FORMAT_D24_UNORM_S8_UINT:
							case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
							case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
								return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

							case DXGI_FORMAT_R16_TYPELESS:
							case DXGI_FORMAT_R16_UNORM:
							case DXGI_FORMAT_D16_UNORM:
								return DXGI_FORMAT_R16_UNORM;

							default:
								return DXGI_FORMAT_UNKNOWN;
						}
					}
					inline DXGI_FORMAT CLOAK_CALL GetStencilFormat(In DXGI_FORMAT format)
					{
						switch (format)
						{
							case DXGI_FORMAT_R32G8X24_TYPELESS:
							case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
							case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
							case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
								return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

							case DXGI_FORMAT_R24G8_TYPELESS:
							case DXGI_FORMAT_D24_UNORM_S8_UINT:
							case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
							case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
								return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

							default:
								return DXGI_FORMAT_UNKNOWN;
						}
					}
					inline DXGI_FORMAT CLOAK_CALL GetRTVFormat(In DXGI_FORMAT format)
					{
						switch (format)
						{
							case DXGI_FORMAT_R32G8X24_TYPELESS:
							case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
							case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
							case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
								return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

							case DXGI_FORMAT_R24G8_TYPELESS:
							case DXGI_FORMAT_D24_UNORM_S8_UINT:
							case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
							case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
								return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

							case DXGI_FORMAT_R32_TYPELESS:
							case DXGI_FORMAT_R32_FLOAT:
							case DXGI_FORMAT_D32_FLOAT:
								return DXGI_FORMAT_R32_FLOAT;

							case DXGI_FORMAT_R16_TYPELESS:
							case DXGI_FORMAT_R16_UNORM:
							case DXGI_FORMAT_D16_UNORM:
								return DXGI_FORMAT_R16_UNORM;

							default:
								return DXGI_FORMAT_UNKNOWN;
						}
					}
					inline D3D12_RESOURCE_DESC CLOAK_CALL CreateResourceDesc(In uint32_t W, In uint32_t H, In uint32_t DepthOrArraySize, In uint32_t numMips, In UINT sampleCount, In DXGI_FORMAT format, In D3D12_RESOURCE_FLAGS flags)
					{
						D3D12_RESOURCE_DESC desc;
						ZeroMemory(&desc, sizeof(desc));
						desc.Alignment = 0;
						desc.DepthOrArraySize = DepthOrArraySize;
						desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
						desc.Flags = flags;
						desc.Format = GetBaseFormat(format);
						desc.Height = static_cast<UINT>(H);
						desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
						desc.MipLevels = numMips;
						desc.SampleDesc.Count = sampleCount;
						desc.SampleDesc.Quality = 0;
						desc.Width = static_cast<UINT64>(W);
						return desc;
					}

					size_t CLOAK_CALL_THIS DepthBuffer::GetResourceViewCount(In API::Rendering::VIEW_TYPE view)
					{
						API::Helper::Lock lock(m_sync);
						if ((m_views & view) == API::Rendering::VIEW_TYPE::NONE) { return 0; }
						switch (view)
						{
							case CloakEngine::API::Rendering::VIEW_TYPE::SRV_DYNAMIC:
							case CloakEngine::API::Rendering::VIEW_TYPE::SRV:
								return (m_useDepth ? 1 : 0) + (m_useStencil ? 1 : 0);
							case CloakEngine::API::Rendering::VIEW_TYPE::DSV:
								if (GetDSVFormat(m_realFormat) != DXGI_FORMAT_UNKNOWN)
								{
									if (GetStencilFormat(m_realFormat) != DXGI_FORMAT_UNKNOWN) { return 4; }
									return 2;
								}
								return 0;
							default:
								break;
						}
						return 0;
					}
					void CLOAK_CALL_THIS DepthBuffer::CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE type)
					{
						API::Helper::Lock lock(m_sync);
						CLOAK_ASSUME(count > 0);
						CLOAK_ASSUME(count >= GetResourceViewCount(type));
						IManager* clm = Engine::Graphic::Core::GetCommandListManager();
						IDevice* dev = clm->GetDevice();
						switch (type)
						{
							case CloakEngine::API::Rendering::VIEW_TYPE::SRV_DYNAMIC:
							case CloakEngine::API::Rendering::VIEW_TYPE::SRV:
							{
								SRV_DESC desc;
								size_t p = 0;
								if (m_useDepth)
								{
									ZeroMemory(&desc, sizeof(desc));
									desc.Format = Casting::CastBackward(GetDepthFormat(m_realFormat));
									if (m_sampleCount == 1)
									{
										desc.ViewDimension = SRV_DIMENSION_TEXTURE2D;
										desc.Texture2D.MipLevels = 1;
									}
									else
									{
										desc.ViewDimension = SRV_DIMENSION_TEXTURE2DMS;
									}
									desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
									if (desc.Format != API::Rendering::Format::UNKNOWN)
									{
										dev->CreateShaderResourceView(this, desc, handles[p].CPU);
										m_SRV[type == API::Rendering::VIEW_TYPE::SRV ? 0 : 2] = handles[p];
										p++;
									}
								}
								if (m_useStencil)
								{
									ZeroMemory(&desc, sizeof(desc));
									desc.Format = Casting::CastBackward(GetStencilFormat(m_realFormat));
									if (m_sampleCount == 1)
									{
										desc.ViewDimension = SRV_DIMENSION_TEXTURE2D;
										desc.Texture2D.MipLevels = 1;
									}
									else
									{
										desc.ViewDimension = SRV_DIMENSION_TEXTURE2DMS;
									}
									desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

									if (desc.Format != API::Rendering::Format::UNKNOWN)
									{
										dev->CreateShaderResourceView(this, desc, handles[p].CPU);
										m_SRV[type == API::Rendering::VIEW_TYPE::SRV ? 1 : 3] = handles[p];
										p++;
									}
								}
								break;
							}
							case CloakEngine::API::Rendering::VIEW_TYPE::DSV:
							{
								DSV_DESC desc;
								ZeroMemory(&desc, sizeof(desc));
								desc.Format = Casting::CastBackward(GetDSVFormat(m_realFormat));
								if (m_sampleCount == 1)
								{
									desc.ViewDimension = DSV_DIMENSION_TEXTURE2D;
									desc.Texture2D.MipSlice = 0;
								}
								else
								{
									desc.ViewDimension = DSV_DIMENSION_TEXTURE2DMS;
								}
								if (desc.Format != API::Rendering::Format::UNKNOWN)
								{
									desc.Flags = DSV_FLAG_NONE;
									if (handles[0].HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP) { dev->CreateDepthStencilView(this, desc, handles[0].CPU); m_DSV[0] = handles[0]; }
									desc.Flags = DSV_FLAG_READ_ONLY_DEPTH;
									if (handles[1].HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP) { dev->CreateDepthStencilView(this, desc, handles[1].CPU); m_DSV[1] = handles[1]; }
									if (GetStencilFormat(m_realFormat) != DXGI_FORMAT_UNKNOWN)
									{
										desc.Flags = DSV_FLAG_READ_ONLY_STENCIL;
										if (handles[2].HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP) { dev->CreateDepthStencilView(this, desc, handles[2].CPU); m_DSV[2] = handles[2]; }
										desc.Flags = DSV_FLAG_READ_ONLY_DEPTH | DSV_FLAG_READ_ONLY_STENCIL;
										if (handles[3].HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP) { dev->CreateDepthStencilView(this, desc, handles[3].CPU); m_DSV[3] = handles[3]; }
									}
								}
								break;
							}
							default:
								CLOAK_ASSUME(count == 0);
								break;
						}
						dev->Release();
					}

					Ret_notnull DepthBuffer* CLOAK_CALL DepthBuffer::Create(In const API::Rendering::DEPTH_DESC& desc, In size_t nodeID, In UINT nodeMask)
					{
						const DXGI_FORMAT dxfrom = Casting::CastForward(desc.Format);
						D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
						if (desc.Depth.UseSRV == false && desc.Stencil.UseSRV == false) { flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE; }
						D3D12_RESOURCE_DESC resDesc = CreateResourceDesc(desc.Width, desc.Height, 1, 1, desc.SampleCount, dxfrom, flags);
						D3D12_CLEAR_VALUE clear;
						ZeroMemory(&clear, sizeof(clear));
						clear.Format = GetClearFormat(dxfrom);
						clear.DepthStencil.Depth = desc.Depth.Clear;
						clear.DepthStencil.Stencil = desc.Stencil.Clear;
						IManager* clm = Engine::Graphic::Core::GetCommandListManager();
						IDevice* dev = clm->GetDevice();

						DepthBuffer* res = new DepthBuffer();
						res->m_views = API::Rendering::VIEW_TYPE::DSV;
						if (desc.Depth.UseSRV || desc.Stencil.UseSRV) { res->m_views |= API::Rendering::VIEW_TYPE::SRV | API::Rendering::VIEW_TYPE::SRV_DYNAMIC; }
						res->m_type = TextureType::TEXTURE;
						res->m_useDepth = desc.Depth.UseSRV;
						res->m_useStencil = desc.Stencil.UseSRV;
						res->m_depthClear = desc.Depth.Clear;
						res->m_stencilClear = desc.Stencil.Clear;

						res->CreateTexture(dev, nodeMask, nodeID, dxfrom, resDesc, clear); 
						RELEASE(dev);
						return res;
					}
					CLOAK_CALL DepthBuffer::DepthBuffer() 
					{
						DEBUG_NAME(DepthBuffer);
					}
					CLOAK_CALL DepthBuffer::~DepthBuffer()
					{
						if (Impl::Global::Game::canThreadRun())
						{
							Engine::Graphic::Core::GetCommandListManager()->GetQueueByNodeID(m_nodeID - 1)->GetDescriptorHeap(HEAP_CBV_SRV_UAV)->Free(&m_SRV[0], 2);
							Engine::Graphic::Core::GetCommandListManager()->GetQueueByNodeID(m_nodeID - 1)->GetDescriptorHeap(HEAP_CBV_SRV_UAV_DYNAMIC)->Free(&m_SRV[2], 2);
							Engine::Graphic::Core::GetCommandListManager()->GetQueueByNodeID(m_nodeID - 1)->GetDescriptorHeap(HEAP_DSV)->Free(m_DSV, 4);
						}
					}
					API::Rendering::ResourceView CLOAK_CALL_THIS DepthBuffer::GetDSV(In_opt API::Rendering::DSV_ACCESS access) const
					{
						API::Helper::Lock lock(m_sync);
						unsigned int p = 0;
						if ((access & API::Rendering::DSV_ACCESS::READ_ONLY_DEPTH) == API::Rendering::DSV_ACCESS::READ_ONLY_DEPTH) { p |= 1; }
						if ((access & API::Rendering::DSV_ACCESS::READ_ONLY_STENCIL) == API::Rendering::DSV_ACCESS::READ_ONLY_STENCIL && GetStencilFormat(m_realFormat) != DXGI_FORMAT_UNKNOWN) { p |= 2; }
						return m_DSV[p];
					}
					API::Rendering::ResourceView CLOAK_CALL_THIS DepthBuffer::GetDepthSRV() const { API::Helper::Lock lock(m_sync); return m_SRV[0]; }
					API::Rendering::ResourceView CLOAK_CALL_THIS DepthBuffer::GetStencilSRV() const { API::Helper::Lock lock(m_sync); return m_SRV[1]; }
					API::Rendering::ResourceView CLOAK_CALL_THIS DepthBuffer::GetDynamicDepthSRV() const { API::Helper::Lock lock(m_sync); return m_SRV[2]; }
					API::Rendering::ResourceView CLOAK_CALL_THIS DepthBuffer::GetDynamicStencilSRV() const { API::Helper::Lock lock(m_sync); return m_SRV[3]; }
					float CLOAK_CALL_THIS DepthBuffer::GetClearDepth() const { return m_depthClear; }
					uint32_t CLOAK_CALL_THIS DepthBuffer::GetClearStencil() const { return m_stencilClear; }
					void CLOAK_CALL_THIS DepthBuffer::Recreate(In const API::Rendering::DEPTH_DESC& desc)
					{
						API::Helper::Lock lock(m_sync);
						API::Helper::Lock ulock = LockUsage();
						if (CloakCheckOK(m_data != nullptr, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, false))
						{
							D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
							if (desc.Depth.UseSRV == false && desc.Stencil.UseSRV == false) { flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE; }
							D3D12_RESOURCE_DESC resDesc = CreateResourceDesc(desc.Width, desc.Height, 1, 1, desc.SampleCount, m_format, flags);
							D3D12_CLEAR_VALUE clear;
							ZeroMemory(&clear, sizeof(clear));
							clear.Format = GetClearFormat(m_format);
							clear.DepthStencil.Depth = m_depthClear;
							clear.DepthStencil.Stencil = m_stencilClear;
							IManager* clm = Engine::Graphic::Core::GetCommandListManager();
							IDevice* dev = clm->GetDevice();
							const size_t nodeID = m_nodeID - 1;
							const UINT nodeMask = clm->GetNodeCount() == 1 ? 0 : static_cast<UINT>(1 << nodeID);

							CreateTexture(dev, nodeMask, nodeID, m_realFormat, resDesc, clear);
							RELEASE(dev);

							if ((m_views & API::Rendering::VIEW_TYPE::SRV) != API::Rendering::VIEW_TYPE::NONE) { CreateResourceViews(2, &m_SRV[0], API::Rendering::VIEW_TYPE::SRV); }
							if ((m_views & API::Rendering::VIEW_TYPE::SRV_DYNAMIC) != API::Rendering::VIEW_TYPE::NONE) { CreateResourceViews(2, &m_SRV[2], API::Rendering::VIEW_TYPE::SRV_DYNAMIC); }
							if ((m_views & API::Rendering::VIEW_TYPE::DSV) != API::Rendering::VIEW_TYPE::NONE) { CreateResourceViews(4, m_DSV, API::Rendering::VIEW_TYPE::DSV); }
						}
					}
					Success(return == true) bool CLOAK_CALL_THIS DepthBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY(ptr, riid, DepthBuffer, DepthBuffer_v1, PixelBuffer);
					}
				}
			}
		}
	}
}
#endif