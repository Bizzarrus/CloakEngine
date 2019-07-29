#include "stdafx.h"
#include "Implementation\Rendering\Resource.h"

#include "CloakEngine\Global\Game.h"
#include "CloakEngine\Helper\StringConvert.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace Resource_v1 {
				CLOAK_CALL Resource::Resource() : Resource(nullptr, D3D12_RESOURCE_STATE_COMMON, true) { }
				CLOAK_CALL Resource::Resource(_In_ ID3D12Resource* base, _In_ D3D12_RESOURCE_STATES curState, _In_ bool canTransition) : m_canTransition(canTransition), m_gpuHandle(GPU_ADDRESS_UNKNOWN)
				{
					m_data = base;
					m_transState = RESOURCE_STATE_INVALID;
					m_curState = curState;
					if (m_data != nullptr) { m_data->AddRef(); }
					API::Global::Game::CreateInterface(IID_PPV_ARGS(&m_sync));
				}
				CLOAK_CALL Resource::~Resource() { DestroyResource(); RELEASE(m_sync); }
				const API::Rendering::GPU_DESCRIPTOR_HANDLE& CLOAK_CALL_THIS Resource::GetGPUAddress() const { return m_gpuHandle; }
				void CLOAK_CALL_THIS Resource::DestroyResource() { RELEASE(m_data); }

				_Ret_maybenull_ ID3D12Resource* CLOAK_CALL_THIS Resource::GetResource() { return m_data; }
				_Ret_maybenull_ const ID3D12Resource* CLOAK_CALL_THIS Resource::GetResource() const { return m_data; }
				_Ret_notnull_ API::Helper::ISyncSection* CLOAK_CALL_THIS Resource::GetSyncSection() { return m_sync; }
				void CLOAK_CALL_THIS Resource::SetResourceState(_In_ D3D12_RESOURCE_STATES newState)
				{
					API::Helper::Lock lock(m_sync);
					m_curState = newState;
				}
				void CLOAK_CALL_THIS Resource::SetTransitionResourceState(_In_ D3D12_RESOURCE_STATES newState)
				{
					API::Helper::Lock lock(m_sync);
					m_transState = newState;
				}
				D3D12_RESOURCE_STATES CLOAK_CALL_THIS Resource::GetResourceState() const
				{
					API::Helper::Lock lock(m_sync);
					return m_curState;
				}
				D3D12_RESOURCE_STATES CLOAK_CALL_THIS Resource::GetTransitionResourceState() const
				{
					API::Helper::Lock lock(m_sync);
					return m_transState;
				}
#ifdef _DEBUG
				void CLOAK_CALL_THIS Resource::SetDebugResourceName(_In_ std::u16string s)
				{
					std::string ns = API::Helper::StringConvert::ConvertToU8(s);
					SetDebugResourceName(ns);
				}
				void CLOAK_CALL_THIS Resource::SetDebugResourceName(_In_ std::string s)
				{
					if (m_data != nullptr)
					{
						HRESULT hRet = m_data->SetName(API::Helper::StringConvert::ConvertToW(s).c_str());
						if (SUCCEEDED(hRet)) { return; }
					}
					API::Global::Log::writeToLog("Failed to set resource name '" + s + "'", API::Global::Log::Typ::WARNING);
				}
#endif

				_Success_(return == true) bool CLOAK_CALL_THIS Resource::iQueryInterface(_In_ REFIID riid, _COM_Outptr_ void** ptr) { CLOAK_QUERY(riid, ptr, SavePtr, Rendering::Resource_v1, Resource); }
			}
		}
	}
}