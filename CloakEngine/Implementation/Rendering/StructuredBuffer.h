#pragma once
#ifndef CE_IMPL_RENDEIRNG_STRUCTUREDBUFFER_H
#define CE_IMPL_RENDEIRNG_STRUCTUREDBUFFER_H

#include "CloakEngine/Rendering/StructuredBuffer.h"

#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/BasicBuffer.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace StructuredBuffer_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{58642D3F-50FF-4AB0-9319-FD4FA2B3D430}") IByteAddressBuffer : public virtual IGraphicBuffer, public virtual API::Rendering::IByteAddressBuffer{
					public:

				};
				RENDERING_INTERFACE CLOAK_UUID("{B1548D34-5469-4F4B-9337-3A043D500C2E}") IStructuredBuffer : public virtual IGraphicBuffer, public virtual API::Rendering::IStructuredBuffer{
					public:

				};
				RENDERING_INTERFACE CLOAK_UUID("{B835BE53-5497-4385-ADDD-7D93C2204093}") IVertexBuffer : public virtual IStructuredBuffer, public virtual API::Rendering::IVertexBuffer{
					public:

				};
				RENDERING_INTERFACE CLOAK_UUID("{B888EE46-8E7B-436E-9272-3AE798F378DE}") IIndexBuffer : public virtual IStructuredBuffer, public virtual API::Rendering::IIndexBuffer{
					public:

				};
				RENDERING_INTERFACE CLOAK_UUID("{5629453C-DC0A-4ABB-8145-13AFAF04C72F}") IConstantBuffer : public virtual IGraphicBuffer, public virtual API::Rendering::IConstantBuffer{
					public:

				};
			}
		}
	}
}

#pragma warning(pop)

#endif