#include "stdafx.h"
#include "Implementation/Helper/SavePtr.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Global/Memory.h"
#include <Windows.h>

namespace CloakEngine {
	namespace API {
		namespace Helper {
			CLOAKENGINE_API bool CLOAK_CALL CanReleaseSavePtr() { return Global::Game::IsRunning(); }
		}
	}
	namespace Impl {
		namespace Helper {
			namespace SavePtr_v1 {
				CLOAK_CALL_THIS SavePtr::SavePtr()
				{
					m_ref = 1;
					m_rdyToDel = false;
#ifdef _DEBUG
					m_adr = Impl::Global::Game::registerPtr(this);
					m_debug = "";
					m_srcFile = "";
#endif
					DEBUG_NAME(SavePtr);
				}
				CLOAK_CALL_THIS SavePtr::~SavePtr()
				{
#ifdef _DEBUG
					if (API::Global::Game::IsRunning()) { Impl::Global::Game::removePtr(m_adr); }
#endif
				}
				Use_annotations HRESULT STDMETHODCALLTYPE SavePtr::QueryInterface(REFIID riid, void** ppvObject)
				{
					if (ppvObject == nullptr) { return E_INVALIDARG; }
					*ppvObject = nullptr;
					bool r = iQueryInterface(riid, ppvObject);
					if (r == false && riid == IID_IUnknown) { *ppvObject = (IUnknown*)this; r = true; }
					if (r == true)
					{
						AddRef();
						return S_OK;
					}
					return E_NOINTERFACE;
				}
				uint64_t CLOAK_CALL_THIS SavePtr::AddRef()
				{
					return m_ref.fetch_add(1) + 1;
				}
				uint64_t CLOAK_CALL_THIS SavePtr::Release()
				{
					uint64_t r = m_ref.fetch_sub(1);
					if (r == 1 && API::Global::Game::IsRunning() && m_rdyToDel.exchange(true) == false) 
					{ 
						if (Delete() == API::Global::Debug::Error::OK) { delete this; }
					}
					else if (r == 0) { CLOAK_ASSUME(false); m_ref = 0; return 0; }
					return r - 1;
				}
				uint64_t CLOAK_CALL_THIS SavePtr::GetRefCount() { return m_ref; }
				API::Global::Debug::Error CLOAK_CALL_THIS SavePtr::Delete()
				{ 
					if (API::Global::Game::IsRunning() && m_rdyToDel == false) { return API::Global::Debug::Error::ILLEGAL_FUNC_CALL; }
					return API::Global::Debug::Error::OK;
				}
				void CLOAK_CALL_THIS SavePtr::SetDebugName(In const std::string& s) 
				{
#ifdef _DEBUG
					m_debug = s; 
#endif
				}
				void CLOAK_CALL_THIS SavePtr::SetSourceFile(In const std::string& s)
				{
#ifdef _DEBUG
					m_srcFile = s;
#endif
				}
				std::string CLOAK_CALL_THIS SavePtr::GetDebugName() const 
				{ 
#ifdef _DEBUG
					return m_debug; 
#else
					return "";
#endif
				}
				std::string CLOAK_CALL_THIS SavePtr::GetSourceFile() const 
				{ 
#ifdef _DEBUG
					return m_srcFile; 
#else
					return "";
#endif
				}
				void CLOAK_CALL_THIS SavePtr::Initialize() {}
				void* CLOAK_CALL SavePtr::operator new(In size_t size)
				{
					return API::Global::Memory::MemoryHeap::Allocate(size);
				}
				void CLOAK_CALL SavePtr::operator delete(In void* ptr)
				{
					API::Global::Memory::MemoryHeap::Free(ptr);
				}


				Success(return == true) bool CLOAK_CALL_THIS SavePtr::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{ 
					if (riid == __uuidof(API::Helper::SavePtr_v1::ISavePtr)) { *ptr = (API::Helper::SavePtr_v1::ISavePtr*)this; return true; }
					else if (riid == __uuidof(SavePtr)) { *ptr = (SavePtr*)this; return true; }
					return false;
				}
			}
		}
	}
}