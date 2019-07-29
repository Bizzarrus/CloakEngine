#pragma once
#ifndef CE_IMPL_RENDEIRNG_DESCRIPTORHANDLE_H
#define CE_IMPL_RENDEIRNG_DESCRIPTORHANDLE_H

#include "CloakEngine/Rendering/Defines.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			class DescriptorHandle {
				public:
					CLOAK_CALL DescriptorHandle();
					CLOAK_CALL DescriptorHandle(In const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle);
					CLOAK_CALL DescriptorHandle(In const API::Rendering::GPU_DESCRIPTOR_HANDLE& gpuHandle);
					CLOAK_CALL DescriptorHandle(In const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle, In const API::Rendering::GPU_DESCRIPTOR_HANDLE& gpuHandle);
					virtual CLOAK_CALL ~DescriptorHandle();
					API::Rendering::CPU_DESCRIPTOR_HANDLE CLOAK_CALL_THIS GetCPUHandle() const;
					API::Rendering::GPU_DESCRIPTOR_HANDLE CLOAK_CALL_THIS GetGPUHandle() const;
					bool CLOAK_CALL_THIS IsNull() const;
					bool CLOAK_CALL_THIS IsShaderVisible() const;
					DescriptorHandle CLOAK_CALL_THIS operator+(In INT scaledOffset) const;
					DescriptorHandle CLOAK_CALL_THIS operator+(In UINT scaledOffset) const;
					DescriptorHandle& CLOAK_CALL_THIS operator+=(In INT scaledOffset);
					DescriptorHandle& CLOAK_CALL_THIS operator+=(In UINT scaledOffset);
				private:
					API::Rendering::CPU_DESCRIPTOR_HANDLE m_cpuHandle;
					API::Rendering::GPU_DESCRIPTOR_HANDLE m_gpuHandle;
			};
		}
	}
}

#endif