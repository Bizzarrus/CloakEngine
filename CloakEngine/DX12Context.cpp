#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/Context.h"
#include "Implementation/Rendering/DX12/Casting.h"
#include "Implementation/Rendering/DX12/Allocator.h"
#include "Implementation/Rendering/DX12/BasicBuffer.h"
#include "Implementation/Rendering/DX12/ColorBuffer.h"
#include "Implementation/Rendering/DX12/DepthBuffer.h"
#include "Implementation/Rendering/DX12/RootSignature.h"
#include "Implementation/Rendering/DX12/PipelineState.h"
#include "Implementation/Rendering/DX12/Manager.h"
#include "Implementation/Rendering/DX12/StructuredBuffer.h"
#include "Implementation/Rendering/DX12/Resource.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Global/Debug.h"

#include "Engine/Graphic/Core.h"

#include <queue>

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace Context_v1 {
					//Any Resource can be implicitly promoted to these states
					//	The first value are all read states (which can be set in any combination), the other values are all write states (which are exclusive)
					constexpr API::Rendering::RESOURCE_STATE PROMOTION_ANY[] = {
						API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE | API::Rendering::STATE_PIXEL_SHADER_RESOURCE | API::Rendering::STATE_COPY_SOURCE,
						API::Rendering::STATE_COPY_DEST,
					};
					//Any Resource that's either a texture with the simultanios access flag, or a buffer, can be implicitly promoted to these states
					//	The first value are all read states (which can be set in any combination), the other values are all write states (which are exclusive)
					constexpr API::Rendering::RESOURCE_STATE PROMOTION_LIMITED[] = {
						API::Rendering::STATE_VERTEX_AND_CONSTANT_BUFFER | API::Rendering::STATE_INDEX_BUFFER | API::Rendering::STATE_RESOLVE_SOURCE | API::Rendering::STATE_INDIRECT_ARGUMENT | API::Rendering::STATE_PREDICATION | API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE | API::Rendering::STATE_PIXEL_SHADER_RESOURCE | API::Rendering::STATE_COPY_SOURCE,
						API::Rendering::STATE_RENDER_TARGET,
						API::Rendering::STATE_UNORDERED_ACCESS,
						API::Rendering::STATE_COPY_DEST,
						API::Rendering::STATE_STREAM_OUT,
						API::Rendering::STATE_RESOLVE_DEST,
					};
					constexpr API::Rendering::RESOURCE_STATE INVALID_COPY_STATES = ~(API::Rendering::STATE_COPY_DEST | API::Rendering::STATE_COPY_SOURCE | API::Rendering::STATE_COMMON);

					constexpr UINT DEFAULT_CLEAR_COLOR[4] = { 0,0,0,0 };

					CLOAK_FORCEINLINE D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE RenderPassBeginAccess(In API::Rendering::RENDER_PASS_ACCESS access)
					{
						switch (access)
						{
							case CloakEngine::API::Rendering::RENDER_PASS_ACCESS_DISCARD:
								return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
							case CloakEngine::API::Rendering::RENDER_PASS_ACCESS_PRESERVE:
								return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
							case CloakEngine::API::Rendering::RENDER_PASS_ACCESS_CLEAR:
								return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
							case CloakEngine::API::Rendering::RENDER_PASS_ACCESS_NO_ACCESS:
								return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
							default:
								CLOAK_ASSUME(false);
								break;
						}
						return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					}
					CLOAK_FORCEINLINE D3D12_RENDER_PASS_ENDING_ACCESS_TYPE RenderPassEndAccess(In API::Rendering::RENDER_PASS_ACCESS access)
					{
						switch (access)
						{
							case CloakEngine::API::Rendering::RENDER_PASS_ACCESS_DISCARD:
								return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
							case CloakEngine::API::Rendering::RENDER_PASS_ACCESS_PRESERVE:
								return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
							case CloakEngine::API::Rendering::RENDER_PASS_ACCESS_NO_ACCESS:
								return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
							case CloakEngine::API::Rendering::RENDER_PASS_ACCESS_CLEAR:
							default:
								CLOAK_ASSUME(false);
								break;
						}
						return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					}
					inline DXGI_FORMAT CLOAK_CALL GetClearFormat(In DXGI_FORMAT format, In bool RTV)
					{
						if (RTV == true)
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
						else
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
					}

					CLOAK_CALL CopyContext::CopyContext(In SingleQueue* queue, In IManager* manager) : m_usedResources(&m_pageAlloc), m_usedQueries(&m_pageAlloc)
					{
						m_sync = nullptr;
						m_queue = queue;
						IQueue* q = m_queue->GetParent();
						m_manager = manager;
						m_queue->CreateCommandList(&m_cmdList, &m_curAlloc);
						m_barriersToFlush = 0;
						m_lastFence = 0;
						m_blockedTransition = false;
						m_inRenderPass = false;
						m_dynAlloc[0] = new DynamicAllocator(DynamicAllocatorType::GPU, q);
						m_dynAlloc[1] = new DynamicAllocator(DynamicAllocatorType::CPU, q);
						m_readBack = new ReadBackAllocator(q);
#ifdef _DEBUG
						for (size_t a = 0; a < ARRAYSIZE(m_dynAlloc); a++) { m_dynAlloc[a]->SetSourceFile(std::string(__FILE__) + " @ " + std::to_string(__LINE__)); }
#endif
						DEBUG_NAME(CopyContext);
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					}
					CLOAK_CALL CopyContext::~CopyContext()
					{
						SAVE_RELEASE(m_sync);
						SAVE_RELEASE(m_readBack);
						for (size_t a = 0; a < ARRAYSIZE(m_dynAlloc); a++) { SAVE_RELEASE(m_dynAlloc[a]); }
					}

					void CLOAK_CALL_THIS CopyContext::WaitForAdapter(In API::Rendering::CONTEXT_TYPE context, In uint32_t nodeID, In uint64_t fenceState)
					{
						WaitForAdapter(m_manager->GetQueue(context, nodeID), fenceState);
					}
					uint64_t CLOAK_CALL_THIS CopyContext::CloseAndExecute(In_opt bool wait, In_opt bool executeDirect)
					{
						uint64_t res = Finish(wait, executeDirect);
						if (executeDirect) { m_queue->FreeContext(this); }
						return res;
					}
					void CLOAK_CALL_THIS CopyContext::CopyBuffer(In API::Rendering::IResource* dst, In API::Rendering::IResource* src)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						Resource* d = nullptr;
						Resource* s = nullptr;
						if (CloakDebugCheckOK(dst != nullptr && src != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							bool hRet = SUCCEEDED(dst->QueryInterface(CE_QUERY_ARGS(&d))) && SUCCEEDED(src->QueryInterface(CE_QUERY_ARGS(&s)));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								d->RegisterUsage(m_queue);
								s->RegisterUsage(m_queue);
								m_usedResources.push(d);
								m_usedResources.push(s);
								API::Helper::Lock lockD(d->GetSyncSection());
								API::Helper::Lock lockS(s->GetSyncSection());
								TransitionResource(dst, API::Rendering::STATE_COPY_DEST);
								TransitionResource(src, API::Rendering::STATE_COPY_SOURCE, true);
								m_cmdList.V0->CopyResource(d->GetResource(), s->GetResource());
							}
						}
						SAVE_RELEASE(d);
						SAVE_RELEASE(s);
					}
					void CLOAK_CALL_THIS CopyContext::CopyBufferRegion(Inout API::Rendering::IResource* dst, In size_t dstOffset, In API::Rendering::IResource* src, In size_t srcOffset, size_t In numBytes)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						Resource* d = nullptr;
						Resource* s = nullptr;
						if (CloakDebugCheckOK(dst != nullptr && src != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							bool hRet = SUCCEEDED(dst->QueryInterface(CE_QUERY_ARGS(&d))) && SUCCEEDED(src->QueryInterface(CE_QUERY_ARGS(&s)));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								d->RegisterUsage(m_queue);
								s->RegisterUsage(m_queue);
								m_usedResources.push(d);
								m_usedResources.push(s);
								API::Helper::Lock lockD(d->GetSyncSection());
								API::Helper::Lock lockS(s->GetSyncSection());
								TransitionResource(dst, API::Rendering::STATE_COPY_DEST);
								TransitionResource(src, API::Rendering::STATE_COPY_SOURCE, true);
								m_cmdList.V0->CopyBufferRegion(d->GetResource(), dstOffset, s->GetResource(), srcOffset, numBytes);
							}
						}
						SAVE_RELEASE(d);
						SAVE_RELEASE(s);
					}
					void CLOAK_CALL_THIS CopyContext::CopySubresource(In API::Rendering::IResource* dst, In UINT dstSubIndex, In API::Rendering::IResource* src, In UINT srcSubIndex)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						if (CloakDebugCheckOK(dst != nullptr && src != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							Resource* dstRc = nullptr;
							Resource* srcRc = nullptr;
							bool hRet = SUCCEEDED(dst->QueryInterface(CE_QUERY_ARGS(&dstRc))) && SUCCEEDED(src->QueryInterface(CE_QUERY_ARGS(&srcRc)));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								dstRc->RegisterUsage(m_queue);
								srcRc->RegisterUsage(m_queue);
								m_usedResources.push(dstRc);
								m_usedResources.push(srcRc);
								API::Helper::Lock lockD(dstRc->GetSyncSection());
								API::Helper::Lock lockS(srcRc->GetSyncSection());
								TransitionResource(dst, API::Rendering::STATE_COPY_DEST);
								TransitionResource(src, API::Rendering::STATE_COPY_SOURCE);
								FlushResourceBarriers();
								D3D12_TEXTURE_COPY_LOCATION dstLoc;
								D3D12_TEXTURE_COPY_LOCATION srcLoc;
								dstLoc.pResource = dstRc->GetResource();
								dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
								dstLoc.SubresourceIndex = dstSubIndex;
								srcLoc.pResource = srcRc->GetResource();
								srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
								srcLoc.SubresourceIndex = srcSubIndex;
								m_cmdList.V0->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
							}
							SAVE_RELEASE(dstRc);
							SAVE_RELEASE(srcRc);
						}
					}
					void CLOAK_CALL_THIS CopyContext::CopyCounter(In API::Rendering::IResource* dst, In size_t dstOffset, In API::Rendering::IStructuredBuffer* src)
					{
						if (CloakDebugCheckOK(src != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							CopyBufferRegion(dst, dstOffset, src->GetCounterBuffer(), 0, sizeof(uint32_t));
						}
					}
					void CLOAK_CALL_THIS CopyContext::ResetCounter(In API::Rendering::IStructuredBuffer* buffer, In_opt uint32_t val)
					{
						if (CloakDebugCheckOK(buffer != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							FillBuffer(buffer->GetCounterBuffer(), 0, val, sizeof(uint32_t));
						}
					}
					void CLOAK_CALL_THIS CopyContext::WriteBuffer(In API::Rendering::IResource* dst, In size_t dstOffset, In_reads_bytes(numBytes) const void* data, In size_t numBytes)
					{
						if (CloakDebugCheckOK(dst != nullptr && data != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							DynAlloc temp;
							m_dynAlloc[1]->Allocate(&temp, numBytes, 512);
							memcpy(temp.Data, data, numBytes);
							CopyBufferRegion(dst, dstOffset, temp.Buffer, temp.Offset, numBytes);
						}
					}
					void CLOAK_CALL_THIS CopyContext::WriteBuffer(Inout API::Rendering::IResource* dst, In UINT firstSubresource, In UINT numSubresources, In_reads(numSubresources) const API::Rendering::SUBRESOURCE_DATA* srds)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						if (CloakDebugCheckOK(dst != nullptr && srds != nullptr && numSubresources > 0, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							Resource* dstRc = nullptr;
							HRESULT hRet = dst->QueryInterface(CE_QUERY_ARGS(&dstRc));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								dstRc->RegisterUsage(m_queue);
								m_usedResources.push(dstRc);
								IDevice* idev = m_manager->GetDevice();
								if (idev != nullptr)
								{
									Device* dev = nullptr;
									if (SUCCEEDED(idev->QueryInterface(CE_QUERY_ARGS(&dev))))
									{
										const size_t memSize = static_cast<size_t>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64))*numSubresources;
										void* mem = m_pageAlloc.Allocate(memSize);
										D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layout = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(mem);
										UINT64* rowSize = reinterpret_cast<UINT64*>(&layout[numSubresources]);
										UINT* numRows = reinterpret_cast<UINT*>(&rowSize[numSubresources]);

										API::Helper::Lock lock(dstRc->GetSyncSection());
										D3D12_RESOURCE_DESC dstDesc = dstRc->GetResource()->GetDesc();
										UINT64 size = 0;
										dev->GetCopyableFootprints(dstDesc, firstSubresource, numSubresources, 0, layout, numRows, rowSize, &size);

										DynAlloc temp;
										uint8_t* srcData = nullptr;

										m_dynAlloc[1]->Allocate(&temp, static_cast<size_t>(size), 512);
										srcData = reinterpret_cast<uint8_t*>(temp.Data);

										bool err = false;
										UINT64 numBytes = 0;
										for (UINT a = 0; a < numSubresources; a++)
										{
											const UINT64 rs = rowSize[a];
											numBytes += rowSize[a] * numRows[a];
											if (rs >= static_cast<UINT64>(-1)) { err = true; }
											else
											{
												const D3D12_SUBRESOURCE_DATA& cur = Casting::CastForward(srds[a]);
												D3D12_MEMCPY_DEST dstData;
												dstData.pData = srcData + layout[a].Offset;
												dstData.RowPitch = layout[a].Footprint.RowPitch;
												dstData.SlicePitch = layout[a].Footprint.RowPitch*numRows[a];
												for (UINT b = 0; b < layout[a].Footprint.Depth; b++)
												{
													uint8_t* dstSlice = reinterpret_cast<uint8_t*>(dstData.pData) + (dstData.SlicePitch*b);
													const uint8_t* srcSlice = reinterpret_cast<const uint8_t*>(cur.pData) + (cur.SlicePitch*b);
													for (UINT c = 0; c < numRows[a]; c++)
													{
														memcpy(dstSlice + (dstData.RowPitch*c), srcSlice + (cur.RowPitch*c), static_cast<size_t>(rs));
													}
												}
											}
										}

										if (dstDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
										{
											CopyBufferRegion(dst, static_cast<size_t>(layout[0].Offset), temp.Buffer, temp.Offset, static_cast<size_t>(numBytes));
										}
										else
										{
											Resource* srcRc = nullptr;
											if (CloakCheckOK(temp.Buffer->QueryInterface(CE_QUERY_ARGS(&srcRc)), API::Global::Debug::Error::GRAPHIC_MAP_ERROR, false))
											{
												D3D12_TEXTURE_COPY_LOCATION dstLoc;
												D3D12_TEXTURE_COPY_LOCATION srcLoc;
												dstLoc.pResource = dstRc->GetResource();
												srcLoc.pResource = srcRc->GetResource();
												dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
												srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
												TransitionResource(temp.Buffer, API::Rendering::STATE_COPY_SOURCE);
												TransitionResource(dst, API::Rendering::STATE_COPY_DEST, true);
												for (UINT a = 0; a < numSubresources; a++)
												{
													dstLoc.SubresourceIndex = firstSubresource + a;
													srcLoc.PlacedFootprint.Offset = layout[a].Offset + temp.Offset;
													srcLoc.PlacedFootprint.Footprint = layout[a].Footprint;
													m_cmdList.V0->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
												}
												srcRc->Release();
											}
										}

										m_pageAlloc.Free(mem);
										dev->Release();
									}
									idev->Release();
								}
								dstRc->Release();
							}
						}
					}
					void CLOAK_CALL_THIS CopyContext::FillBuffer(In API::Rendering::IResource* dst, In size_t dstOffset, In const API::Rendering::DWParam& val, In size_t numBytes)
					{
						if (CloakDebugCheckOK(dst != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							DynAlloc temp;
							m_dynAlloc[1]->Allocate(&temp, numBytes, 512);
							memset(temp.Data, val.IntVal, numBytes);
							CopyBufferRegion(dst, dstOffset, temp.Buffer, temp.Offset, numBytes);
						}
					}
					API::Rendering::ReadBack CLOAK_CALL_THIS CopyContext::ReadBack(In API::Rendering::IPixelBuffer* src, In_opt UINT subresource)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						API::Rendering::ReadBack rb(nullptr, 0, 0);
						if (CloakDebugCheckOK(src != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							TransitionResource(src, API::Rendering::STATE_COPY_SOURCE, true);
							PixelBuffer* srcRc = nullptr;
							const HRESULT hRet = src->QueryInterface(CE_QUERY_ARGS(&srcRc));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								IDevice* idev = m_manager->GetDevice();
								if (idev != nullptr)
								{
									Device* dev = nullptr;
									if (SUCCEEDED(idev->QueryInterface(CE_QUERY_ARGS(&dev))))
									{
										D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
										UINT64 rowSize;
										UINT numRows;

										API::Helper::Lock lock(srcRc->GetSyncSection());
										D3D12_RESOURCE_DESC srcDesc = srcRc->GetResource()->GetDesc();
										dev->GetCopyableFootprints(srcDesc, subresource, 1, 0, &layout, &numRows, &rowSize, nullptr);
										const size_t size = layout.Footprint.RowPitch * layout.Footprint.Height;

										rb = m_readBack->Allocate(size, layout.Footprint.RowPitch, static_cast<UINT>(layout.Footprint.RowPitch - (GetFormatSize(srcRc->GetFormat())*layout.Footprint.Width)), 512);
										Resource* dstRc = nullptr;
										if (SUCCEEDED(rb.GetResource()->QueryInterface(CE_QUERY_ARGS(&dstRc))))
										{
											dstRc->RegisterUsage(m_queue);
											srcRc->RegisterUsage(m_queue);
											m_usedResources.push(dstRc);
											m_usedResources.push(srcRc);
											D3D12_TEXTURE_COPY_LOCATION dstLoc;
											dstLoc.pResource = dstRc->GetResource();
											dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
											dstLoc.PlacedFootprint.Offset = rb.GetOffset();
											dstLoc.PlacedFootprint.Footprint = layout.Footprint;

											D3D12_TEXTURE_COPY_LOCATION srcLoc;
											srcLoc.pResource = srcRc->GetResource();
											srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
											srcLoc.SubresourceIndex = subresource;
											
											m_cmdList.V0->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
										}
										SAVE_RELEASE(dstRc);
									}
									RELEASE(dev);
								}
								SAVE_RELEASE(idev);
							}
							SAVE_RELEASE(srcRc);
						}
						return rb;
					}
					API::Rendering::ReadBack CLOAK_CALL_THIS CopyContext::ReadBack(In API::Rendering::IGraphicBuffer* src)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						API::Rendering::ReadBack rb(nullptr, 0, 0);
						if (CloakDebugCheckOK(src != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							TransitionResource(src, API::Rendering::STATE_COPY_SOURCE, true);
							GraphicBuffer* srcRc = nullptr;
							const HRESULT hRet = src->QueryInterface(CE_QUERY_ARGS(&srcRc));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								IDevice* idev = m_manager->GetDevice();
								if (idev != nullptr)
								{
									Device* dev = nullptr;
									if (SUCCEEDED(idev->QueryInterface(CE_QUERY_ARGS(&dev))))
									{
										D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
										UINT64 rowSize;
										UINT numRows;

										API::Helper::Lock lock(srcRc->GetSyncSection());
										D3D12_RESOURCE_DESC srcDesc = srcRc->GetResource()->GetDesc();
										dev->GetCopyableFootprints(srcDesc, 0, 1, 0, &layout, &numRows, &rowSize, nullptr);
										const size_t size = layout.Footprint.RowPitch * layout.Footprint.Height;

										rb = m_readBack->Allocate(size, static_cast<UINT>(size), 0, 512);
										Resource* dstRc = nullptr;
										if (SUCCEEDED(rb.GetResource()->QueryInterface(CE_QUERY_ARGS(&dstRc))))
										{
											dstRc->RegisterUsage(m_queue);
											srcRc->RegisterUsage(m_queue);
											m_usedResources.push(dstRc);
											m_usedResources.push(srcRc);
											m_cmdList.V0->CopyBufferRegion(dstRc->GetResource(), rb.GetOffset(), srcRc->GetResource(), 0, size);
										}
										SAVE_RELEASE(dstRc);
									}
									RELEASE(dev);
								}
								SAVE_RELEASE(idev);
							}
							SAVE_RELEASE(srcRc);
						}
						return rb;
					}
					void CLOAK_CALL_THIS CopyContext::BeginQuery(In API::Rendering::IQuery* query)
					{
						Query* q = nullptr;
						const HRESULT hRet = query->QueryInterface(CE_QUERY_ARGS(&q));
						if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							q->OnBegin(m_cmdList.V0.Get(), m_queue);
							q->Release();
						}
					}
					void CLOAK_CALL_THIS CopyContext::EndQuery(In API::Rendering::IQuery* query)
					{
						Query* q = nullptr;
						const HRESULT hRet = query->QueryInterface(CE_QUERY_ARGS(&q));
						if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							q->OnEnd(m_cmdList.V0.Get(), m_queue);
							m_usedQueries.push(q);
						}
					}
					void CLOAK_CALL_THIS CopyContext::TransitionResource(In API::Rendering::IResource* resource, In_opt API::Rendering::RESOURCE_STATE newState, In_opt bool flush)
					{
						CLOAK_ASSUME(newState != RESOURCE_STATE_INVALID);
						if (CloakDebugCheckOK(resource != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							Resource* rc = nullptr;
							HRESULT hRet = resource->QueryInterface(CE_QUERY_ARGS(&rc));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								if (rc->CanTransition())
								{
									rc->RegisterUsage(m_queue);
									m_usedResources.push(rc);
									API::Helper::Lock lock(rc->GetSyncSection());
									API::Rendering::RESOURCE_STATE oldState = rc->GetResourceState();
									CLOAK_ASSUME(m_queue->GetType() != QUEUE_TYPE_COPY || ((oldState & INVALID_COPY_STATES) == 0 && (newState & INVALID_COPY_STATES) == 0));
									if ((oldState & newState) != newState || (newState == API::Rendering::STATE_COMMON && oldState != API::Rendering::STATE_COMMON))
									{
										bool autoPromotion = false;
										bool autoDecay = false;
										if (oldState == API::Rendering::STATE_COMMON)
										{
											if (rc->SupportStatePromotion() == true)
											{
												for (size_t a = 0; a < ARRAYSIZE(PROMOTION_LIMITED); a++)
												{
													if ((newState & PROMOTION_LIMITED[a]) == newState)
													{
														autoPromotion = true;
														autoDecay = (a == 0); //Only decay read-only states
														break;
													}
												}
											}
											else
											{
												for (size_t a = 0; a < ARRAYSIZE(PROMOTION_ANY); a++)
												{
													if ((newState & PROMOTION_ANY[a]) == newState)
													{
														autoPromotion = true;
														autoDecay = (a == 0); //Only decay read-only states
														break;
													}
												}
											}
										}

										if (m_blockedTransition == false || autoPromotion == true)
										{
											if (autoPromotion == false)
											{
												D3D12_RESOURCE_BARRIER& desc = m_barrierBuffer[m_barriersToFlush++];
												desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
												desc.Transition.pResource = rc->GetResource();
												desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
												desc.Transition.StateBefore = Casting::CastForward(oldState);
												desc.Transition.StateAfter = Casting::CastForward(newState);
												if (newState == rc->GetTransitionResourceState())
												{
													desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
													rc->SetResourceState(RESOURCE_STATE_INVALID);
												}
												else { desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE; }
												if (flush || m_barriersToFlush == 16)
												{
													m_cmdList.V0->ResourceBarrier(m_barriersToFlush, m_barrierBuffer);
													m_barriersToFlush = 0;
												}
											}
											else if (flush) { FlushResourceBarriers(); }
											rc->SetResourceState(newState, autoDecay);
										}
									}
									else if (newState == API::Rendering::STATE_UNORDERED_ACCESS) { InsertUAVBarrier(resource, flush); }
									else if (flush) { FlushResourceBarriers(); }
								}
								else if (flush) { FlushResourceBarriers(); }
								rc->Release();
							}
						}
					}
					void CLOAK_CALL_THIS CopyContext::BeginTransitionResource(In API::Rendering::IResource* resource, In API::Rendering::RESOURCE_STATE newState, In_opt bool flush)
					{
						CLOAK_ASSUME(newState != RESOURCE_STATE_INVALID);
						if (CloakDebugCheckOK(resource != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false) && m_blockedTransition == false)
						{
							Resource* rc = nullptr;
							HRESULT hRet = resource->QueryInterface(CE_QUERY_ARGS(&rc));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								rc->RegisterUsage(m_queue);
								m_usedResources.push(rc);
								API::Helper::Lock lock(rc->GetSyncSection());
								API::Rendering::RESOURCE_STATE cts = rc->GetTransitionResourceState();
								if (cts != RESOURCE_STATE_INVALID) { TransitionResource(resource, cts); }
								API::Rendering::RESOURCE_STATE oldState = rc->GetResourceState();
								if (((oldState & newState) != newState || (newState == 0 && oldState != 0)) && m_blockedTransition == false)
								{
									D3D12_RESOURCE_BARRIER& desc = m_barrierBuffer[m_barriersToFlush++];
									desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
									desc.Transition.pResource = rc->GetResource();
									desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
									desc.Transition.StateBefore = Casting::CastForward(oldState);
									desc.Transition.StateAfter = Casting::CastForward(newState);
									desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
									rc->SetTransitionResourceState(newState);
								}
								if (flush || m_barriersToFlush == 16)
								{
									m_cmdList.V0->ResourceBarrier(m_barriersToFlush, m_barrierBuffer);
									m_barriersToFlush = 0;
								}
								rc->Release();
							}
						}
					}
					void CLOAK_CALL_THIS CopyContext::InsertUAVBarrier(In API::Rendering::IResource* resource, In_opt bool flush)
					{
						if (CloakDebugCheckOK(resource != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							Resource* rc = nullptr;
							HRESULT hRet = resource->QueryInterface(CE_QUERY_ARGS(&rc));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								rc->RegisterUsage(m_queue);
								m_usedResources.push(rc);
								API::Helper::Lock lock(rc->GetSyncSection());
								D3D12_RESOURCE_BARRIER& desc = m_barrierBuffer[m_barriersToFlush++];
								desc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
								desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
								desc.UAV.pResource = rc->GetResource();

								if (flush || m_barriersToFlush == 16)
								{
									m_cmdList.V0->ResourceBarrier(m_barriersToFlush, m_barrierBuffer);
									m_barriersToFlush = 0;
								}
								rc->Release();
							}
						}
					}
					void CLOAK_CALL_THIS CopyContext::InsertAliasBarrier(In API::Rendering::IResource* before, In API::Rendering::IResource* after, In_opt bool flush)
					{
						if (CloakDebugCheckOK(before != nullptr && after != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							Resource* bef = nullptr;
							Resource* aft = nullptr;
							bool hRet = SUCCEEDED(before->QueryInterface(CE_QUERY_ARGS(&bef))) && SUCCEEDED(after->QueryInterface(CE_QUERY_ARGS(&aft)));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								bef->RegisterUsage(m_queue);
								aft->RegisterUsage(m_queue);
								m_usedResources.push(bef);
								m_usedResources.push(aft);
								API::Helper::Lock lockB(bef->GetSyncSection());
								API::Helper::Lock lockA(aft->GetSyncSection());
								D3D12_RESOURCE_BARRIER& desc = m_barrierBuffer[m_barriersToFlush++];
								desc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
								desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
								desc.Aliasing.pResourceBefore = bef->GetResource();
								desc.Aliasing.pResourceAfter = aft->GetResource();

								if (flush || m_barriersToFlush == 16)
								{
									m_cmdList.V0->ResourceBarrier(m_barriersToFlush, m_barrierBuffer);
									m_barriersToFlush = 0;
								}
							}
							SAVE_RELEASE(aft);
							SAVE_RELEASE(bef);
						}
					}
					void CLOAK_CALL_THIS CopyContext::CopyResource(In API::Rendering::IResource* dst, In ID3D12Resource* src)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						if (CloakDebugCheckOK(dst != nullptr && src != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							Resource* dstRc = nullptr;
							HRESULT hRet = dst->QueryInterface(CE_QUERY_ARGS(&dstRc));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								dstRc->RegisterUsage(m_queue);
								m_usedResources.push(dstRc);
								API::Helper::Lock lock(dstRc->GetSyncSection());
								TransitionResource(dst, API::Rendering::STATE_COPY_DEST, true);
								m_cmdList.V0->CopyResource(dstRc->GetResource(), src);
							}
							SAVE_RELEASE(dstRc);
						}
					}
					void CLOAK_CALL_THIS CopyContext::CopyTextureRegion(In ID3D12Resource* dst, In ID3D12Resource* src, In_reads(numSubresource) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts, In UINT firstSubresource, In UINT numSubresource)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						D3D12_TEXTURE_COPY_LOCATION dstLoc;
						dstLoc.pResource = dst;
						dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

						D3D12_TEXTURE_COPY_LOCATION srcLoc;
						srcLoc.pResource = src;
						srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
						for (UINT a = 0; a < numSubresource; a++)
						{
							srcLoc.SubresourceIndex = firstSubresource + a;
							dstLoc.PlacedFootprint = layouts[a];
							m_cmdList.V0->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
						}
					}
					void CLOAK_CALL_THIS CopyContext::FlushResourceBarriers()
					{
						if (m_barriersToFlush > 0)
						{
							m_cmdList.V0->ResourceBarrier(m_barriersToFlush, m_barrierBuffer);
							m_barriersToFlush = 0;
						}
					}
					uint64_t CLOAK_CALL_THIS CopyContext::GetLastFence() const { return m_lastFence; }
					ISingleQueue* CLOAK_CALL_THIS CopyContext::GetQueue() const { return m_queue; }
					void CLOAK_CALL_THIS CopyContext::WaitForAdapter(In IAdapter* queue, In uint64_t value)
					{
						if (queue != m_queue && queue->IsFenceComplete(value) == false)
						{
							API::Helper::Lock lock(m_sync);
							auto f = m_waitingQueues.find(queue);
							if (f == m_waitingQueues.end()) { m_waitingQueues[queue] = value; }
							else { f->second = max(f->second, value); }
						}
					}
					void CLOAK_CALL_THIS CopyContext::CleanAndDiscard(In uint64_t fence)
					{
						const QUEUE_TYPE queueT = m_queue->GetType();
						for (size_t a = 0; a < ARRAYSIZE(m_dynAlloc); a++) { m_dynAlloc[a]->Clean(fence, queueT); }
						m_readBack->Clean(fence, queueT);
						m_queue->DiscardAllocator(m_curAlloc, fence);
						m_curAlloc = nullptr;
						m_blockedTransition = false;
						m_lastFence = fence;
						for (const auto& a : m_usedQueries) { a->Clear(fence); }
						m_usedQueries.clear();
						for (const auto& a : m_usedResources) { a->UnregisterUsage(fence); }
						m_usedResources.clear();
						m_pageAlloc.Clear();
					}
					void CLOAK_CALL_THIS CopyContext::BlockTransitions(In bool block)
					{
						m_blockedTransition = block;
					}
					void CLOAK_CALL_THIS CopyContext::Reset()
					{
						if (m_curAlloc == nullptr)
						{
							Manager* man = nullptr;
							if (SUCCEEDED(m_manager->QueryInterface(CE_QUERY_ARGS(&man))))
							{
								m_curAlloc = m_queue->RequestAllocator();
								HRESULT hRet = m_cmdList.V0->Reset(m_curAlloc, nullptr);
								CLOAK_ASSUME(SUCCEEDED(hRet));
								man->Release();
							}
							m_barriersToFlush = 0;
						}
					}
					uint64_t CLOAK_CALL_THIS CopyContext::Finish(In_opt bool wait, In_opt bool executeDirect)
					{
						API::Helper::Lock lock(m_sync);
						uint64_t res = 0;
						for (const auto& a : m_waitingQueues) { m_queue->WaitForAdapter(a.first, a.second); }
						m_waitingQueues.clear();
						FlushResourceBarriers();
						if (m_curAlloc != nullptr)
						{
							HRESULT hRet = m_cmdList.V0->Close();
							if (CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_LIST_CLOSE, true))
							{
								if (CloakDebugCheckOK(wait == false || executeDirect == true, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
								{
									if (executeDirect)
									{
										uint64_t fence = m_queue->ExecuteList(m_cmdList);
										CleanAndDiscard(fence);
										if (wait) { m_queue->WaitForFence(fence); }
										res = fence;
									}
									else { m_queue->ExecuteListDeffered(m_cmdList, this); }
								}
							}
						}
						return res;
					}
					Success(return == true) bool CLOAK_CALL_THIS CopyContext::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Rendering::ICopyContext)) { *ptr = (API::Rendering::ICopyContext*)this; return true; }
						else if (riid == __uuidof(Impl::Rendering::ICopyContext)) { *ptr = (Impl::Rendering::ICopyContext*)this; return true; }
						else if (riid == __uuidof(CopyContext)) { *ptr = (CopyContext*)this; return true; }
						return SavePtr::iQueryInterface(riid, ptr);
					}

					CLOAK_CALL Context::Context(In SingleQueue* queue, In IManager* manager) : CopyContext(queue, manager), m_2DContext(this, &m_pageAlloc), m_3DContext(this, &m_pageAlloc)
					{
						IQueue* q = m_queue->GetParent();
						m_dynHeap = new DynamicDescriptorHeap(this, q);
						m_curPSO = nullptr;
						m_curRoot = nullptr;
						m_curIndexBuffer = nullptr;
						for (size_t a = 0; a < ARRAYSIZE(m_curVertexBuffer); a++) { m_curVertexBuffer[a] = nullptr; }
						for (size_t a = 0; a < ARRAYSIZE(m_curRV); a++) { m_curRV[a].Type = RVCacheType::NONE; }
						for (size_t a = 0; a < ARRAYSIZE(m_curRTV); a++) { m_curRTV[a] = nullptr; }
						m_curDSV = nullptr;
						m_curRTVCount = 0;
						m_curDSVAccess = API::Rendering::DSV_ACCESS::NONE;
						m_rootSig = nullptr;
						m_lastSamplerMipLevel = SAMPLER_MAX_MIP_CLAMP;
						m_useTessellation = false;
						m_topology = API::Rendering::TOPOLOGY_UNDEFINED;
						m_usage = Usage::NONE;
						m_updatedDynHeap = true;
#ifdef _DEBUG
						m_dynHeap->SetSourceFile(std::string(__FILE__) + " @ " + std::to_string(__LINE__));
#endif
						for (size_t a = 0; a < ARRAYSIZE(m_currentHeaps); a++) { m_currentHeaps[a] = nullptr; }
						DEBUG_NAME(Context);
					}
					CLOAK_CALL Context::~Context()
					{
						SAVE_RELEASE(m_rootSig);
						SAVE_RELEASE(m_dynHeap);
						RELEASE(m_curPSO);
						RELEASE(m_curRoot);
					}
					
					void CLOAK_CALL_THIS Context::ClearUAV(In API::Rendering::IStructuredBuffer* target)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						if (target != nullptr)
						{
							GraphicBuffer* gb = nullptr;
							if (SUCCEEDED(target->QueryInterface(CE_QUERY_ARGS(&gb))))
							{
								TransitionResource(target, API::Rendering::STATE_UNORDERED_ACCESS, true);
								D3D12_CPU_DESCRIPTOR_HANDLE cpuUav = Casting::CastForward(target->GetUAV().CPU);
								D3D12_GPU_DESCRIPTOR_HANDLE uav = m_dynHeap->UploadDirect(cpuUav);
								m_updatedDynHeap = true;
								m_cmdList.V0->ClearUnorderedAccessViewUint(uav, cpuUav, gb->GetResource(), DEFAULT_CLEAR_COLOR, 0, nullptr);
								gb->Release();
							}
						}
					}
					void CLOAK_CALL_THIS Context::ClearUAV(In API::Rendering::IByteAddressBuffer* target)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						if (target != nullptr)
						{
							GraphicBuffer* gb = nullptr;
							if (SUCCEEDED(target->QueryInterface(CE_QUERY_ARGS(&gb))))
							{
								TransitionResource(target, API::Rendering::STATE_UNORDERED_ACCESS, true);
								D3D12_CPU_DESCRIPTOR_HANDLE cpuUav = Casting::CastForward(target->GetUAV().CPU);
								D3D12_GPU_DESCRIPTOR_HANDLE uav = m_dynHeap->UploadDirect(cpuUav);
								m_updatedDynHeap = true;
								m_cmdList.V0->ClearUnorderedAccessViewUint(uav, cpuUav, gb->GetResource(), DEFAULT_CLEAR_COLOR, 0, nullptr);
								gb->Release();
							}
						}
					}
					void CLOAK_CALL_THIS Context::ClearUAV(In API::Rendering::IColorBuffer* target)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						if (target != nullptr)
						{
							ColorBuffer* gb = nullptr;
							if (SUCCEEDED(target->QueryInterface(CE_QUERY_ARGS(&gb))))
							{
								TransitionResource(target, API::Rendering::STATE_UNORDERED_ACCESS, true);
								D3D12_CPU_DESCRIPTOR_HANDLE cpuUav = Casting::CastForward(target->GetUAV().CPU);
								D3D12_GPU_DESCRIPTOR_HANDLE uav = m_dynHeap->UploadDirect(cpuUav);
								m_updatedDynHeap = true;
								D3D12_RECT clearRect = { 0,0, static_cast<LONG>(target->GetWidth()), static_cast<LONG>(target->GetHeight()) };
								const API::Helper::Color::RGBA& rgba = target->GetClearColor();
								FLOAT clearColor[4] = { rgba.R,rgba.G,rgba.B,rgba.A };

								m_cmdList.V0->ClearUnorderedAccessViewFloat(uav, cpuUav, gb->GetResource(), clearColor, 1, &clearRect);
								gb->Release();
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetConstants(In UINT rootIndex, In UINT numConstants, In_reads(numConstants) const API::Rendering::DWParam* constants)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						m_curRV[rootIndex].Type = RVCacheType::NONE;
						switch (m_usage)
						{
							case Usage::GRAPHIC:
								m_cmdList.V0->SetGraphicsRoot32BitConstants(rootIndex, numConstants, constants, 0);
								break;
							case Usage::COMPUTE:
								m_cmdList.V0->SetComputeRoot32BitConstants(rootIndex, numConstants, constants, 0);
								break;
							default:
								break;
						}
					}
					void CLOAK_CALL_THIS Context::SetConstants(In UINT rootIndex, In const API::Rendering::DWParam& X)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						m_curRV[rootIndex].Type = RVCacheType::NONE;
						switch (m_usage)
						{
							case Usage::GRAPHIC:
								m_cmdList.V0->SetGraphicsRoot32BitConstant(rootIndex, X.UIntVal, 0);
								break;
							case Usage::COMPUTE:
								m_cmdList.V0->SetComputeRoot32BitConstant(rootIndex, X.UIntVal, 0);
								break;
							default:
								break;
						}
					}
					void CLOAK_CALL_THIS Context::SetConstants(In UINT rootIndex, In const API::Rendering::DWParam& X, In const API::Rendering::DWParam& Y)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						m_curRV[rootIndex].Type = RVCacheType::NONE;
						API::Rendering::DWParam p[] = { X,Y };
						switch (m_usage)
						{
							case Usage::GRAPHIC:
								m_cmdList.V0->SetGraphicsRoot32BitConstants(rootIndex, ARRAYSIZE(p), p, 0);
								break;
							case Usage::COMPUTE:
								m_cmdList.V0->SetComputeRoot32BitConstants(rootIndex, ARRAYSIZE(p), p, 0);
								break;
							default:
								break;
						}
					}
					void CLOAK_CALL_THIS Context::SetConstants(In UINT rootIndex, In const API::Rendering::DWParam& X, In const API::Rendering::DWParam& Y, In const API::Rendering::DWParam& Z)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						m_curRV[rootIndex].Type = RVCacheType::NONE;
						API::Rendering::DWParam p[] = { X,Y,Z };
						switch (m_usage)
						{
							case Usage::GRAPHIC:
								m_cmdList.V0->SetGraphicsRoot32BitConstants(rootIndex, ARRAYSIZE(p), p, 0);
								break;
							case Usage::COMPUTE:
								m_cmdList.V0->SetComputeRoot32BitConstants(rootIndex, ARRAYSIZE(p), p, 0);
								break;
							default:
								break;
						}
					}
					void CLOAK_CALL_THIS Context::SetConstants(In UINT rootIndex, In const API::Rendering::DWParam& X, In const API::Rendering::DWParam& Y, In const API::Rendering::DWParam& Z, In const API::Rendering::DWParam& W)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						m_curRV[rootIndex].Type = RVCacheType::NONE;
						API::Rendering::DWParam p[] = { X,Y,Z,W };
						switch (m_usage)
						{
							case Usage::GRAPHIC:
								m_cmdList.V0->SetGraphicsRoot32BitConstants(rootIndex, ARRAYSIZE(p), p, 0);
								break;
							case Usage::COMPUTE:
								m_cmdList.V0->SetComputeRoot32BitConstants(rootIndex, ARRAYSIZE(p), p, 0);
								break;
							default:
								break;
						}
					}
					void CLOAK_CALL_THIS Context::SetConstantsByPos(In UINT rootIndex, In const API::Rendering::DWParam& X, In UINT pos)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						m_curRV[rootIndex].Type = RVCacheType::NONE;
						switch (m_usage)
						{
							case Usage::GRAPHIC:
								m_cmdList.V0->SetGraphicsRoot32BitConstant(rootIndex, X.UIntVal, pos);
								break;
							case Usage::COMPUTE:
								m_cmdList.V0->SetComputeRoot32BitConstant(rootIndex, X.UIntVal, pos);
								break;
							default:
								break;
						}
					}
					void CLOAK_CALL_THIS Context::SetConstantBuffer(In UINT rootIndex, In API::Rendering::IGraphicBuffer* cbv)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						if (CloakDebugCheckOK(cbv != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							const API::Rendering::GPU_ADDRESS adr = cbv->GetGPUAddress().ptr;
							if (m_curRV[rootIndex].Type != RVCacheType::ADDRESS || m_curRV[rootIndex].Address != adr)
							{
								m_curRV[rootIndex].Type = RVCacheType::ADDRESS;
								m_curRV[rootIndex].Address = adr;

								TransitionResource(cbv, API::Rendering::STATE_VERTEX_AND_CONSTANT_BUFFER, true);

								switch (m_usage)
								{
									case Usage::GRAPHIC:
										m_cmdList.V0->SetGraphicsRootConstantBufferView(rootIndex, static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(adr));
										break;
									case Usage::COMPUTE:
										m_cmdList.V0->SetComputeRootConstantBufferView(rootIndex, static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(adr));
										break;
									default:
										break;
								}
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetConstantBufferByAddress(In UINT rootIndex, In API::Rendering::GPU_ADDRESS cbv)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						if (m_curRV[rootIndex].Type != RVCacheType::ADDRESS || m_curRV[rootIndex].Address != cbv)
						{
							m_curRV[rootIndex].Type = RVCacheType::ADDRESS;
							m_curRV[rootIndex].Address = cbv;

							switch (m_usage)
							{
								case Usage::GRAPHIC:
									m_cmdList.V0->SetGraphicsRootConstantBufferView(rootIndex, static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(cbv));
									break;
								case Usage::COMPUTE:
									m_cmdList.V0->SetComputeRootConstantBufferView(rootIndex, static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(cbv));
									break;
								default:
									break;
							}
						}
					}
					void CLOAK_CALL_THIS Context::BeginRenderPass(In UINT numTargets, In_reads(numTargets) const API::Rendering::RENDER_PASS_RTV* RTVs, In const API::Rendering::RENDER_PASS_DSV* DSV, In API::Rendering::RENDER_PASS_FLAG flags)
					{
						if (m_cmdList.V4 != nullptr)
						{
							if (m_inRenderPass == true) { m_cmdList.V4->EndRenderPass(); }

							D3D12_RENDER_PASS_RENDER_TARGET_DESC* rtv = nullptr;
							if (numTargets > 0)
							{
								rtv = reinterpret_cast<D3D12_RENDER_PASS_RENDER_TARGET_DESC*>(m_pageAlloc.Allocate(numTargets * sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC)));
								for (UINT a = 0; a < numTargets; a++)
								{
									API::Rendering::ResourceView rrv = RTVs[a].Buffer->GetRTV();
									rtv[a].cpuDescriptor = Casting::CastForward(rrv.CPU);
									rtv[a].BeginningAccess.Type = RenderPassBeginAccess(RTVs[a].BeginAccess);
									if (RTVs[a].BeginAccess == API::Rendering::RENDER_PASS_ACCESS_CLEAR)
									{
										rtv[a].BeginningAccess.Clear.ClearValue.Format = GetClearFormat(Casting::CastForward(RTVs[a].Buffer->GetFormat()), true);
										const API::Helper::Color::RGBA cc = RTVs[a].Buffer->GetClearColor();
										rtv[a].BeginningAccess.Clear.ClearValue.Color[0] = cc.R;
										rtv[a].BeginningAccess.Clear.ClearValue.Color[1] = cc.G;
										rtv[a].BeginningAccess.Clear.ClearValue.Color[2] = cc.B;
										rtv[a].BeginningAccess.Clear.ClearValue.Color[3] = cc.A;
									}
									rtv[a].EndingAccess.Type = RenderPassEndAccess(RTVs[a].EndAccess);
									TransitionResource(RTVs[a].Buffer.Get(), API::Rendering::STATE_RENDER_TARGET);
									IResource* cb = nullptr;
									if (SUCCEEDED(RTVs[a].Buffer->QueryInterface(CE_QUERY_ARGS(&cb))))
									{
										cb->RegisterUsage(m_queue);
										m_usedResources.push(cb);
										cb->Release();
									}
								}
							}
							if (DSV != nullptr)
							{
								D3D12_RENDER_PASS_DEPTH_STENCIL_DESC dsv;
								API::Rendering::ResourceView drv = DSV->Buffer->GetDSV(DSV->DSVAccess);
								dsv.cpuDescriptor = Casting::CastForward(drv.CPU);
								dsv.DepthBeginningAccess.Type = RenderPassBeginAccess(DSV->Depth.BeginAccess);
								if (DSV->Depth.BeginAccess == API::Rendering::RENDER_PASS_ACCESS_CLEAR)
								{
									dsv.DepthBeginningAccess.Clear.ClearValue.Format = GetClearFormat(Casting::CastForward(DSV->Buffer->GetFormat()), false);
									dsv.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = DSV->Buffer->GetClearDepth();
									dsv.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = DSV->Buffer->GetClearStencil();
								}
								dsv.DepthEndingAccess.Type = RenderPassEndAccess(DSV->Depth.EndAccess);
								dsv.StencilBeginningAccess.Type = RenderPassBeginAccess(DSV->Stencil.BeginAccess);
								if (DSV->Stencil.BeginAccess == API::Rendering::RENDER_PASS_ACCESS_CLEAR)
								{
									dsv.StencilBeginningAccess.Clear.ClearValue.Format = GetClearFormat(Casting::CastForward(DSV->Buffer->GetFormat()), false);
									dsv.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Depth = DSV->Buffer->GetClearDepth();
									dsv.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = DSV->Buffer->GetClearStencil();
								}
								dsv.StencilEndingAccess.Type = RenderPassEndAccess(DSV->Stencil.EndAccess);

								if ((DSV->DSVAccess & API::Rendering::DSV_ACCESS::READ_ONLY_DEPTH) == API::Rendering::DSV_ACCESS::READ_ONLY_DEPTH) { TransitionResource(DSV->Buffer.Get(), API::Rendering::STATE_DEPTH_READ); }
								else { TransitionResource(DSV->Buffer.Get(), API::Rendering::STATE_DEPTH_WRITE); }
								IResource* cb = nullptr;
								if (SUCCEEDED(DSV->Buffer->QueryInterface(CE_QUERY_ARGS(&cb))))
								{
									cb->RegisterUsage(m_queue);
									m_usedResources.push(cb);
									cb->Release();
								}

								m_cmdList.V4->BeginRenderPass(numTargets, rtv, &dsv, Casting::CastForward(flags));
							}
							else
							{
								m_cmdList.V4->BeginRenderPass(numTargets, rtv, nullptr, Casting::CastForward(flags));
							}

							m_inRenderPass = true;
							m_pageAlloc.Free(rtv);
						}
						else
						{
							if (DSV != nullptr)
							{
								if (DSV->Depth.BeginAccess == API::Rendering::RENDER_PASS_ACCESS_CLEAR)
								{
									if (DSV->Stencil.BeginAccess == API::Rendering::RENDER_PASS_ACCESS_CLEAR) { ClearDepthAndStencil(DSV->Buffer.Get()); }
									else { ClearDepth(DSV->Buffer.Get()); }
								}
								else if (DSV->Stencil.BeginAccess == API::Rendering::RENDER_PASS_ACCESS_CLEAR)
								{
									ClearStencil(DSV->Buffer.Get());
								}
							}
							if (numTargets > 0)
							{
								API::Rendering::IColorBuffer** rtvs = reinterpret_cast<API::Rendering::IColorBuffer**>(m_pageAlloc.Allocate(numTargets * sizeof(API::Rendering::IColorBuffer*)));
								for (UINT a = 0; a < numTargets; a++)
								{
									if (RTVs[a].BeginAccess == API::Rendering::RENDER_PASS_ACCESS_CLEAR) { ClearColor(RTVs[a].Buffer.Get()); }
									rtvs[a] = RTVs[a].Buffer.Get();
								}
								if (DSV != nullptr) { SetRenderTargets(numTargets, rtvs, DSV->Buffer.Get(), DSV->DSVAccess); }
								else { SetRenderTargets(numTargets, rtvs); }
								m_pageAlloc.Free(rtvs);
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetWorker(In IRenderWorker* worker, In size_t passID, In API::Files::MaterialType matType, In API::Rendering::DRAW_TYPE drawType, In const API::Global::Math::Vector& camPos, In float camViewDist, In size_t camID, In API::Files::LOD maxLOD)
					{
						m_3DContext.SetWorker(worker, passID, matType, drawType, camPos, camViewDist, camID, maxLOD);
						//TODO
					}
					API::Rendering::IPipelineState* CLOAK_CALL_THIS Context::GetCurrentPipelineState() const { return m_curPSO; }
					IRootSignature* CLOAK_CALL_THIS Context::GetCurrentRootSignature() const { return m_rootSig; }
					void CLOAK_CALL_THIS Context::SetDynamicCBV(In UINT rootIndex, In size_t size, In_reads_bytes(size) const void* data)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						m_curRV[rootIndex].Type = RVCacheType::NONE;
						if (CloakDebugCheckOK(data != nullptr && IsAligned(data, 16), API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							DynAlloc cb;
							m_dynAlloc[1]->Allocate(&cb, size, 512);
							memcpy(cb.Data, data, size);
							SetConstantBufferByAddress(rootIndex, cb.GPUAddress);
						}
					}
					void CLOAK_CALL_THIS Context::SetDynamicSRV(In UINT rootIndex, In size_t size, In_reads_bytes(size) const void* data)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						m_curRV[rootIndex].Type = RVCacheType::NONE;
						if (CloakDebugCheckOK(data != nullptr && IsAligned(data, 16), API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							DynAlloc cb;
							m_dynAlloc[1]->Allocate(&cb, size, 512);
							memcpy(cb.Data, data, size);
							switch(m_usage)
							{
								case Usage::GRAPHIC:
									m_cmdList.V0->SetGraphicsRootShaderResourceView(rootIndex, static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(cb.GPUAddress));
									break;
								case Usage::COMPUTE:
									m_cmdList.V0->SetComputeRootShaderResourceView(rootIndex, static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(cb.GPUAddress));
									break;
								default:
									break;
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetSRV(In UINT rootIndex, In const API::Rendering::ResourceView& srv)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						if (srv.HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP)
						{
							if (CloakCheckOK(srv.NodeMask == m_queue->GetNodeMask(), API::Global::Debug::Error::GRAPHIC_WRONG_NODE, false) && (m_curRV[rootIndex].Type != RVCacheType::SRV || m_curRV[rootIndex].View != srv))
							{
								m_curRV[rootIndex].Type = RVCacheType::SRV;
								m_curRV[rootIndex].View = srv;

								D3D12_DESCRIPTOR_HEAP_TYPE ht[1] = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
								ID3D12DescriptorHeap* he[1] = { nullptr };
								DescriptorHeap* heap = m_queue->GetDescriptorHeap(Casting::CastBackward(ht[0]));
								uint8_t detLevel = 0;
								const UINT ts = m_rootSig->GetTableSize(rootIndex);
								CLOAK_ASSUME(ts > 0);
								Impl::Rendering::IResource** trbp = reinterpret_cast<Impl::Rendering::IResource**>(m_pageAlloc.Allocate(ts * sizeof(Impl::Rendering::IResource*)));
								heap->GetOwner(srv, ts, trbp);

								for (UINT a = 0; a < ts; a++)
								{
									IResource* rs = trbp[a];
									if (CloakDebugCheckOK(rs != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
									{
										rs->RegisterUsage(m_queue);
										m_usedResources.push(rs);
										const API::Rendering::RESOURCE_STATE nstate = m_usage == Usage::GRAPHIC ? (API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE | API::Rendering::STATE_PIXEL_SHADER_RESOURCE) : API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE;
										const API::Rendering::RESOURCE_STATE ostate = rs->GetResourceState();
										if ((nstate & ostate) != nstate) { TransitionResource(rs, nstate); }
										const uint8_t ndl = rs->GetDetailLevel();
										CLOAK_ASSUME(ndl < SAMPLER_MAX_MIP_CLAMP);
										detLevel = max(detLevel, ndl);
										rs->Release();
									}
								}
								m_pageAlloc.Free(trbp);

								FlushResourceBarriers();
								if (m_lastSamplerMipLevel != detLevel) { m_rootSig->SetSamplers(detLevel, this, m_queue->GetNodeID() - 1); m_lastSamplerMipLevel = detLevel; }
								he[0] = heap->GetHeap(srv.HeapID);
								CLOAK_ASSUME(he[0] != nullptr);
								SetDescriptorHeaps(ht, he, 1);
								switch(m_usage)
								{
									case Usage::GRAPHIC:
										m_cmdList.V0->SetGraphicsRootDescriptorTable(rootIndex, Casting::CastForward(srv.GPU));
										break;
									case Usage::COMPUTE:
										m_cmdList.V0->SetComputeRootDescriptorTable(rootIndex, Casting::CastForward(srv.GPU));
										break;
									default:
										break;
								}
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetUAV(In UINT rootIndex, In const API::Rendering::ResourceView& uav)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						if (uav.HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP)
						{
							if (CloakCheckOK(uav.NodeMask == m_queue->GetNodeMask(), API::Global::Debug::Error::GRAPHIC_WRONG_NODE, false) && (m_curRV[rootIndex].Type != RVCacheType::UAV || m_curRV[rootIndex].View != uav))
							{
								m_curRV[rootIndex].Type = RVCacheType::UAV;
								m_curRV[rootIndex].View = uav;

								D3D12_DESCRIPTOR_HEAP_TYPE ht[1] = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
								ID3D12DescriptorHeap* he[1] = { nullptr };
								DescriptorHeap* heap = m_queue->GetDescriptorHeap(Casting::CastBackward(ht[0]));
								const UINT ts = m_rootSig->GetTableSize(rootIndex);
								CLOAK_ASSUME(ts > 0);
								Impl::Rendering::IResource** trbp = reinterpret_cast<Impl::Rendering::IResource**>(m_pageAlloc.Allocate(ts * sizeof(Impl::Rendering::IResource*)));
								heap->GetOwner(uav, ts, trbp);
								for (UINT a = 0; a < ts; a++)
								{
									IResource* rs = trbp[a];
									if (CloakDebugCheckOK(rs != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
									{
										rs->RegisterUsage(m_queue);
										m_usedResources.push(rs);
										const API::Rendering::RESOURCE_STATE nstate = API::Rendering::STATE_UNORDERED_ACCESS;
										API::Rendering::RESOURCE_STATE ostate = rs->GetResourceState();
										if ((nstate & ostate) != nstate) { TransitionResource(rs, nstate); }
										StructuredBuffer* sb = nullptr;
										HRESULT hr = rs->QueryInterface(CE_QUERY_ARGS(&sb));
										if (SUCCEEDED(hr))
										{
											API::Rendering::IByteAddressBuffer* counter = sb->GetCounterBuffer();
											if (counter != nullptr)
											{
												API::Rendering::RESOURCE_STATE ostate = rs->GetResourceState();
												if ((nstate & ostate) != nstate) { TransitionResource(counter, API::Rendering::STATE_UNORDERED_ACCESS); }
												counter->Release();
											}
											sb->Release();
										}
										rs->Release();
									}
								}
								m_pageAlloc.Free(trbp);

								FlushResourceBarriers();
								he[0] = heap->GetHeap(uav.HeapID);
								CLOAK_ASSUME(he[0] != nullptr);
								SetDescriptorHeaps(ht, he, 1);
								switch(m_usage)
								{
									case Usage::GRAPHIC:
										m_cmdList.V0->SetGraphicsRootDescriptorTable(rootIndex, Casting::CastForward(uav.GPU));
										break;
									case Usage::COMPUTE:
										m_cmdList.V0->SetComputeRootDescriptorTable(rootIndex, Casting::CastForward(uav.GPU));
										break;
									default:
										break;
								}
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetCBV(In UINT rootIndex, In const API::Rendering::ResourceView& cbv)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						if (cbv.HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP)
						{
							if (CloakCheckOK(cbv.NodeMask == m_queue->GetNodeMask(), API::Global::Debug::Error::GRAPHIC_WRONG_NODE, false) && (m_curRV[rootIndex].Type != RVCacheType::CBV || m_curRV[rootIndex].View != cbv))
							{
								m_curRV[rootIndex].Type = RVCacheType::CBV;
								m_curRV[rootIndex].View = cbv;

								D3D12_DESCRIPTOR_HEAP_TYPE ht[1] = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
								ID3D12DescriptorHeap* he[1] = { nullptr };
								DescriptorHeap* heap = m_queue->GetDescriptorHeap(Casting::CastBackward(ht[0]));
								const UINT ts = m_rootSig->GetTableSize(rootIndex);
								CLOAK_ASSUME(ts > 0);
								Impl::Rendering::IResource** trbp = reinterpret_cast<Impl::Rendering::IResource**>(m_pageAlloc.Allocate(ts * sizeof(Impl::Rendering::IResource*)));
								heap->GetOwner(cbv, ts, trbp);
								for (UINT a = 0; a < ts; a++)
								{
									IResource* rs = trbp[a];
									if (CloakDebugCheckOK(rs != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
									{
										rs->RegisterUsage(m_queue);
										m_usedResources.push(rs);
										const API::Rendering::RESOURCE_STATE nstate = m_usage == Usage::GRAPHIC ? (API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE | API::Rendering::STATE_PIXEL_SHADER_RESOURCE) : API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE;
										const API::Rendering::RESOURCE_STATE ostate = rs->GetResourceState();
										if ((nstate & ostate) != nstate) { TransitionResource(rs, nstate); }
										rs->Release();
									}
								}
								m_pageAlloc.Free(trbp);
								FlushResourceBarriers();
								he[0] = heap->GetHeap(cbv.HeapID);
								CLOAK_ASSUME(he[0] != nullptr);
								SetDescriptorHeaps(ht, he, 1);
								switch(m_usage)
								{
									case Usage::GRAPHIC:
										m_cmdList.V0->SetGraphicsRootDescriptorTable(rootIndex, Casting::CastForward(cbv.GPU));
										break;
									case Usage::COMPUTE:
										m_cmdList.V0->SetComputeRootDescriptorTable(rootIndex, Casting::CastForward(cbv.GPU));
										break;
									default:
										break;
								}
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetBufferSRV(In UINT rootIndex, In API::Rendering::IGraphicBuffer* srv)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						if (CloakDebugCheckOK(srv != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							const API::Rendering::GPU_ADDRESS adr = srv->GetGPUAddress().ptr;
							if (m_curRV[rootIndex].Type != RVCacheType::ADDRESS || m_curRV[rootIndex].Address != adr)
							{
								m_curRV[rootIndex].Type = RVCacheType::ADDRESS;
								m_curRV[rootIndex].Address = adr;

								GraphicBuffer* gb = nullptr;
								HRESULT hRet = srv->QueryInterface(CE_QUERY_ARGS(&gb));
								if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
								{
									gb->RegisterUsage(m_queue);
									m_usedResources.push(gb);

									const API::Rendering::RESOURCE_STATE nstate = m_usage == Usage::GRAPHIC ? (API::Rendering::STATE_PIXEL_SHADER_RESOURCE | API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE) : API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE;
									const API::Rendering::RESOURCE_STATE ostate = gb->GetResourceState();
									if ((nstate & ostate) != nstate) { TransitionResource(gb, nstate); }
									switch (m_usage)
									{
										case Usage::GRAPHIC:
											m_cmdList.V0->SetGraphicsRootShaderResourceView(rootIndex, static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(adr));
											break;
										case Usage::COMPUTE:
											m_cmdList.V0->SetComputeRootShaderResourceView(rootIndex, static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(adr));
											break;
										default:
											break;
									}
									gb->Release();
								}
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetBufferUAV(In UINT rootIndex, In API::Rendering::IGraphicBuffer* uav)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						if (CloakDebugCheckOK(uav != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							const API::Rendering::GPU_ADDRESS adr = uav->GetGPUAddress().ptr;
							if (m_curRV[rootIndex].Type != RVCacheType::ADDRESS || m_curRV[rootIndex].Address != adr)
							{
								m_curRV[rootIndex].Type = RVCacheType::ADDRESS;
								m_curRV[rootIndex].Address = adr;

								GraphicBuffer* gb = nullptr;
								HRESULT hRet = uav->QueryInterface(CE_QUERY_ARGS(&gb));
								if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
								{
									gb->RegisterUsage(m_queue);
									m_usedResources.push(gb);

									const API::Rendering::RESOURCE_STATE nstate = API::Rendering::STATE_UNORDERED_ACCESS;
									const API::Rendering::RESOURCE_STATE ostate = gb->GetResourceState();
									if ((nstate & ostate) != nstate) { TransitionResource(gb, nstate); }
									switch (m_usage)
									{
										case Usage::GRAPHIC:
											m_cmdList.V0->SetGraphicsRootUnorderedAccessView(rootIndex, static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(adr));
											break;
										case Usage::COMPUTE:
											m_cmdList.V0->SetComputeRootUnorderedAccessView(rootIndex, static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(adr));
											break;
										default:
											break;
									}
									gb->Release();
								}
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetDescriptorTable(In UINT rootIndex, In const API::Rendering::GPU_DESCRIPTOR_HANDLE& gpuAddress)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						if (m_curRV[rootIndex].Type != RVCacheType::ADDRESS || m_curRV[rootIndex].Address != gpuAddress.ptr)
						{
							m_curRV[rootIndex].Type = RVCacheType::ADDRESS;
							m_curRV[rootIndex].Address = gpuAddress.ptr;

							switch(m_usage)
							{
								case Usage::GRAPHIC:
									m_cmdList.V0->SetGraphicsRootDescriptorTable(rootIndex, Casting::CastForward(gpuAddress));
									break;
								case Usage::COMPUTE:
									m_cmdList.V0->SetComputeRootDescriptorTable(rootIndex, Casting::CastForward(gpuAddress));
									break;
								default:
									break;
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetDescriptorTable(In UINT rootIndex, In UINT offset, In_reads(count) const API::Rendering::ResourceView* views, In UINT count)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						m_curRV[rootIndex].Type = RVCacheType::NONE;
						API::Rendering::CPU_DESCRIPTOR_HANDLE* handles = reinterpret_cast<API::Rendering::CPU_DESCRIPTOR_HANDLE*>(m_pageAlloc.Allocate(sizeof(API::Rendering::CPU_DESCRIPTOR_HANDLE)*count));
						IDescriptorHeap* heap = m_queue->GetDescriptorHeap(HEAP_CBV_SRV_UAV_DYNAMIC);
						const API::Rendering::RESOURCE_STATE nstate = m_usage == Usage::GRAPHIC ? (API::Rendering::STATE_PIXEL_SHADER_RESOURCE | API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE) : (API::Rendering::STATE_NON_PIXEL_SHADER_RESOURCE);
						for (UINT a = 0; a < count; a++)
						{
							handles[a] = views[a].CPU;
							IResource* rs = nullptr;
							heap->GetOwner(views[a], 1, &rs);
							if (CloakDebugCheckOK(rs != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								rs->RegisterUsage(m_queue);
								m_usedResources.push(rs);
								const API::Rendering::RESOURCE_STATE ostate = rs->GetResourceState();
								if ((nstate & ostate) != nstate) { TransitionResource(rs, nstate); }
								rs->Release();
							}
						}
						FlushResourceBarriers();
						switch(m_usage)
						{
							case Usage::GRAPHIC:
								m_updatedDynHeap = true;
								m_dynHeap->SetGraphicDescriptorHandles(rootIndex, offset, count, handles);
								break;
							case Usage::COMPUTE:
								m_updatedDynHeap = true;
								m_dynHeap->SetComputeDescriptorHandles(rootIndex, offset, count, handles);
								break;
							default:
								break;
						}
						m_pageAlloc.Free(handles);
					}

					void CLOAK_CALL_THIS Context::SetRenderTargets(In UINT numTargets, In_reads(numTargets) API::Rendering::IColorBuffer** RTVs, In_opt API::Rendering::IDepthBuffer* DSV, In_opt API::Rendering::DSV_ACCESS access)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						CLOAK_ASSUME(numTargets <= API::Rendering::MAX_RENDER_TARGET);
						if (m_usage == Usage::GRAPHIC)
						{
							if (numTargets != m_curRTVCount || m_curDSV != DSV || m_curDSVAccess != access)
							{
								goto rtv_changed;
							}
							for (size_t a = 0; a < m_curRTVCount; a++)
							{
								if (m_curRTV[a] != RTVs[a]) { goto rtv_changed; }
							}
							return; //Nothing changed
						rtv_null:
							CloakError(API::Global::Debug::Error::ILLEGAL_ARGUMENT, false);
							return;

						rtv_changed:
							m_curRTVCount = numTargets;
							m_curDSV = DSV;
							for (size_t a = 0; a < numTargets; a++) { m_curRTV[a] = RTVs[a]; }
							m_curDSVAccess = access;

							D3D12_CPU_DESCRIPTOR_HANDLE rtv[API::Rendering::MAX_RENDER_TARGET];
							D3D12_CPU_DESCRIPTOR_HANDLE dsv;
							ZeroMemory(rtv, API::Rendering::MAX_RENDER_TARGET * sizeof(D3D12_CPU_DESCRIPTOR_HANDLE));
							for (size_t a = 0; a < numTargets; a++)
							{
								if (RTVs[a] == nullptr) 
								{ goto rtv_null; }
								API::Rendering::ResourceView rrv = RTVs[a]->GetRTV();
								Casting::CastForward(rrv.CPU, &rtv[a]);
								if (rtv[a].ptr == 0) 
								{ goto rtv_null; }
							}
							if (DSV != nullptr)
							{
								API::Rendering::ResourceView drv = DSV->GetDSV(access);
								Casting::CastForward(drv.CPU, &dsv);
								if (dsv.ptr == 0) 
								{ goto rtv_null; }
							}

							//We now can be sure that the rtvs can be used:
							for (size_t a = 0; a < numTargets; a++)
							{
								TransitionResource(RTVs[a], API::Rendering::STATE_RENDER_TARGET);
								IResource* cb = nullptr;
								if (SUCCEEDED(RTVs[a]->QueryInterface(CE_QUERY_ARGS(&cb))))
								{
									cb->RegisterUsage(m_queue);
									m_usedResources.push(cb);
									cb->Release();
								}
							}
							if (DSV != nullptr)
							{
								IResource* db = nullptr;
								if (SUCCEEDED(DSV->QueryInterface(CE_QUERY_ARGS(&db))))
								{
									db->RegisterUsage(m_queue);
									m_usedResources.push(db);
									db->Release();
								}

								if ((access & API::Rendering::DSV_ACCESS::READ_ONLY_DEPTH) == API::Rendering::DSV_ACCESS::READ_ONLY_DEPTH) { TransitionResource(DSV, API::Rendering::STATE_DEPTH_READ); }
								else { TransitionResource(DSV, API::Rendering::STATE_DEPTH_WRITE); }

								FlushResourceBarriers();
								m_cmdList.V0->OMSetRenderTargets(numTargets, rtv, FALSE, &dsv);
							}
							else
							{
								FlushResourceBarriers();
								m_cmdList.V0->OMSetRenderTargets(numTargets, rtv, FALSE, nullptr);
							}
						}
					}
					void CLOAK_CALL_THIS Context::BeginRenderPass(In UINT numTargets, In_reads(numTargets) const API::Rendering::RENDER_PASS_RTV* RTVs, In const API::Rendering::RENDER_PASS_DSV& DSV, In_opt API::Rendering::RENDER_PASS_FLAG flags)
					{
						BeginRenderPass(numTargets, RTVs, &DSV, flags);
					}
					void CLOAK_CALL_THIS Context::BeginRenderPass(In UINT numTargets, In_reads(numTargets) const API::Rendering::RENDER_PASS_RTV* RTVs, In_opt API::Rendering::RENDER_PASS_FLAG flags)
					{
						BeginRenderPass(numTargets, RTVs, nullptr, flags);
					}
					void CLOAK_CALL_THIS Context::BeginRenderPass(In const API::Rendering::RENDER_PASS_DSV& DSV, In_opt API::Rendering::RENDER_PASS_FLAG flags)
					{
						BeginRenderPass(0, nullptr, &DSV, flags);
					}
					void CLOAK_CALL_THIS Context::EndRenderPass()
					{
						if (m_cmdList.V4 != nullptr)
						{
							CLOAK_ASSUME(m_inRenderPass == true);
							m_cmdList.V4->EndRenderPass();
							m_inRenderPass = false;
						}
					}
					void CLOAK_CALL_THIS Context::SetViewport(In const API::Rendering::VIEWPORT& vp)
					{
						SetViewport(1, &vp);
					}
					void CLOAK_CALL_THIS Context::SetViewport(In size_t count, In_reads(count) const API::Rendering::VIEWPORT* vp)
					{
						if (m_usage == Usage::GRAPHIC && CloakDebugCheckOK(count <= API::Rendering::MAX_RENDER_TARGET, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							bool upd = false;
							for (size_t a = 0; a < count; a++)
							{
								D3D12_VIEWPORT v = Casting::CastForward(vp[a]);
								if (std::abs(v.TopLeftX - m_curViewport[a].TopLeftX) > 0.5f || std::abs(v.TopLeftY - m_curViewport[a].TopLeftY) > 0.5f || std::abs(v.Width - m_curViewport[a].Width) > 0.5f || std::abs(v.Height - m_curViewport[a].Height) > 0.5f || std::abs(v.MinDepth - m_curViewport[a].MinDepth) > 1e-3f || std::abs(v.MaxDepth - m_curViewport[a].MaxDepth) > 1e-3f)
								{
									m_curViewport[a] = v;
									upd = true;
								}
							}
							if (upd == true) { m_cmdList.V0->RSSetViewports(static_cast<UINT>(count), m_curViewport); }
						}
					}
					void CLOAK_CALL_THIS Context::SetViewport(In FLOAT x, In FLOAT y, In FLOAT width, In FLOAT height, In_opt FLOAT minDepth, In_opt FLOAT maxDepth)
					{
						if (m_usage == Usage::GRAPHIC)
						{
							D3D12_VIEWPORT v;
							v.Width = width;
							v.Height = height;
							v.TopLeftX = x;
							v.TopLeftY = y;
							v.MinDepth = minDepth;
							v.MaxDepth = maxDepth;
							if (std::abs(v.TopLeftX - m_curViewport[0].TopLeftX) > 0.5f || std::abs(v.TopLeftY - m_curViewport[0].TopLeftY) > 0.5f || std::abs(v.Width - m_curViewport[0].Width) > 0.5f || std::abs(v.Height - m_curViewport[0].Height) > 0.5f || std::abs(v.MinDepth - m_curViewport[0].MinDepth) > 1e-3f || std::abs(v.MaxDepth - m_curViewport[0].MaxDepth) > 1e-3f)
							{
								m_curViewport[0] = v;
								m_cmdList.V0->RSSetViewports(1, &v);
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetScissor(In const API::Rendering::SCISSOR_RECT& scissor)
					{
						SetScissor(1, &scissor);
					}
					void CLOAK_CALL_THIS Context::SetScissor(In size_t count, In_reads(count) const API::Rendering::SCISSOR_RECT* scissor)
					{
						if (m_usage == Usage::GRAPHIC && CloakDebugCheckOK(count <= API::Rendering::MAX_RENDER_TARGET, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							bool upd = false;
							for (size_t a = 0; a < count; a++)
							{
								D3D12_RECT rct;
								rct.left = scissor[a].TopLeftX;
								rct.top = scissor[a].TopLeftY;
								rct.right = scissor[a].TopLeftX + scissor[a].Width;
								rct.bottom = scissor[a].TopLeftY + scissor[a].Height;
								if (m_curScissor[a].left != rct.left || m_curScissor[a].right != rct.right || m_curScissor[a].top != rct.top || m_curScissor[a].bottom != rct.bottom)
								{
									m_curScissor[a] = rct;
									upd = true;
								}
							}
							if (upd == true) { m_cmdList.V0->RSSetScissorRects(static_cast<UINT>(count), m_curScissor); }
						}
					}
					void CLOAK_CALL_THIS Context::SetScissor(In LONG x, In LONG y, In LONG width, In LONG height)
					{
						if (m_usage == Usage::GRAPHIC)
						{
							D3D12_RECT rct;
							rct.left = x;
							rct.top = y;
							rct.right = x + width;
							rct.bottom = y + height;
							if (m_curScissor[0].left != rct.left || m_curScissor[0].right != rct.right || m_curScissor[0].top != rct.top || m_curScissor[0].bottom != rct.bottom)
							{
								m_curScissor[0] = rct;
								m_cmdList.V0->RSSetScissorRects(1, &rct);
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetViewportAndScissor(In const API::Rendering::VIEWPORT& vp)
					{
						SetViewport(vp);
						SetScissor(static_cast<LONG>(vp.TopLeftX), static_cast<LONG>(vp.TopLeftY), static_cast<LONG>(vp.Width), static_cast<LONG>(vp.Height));
					}
					void CLOAK_CALL_THIS Context::SetViewportAndScissor(In const API::Rendering::SCISSOR_RECT& vp)
					{
						SetViewport(static_cast<FLOAT>(vp.TopLeftX), static_cast<FLOAT>(vp.TopLeftY), static_cast<FLOAT>(vp.Width), static_cast<FLOAT>(vp.Height));
						SetScissor(vp);
					}
					void CLOAK_CALL_THIS Context::SetViewportAndScissor(In const API::Rendering::VIEWPORT& vp, In const API::Rendering::SCISSOR_RECT& scissor)
					{
						SetViewport(vp);
						SetScissor(scissor);
					}
					void CLOAK_CALL_THIS Context::SetViewportAndScissor(In size_t count, In_reads(count) const API::Rendering::VIEWPORT* vp)
					{
						if (m_usage == Usage::GRAPHIC && CloakDebugCheckOK(count <= API::Rendering::MAX_RENDER_TARGET, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							SetViewport(count, vp);
							bool upd = false;
							for (size_t a = 0; a < count; a++)
							{
								D3D12_RECT rct;
								rct.left = static_cast<LONG>(vp[a].TopLeftX);
								rct.top = static_cast<LONG>(vp[a].TopLeftY);
								rct.right = static_cast<LONG>(vp[a].TopLeftX + vp[a].Width);
								rct.bottom = static_cast<LONG>(vp[a].TopLeftY + vp[a].Height);
								if (m_curScissor[a].left != rct.left || m_curScissor[a].right != rct.right || m_curScissor[a].top != rct.top || m_curScissor[a].bottom != rct.bottom)
								{
									m_curScissor[a] = rct;
									upd = true;
								}
							}
							if (upd == true) { m_cmdList.V0->RSSetScissorRects(static_cast<UINT>(count), m_curScissor); }
						}
					}
					void CLOAK_CALL_THIS Context::SetViewportAndScissor(In size_t count, In_reads(count) const API::Rendering::SCISSOR_RECT* vp)
					{
						if (m_usage == Usage::GRAPHIC && CloakDebugCheckOK(count <= API::Rendering::MAX_RENDER_TARGET, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							bool upd = false;
							for (size_t a = 0; a < count; a++)
							{
								D3D12_VIEWPORT v;
								v.MinDepth = 0;
								v.MaxDepth = 1;
								v.TopLeftX = static_cast<FLOAT>(vp[a].TopLeftX);
								v.TopLeftY = static_cast<FLOAT>(vp[a].TopLeftY);
								v.Width = static_cast<FLOAT>(vp[a].Width);
								v.Height = static_cast<FLOAT>(vp[a].Height);
								if (std::abs(v.TopLeftX - m_curViewport[a].TopLeftX) > 0.5f || std::abs(v.TopLeftY - m_curViewport[a].TopLeftY) > 0.5f || std::abs(v.Width - m_curViewport[a].Width) > 0.5f || std::abs(v.Height - m_curViewport[a].Height) > 0.5f || std::abs(v.MinDepth - m_curViewport[a].MinDepth) > 1e-3f || std::abs(v.MaxDepth - m_curViewport[a].MaxDepth) > 1e-3f)
								{
									m_curViewport[a] = v;
									upd = true;
								}
							}
							if (upd == true) { m_cmdList.V0->RSSetViewports(static_cast<UINT>(count), m_curViewport); }
							SetScissor(count, vp);
						}
					}
					void CLOAK_CALL_THIS Context::SetViewportAndScissor(In size_t countVP, In_reads(countVP) const API::Rendering::VIEWPORT* vp, In size_t countSC, In_reads(countSC) const API::Rendering::SCISSOR_RECT* scissor)
					{
						SetViewport(countVP, vp);
						SetScissor(countSC, scissor);
					}
					void CLOAK_CALL_THIS Context::SetViewportAndScissor(In FLOAT x, In FLOAT y, In FLOAT width, In FLOAT height, In_opt FLOAT minDepth, In_opt FLOAT maxDepth)
					{
						SetViewport(x, y, width, height, minDepth, maxDepth);
						SetScissor(static_cast<LONG>(x), static_cast<LONG>(y), static_cast<LONG>(width), static_cast<LONG>(height));
					}
					void CLOAK_CALL_THIS Context::SetPrimitiveTopology(In API::Rendering::PRIMITIVE_TOPOLOGY topology) 
					{ 
						if (m_useTessellation == true && topology == API::Rendering::TOPOLOGY_TRIANGLELIST) { topology = API::Rendering::TOPOLOGY_3_CONTROL_POINT_PATCHLIST; }
						else if (m_useTessellation == false && topology == API::Rendering::TOPOLOGY_3_CONTROL_POINT_PATCHLIST) { topology = API::Rendering::TOPOLOGY_TRIANGLELIST; }
						if (m_topology != topology)
						{
							m_topology = topology;
							m_cmdList.V0->IASetPrimitiveTopology(Casting::CastForward(topology));
						}
					}
					API::Rendering::IContext2D* CLOAK_CALL_THIS Context::Enable2D(In UINT textureSlot)
					{ 
						if (m_usage == Usage::GRAPHIC) 
						{
							m_2DContext.Initialize(textureSlot);
							return &m_2DContext; 
						}
						return nullptr;
					}
					API::Rendering::IContext3D* CLOAK_CALL_THIS Context::Enable3D(In const API::Global::Math::Matrix& WorldToProjection, In_opt bool cullNear, In_opt bool useIndirectBuffer, In_opt UINT rootIndexMaterial, In_opt UINT rootIndexLocalBuffer, In_opt UINT rootIndexDisplacement, In_opt UINT rootIndexMetaConstant)
					{
						if (m_usage == Usage::GRAPHIC)
						{
							m_3DContext.Initialize(m_manager->GetHardwareSetting(), useIndirectBuffer, 1, &WorldToProjection, rootIndexLocalBuffer, rootIndexMaterial, rootIndexDisplacement, rootIndexMetaConstant, m_useTessellation, cullNear);
							return &m_3DContext;
						}
						return nullptr;
					}
					API::Rendering::IContext3D* CLOAK_CALL_THIS Context::Enable3D(In_range(0, API::Rendering::MAX_VIEW_INSTANCES) size_t camCount, In_reads(camCount) const API::Global::Math::Matrix* WorldToProjection, In_opt bool cullNear, In_opt bool useIndirectBuffer, In_opt UINT rootIndexMaterial, In_opt UINT rootIndexLocalBuffer, In_opt UINT rootIndexDisplacement, In_opt UINT rootIndexMetaConstant)
					{
						if (m_usage == Usage::GRAPHIC)
						{
							CLOAK_ASSUME(camCount <= API::Rendering::MAX_VIEW_INSTANCES);
							m_3DContext.Initialize(m_manager->GetHardwareSetting(), useIndirectBuffer, camCount, WorldToProjection, rootIndexLocalBuffer, rootIndexMaterial, rootIndexDisplacement, rootIndexMetaConstant, m_useTessellation, cullNear);
							return &m_3DContext;
						}
						return nullptr;
					}

					void CLOAK_CALL_THIS Context::SetRootSignature(In IRootSignature* sig)
					{
						SAVE_RELEASE(m_rootSig);
						if (sig != nullptr && SUCCEEDED(sig->QueryInterface(CE_QUERY_ARGS(&m_rootSig))))
						{
							ID3D12RootSignature* rsig = m_rootSig->GetSignature();
							CLOAK_ASSUME(rsig != nullptr);
							if (rsig != m_curRoot)
							{
								SWAP(m_curRoot, rsig);
								if (m_usage == Usage::GRAPHIC)
								{
									m_cmdList.V0->SetGraphicsRootSignature(rsig);
									m_dynHeap->ParseGraphicRootSignature(sig);
									m_updatedDynHeap = true;
								}
								else if (m_usage == Usage::COMPUTE)
								{
									m_cmdList.V0->SetComputeRootSignature(rsig);
									m_dynHeap->ParseComputeRootSignature(sig);
									m_updatedDynHeap = true;
								}
								//Resetting cache values
								m_curIndexBuffer = nullptr;
								for (size_t a = 0; a < ARRAYSIZE(m_curVertexBuffer); a++) { m_curVertexBuffer[a] = nullptr; }
								for (size_t a = 0; a < ARRAYSIZE(m_curRV); a++) { m_curRV[a].Type = RVCacheType::NONE; }
								for (size_t a = 0; a < ARRAYSIZE(m_curRTV); a++) { m_curRTV[a] = nullptr; }
								m_curDSV = nullptr;
								m_curDSVAccess = API::Rendering::DSV_ACCESS::NONE;
								m_curRTVCount = 0;
							}
							const size_t nodeID = m_queue->GetNodeID() - 1;
							const size_t samplerHeap = m_rootSig->GetSamplerHeap(nodeID);
							if (samplerHeap != API::Rendering::HANDLEINFO_INVALID_HEAP)
							{
								D3D12_DESCRIPTOR_HEAP_TYPE ht[1] = { D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER };
								ID3D12DescriptorHeap* he[1] = { nullptr };
								DescriptorHeap* heap = m_queue->GetDescriptorHeap(Casting::CastBackward(ht[0]));
								he[0] = heap->GetHeap(samplerHeap);
								CLOAK_ASSUME(he[0] != nullptr);
								if (he[0] != nullptr) { SetDescriptorHeaps(ht, he, 1); }
								m_lastSamplerMipLevel = SAMPLER_MAX_MIP_CLAMP;
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetPipelineState(In API::Rendering::IPipelineState* pso)
					{
						SetPipelineState(pso, true);
					}
					void CLOAK_CALL_THIS Context::SetPipelineState(In API::Rendering::IPipelineState* pso, In bool setRoot)
					{
						if (pso != m_curPSO)
						{
							SWAP(m_curPSO, pso);
							PipelineState* dxPSO = nullptr;
							if (pso != nullptr && SUCCEEDED(pso->QueryInterface(CE_QUERY_ARGS(&dxPSO))))
							{
								m_useTessellation = dxPSO->UseTessellation();
								if (m_useTessellation == true && m_topology == API::Rendering::TOPOLOGY_TRIANGLELIST) { SetPrimitiveTopology(API::Rendering::TOPOLOGY_3_CONTROL_POINT_PATCHLIST); }
								else if (m_useTessellation == false && m_topology == API::Rendering::TOPOLOGY_3_CONTROL_POINT_PATCHLIST) { SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLELIST); }
								ID3D12PipelineState* p = dxPSO->GetPSO();
								if (p == nullptr) { m_usage = Usage::NONE; }
								else
								{
									const PIPELINE_STATE_TYPE t = dxPSO->GetType();
									if (t == PIPELINE_STATE_TYPE_GRAPHIC) { m_usage = Usage::GRAPHIC; }
									else if (t == PIPELINE_STATE_TYPE_COMPUTE) { m_usage = Usage::COMPUTE; }
									else { m_usage = Usage::NONE; }
								}
								if (p != nullptr) { m_cmdList.V0->SetPipelineState(p); }
								if (setRoot) { SetRootSignature(dxPSO->GetRoot()); }
								dxPSO->Release();
								m_lastViewMask = 0;
							}
							else { m_usage = Usage::NONE; }
						}
					}
					void CLOAK_CALL_THIS Context::SetDescriptorHeaps(In_reads(count) const D3D12_DESCRIPTOR_HEAP_TYPE* Type, In_reads(count) ID3D12DescriptorHeap* const* Heaps, In_opt UINT count)
					{
						bool c = false;
						for (UINT a = 0; a < count; a++)
						{
							if (m_currentHeaps[Type[a]] != Heaps[a])
							{
								m_currentHeaps[Type[a]] = Heaps[a];
								c = true;
							}
						}
						if (c) 
						{ 
							BindDescriptorHeaps(); 
							for (size_t a = 0; a < ARRAYSIZE(m_curRV); a++) { m_curRV[a].Type = RVCacheType::NONE; }
						}
					}
					void CLOAK_CALL_THIS Context::SetDynamicDescriptors(In UINT rootIndex, In UINT offset, In_reads(count) const API::Rendering::CPU_DESCRIPTOR_HANDLE* handles, In_opt UINT count)
					{
						CLOAK_ASSUME(rootIndex < Impl::Rendering::MAX_ROOT_PARAMETERS);
						switch(m_usage)
						{
							case Usage::GRAPHIC:
								m_updatedDynHeap = true;
								m_dynHeap->SetGraphicDescriptorHandles(rootIndex, offset, count, handles);
								break;
							case Usage::COMPUTE:
								m_updatedDynHeap = true;
								m_dynHeap->SetComputeDescriptorHandles(rootIndex, offset, count, handles);
								break;
							default:
								break;
						}
					}

					void CLOAK_CALL_THIS Context::ClearColor(In API::Rendering::IColorBuffer* target)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						TransitionResource(target, API::Rendering::STATE_RENDER_TARGET, true);
						const API::Helper::Color::RGBA& rgba = target->GetClearColor();
						FLOAT clearColor[4] = { rgba.R,rgba.G,rgba.B,rgba.A };
						m_cmdList.V0->ClearRenderTargetView(Casting::CastForward(target->GetRTV().CPU), clearColor, 0, nullptr);
						IResource* cb = nullptr;
						if (SUCCEEDED(target->QueryInterface(CE_QUERY_ARGS(&cb))))
						{
							cb->RegisterUsage(m_queue);
							m_usedResources.push(cb);
							cb->Release();
						}
					}
					void CLOAK_CALL_THIS Context::ClearDepth(In API::Rendering::IDepthBuffer* target)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						TransitionResource(target, API::Rendering::STATE_DEPTH_WRITE, true);
						m_cmdList.V0->ClearDepthStencilView(Casting::CastForward(target->GetDSV().CPU), D3D12_CLEAR_FLAG_DEPTH, target->GetClearDepth(), target->GetClearStencil(), 0, nullptr);
						IResource* cb = nullptr;
						if (SUCCEEDED(target->QueryInterface(CE_QUERY_ARGS(&cb))))
						{
							cb->RegisterUsage(m_queue);
							m_usedResources.push(cb);
							cb->Release();
						}
					}
					void CLOAK_CALL_THIS Context::ClearStencil(In API::Rendering::IDepthBuffer* target)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						TransitionResource(target, API::Rendering::STATE_DEPTH_WRITE, true);
						m_cmdList.V0->ClearDepthStencilView(Casting::CastForward(target->GetDSV().CPU), D3D12_CLEAR_FLAG_STENCIL, target->GetClearDepth(), target->GetClearStencil(), 0, nullptr);
						IResource* cb = nullptr;
						if (SUCCEEDED(target->QueryInterface(CE_QUERY_ARGS(&cb))))
						{
							cb->RegisterUsage(m_queue);
							m_usedResources.push(cb);
							cb->Release();
						}
					}
					void CLOAK_CALL_THIS Context::ClearDepthAndStencil(In API::Rendering::IDepthBuffer* target)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						TransitionResource(target, API::Rendering::STATE_DEPTH_WRITE, true);
						m_cmdList.V0->ClearDepthStencilView(Casting::CastForward(target->GetDSV().CPU), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, target->GetClearDepth(), target->GetClearStencil(), 0, nullptr);
						IResource* cb = nullptr;
						if (SUCCEEDED(target->QueryInterface(CE_QUERY_ARGS(&cb))))
						{
							cb->RegisterUsage(m_queue);
							m_usedResources.push(cb);
							cb->Release();
						}
					}
					void CLOAK_CALL_THIS Context::SetIndexBuffer(In API::Rendering::IIndexBuffer* buffer)
					{
						if (m_usage == Usage::GRAPHIC)
						{
							if (buffer != nullptr)
							{
								if (buffer != m_curIndexBuffer)
								{
									m_curIndexBuffer = buffer;
									IndexBuffer* buf = nullptr;
									HRESULT hRet = buffer->QueryInterface(CE_QUERY_ARGS(&buf));
									if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
									{
										buf->RegisterUsage(m_queue);
										m_usedResources.push(buf);
										TransitionResource(buffer, API::Rendering::STATE_INDEX_BUFFER, true);
										D3D12_INDEX_BUFFER_VIEW ibv = Casting::CastForward(buf->GetIBV());
										m_cmdList.V0->IASetIndexBuffer(&ibv);
									}
									SAVE_RELEASE(buf);
								}
							}
							else if(m_curIndexBuffer != nullptr)
							{
								m_curIndexBuffer = nullptr;
								m_cmdList.V0->IASetIndexBuffer(nullptr);
							}
						}
					}
					void CLOAK_CALL_THIS Context::SetVertexBuffer(In API::Rendering::IVertexBuffer* buffer, In_opt UINT slot)
					{
						CLOAK_ASSUME(slot < ARRAYSIZE(m_curVertexBuffer));
						if (m_usage == Usage::GRAPHIC && buffer != nullptr && buffer != m_curVertexBuffer[slot])
						{
							m_curVertexBuffer[slot] = buffer;
							VertexBuffer* buf = nullptr;
							HRESULT hRet = buffer->QueryInterface(CE_QUERY_ARGS(&buf));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								buf->RegisterUsage(m_queue);
								m_usedResources.push(buf);
								TransitionResource(buffer, API::Rendering::STATE_VERTEX_AND_CONSTANT_BUFFER, true);
								D3D12_VERTEX_BUFFER_VIEW vbv = Casting::CastForward(buf->GetVBV());
								m_cmdList.V0->IASetVertexBuffers(slot, 1, &vbv);
							}
							SAVE_RELEASE(buf);
						}
					}
					void CLOAK_CALL_THIS Context::SetVertexBuffer(In UINT count, In_reads(count) API::Rendering::IVertexBuffer** buffer, In_opt UINT slot)
					{
						if (m_usage == Usage::GRAPHIC)
						{
							D3D12_VERTEX_BUFFER_VIEW* vb = reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW*>(m_pageAlloc.Allocate(sizeof(D3D12_VERTEX_BUFFER_VIEW)*count));
							bool upd = false;
							for (size_t a = 0; a < count; a++)
							{
								CLOAK_ASSUME(a + slot < ARRAYSIZE(m_curVertexBuffer));
								if (buffer[a] != m_curVertexBuffer[a + slot])
								{
									m_curVertexBuffer[a + slot] = buffer[a];
									upd = true;
									if (buffer[a] != nullptr)
									{
										VertexBuffer* buf = nullptr;
										HRESULT hRet = buffer[a]->QueryInterface(CE_QUERY_ARGS(&buf));
										if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
										{
											buf->RegisterUsage(m_queue);
											m_usedResources.push(buf);
											TransitionResource(buffer[a], API::Rendering::STATE_VERTEX_AND_CONSTANT_BUFFER);
											vb[a] = Casting::CastForward(buf->GetVBV());
										}
										SAVE_RELEASE(buf);
									}
								}
							}
							if (upd == true)
							{
								FlushResourceBarriers();
								m_cmdList.V0->IASetVertexBuffers(slot, count, vb);
							}
							m_pageAlloc.Free(vb);
						}
					}
					void CLOAK_CALL_THIS Context::SetDynamicVertexBuffer(In UINT slot, In size_t numVertices, In size_t vertexStride, In_reads_bytes(numVertices*vertexStride) const void* data)
					{
						if (m_usage == Usage::GRAPHIC && CloakDebugCheckOK(data != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							size_t size = AlignUp(numVertices*vertexStride, 16);
							DynAlloc vb;
							m_dynAlloc[1]->Allocate(&vb, size, 512);
							memcpy(vb.Data, data, numVertices*vertexStride);
							D3D12_VERTEX_BUFFER_VIEW desc;
							desc.BufferLocation = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(vb.GPUAddress);
							desc.SizeInBytes = static_cast<UINT>(size);
							desc.StrideInBytes = static_cast<UINT>(vertexStride);
							m_cmdList.V0->IASetVertexBuffers(slot, 1, &desc);
						}
					}
					void CLOAK_CALL_THIS Context::SetDynamicIndexBuffer(In size_t count, In_reads(count) const uint16_t* data)
					{
						if (m_usage == Usage::GRAPHIC && CloakDebugCheckOK(data != nullptr && IsAligned(data, 16), API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							size_t size = AlignUp(count*sizeof(uint16_t), 16);
							DynAlloc ib;
							m_dynAlloc[1]->Allocate(&ib, size, 512);
							memcpy(ib.Data, data, count * sizeof(uint16_t));
							D3D12_INDEX_BUFFER_VIEW desc;
							desc.BufferLocation= static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(ib.GPUAddress);
							desc.SizeInBytes = static_cast<UINT>(count*sizeof(uint16_t));
							desc.Format = DXGI_FORMAT_R16_UINT;
							m_cmdList.V0->IASetIndexBuffer(&desc);
						}
					}
					void CLOAK_CALL_THIS Context::SetDepthStencilRefValue(In UINT val)
					{
						if (m_usage == Usage::GRAPHIC) { m_cmdList.V0->OMSetStencilRef(val); }
					}
					void CLOAK_CALL_THIS Context::SetBlendFactor(In const API::Helper::Color::RGBA& blendColor)
					{
						if (m_usage == Usage::GRAPHIC)
						{
							FLOAT col[4] = { blendColor.R,blendColor.G,blendColor.B,blendColor.A };
							m_cmdList.V0->OMSetBlendFactor(col);
						}
					}
					void CLOAK_CALL_THIS Context::SetViewInstanceMask(In UINT mask)
					{
						if (m_usage == Usage::GRAPHIC && mask != m_lastViewMask && m_cmdList.V1 != nullptr)
						{
							CLOAK_ASSUME(m_manager->GetHardwareSetting().ViewInstanceSupport != API::Rendering::VIEW_INSTANCE_NOT_SUPPORTED);
							m_lastViewMask = mask;
							m_cmdList.V1->SetViewInstanceMask(mask);
						}
					}
					void CLOAK_CALL_THIS Context::Draw(In UINT count, In_opt UINT offset)
					{
						DrawInstanced(count, 1, offset);
					}
					void CLOAK_CALL_THIS Context::DrawIndexed(In UINT count, In_opt UINT indexOffset, In_opt INT vertexOffset)
					{
						DrawIndexedInstanced(count, 1, indexOffset, vertexOffset);
					}
					void CLOAK_CALL_THIS Context::DrawInstanced(In UINT countPerInstance, In UINT instanceCount, In_opt UINT vertexOffset, In_opt UINT instanceOffset)
					{
						if (m_usage == Usage::GRAPHIC)
						{
							FlushResourceBarriers();
							if (m_updatedDynHeap == true) { m_updatedDynHeap = false; m_dynHeap->CommitGraphicDescriptorTables(m_cmdList.V0.Get()); }
							m_cmdList.V0->DrawInstanced(countPerInstance, instanceCount, vertexOffset, instanceOffset);
						}
					}
					void CLOAK_CALL_THIS Context::DrawIndexedInstanced(In UINT countPerInstance, In UINT instanceCount, In_opt UINT indexOffset, In_opt INT vertexOffset, In_opt UINT instanceOffset)
					{
						if (m_usage == Usage::GRAPHIC)
						{
							FlushResourceBarriers();
							if (m_updatedDynHeap == true) { m_updatedDynHeap = false; m_dynHeap->CommitGraphicDescriptorTables(m_cmdList.V0.Get()); }
							m_cmdList.V0->DrawIndexedInstanced(countPerInstance, instanceCount, indexOffset, vertexOffset, instanceOffset);
						}
					}
					void CLOAK_CALL_THIS Context::ExecuteIndirect(In API::Rendering::IStructuredBuffer* argBuffer, In_opt size_t argOffset)
					{

						if (m_usage == Usage::GRAPHIC)
						{
							FlushResourceBarriers();
							if (m_updatedDynHeap == true) { m_updatedDynHeap = false; m_dynHeap->CommitGraphicDescriptorTables(m_cmdList.V0.Get()); }
							
							//TODO
							/*
							m_CommandList->ExecuteIndirect(Graphics::DrawIndirectCommandSignature.GetSignature(), 1, ArgumentBuffer.GetResource(),
				(UINT64)ArgumentBufferOffset, nullptr, 0);
							*/
						}
						else if (m_usage == Usage::COMPUTE)
						{
							FlushResourceBarriers();
							if (m_updatedDynHeap == true) { m_updatedDynHeap = false; m_dynHeap->CommitComputeDescriptorTables(m_cmdList.V0.Get()); }

							//TODO
							/*
							m_CommandList->ExecuteIndirect(Graphics::DrawIndirectCommandSignature.GetSignature(), 1, ArgumentBuffer.GetResource(),
							(UINT64)ArgumentBufferOffset, nullptr, 0);
							*/
						}
					}
					void CLOAK_CALL_THIS Context::SetPredication(In API::Rendering::IQuery* query, In bool value)
					{
						//TODO
					}
					void CLOAK_CALL_THIS Context::ResetPredication()
					{
						//TODO
					}
					void CLOAK_CALL_THIS Context::ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In API::Rendering::IColorBuffer* src)
					{
						ResolveMultiSample(dst, 0, src, 0);
					}
					void CLOAK_CALL_THIS Context::ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In UINT dstSubResource, In API::Rendering::IColorBuffer* src, In UINT srcSubResource)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						if (CloakDebugCheckOK(dst != nullptr && src != nullptr && dst->GetWidth() == src->GetWidth() && dst->GetHeight() == src->GetHeight() && dst->GetSampleCount() == 1 && src->GetSampleCount() > 1 && dst->GetFormat() == src->GetFormat(), API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							ColorBuffer* d = nullptr;
							ColorBuffer* s = nullptr;
							bool hRet = SUCCEEDED(dst->QueryInterface(CE_QUERY_ARGS(&d))) && SUCCEEDED(src->QueryInterface(CE_QUERY_ARGS(&s)));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								d->RegisterUsage(m_queue);
								s->RegisterUsage(m_queue);
								m_usedResources.push(d);
								m_usedResources.push(s);
								API::Helper::Lock lockD(d->GetSyncSection());
								API::Helper::Lock lockS(s->GetSyncSection());
								TransitionResource(dst, API::Rendering::STATE_RESOLVE_DEST);
								TransitionResource(src, API::Rendering::STATE_RESOLVE_SOURCE, true);
								m_cmdList.V0->ResolveSubresource(d->GetResource(), dstSubResource, s->GetResource(), srcSubResource, Casting::CastForward(d->GetFormat()));
							}
							SAVE_RELEASE(d);
							SAVE_RELEASE(s);
						}	
					}
					void CLOAK_CALL_THIS Context::ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In API::Rendering::IColorBuffer* src, In UINT dstX, In UINT dstY)
					{
						ResolveMultiSample(dst, 0, src, 0, dstX, dstY);
					}
					void CLOAK_CALL_THIS Context::ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In UINT dstSubResource, In API::Rendering::IColorBuffer* src, In UINT srcSubResource, In UINT dstX, In UINT dstY)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						if (CloakDebugCheckOK(dst != nullptr && src != nullptr && dst->GetSampleCount() == 1 && src->GetSampleCount() > 1 && dst->GetFormat() == src->GetFormat() && dstX < dst->GetWidth() && dstY < dst->GetHeight() && dst->GetWidth() - dstX >= src->GetWidth() && dst->GetHeight() - dstY >= src->GetHeight(), API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							ColorBuffer* d = nullptr;
							ColorBuffer* s = nullptr;
							bool hRet = SUCCEEDED(dst->QueryInterface(CE_QUERY_ARGS(&d))) && SUCCEEDED(src->QueryInterface(CE_QUERY_ARGS(&s)));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								if (m_cmdList.V1 != nullptr)
								{
									d->RegisterUsage(m_queue);
									s->RegisterUsage(m_queue);
									m_usedResources.push(d);
									m_usedResources.push(s);
									API::Helper::Lock lockD(d->GetSyncSection());
									API::Helper::Lock lockS(s->GetSyncSection());
									TransitionResource(dst, API::Rendering::STATE_RESOLVE_DEST);
									TransitionResource(src, API::Rendering::STATE_RESOLVE_SOURCE, true);
									m_cmdList.V1->ResolveSubresourceRegion(d->GetResource(), dstSubResource, dstX, dstY, s->GetResource(), srcSubResource, nullptr, Casting::CastForward(d->GetFormat()), D3D12_RESOLVE_MODE_AVERAGE);
								}
							}
							SAVE_RELEASE(d);
							SAVE_RELEASE(s);
						}
					}
					void CLOAK_CALL_THIS Context::ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In API::Rendering::IColorBuffer* src, In UINT dstX, In UINT dstY, In UINT srcX, In UINT srcY, In UINT W, In UINT H)
					{
						ResolveMultiSample(dst, 0, src, 0, dstX, dstY, srcX, srcY, W, H);
					}
					void CLOAK_CALL_THIS Context::ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In UINT dstSubResource, In API::Rendering::IColorBuffer* src, In UINT srcSubResource, In UINT dstX, In UINT dstY, In UINT srcX, In UINT srcY, In UINT W, In UINT H)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						if (CloakDebugCheckOK(dst != nullptr && src != nullptr && dst->GetSampleCount() == 1 && src->GetSampleCount() > 1 && dst->GetFormat() == src->GetFormat() && dstX < dst->GetWidth() && dstY < dst->GetHeight() && srcX + W <= src->GetWidth() && srcY + H <= src->GetHeight() && dst->GetWidth() - dstX >= W && dst->GetHeight() - dstY >= H, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							ColorBuffer* d = nullptr;
							ColorBuffer* s = nullptr;
							bool hRet = SUCCEEDED(dst->QueryInterface(CE_QUERY_ARGS(&d))) && SUCCEEDED(src->QueryInterface(CE_QUERY_ARGS(&s)));
							if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
							{
								if (m_cmdList.V1 != nullptr)
								{
									d->RegisterUsage(m_queue);
									s->RegisterUsage(m_queue);
									m_usedResources.push(d);
									m_usedResources.push(s);
									API::Helper::Lock lockD(d->GetSyncSection());
									API::Helper::Lock lockS(s->GetSyncSection());
									TransitionResource(dst, API::Rendering::STATE_RESOLVE_DEST);
									TransitionResource(src, API::Rendering::STATE_RESOLVE_SOURCE, true);
									D3D12_RECT r;
									r.left = srcX;
									r.top = srcY;
									r.right = srcX + W;
									r.bottom = srcY + H;
									m_cmdList.V1->ResolveSubresourceRegion(d->GetResource(), dstSubResource, dstX, dstY, s->GetResource(), srcSubResource, &r, Casting::CastForward(d->GetFormat()), D3D12_RESOLVE_MODE_AVERAGE);
								}
							}
							SAVE_RELEASE(d);
							SAVE_RELEASE(s);
						}
					}
					void CLOAK_CALL_THIS Context::SetDepthBounds(In float min, In float max)
					{
						if (m_usage == Usage::GRAPHIC && m_cmdList.V1 != nullptr)
						{
							CLOAK_ASSUME(min < max);
							CLOAK_ASSUME(m_manager->GetHardwareSetting().AllowDepthBoundsTests == true);
							m_cmdList.V1->OMSetDepthBounds(min, max);
						}
					}
					void CLOAK_CALL_THIS Context::SetShadingRate(In API::Rendering::SHADING_RATE xAxis, In API::Rendering::SHADING_RATE yAxis)
					{
						if (m_usage == Usage::GRAPHIC && m_cmdList.V5 != nullptr)
						{
							D3D12_SHADING_RATE srx = static_cast<D3D12_SHADING_RATE>(min(xAxis, yAxis + 1));
							D3D12_SHADING_RATE sry = static_cast<D3D12_SHADING_RATE>(min(yAxis, xAxis + 1));
							D3D12_SHADING_RATE_COMBINER comb[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT];
							for (size_t a = 0; a < ARRAYSIZE(comb); a++) { comb[a] = D3D12_SHADING_RATE_COMBINER::D3D12_SHADING_RATE_COMBINER_MAX; }
							m_cmdList.V5->RSSetShadingRate(static_cast<D3D12_SHADING_RATE>(D3D12_MAKE_COARSE_SHADING_RATE(srx, sry)), comb);
						}
					}

					void CLOAK_CALL_THIS Context::Dispatch(In_opt size_t groupCountX, In_opt size_t groupCountY, In_opt size_t groupCountZ)
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						if (m_usage == Usage::COMPUTE)
						{
							FlushResourceBarriers();
							if (m_updatedDynHeap == true) { m_updatedDynHeap = false; m_dynHeap->CommitComputeDescriptorTables(m_cmdList.V0.Get()); }
							m_cmdList.V0->Dispatch(static_cast<UINT>(groupCountX), static_cast<UINT>(groupCountY), static_cast<UINT>(groupCountZ));
						}
					}
					void CLOAK_CALL_THIS Context::Dispatch1D(In size_t threadCount, In_opt size_t size)
					{
						Dispatch(DivideByMultiple(threadCount, size));
					}
					void CLOAK_CALL_THIS Context::Dispatch2D(In size_t threadCountX, In size_t threadCountY, In_opt size_t sizeX, In_opt size_t sizeY)
					{
						Dispatch(DivideByMultiple(threadCountX, sizeX), DivideByMultiple(threadCountY, sizeY));
					}
					void CLOAK_CALL_THIS Context::Dispatch3D(In size_t threadCountX, In size_t threadCountY, In size_t threadCountZ, In_opt size_t sizeX, In_opt size_t sizeY, In_opt size_t sizeZ)
					{
						Dispatch(DivideByMultiple(threadCountX, sizeX), DivideByMultiple(threadCountY, sizeY), DivideByMultiple(threadCountZ, sizeZ));
					}

					void CLOAK_CALL_THIS Context::Reset()
					{
						CopyContext::Reset();
						RELEASE(m_curPSO);
						RELEASE(m_curRoot);
						m_curIndexBuffer = nullptr;
						for (size_t a = 0; a < ARRAYSIZE(m_curVertexBuffer); a++) { m_curVertexBuffer[a] = nullptr; }
						for (size_t a = 0; a < ARRAYSIZE(m_curRV); a++) { m_curRV[a].Type = RVCacheType::NONE; }
						for (size_t a = 0; a < ARRAYSIZE(m_curRTV); a++) { m_curRTV[a] = nullptr; }
						m_curDSV = nullptr;
						m_curDSVAccess = API::Rendering::DSV_ACCESS::NONE;
						m_curRTVCount = 0;
						m_useTessellation = false;
						m_topology = API::Rendering::TOPOLOGY_UNDEFINED;
						for (size_t a = 0; a < ARRAYSIZE(m_currentHeaps); a++) { m_currentHeaps[a] = nullptr; }
						ZeroMemory(&m_curViewport, sizeof(m_curViewport));
						ZeroMemory(&m_curScissor, sizeof(m_curScissor));
					}
					void CLOAK_CALL_THIS Context::CleanAndDiscard(In uint64_t fence)
					{
						const QUEUE_TYPE queueT = m_queue->GetType();
						m_dynHeap->CleanUsedHeaps(fence, queueT);
						m_updatedDynHeap = true;
						CopyContext::CleanAndDiscard(fence);
					}

					Success(return == true) bool CLOAK_CALL_THIS Context::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Rendering::IContext)) { *ptr = (API::Rendering::IContext*)this; return true; }
						else if (riid == __uuidof(Impl::Rendering::IContext)) { *ptr = (Impl::Rendering::IContext*)this; return true; }
						else if (riid == __uuidof(Context)) { *ptr = (Context*)this; return true; }
						return CopyContext::iQueryInterface(riid, ptr);
					}
					uint64_t CLOAK_CALL_THIS Context::Finish(In_opt bool wait, In_opt bool executeDirect)
					{
						API::Helper::Lock lock(m_sync);
						m_usage = Usage::NONE;
						SAVE_RELEASE(m_rootSig);
						m_lastSamplerMipLevel = SAMPLER_MAX_MIP_CLAMP;
						if (m_inRenderPass == true) { EndRenderPass(); }
						return CopyContext::Finish(wait, executeDirect);
					}
					void CLOAK_CALL_THIS Context::BindDescriptorHeaps()
					{
						CLOAK_ASSUME(m_inRenderPass == false);
						UINT heaps = 0;
						ID3D12DescriptorHeap* toBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
						for (UINT a = 0; a < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; a++)
						{
							ID3D12DescriptorHeap* heap = m_currentHeaps[a];
							if (heap != nullptr) { toBind[heaps++] = heap; }
						}
						if (heaps > 0) { m_cmdList.V0->SetDescriptorHeaps(heaps, toBind); }
					}
				}
			}
		}
	}
}
#endif