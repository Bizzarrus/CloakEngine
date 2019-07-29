#pragma once
#ifndef CE_IMPL_RENDEIRNG_SWAPCHAIN_H
#define CE_IMPL_RENDEIRNG_SWAPCHAIN_H

#include "CloakEngine/Global/Graphic.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Rendering/ColorBuffer.h"
#include "Implementation/Rendering/Defines.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace SwapChain_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{002727E0-E9CA-4EA0-B339-AF0D6EBBAD83}") ISwapChain : public virtual API::Helper::SavePtr_v1::ISavePtr {
					public:
						virtual HRESULT CLOAK_CALL_THIS Recreate(In const API::Global::Graphic::Settings& gset) = 0;
						virtual SWAP_CHAIN_RESULT CLOAK_CALL_THIS Present(In bool VSync, In PRESENT_FLAG flag) = 0;
						virtual HRESULT CLOAK_CALL_THIS ResizeBuffers(In UINT frameCount, In UINT width, In UINT height) = 0;
						virtual bool CLOAK_CALL_THIS GetFullscreenState() const = 0;
						virtual void CLOAK_CALL_THIS SetFullscreenState(In bool state) = 0;
						virtual void CLOAK_CALL_THIS ResizeTarget(Inout SWAPCHAIN_MODE* mode) = 0;
						virtual UINT CLOAK_CALL_THIS GetCurrentBackBufferIndex() = 0;
						virtual bool CLOAK_CALL_THIS SupportHDR() = 0;
						virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetCurrentBackBuffer() = 0;
				};
			}
		}
	}
}

#endif