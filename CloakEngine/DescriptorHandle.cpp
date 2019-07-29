#include "stdafx.h"
#include "Implementation/Rendering/DescriptorHandle.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			CLOAK_CALL DescriptorHandle::DescriptorHandle() :DescriptorHandle({ CPU_ADDRESS_NULL }, { GPU_ADDRESS_NULL }) {}
			CLOAK_CALL DescriptorHandle::DescriptorHandle(In const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle) : DescriptorHandle(cpuHandle, { GPU_ADDRESS_NULL }) {}
			CLOAK_CALL DescriptorHandle::DescriptorHandle(In const API::Rendering::GPU_DESCRIPTOR_HANDLE& gpuHandle) : DescriptorHandle({ CPU_ADDRESS_NULL }, gpuHandle) {}
			CLOAK_CALL DescriptorHandle::DescriptorHandle(In const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle, In const API::Rendering::GPU_DESCRIPTOR_HANDLE& gpuHandle) : m_cpuHandle(cpuHandle), m_gpuHandle(gpuHandle) {}
			CLOAK_CALL DescriptorHandle::~DescriptorHandle() {}
			API::Rendering::CPU_DESCRIPTOR_HANDLE CLOAK_CALL_THIS DescriptorHandle::GetCPUHandle() const { return m_cpuHandle; }
			API::Rendering::GPU_DESCRIPTOR_HANDLE CLOAK_CALL_THIS DescriptorHandle::GetGPUHandle() const { return m_gpuHandle; }
			bool CLOAK_CALL_THIS DescriptorHandle::IsNull() const { return m_cpuHandle.ptr == CPU_ADDRESS_NULL; }
			bool CLOAK_CALL_THIS DescriptorHandle::IsShaderVisible() const { return m_gpuHandle.ptr != GPU_ADDRESS_NULL; }
			DescriptorHandle CLOAK_CALL_THIS DescriptorHandle::operator+(In INT scaledOffset) const
			{
				DescriptorHandle ret = *this;
				ret += scaledOffset;
				return ret;
			}
			DescriptorHandle CLOAK_CALL_THIS DescriptorHandle::operator+(In UINT scaledOffset) const
			{
				DescriptorHandle ret = *this;
				ret += scaledOffset;
				return ret;
			}
			DescriptorHandle& CLOAK_CALL_THIS DescriptorHandle::operator+=(In INT scaledOffset)
			{
				if (m_cpuHandle.ptr != CPU_ADDRESS_NULL) { m_cpuHandle.ptr += scaledOffset; }
				if (m_gpuHandle.ptr != GPU_ADDRESS_NULL) { m_gpuHandle.ptr += scaledOffset; }
				return *this;
			}
			DescriptorHandle& CLOAK_CALL_THIS DescriptorHandle::operator+=(In UINT scaledOffset)
			{
				if (m_cpuHandle.ptr != CPU_ADDRESS_NULL) { m_cpuHandle.ptr += scaledOffset; }
				if (m_gpuHandle.ptr != GPU_ADDRESS_NULL) { m_gpuHandle.ptr += scaledOffset; }
				return *this;
			}
		}
	}
}