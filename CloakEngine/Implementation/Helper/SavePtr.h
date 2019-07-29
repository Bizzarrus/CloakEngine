#pragma once
#ifndef CE_IMPL_HELPER_SAVEPTR_H
#define CE_IMPL_HELPER_SAVEPTR_H

#include "CloakEngine/Helper/SavePtr.h"
#include "Implementation/Global/Game.h"

#include <atomic>

#define CLOAK_QUERY(riid,ptr,super,path,name) if(riid==__uuidof(API::path::I##name)){*ptr=(API::path::I##name*)this; return true;} else if(riid==__uuidof(Impl::path::name)){*ptr=(Impl::path::name*)this; return true;} return super::iQueryInterface(riid,ptr)
#ifdef _DEBUG
#define DEBUG_NAME(name) SetDebugName(#name)
#define DEBUG_FILE(name) SetSourceFile(name)
#else
#define DEBUG_NAME(name)
#define DEBUG_FILE(name)
#endif

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Helper {
			inline namespace SavePtr_v1 {
				class CLOAK_UUID("{E3B4D5F7-8D94-4D29-BC89-A8E82ED94A5E}") SavePtr : public virtual API::Helper::SavePtr_v1::ISavePtr {
					public:
						CLOAK_CALL_THIS SavePtr();
						virtual CLOAK_CALL_THIS ~SavePtr();
						virtual API::Global::Debug::Error CLOAK_CALL_THIS Delete() override;
						Use_annotations virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
						virtual uint64_t CLOAK_CALL_THIS AddRef() override;
						virtual uint64_t CLOAK_CALL_THIS Release() override;
						virtual uint64_t CLOAK_CALL_THIS GetRefCount() override;
						virtual void CLOAK_CALL_THIS SetDebugName(In const std::string& s) override;
						virtual void CLOAK_CALL_THIS SetSourceFile(In const std::string& s) override;
						virtual std::string CLOAK_CALL_THIS GetDebugName() const override;
						virtual std::string CLOAK_CALL_THIS GetSourceFile() const override;

						virtual void CLOAK_CALL_THIS Initialize();		
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr);
						std::atomic<uint32_t> m_ref;
					private:
						std::atomic<bool> m_rdyToDel;
#ifdef _DEBUG
						std::string m_debug;
						std::string m_srcFile;
						Impl::Global::Game::ptrAdr m_adr;
#endif
				};
			}
		}
	}
}

#pragma warning(pop)

#endif
