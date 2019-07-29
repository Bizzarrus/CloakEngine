#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/SwapChain.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/DX12/Casting.h"

#include "Engine/WindowHandler.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace SwapChain_v1 {
					inline UINT CLOAK_CALL CreateFlags(In const API::Rendering::Hardware& hardware)
					{
						UINT f = 0;
						if (hardware.AllowTearing == true) { f |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; }
						return f;
					}

					CLOAK_CALL SwapChain::SwapChain(In Manager* manager, In const API::Rendering::Hardware& hardware) : m_manager(manager), m_flags(CreateFlags(hardware))
					{
						m_swap = nullptr;
						m_hdr = false;
						m_sync = nullptr;
						m_buffers = nullptr;
						m_bufferCount = 0;
						m_curBuffer = 0;
						m_W = 0;
						m_H = 0;
						m_forceResize = false;
						m_validBuffer = false;
						DEBUG_NAME(SwapChain);
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					}
					CLOAK_CALL SwapChain::~SwapChain()
					{
						for (size_t a = 0; a < m_bufferCount; a++) { SAVE_RELEASE(m_buffers[a]); }
						API::Global::Memory::MemoryHeap::Free(m_buffers);
						RELEASE(m_swap);
						SAVE_RELEASE(m_sync);
					}
					ID3D12Resource* CLOAK_CALL_THIS SwapChain::GetBuffer(In UINT texNum) const
					{
						ID3D12Resource* rs = nullptr;
						if (SUCCEEDED(m_swap->GetBuffer(texNum, CE_QUERY_ARGS(&rs)))) { return rs; }
						return nullptr;
					}

					HRESULT CLOAK_CALL_THIS SwapChain::Recreate(In const API::Global::Graphic::Settings& gset)
					{
						API::Helper::Lock lock(m_sync);
						m_validBuffer = false;
						const bool rab = gset.BackBufferCount + 1 != m_bufferCount;
						if (rab) 
						{ 
							for (size_t a = 0; a < m_bufferCount; a++) { SAVE_RELEASE(m_buffers[a]); }
							API::Global::Memory::MemoryHeap::Free(m_buffers); 
							m_bufferCount = gset.BackBufferCount + 1;
							m_buffers = reinterpret_cast<ColorBuffer**>(API::Global::Memory::MemoryHeap::Allocate(m_bufferCount * sizeof(ColorBuffer*)));
							for (size_t a = 0; a < m_bufferCount; a++) { m_buffers[a] = nullptr; }
						}
						else
						{
							for (size_t a = 0; a < m_bufferCount; a++) { m_buffers[a]->FreeSwapChain(); }
						}

						IDXGISwapChain3* oldSwap = m_swap;
						DXGI_SWAP_CHAIN_DESC desc;
						desc.BufferCount = gset.BackBufferCount + 1;
						desc.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
						desc.BufferDesc.Height = gset.Resolution.Height;
						desc.BufferDesc.Width = gset.Resolution.Width;
						desc.BufferDesc.RefreshRate.Denominator = 0;
						desc.BufferDesc.RefreshRate.Numerator = 0;
						desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
						desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
						desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
						desc.Flags = m_flags;
						desc.OutputWindow = Engine::WindowHandler::getWindow();
						desc.SampleDesc.Count = 1;
						desc.SampleDesc.Quality = 0;
						desc.Windowed = gset.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN ? FALSE : TRUE;
						desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
						m_W = gset.Resolution.Width;
						m_H = gset.Resolution.Height;
						m_forceResize = false;
						IDXGISwapChain* swap = nullptr;
						IDXGIFactory* factory = nullptr;
						HRESULT hRet = E_FAIL;
						if (m_manager->GetFactory(CE_QUERY_ARGS(&factory)))
						{
							hRet = factory->CreateSwapChain(m_manager->GetCommandQueue(), &desc, &swap);
							if (SUCCEEDED(hRet))
							{
								hRet = swap->QueryInterface(CE_QUERY_ARGS(&m_swap));
							}
							if (SUCCEEDED(hRet)) 
							{ 
								UpdateHDRSupport(); 
								//TODO: Laptop dedicated gpu seems to have problems with this command. Removing this command
								//	seems to move the crash from the first Present call to the second(?)
								hRet = m_swap->SetColorSpace1(m_hdr ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
								CLOAK_ASSUME(SUCCEEDED(hRet));
							}
							SAVE_RELEASE(factory);
						}
						if (SUCCEEDED(hRet))
						{
							for (size_t a = 0; a < m_bufferCount; a++)
							{
								if (m_buffers[a] == nullptr)
								{
									m_buffers[a] = ColorBuffer::CreateFromSwapChain(this, static_cast<uint32_t>(a));
								}
								else
								{
									m_buffers[a]->RecreateFromSwapChain(this, static_cast<uint32_t>(a));
								}
							}
							if (rab)
							{
								API::Rendering::IColorBuffer** buf = NewArray(API::Rendering::IColorBuffer*, m_bufferCount);
								for (size_t a = 0; a < m_bufferCount; a++) { buf[a] = m_buffers[a]; }
								m_manager->GenerateViews(buf, m_bufferCount, API::Rendering::VIEW_TYPE::RTV, API::Rendering::DescriptorPageType::UTILITY);
								DeleteArray(buf);
							}
							m_curBuffer = m_swap->GetCurrentBackBufferIndex();
						}
						RELEASE(swap);
						RELEASE(oldSwap);
						return hRet;
					}
					SWAP_CHAIN_RESULT CLOAK_CALL_THIS SwapChain::Present(In bool VSync, In PRESENT_FLAG flag)
					{
						bool validBuffer = true;
						UINT flg = 0;
						if ((flag & PRESENT_TEST) != 0) { flg |= DXGI_PRESENT_TEST; validBuffer = false; }
						else 
						{
							//if (m_validBuffer == true) { flg |= DXGI_PRESENT_DO_NOT_SEQUENCE; } //TODO: This does not work in window mode(?)
							if ((flag & PRESENT_NO_WAIT) != 0) { flg |= DXGI_PRESENT_DO_NOT_WAIT; }
							if (VSync == false && GetFullscreenState() == false) { flg |= DXGI_PRESENT_ALLOW_TEARING; }
						}
						//TODO: Laptop dedicated GPU has some problem with swap chain present (crashes after present call)
						//	See SetColorSpace1
						m_validBuffer = validBuffer;
						HRESULT hRet = m_swap->Present(VSync ? 1 : 0, flg);
						m_curBuffer = m_swap->GetCurrentBackBufferIndex();
						switch (hRet)
						{
							case S_OK: return SWAP_CHAIN_RESULT::OK;
							case DXGI_ERROR_WAS_STILL_DRAWING: return SWAP_CHAIN_RESULT::NEED_RETRY;
							case DXGI_STATUS_OCCLUDED: 
								if (GetFullscreenState() == false) { return SWAP_CHAIN_RESULT::OCCLUDED; }
								break;
							default: break;
						}
						return SWAP_CHAIN_RESULT::FAILED;
					}
					HRESULT CLOAK_CALL_THIS SwapChain::ResizeBuffers(In UINT frameCount, In UINT width, In UINT height)
					{
						API::Helper::Lock lock(m_sync);
						if (m_W != width || m_H != height || m_forceResize.exchange(false) == true)
						{
							m_validBuffer = false;
							m_W = width;
							m_H = height;
							const bool rab = frameCount != m_bufferCount;
							if (rab)
							{
								for (size_t a = 0; a < m_bufferCount; a++) { SAVE_RELEASE(m_buffers[a]); }
								API::Global::Memory::MemoryHeap::Free(m_buffers);
								m_bufferCount = frameCount;
								m_buffers = reinterpret_cast<ColorBuffer**>(API::Global::Memory::MemoryHeap::Allocate(m_bufferCount * sizeof(ColorBuffer*)));
								for (size_t a = 0; a < m_bufferCount; a++) { m_buffers[a] = nullptr; }
							}
							else
							{
								for (size_t a = 0; a < m_bufferCount; a++) { m_buffers[a]->FreeSwapChain(); }
							}
							HRESULT hRet = m_swap->ResizeBuffers(frameCount, width, height, DXGI_FORMAT_UNKNOWN, m_flags);
							if (CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_RESIZE, false))
							{
								if (rab)
								{
									API::Rendering::IColorBuffer** buf = NewArray(API::Rendering::IColorBuffer*, m_bufferCount);
									for (size_t a = 0; a < m_bufferCount; a++) { buf[a] = m_buffers[a]; }
									m_manager->GenerateViews(buf, m_bufferCount, API::Rendering::VIEW_TYPE::RTV, API::Rendering::DescriptorPageType::UTILITY);
									DeleteArray(buf);
								}
								else
								{
									for (size_t a = 0; a < m_bufferCount; a++) { m_buffers[a]->RecreateFromSwapChain(this, static_cast<uint32_t>(a)); }
								}
								m_curBuffer = m_swap->GetCurrentBackBufferIndex();
							}
							return hRet;
						}
						return S_OK;
					}
					bool CLOAK_CALL_THIS SwapChain::GetFullscreenState() const
					{
						if (m_swap != nullptr)
						{
							BOOL r;
							m_swap->GetFullscreenState(&r, nullptr);
							return r == TRUE;
						}
						return false;
					}
					void CLOAK_CALL_THIS SwapChain::SetFullscreenState(In bool state)
					{
						m_swap->SetFullscreenState(state ? TRUE : FALSE, nullptr);
						m_forceResize = true;
						CloakDebugLog("Set Fullscreen State: " + std::to_string(state));
					}
					void CLOAK_CALL_THIS SwapChain::ResizeTarget(Inout SWAPCHAIN_MODE* mode)
					{
						DXGI_MODE_DESC md = Casting::CastForward(*mode);
						m_swap->ResizeTarget(&md);
						*mode = Casting::CastBackward(md);
					}
					UINT CLOAK_CALL_THIS SwapChain::GetCurrentBackBufferIndex()
					{
						return m_curBuffer;
					}
					bool CLOAK_CALL_THIS SwapChain::SupportHDR() 
					{
						if (m_manager->UpdateFactory()) { UpdateHDRSupport(); }
						return m_hdr; 
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS SwapChain::GetCurrentBackBuffer()
					{
						API::Helper::Lock lock(m_sync);
						return m_buffers[m_curBuffer];
					}


					void CLOAK_CALL_THIS SwapChain::UpdateHDRSupport()
					{
						IDXGIOutput* output = nullptr;
						HRESULT hRet = m_swap->GetContainingOutput(&output);
						if (SUCCEEDED(hRet))
						{
							IDXGIOutput6* on = nullptr;
							hRet = output->QueryInterface(CE_QUERY_ARGS(&on));
							if (SUCCEEDED(hRet))
							{
								DXGI_OUTPUT_DESC1 desc;
								hRet = on->GetDesc1(&desc);
								if (SUCCEEDED(hRet)) 
								{ 
									const bool hdr = desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020; 
									if (m_hdr.exchange(hdr) != hdr)
									{
										m_swap->SetColorSpace1(hdr ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
									}
								}
							}
							SAVE_RELEASE(on);
						}
						SAVE_RELEASE(output);
					}
					Success(return == true) bool CLOAK_CALL_THIS SwapChain::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY_IMPL(ptr, riid, SwapChain, SwapChain_v1, SavePtr);
					}
				}
			}
		}
	}
}
#endif