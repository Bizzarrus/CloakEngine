#pragma once
#ifndef CE_IMPL_RENDERING_DX12_SWAPCHAIN_H
#define CE_IMPL_RENDERING_DX12_SWAPCHAIN_H

#include "CloakEngine/Helper/SyncSection.h"

#include "Implementation/Rendering/SwapChain.h"
#include "Implementation/Helper/SavePtr.h"

#include "Implementation/Rendering/DX12/Manager.h"
#include "Implementation/Rendering/DX12/ColorBuffer.h"

#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace SwapChain_v1 {
					class CLOAK_UUID("{503AAAF3-36AC-4EA3-B9AF-054098E8E44B}") SwapChain : public virtual ISwapChain, public Impl::Helper::SavePtr_v1::SavePtr {
						public:
							CLOAK_CALL SwapChain(In Manager* manager, In const API::Rendering::Hardware& hardware);
							virtual CLOAK_CALL ~SwapChain();
							ID3D12Resource* CLOAK_CALL_THIS GetBuffer(In UINT texNum) const;

							HRESULT CLOAK_CALL_THIS Recreate(In const API::Global::Graphic::Settings& gset) override;
							SWAP_CHAIN_RESULT CLOAK_CALL_THIS Present(In bool VSync, In PRESENT_FLAG flag) override;
							HRESULT CLOAK_CALL_THIS ResizeBuffers(In UINT frameCount, In UINT width, In UINT height) override;
							bool CLOAK_CALL_THIS GetFullscreenState() const override;
							void CLOAK_CALL_THIS SetFullscreenState(In bool state) override;
							void CLOAK_CALL_THIS ResizeTarget(Inout SWAPCHAIN_MODE* mode) override;
							UINT CLOAK_CALL_THIS GetCurrentBackBufferIndex() override;
							bool CLOAK_CALL_THIS SupportHDR() override;
							API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetCurrentBackBuffer() override;
						protected:
							void CLOAK_CALL_THIS UpdateHDRSupport();
							Success(return == true) bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							API::Helper::ISyncSection* m_sync;
							Manager* const m_manager;
							const UINT m_flags;
							IDXGISwapChain3* m_swap;
							ColorBuffer** m_buffers;
							size_t m_bufferCount;
							std::atomic<UINT> m_curBuffer;
							std::atomic<bool> m_hdr;
							UINT m_W;
							UINT m_H;
							std::atomic<bool> m_forceResize;
							std::atomic<bool> m_validBuffer;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif