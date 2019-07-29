#pragma once
#ifndef CE_IMPL_RENDERING_DX12_COLORBUFFER_H
#define CE_IMPL_RENDERING_DX12_COLORBUFFER_H

#include "Implementation/Rendering/ColorBuffer.h"

#include "Implementation/Rendering/DX12/BasicBuffer.h"
#include "Implementation/Rendering/DX12/Device.h"

#include <d3d11on12.h>
#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace ColorBuffer_v1 {
					class CLOAK_UUID("{DF3A25BC-EC7B-498E-A925-31D6DEE389A8}") ColorBuffer : public virtual IColorBuffer, public PixelBuffer {
						friend class Device;
						public:
							When(base != nullptr, Ret_notnull) static ColorBuffer* CLOAK_CALL CreateFromSwapChain(In ISwapChain* base, In uint32_t textureNum);
							Ret_notnull static ColorBuffer* CLOAK_CALL Create(In const API::Rendering::TEXTURE_DESC& desc, In size_t nodeID, In UINT nodeMask);

							virtual CLOAK_CALL ~ColorBuffer();
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetSRV(In_opt bool cube = false) const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetUAV(In_opt UINT num = 0) const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetDynamicSRV(In_opt bool cube = false) const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetDynamicUAV(In_opt UINT num = 0) const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetRTV(In_opt UINT num = 0) const override;
							virtual const API::Helper::Color::RGBA& CLOAK_CALL_THIS GetClearColor() const override;
							virtual void CLOAK_CALL_THIS Recreate(In const API::Rendering::TEXTURE_DESC& desc) override;
							virtual uint8_t CLOAK_CALL_THIS GetDetailLevel() const override;
							virtual void CLOAK_CALL_THIS SetDetailLevel(In uint32_t lvl) override;
							virtual void CLOAK_CALL_THIS FreeSwapChain();
							virtual void CLOAK_CALL_THIS RecreateFromSwapChain(In ISwapChain* base, In uint32_t TextureNum);
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							CLOAK_CALL ColorBuffer();
							virtual size_t CLOAK_CALL_THIS GetResourceViewCount(In API::Rendering::VIEW_TYPE view) override;
							virtual void CLOAK_CALL_THIS CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE) override;

							API::Helper::Color::RGBA m_color;
							std::atomic<uint8_t> m_dlvl;
							API::Rendering::ResourceView m_SRV[4];
							API::Rendering::ResourceView* m_RTV;
							API::Rendering::ResourceView* m_UAV;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif