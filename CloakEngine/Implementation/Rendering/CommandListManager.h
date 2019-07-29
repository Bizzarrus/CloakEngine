#pragma once
#ifndef CE_IMPL_RENDERING_COMMANDLISTMANAGER_H
#define CE_IMPL_RENDERING_COMMANDLISTMANAGER_H

#include "CloakEngine\Defines.h"
#include "CloakEngine\Helper\SyncSection.h"
#include "Allocator.h"

#include <stdint.h>
#include <d3d12.h>
#include <d3d11on12.h>
#include <dxgi1_4.h>
#include <vector>
#include <queue>
#include <atomic>

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace Context_v1 { class Context; }
			class CommandListManager {
				friend class Context_v1::Context;
				public:
					CLOAK_CALL CommandListManager();
					CLOAK_CALL ~CommandListManager();
					HRESULT CLOAK_CALL_THIS Create(_Out_ ID3D12Device** device, _Out_ IDXGIFactory4** factory);
					void CLOAK_CALL_THIS Shutdown();
					void CLOAK_CALL_THIS IdleGPU();
					uint64_t CLOAK_CALL_THIS FlushExecutions(_In_opt_ bool wait = false);
					bool CLOAK_CALL_THIS IsReady() const;
					bool CLOAK_CALL_THIS IsFenceComplete(_In_ uint64_t val);
					bool CLOAK_CALL_THIS WaitForFence(_In_ uint64_t val);
					uint64_t CLOAK_CALL_THIS IncrementFence();
					_Ret_notnull_ ID3D12CommandQueue* CLOAK_CALL_THIS GetCommandQueue() const;
					_Ret_notnull_ DescriptorHeap* CLOAK_CALL_THIS GetDescriptorHeap(_In_ D3D12_DESCRIPTOR_HEAP_TYPE heapType);
					_Maybenull_ ID3D12Device* CLOAK_CALL_THIS GetDevice() const;
					_Maybenull_ ID3D11On12Device* CLOAK_CALL_THIS GetD11Device() const;
				private:
					void CLOAK_CALL_THIS CreateCommandList(_Outptr_ ID3D12GraphicsCommandList** List, _Outptr_ ID3D12CommandAllocator** Allocator);
					void CLOAK_CALL_THIS DiscardAllocator(_In_ uint64_t fenceValForReset, _In_ ID3D12CommandAllocator* alloc);
					void CLOAK_CALL_THIS ExecuteDeffered(_In_ Context* context);
					uint64_t CLOAK_CALL_THIS ExecuteCommandList(_In_ ID3D12CommandList* list);
					_Ret_notnull_ ID3D12CommandAllocator* CLOAK_CALL_THIS RequestAllocator();

					IDXGIFactory4* m_factory;
					ID3D12Device* m_device;
					ID3D12Fence* m_fence;
					ID3D12CommandQueue* m_cmdQueue;
					ID3D11On12Device* m_d11Device;
					ID3D11DeviceContext* m_d11Context;
					D3D_FEATURE_LEVEL m_d11FeatureLvl;
					uint64_t m_nextFenceVal;
					uint64_t m_lastFenceVal;
					HANDLE m_fenceEvent;
					std::vector<ID3D12CommandAllocator*> m_allocPool;
					std::vector<Context*> m_toExecute;
					std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_rdyAllocs;
					API::Helper::ISyncSection* m_syncAlloc;
					API::Helper::ISyncSection* m_syncFence;
					API::Helper::ISyncSection* m_syncEvent;
					API::Helper::ISyncSection* m_syncExecute;
					API::Helper::ISyncSection* m_syncQueue;
					std::atomic<bool> m_isReady;
					std::atomic<bool> m_init;
					DescriptorHeap m_descHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = 
					{	D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
						D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
						D3D12_DESCRIPTOR_HEAP_TYPE_RTV ,
						D3D12_DESCRIPTOR_HEAP_TYPE_DSV 
					};
			};
		}
	}
}

#endif