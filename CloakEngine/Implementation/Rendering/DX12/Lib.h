#pragma once
#ifndef CE_IMPL_RENDERING_DX12_LIB_H
#define CE_IMPL_RENDERING_DX12_LIB_H

#include "Implementation/Rendering/Manager.h"

#ifndef NO_LIB_LOAD
#include <d3d12.h>
#include <d3d11on12.h>
#include <dxgi1_4.h>

#ifdef DEFINE_DX12_FUNCS
#define EXTERN_DX12 
#else
#define EXTERN_DX12 extern
#endif

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT Flags, REFIID riid, Outptr void **ppFactory);

#endif

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace Lib {
					IManager* CLOAK_CALL CreateManager();

#ifndef NO_LIB_LOAD
					EXTERN_DX12 PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
					EXTERN_DX12 PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
					EXTERN_DX12 PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
					EXTERN_DX12 PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
					EXTERN_DX12 PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
					//EXTERN_DX12 PFN_D3D11ON12_CREATE_DEVICE D3D11On12CreateDevice;
					EXTERN_DX12 PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
#endif
				}
			}
		}
	}
}

#endif