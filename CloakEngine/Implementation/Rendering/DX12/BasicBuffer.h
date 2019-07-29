#pragma once
#ifndef CE_IMPL_RENDERING_DX12_BASICBUFFER_H
#define CE_IMPL_RENDERING_DX12_BASICBUFFER_H

#include "Implementation/Rendering/BasicBuffer.h"
#include "Implementation/Rendering/Allocator.h"

#include "Implementation/Rendering/DX12/Resource.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace BasicBuffer_v1 {
					class CLOAK_UUID("{9C23ED71-F40A-4F61-BD88-1668EBC1C1A0}") PixelBuffer : public virtual IPixelBuffer, public Resource {
						friend class Resource;
						public:
							virtual CLOAK_CALL ~PixelBuffer();
							virtual uint32_t CLOAK_CALL_THIS GetWidth() const override;
							virtual uint32_t CLOAK_CALL_THIS GetHeight() const override;
							virtual uint32_t CLOAK_CALL_THIS GetDepth() const override;
							virtual uint32_t CLOAK_CALL_THIS GetSampleCount() const override;
							virtual API::Rendering::Format CLOAK_CALL_THIS GetFormat() const override;
							virtual UINT CLOAK_CALL_THIS GetSubresourceIndex(In UINT mipMap, In UINT arrayIndex = 0, In_opt UINT planeSlice = 0) const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							CLOAK_CALL PixelBuffer();
							virtual void CLOAK_CALL_THIS CreateTexture(In IDevice* device, In UINT nodeMask, In size_t nodeID, In DXGI_FORMAT format, In const D3D12_RESOURCE_DESC& desc, In const D3D12_CLEAR_VALUE& clearVal);
							virtual void CLOAK_CALL_THIS UseTexture(In_opt ID3D12Resource* resource, In D3D12_RESOURCE_STATES state);

							UINT64 m_width;
							UINT m_height;
							UINT m_size;
							UINT m_numMipMaps;
							UINT m_sampleCount;
							DXGI_FORMAT m_format;
							DXGI_FORMAT m_realFormat;
							TextureType m_type;
							API::Rendering::VIEW_TYPE m_views;
					};
					class CLOAK_UUID("{E282B12C-DC95-4AA8-A2CE-80133CAA2009}") GraphicBuffer : public virtual IGraphicBuffer, public Resource {
						friend class Resource;
						public:
							virtual CLOAK_CALL ~GraphicBuffer();
							virtual const API::Rendering::GPU_DESCRIPTOR_HANDLE& CLOAK_CALL_THIS GetGPUAddress() const override;
							virtual uint32_t CLOAK_CALL_THIS GetSize() const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							CLOAK_CALL GraphicBuffer();
							virtual void CLOAK_CALL_THIS InitBuffer(In UINT nodeMask, In size_t nodeID, In uint32_t numElements, In uint32_t elementSize, In_reads_bytes(numElements*elementSize) const void* data = nullptr, In_opt API::Rendering::RESOURCE_STATE state = API::Rendering::STATE_COMMON);
							virtual void CLOAK_CALL_THIS InitBuffer(In UINT nodeMask, In size_t nodeID, In uint32_t numElements, In uint32_t elementSize, In API::Rendering::RESOURCE_STATE state);

							uint32_t m_size;
							uint32_t m_eCount;
							uint32_t m_eSize;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif
