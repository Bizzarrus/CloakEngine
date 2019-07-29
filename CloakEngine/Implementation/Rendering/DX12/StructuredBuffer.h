#pragma once
#ifndef CE_IMPL_RENDERING_DX12_STRUCTUREDBUFFER_H
#define CE_IMPL_RENDERING_DX12_STRUCTUREDBUFFER_H

#include "CloakEngine/Rendering/Context.h"

#include "Implementation/Rendering/StructuredBuffer.h"

#include "Implementation/Rendering/DX12/BasicBuffer.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace StructuredBuffer_v1 {
					class StructuredBuffer;
					class CLOAK_UUID("{93B3F8BB-5F98-4C92-B029-37C93E70D079}") ByteAddressBuffer : public GraphicBuffer, public virtual IByteAddressBuffer{
						friend class StructuredBuffer;
						public:
							static ByteAddressBuffer* CLOAK_CALL Create(In UINT nodeMask, In size_t nodeID, In uint32_t numElements);
							virtual CLOAK_CALL ~ByteAddressBuffer();
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetUAV() const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetSRV() const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							CLOAK_CALL ByteAddressBuffer();
							virtual size_t CLOAK_CALL_THIS GetResourceViewCount(In API::Rendering::VIEW_TYPE view) override;
							virtual void CLOAK_CALL_THIS CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE) override;

							API::Rendering::ResourceView m_SRV;
							API::Rendering::ResourceView m_UAV;
					};
					class CLOAK_UUID("{AECFA8DB-8E30-453B-ADD6-D62F76AF5CAE}") StructuredBuffer : public GraphicBuffer, public virtual IStructuredBuffer {
						public:
							static StructuredBuffer* CLOAK_CALL Create(In UINT nodeMask, In size_t nodeID, In bool counterBuffer, In uint32_t numElements, In uint32_t elementSize, In_reads_opt(numElements*elementSize) const void* data = nullptr);
							virtual CLOAK_CALL ~StructuredBuffer();
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetUAV() const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetSRV() const override;
							Ret_notnull virtual API::Rendering::IByteAddressBuffer* CLOAK_CALL_THIS GetCounterBuffer() override;
							virtual void CLOAK_CALL_THIS Recreate(In UINT elementCount, In UINT elementSize, In_reads_opt(elementSize) const void* data = nullptr) override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							virtual void CLOAK_CALL_THIS InitBuffer(In UINT nodeMask, In size_t nodeID, In uint32_t numElements, In uint32_t elementSize, In_reads_bytes(numElements*elementSize) const void* data = nullptr, In_opt API::Rendering::RESOURCE_STATE state = API::Rendering::STATE_COMMON);
							virtual void CLOAK_CALL_THIS InitBuffer(In UINT nodeMask, In size_t nodeID, In uint32_t numElements, In uint32_t elementSize, In API::Rendering::RESOURCE_STATE state);
							CLOAK_CALL StructuredBuffer(In bool useByteAddressBuffer);
							virtual size_t CLOAK_CALL_THIS GetResourceViewCount(In API::Rendering::VIEW_TYPE view) override;
							virtual void CLOAK_CALL_THIS CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE) override;

							API::Rendering::ResourceView m_SRV;
							API::Rendering::ResourceView m_UAV;
							ByteAddressBuffer* m_byteAddress;
					};
					class CLOAK_UUID("{BC477CF3-7973-4C87-8B5F-931419670E25}") VertexBuffer : public StructuredBuffer, public virtual IVertexBuffer {
						public:
							static VertexBuffer* CLOAK_CALL Create(In UINT nodeMask, In size_t nodeID, In UINT numElements, In UINT elementSize, In_reads_opt(numElements*elementSize) const void* data);
							virtual CLOAK_CALL ~VertexBuffer();

							virtual API::Rendering::VERTEX_BUFFER_VIEW CLOAK_CALL_THIS GetVBV(In_opt uint32_t offset = 0) const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							CLOAK_CALL VertexBuffer();
					};
					class CLOAK_UUID("{0E8D2186-4C5F-4E3C-943C-0347A70BF106}") IndexBuffer : public StructuredBuffer, public virtual IIndexBuffer {
						public:
							static IndexBuffer* CLOAK_CALL Create(In UINT nodeMask, In size_t nodeID, In UINT numElements, In bool b32, In_reads_opt(numElements*(b32 ? 4 : 2)) const void* data);
							virtual CLOAK_CALL ~IndexBuffer();
							virtual bool CLOAK_CALL_THIS Is32Bit() const override;

							virtual API::Rendering::INDEX_BUFFER_VIEW CLOAK_CALL_THIS GetIBV(In_opt uint32_t offset = 0) const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							CLOAK_CALL IndexBuffer(In bool b32);

							const bool m_32Bit;
					};
					class CLOAK_UUID("{372ABC06-FC85-4CB9-8770-91DEEF02F06D}") ConstantBuffer : public GraphicBuffer, public virtual IConstantBuffer {
						public:
							static ConstantBuffer* CLOAK_CALL Create(In UINT nodeMask, In size_t nodeID, In UINT elementSize, In_reads_opt(elementSize) const void* data);
							virtual CLOAK_CALL ~ConstantBuffer();

							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetCBV() const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							CLOAK_CALL ConstantBuffer();
							virtual size_t CLOAK_CALL_THIS GetResourceViewCount(In API::Rendering::VIEW_TYPE view) override;
							virtual void CLOAK_CALL_THIS CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE) override;

							API::Rendering::ResourceView m_CBV;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif