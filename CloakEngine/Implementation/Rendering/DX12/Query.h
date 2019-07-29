#pragma once
#ifndef CE_IMPL_RENDERING_DX12_QUERY_H
#define CE_IMPL_RENDERING_DX12_QUERY_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Rendering/Query.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "Implementation/Rendering/Query.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/DX12/Resource.h"
#include "Implementation/Rendering/DX12/Device.h"
#include "Implementation/Rendering/Queue.h"
#include "Implementation/Helper/SavePtr.h"

#include <d3d12.h>
#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace Query_v1 {
					class CLOAK_UUID("{C1EE681A-0DE5-4CA5-A5F9-3C03AC9F179D}") QueryPoolPage : public Resource {
						public:
							static constexpr size_t ELEMENT_COUNT = 64;

							CLOAK_CALL QueryPoolPage(In size_t elementSize, In ID3D12QueryHeap* heap, In ID3D12Resource* base, In IQueue* queue);
							virtual CLOAK_CALL ~QueryPoolPage();

							virtual bool CLOAK_CALL_THIS TryToAllocate(In QUEUE_TYPE queue, Out UINT* offset, Out UINT* index);
							virtual void CLOAK_CALL_THIS Free(In UINT index);
							virtual bool CLOAK_CALL_THIS TryReadback(In UINT index, Out void* data, Inout uint64_t* UpdateFlag);
							virtual void CLOAK_CALL_THIS Clear(In UINT index, In uint64_t fence);
							virtual ID3D12QueryHeap* CLOAK_CALL_THIS GetHeap() const;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							struct Usage {
								QUEUE_TYPE Queue;
								uint64_t Fence;
								uint64_t LastFence;
								uint64_t Update;
								uint8_t Flags;
							};

							const size_t m_elementSize;
							ID3D12QueryHeap* const m_heap;
							IQueue* const m_queue;
							Usage m_usage[ELEMENT_COUNT];
					};
					class QueryPool {
						public:
							CLOAK_CALL QueryPool(In IQueue* queue);
							virtual CLOAK_CALL ~QueryPool();

							virtual void CLOAK_CALL_THIS Allocate(In API::Rendering::QUERY_TYPE type, In QUEUE_TYPE queue, Out QueryPoolPage** page, Out UINT* offset, Out UINT* index);
						protected:
							class Pool {
								public:
									CLOAK_CALL Pool(In D3D12_QUERY_HEAP_TYPE type, In IQueue* queue, In size_t numQueries);
									virtual CLOAK_CALL ~Pool();

									virtual void CLOAK_CALL_THIS Allocate(Out QueryPoolPage** page, In QUEUE_TYPE queue, Out UINT* offset, Out UINT* index);
								protected:
									virtual QueryPoolPage* CLOAK_CALL_THIS CreateNewPage();

									const D3D12_QUERY_HEAP_TYPE m_type;
									const size_t m_numQueries;
									IQueue* const m_queue;
									API::Helper::ISyncSection* m_sync;
									API::List<QueryPoolPage*> m_heap;
							};

							Pool m_poolOcclusion;
							Pool m_poolTimestamp;
							Pool m_poolPipelineStatistics;
							Pool m_poolStreamOutputSingle;
							Pool m_poolStreamOutputAll;
					};
					class CLOAK_UUID("{727E76D1-7F22-49E9-BDFF-D802D886F143}") Query : public virtual Impl::Rendering::Query_v1::IQuery, public Impl::Helper::SavePtr_v1::SavePtr{
						public:
							CLOAK_CALL Query(In QueryPool* pool, In QUEUE_TYPE queue, In API::Rendering::QUERY_TYPE type);
							virtual CLOAK_CALL ~Query();

							virtual API::Rendering::QUERY_TYPE CLOAK_CALL_THIS GetType() const override;
							virtual const API::Rendering::QUERY_DATA& CLOAK_CALL_THIS GetData() override;

							virtual void CLOAK_CALL_THIS OnBegin(In ID3D12GraphicsCommandList* cmdList, In Impl::Rendering::IAdapter* queue);
							virtual void CLOAK_CALL_THIS OnEnd(In ID3D12GraphicsCommandList* cmdList, In Impl::Rendering::IAdapter* queue);
							virtual void CLOAK_CALL_THIS Clear(In uint64_t fence);
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							API::Helper::ISyncSection* m_sync;
							QueryPoolPage* m_page;
							UINT m_offset;
							UINT m_index;
							uint64_t m_maxFence;
							uint64_t m_updateFlag;
							uint32_t m_beginCount;
							uint32_t m_usageCount;
							API::Rendering::QUERY_DATA m_data;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif