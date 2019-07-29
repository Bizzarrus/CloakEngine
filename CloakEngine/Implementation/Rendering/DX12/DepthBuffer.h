#pragma once
#ifndef CE_IMPL_RENDERING_DX12_DEPTHBUFFER_H
#define CE_IMPL_RENDERING_DX12_DEPTHBUFFER_H

#include "Implementation/Rendering/DepthBuffer.h"

#include "Implementation/Rendering/DX12/BasicBuffer.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace DepthBuffer_v1 {
					class CLOAK_UUID("{5FE5B4CB-BDB8-4091-9C20-7C201E94719E}") DepthBuffer : public virtual IDepthBuffer, public PixelBuffer{
						public:
							Ret_notnull static DepthBuffer* CLOAK_CALL Create(In const API::Rendering::DEPTH_DESC& desc, In size_t nodeID, In UINT nodeMask);

							virtual CLOAK_CALL ~DepthBuffer();
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetDSV(In_opt API::Rendering::DSV_ACCESS access = API::Rendering::DSV_ACCESS::NONE) const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetDepthSRV() const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetStencilSRV() const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetDynamicDepthSRV() const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetDynamicStencilSRV() const override;
							virtual float CLOAK_CALL_THIS GetClearDepth() const override;
							virtual uint32_t CLOAK_CALL_THIS GetClearStencil() const override;
							virtual void CLOAK_CALL_THIS Recreate(In const API::Rendering::DEPTH_DESC& desc) override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							CLOAK_CALL DepthBuffer();
							virtual size_t CLOAK_CALL_THIS GetResourceViewCount(In API::Rendering::VIEW_TYPE view) override;
							virtual void CLOAK_CALL_THIS CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE) override;

							API::Rendering::ResourceView m_DSV[4];
							API::Rendering::ResourceView m_SRV[4];
							FLOAT m_depthClear;
							uint8_t m_stencilClear;
							bool m_useDepth;
							bool m_useStencil;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif