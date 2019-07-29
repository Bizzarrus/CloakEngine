#pragma once
#ifndef CE_IMPL_RENDERING_DX12_RESOURCE_H
#define CE_IMPL_RENDERING_DX12_RESOURCE_H

#include "Implementation/Rendering/Resource.h"
#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Rendering/DX12/Device.h"
#include "Implementation/Rendering/Context.h"

#include "CloakEngine/Helper/SyncSection.h"

#include <d3d12.h>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace Resource_v1 {
					class CLOAK_UUID("{5676DC74-4CBA-45E4-BC9E-E24C04C127BE}") Resource : public virtual IResource, public Impl::Helper::SavePtr_v1::SavePtr {
						friend class Device;
						public:
							static void CLOAK_CALL GenerateViews(In_reads(bufferSize) Resource* buffer[], In size_t bufferSize, In API::Rendering::VIEW_TYPE Type, In_opt API::Rendering::DescriptorPageType Page = API::Rendering::DescriptorPageType::UTILITY);

							virtual CLOAK_CALL ~Resource();
							virtual const API::Rendering::GPU_DESCRIPTOR_HANDLE& CLOAK_CALL_THIS GetGPUAddress() const override;

							Ret_maybenull virtual ID3D12Resource* CLOAK_CALL_THIS GetResource();
							Ret_maybenull virtual const ID3D12Resource* CLOAK_CALL_THIS GetResource() const;
							Ret_notnull virtual API::Helper::ISyncSection* CLOAK_CALL_THIS GetSyncSection() override;

							virtual void CLOAK_CALL_THIS SetResourceState(In API::Rendering::RESOURCE_STATE newState, In_opt bool autoPromote = false) override;
							virtual void CLOAK_CALL_THIS SetTransitionResourceState(In API::Rendering::RESOURCE_STATE newState) override;
							virtual API::Rendering::RESOURCE_STATE CLOAK_CALL_THIS GetResourceState() const override;
							virtual API::Rendering::RESOURCE_STATE CLOAK_CALL_THIS GetTransitionResourceState() const override;
							virtual bool CLOAK_CALL_THIS CanTransition() const override;
							virtual bool CLOAK_CALL_THIS SupportStatePromotion() const override;
							virtual uint8_t CLOAK_CALL_THIS GetDetailLevel() const override;
							virtual size_t CLOAK_CALL_THIS GetNodeID() const override;
							virtual void CLOAK_CALL_THIS RegisterUsage(In Impl::Rendering::IAdapter* queue) override;
							virtual void CLOAK_CALL_THIS UnregisterUsage(In uint64_t fence) override;

							virtual void CLOAK_CALL_THIS MakeResident();
							virtual void CLOAK_CALL_THIS Evict();

							void* CLOAK_CALL operator new(In size_t s);
							void CLOAK_CALL operator delete(In void* ptr);
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							CLOAK_CALL Resource();
							CLOAK_CALL Resource(In ID3D12Resource* base, In API::Rendering::RESOURCE_STATE curState, In size_t nodeID, In_opt bool canTransition = true);
							virtual size_t CLOAK_CALL_THIS GetResourceViewCount(In API::Rendering::VIEW_TYPE view);
							virtual void CLOAK_CALL_THIS CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE);
							virtual void CLOAK_CALL_THIS iUpdateDecay(In D3D12_RESOURCE_DIMENSION dimension, In D3D12_RESOURCE_FLAGS flags);

							API::Helper::Lock CLOAK_CALL_THIS LockUsage();

							API::Rendering::RESOURCE_STATE m_curState;
							API::Rendering::RESOURCE_STATE m_transState;
							ID3D12Resource* m_data;
							API::Rendering::GPU_DESCRIPTOR_HANDLE m_gpuHandle;
							API::Helper::ISyncSection* m_sync;
							API::Helper::IConditionSyncSection* m_syncUsage;
							size_t m_nodeID;
							Impl::Rendering::IAdapter* m_lastQueue;
							Impl::Rendering::IAdapter* m_usageQueue;
							uint64_t m_lastFence;
							uint64_t m_usageCount;
							const bool m_canTransition;
							bool m_statePromoted;
							bool m_resident;
							std::atomic<bool> m_canDecay;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif