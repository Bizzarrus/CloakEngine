#include "stdafx.h"
#include "Implementation/Rendering/DX12/Sampler.h"
#include "Implementation/Rendering/DX12/Casting.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				CLOAK_CALL Sampler::Sampler(In IDescriptorHeap* heap, In API::Rendering::ResourceView handles[SAMPLER_MAX_MIP_CLAMP], In const API::Rendering::SAMPLER_DESC& desc, In UINT rootID, In IDevice* device)
				{
					m_heap = heap;
					m_rootID = rootID;
					API::Rendering::SAMPLER_DESC d = desc;
					for (size_t a = 0; a < SAMPLER_MAX_MIP_CLAMP; a++)
					{
						d.MinLOD = min(desc.MinLOD + a, desc.MaxLOD);
						device->CreateSampler(d, handles[a].CPU);
						m_handles[a] = handles[a];
					}
				}
				CLOAK_CALL Sampler::~Sampler()
				{
					for (size_t a = 0; a < SAMPLER_MAX_MIP_CLAMP; a++)
					{
						m_heap->Free(m_handles[a]);
					}
				}
				UINT CLOAK_CALL_THIS Sampler::GetRootIndex() const { return m_rootID; }
				const API::Rendering::GPU_DESCRIPTOR_HANDLE& CLOAK_CALL_THIS Sampler::GetGPUAddress(In_opt UINT minMipLevel) const { return m_handles[min(minMipLevel, SAMPLER_MAX_MIP_CLAMP - 1)].GPU; }
			}
		}
	}
}