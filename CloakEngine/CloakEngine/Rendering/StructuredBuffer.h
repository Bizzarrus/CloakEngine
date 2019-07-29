#pragma once
#ifndef CE_API_RENDERING_STRUCTUREDBUFFER_H
#define CE_API_RENDERING_STRUCTUREDBUFFER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Rendering/BasicBuffer.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace StructuredBuffer_v1 {
				CLOAK_INTERFACE_ID("{FF0EA53A-8DFE-4919-9101-F5191FCA13CE}") IByteAddressBuffer : public virtual IGraphicBuffer{
					public:
						virtual ResourceView CLOAK_CALL_THIS GetUAV() const = 0;
						virtual ResourceView CLOAK_CALL_THIS GetSRV() const = 0;
				};
				CLOAK_INTERFACE_ID("{FF0EA53A-8DFE-4919-9101-F5191FCA13CE}") IStructuredBuffer : public virtual IGraphicBuffer{
					public:
						virtual ResourceView CLOAK_CALL_THIS GetUAV() const = 0;
						virtual ResourceView CLOAK_CALL_THIS GetSRV() const = 0;
						virtual IByteAddressBuffer* CLOAK_CALL_THIS GetCounterBuffer() = 0;
						virtual void CLOAK_CALL_THIS Recreate(In UINT elementCount, In UINT elementSize, In_reads_opt(elementSize) const void* data = nullptr) = 0;
				};
				CLOAK_INTERFACE_ID("{23D71405-3259-4A8A-835C-C0EB1D440E52}") IVertexBuffer : public virtual IStructuredBuffer{
					public:
						virtual VERTEX_BUFFER_VIEW CLOAK_CALL_THIS GetVBV(In_opt uint32_t offset = 0) const = 0;
				};
				CLOAK_INTERFACE_ID("{FE8505A4-5625-4B9C-AF39-905C3D8FE5D0}") IIndexBuffer : public virtual IStructuredBuffer{
					public:
						virtual bool CLOAK_CALL_THIS Is32Bit() const = 0;
						virtual INDEX_BUFFER_VIEW CLOAK_CALL_THIS GetIBV(In_opt uint32_t offset = 0) const = 0;
				};
				CLOAK_INTERFACE_ID("{AE549441-1EC8-4B15-827B-9307D35B8298}") IConstantBuffer : public virtual IGraphicBuffer{
					public:
						virtual ResourceView CLOAK_CALL_THIS GetCBV() const = 0;
				};
			}
		}
	}
}

#endif
