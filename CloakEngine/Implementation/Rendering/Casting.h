#pragma once
#ifndef CE_IMPL_RENDERING_CASTING_H
#define CE_IMPL_RENDERING_CASTING_H

#include "CloakEngine\Defines.h"
#include "CloakEngine\Rendering\Defines.h"

#include <d3d12.h>

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace Casting {
				void CLOAK_CALL CastCPUHandleForward(_In_ const API::Rendering::CPU_DESCRIPTOR_HANDLE& api, _Out_ D3D12_CPU_DESCRIPTOR_HANDLE* dx);
				D3D12_CPU_DESCRIPTOR_HANDLE CLOAK_CALL CastCPUHandleForward(_In_ const API::Rendering::CPU_DESCRIPTOR_HANDLE& api);
				void CLOAK_CALL CastCPUHandleBackward(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& dx, _Out_ API::Rendering::CPU_DESCRIPTOR_HANDLE* api);
				API::Rendering::CPU_DESCRIPTOR_HANDLE CLOAK_CALL CastCPUHandleBackward(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& dx);
				void CLOAK_CALL CastGPUHandleForward(_In_ const API::Rendering::GPU_DESCRIPTOR_HANDLE& api, _Out_ D3D12_GPU_DESCRIPTOR_HANDLE* dx);
				D3D12_GPU_DESCRIPTOR_HANDLE CLOAK_CALL CastGPUHandleForward(_In_ const API::Rendering::GPU_DESCRIPTOR_HANDLE& api);
				void CLOAK_CALL CastGPUHandleBackward(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE& dx, _Out_ API::Rendering::GPU_DESCRIPTOR_HANDLE* api);
				API::Rendering::GPU_DESCRIPTOR_HANDLE CLOAK_CALL CastGPUHandleBackward(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE& dx);
				void CLOAK_CALL CastIBVForward(_In_ const API::Rendering::INDEX_BUFFER_VIEW& api, _Out_ D3D12_INDEX_BUFFER_VIEW* dx);
				D3D12_INDEX_BUFFER_VIEW CLOAK_CALL CastIBVForward(_In_ const API::Rendering::INDEX_BUFFER_VIEW& api);
				void CLOAK_CALL CastIBVBackward(_In_ const D3D12_INDEX_BUFFER_VIEW& api, _Out_ API::Rendering::INDEX_BUFFER_VIEW* dx);
				API::Rendering::INDEX_BUFFER_VIEW CLOAK_CALL CastIBVBackward(_In_ const D3D12_INDEX_BUFFER_VIEW& api);
				void CLOAK_CALL CastVBVForward(_In_ const API::Rendering::VERTEX_BUFFER_VIEW& api, _Out_ D3D12_VERTEX_BUFFER_VIEW* dx);
				D3D12_VERTEX_BUFFER_VIEW CLOAK_CALL CastVBVForward(_In_ const API::Rendering::VERTEX_BUFFER_VIEW& api);
				void CLOAK_CALL CastVBVBackward(_In_ const D3D12_VERTEX_BUFFER_VIEW& api, _Out_ API::Rendering::VERTEX_BUFFER_VIEW* dx);
				API::Rendering::VERTEX_BUFFER_VIEW CLOAK_CALL CastVBVBackward(_In_ const D3D12_VERTEX_BUFFER_VIEW& api);
				void CLOAK_CALL CastFormatForward(_In_ const API::Rendering::Format& api, _Out_ DXGI_FORMAT* dx);
				DXGI_FORMAT CLOAK_CALL CastFormatForward(_In_ const API::Rendering::Format& api);
				void CLOAK_CALL CastFormatBackward(_In_ const DXGI_FORMAT& api, _Out_ API::Rendering::Format* dx);
				API::Rendering::Format CLOAK_CALL CastFormatBackward(_In_ const DXGI_FORMAT& api);
			}
		}
	}
}

#endif