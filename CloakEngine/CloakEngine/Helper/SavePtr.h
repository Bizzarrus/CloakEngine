#pragma once
#ifndef CE_API_HELPER_SAVEPTR_H
#define CE_API_HELPER_SAVEPTR_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/UIDs.h"
#include "CloakEngine/Global/Debug.h"

#include <string>

#define SAVE_RELEASE(p) CloakEngine::API::Helper::SaveRelease(&p)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Helper {
			inline namespace SavePtr_v1 {
				CLOAK_INTERFACE IBasicPtr{
					public:
						virtual uint64_t CLOAK_CALL_THIS AddRef() = 0;
						virtual uint64_t CLOAK_CALL_THIS Release() = 0;
				};
				CLOAK_INTERFACE_ID("{EEC9FA96-0416-49B0-8EA9-C09169559A2E}") ISavePtr : public virtual IBasicPtr {
					public:
						virtual uint64_t CLOAK_CALL_THIS GetRefCount() = 0;
						virtual Global::Debug::Error CLOAK_CALL_THIS Delete() = 0;
						Success(return == S_OK) Post_satisfies(*ppvObject != nullptr) virtual HRESULT STDMETHODCALLTYPE QueryInterface(In REFIID riid, Outptr void** ppvObject) = 0;
						virtual uint64_t CLOAK_CALL_THIS AddRef() = 0;
						virtual uint64_t CLOAK_CALL_THIS Release() = 0;
						virtual void CLOAK_CALL_THIS SetDebugName(In const std::string& s) = 0;
						virtual void CLOAK_CALL_THIS SetSourceFile(In const std::string& s) = 0;
						virtual std::string CLOAK_CALL_THIS GetDebugName() const = 0;
						virtual std::string CLOAK_CALL_THIS GetSourceFile() const = 0;
				};
			}
			CLOAKENGINE_API bool CLOAK_CALL CanReleaseSavePtr();
			template<typename T> void CLOAK_CALL SaveRelease(Inout T** p)
			{
				T* n = *p;
				*p = nullptr;
				if (n != nullptr && CanReleaseSavePtr()) { n->Release(); }
			}
			template<typename T> void CLOAK_CALL SaveRelease(Inout T* const* p)
			{
				T* n = *p;
				if (n != nullptr && CanReleaseSavePtr()) { n->Release(); }
			}
		}
	}
}

#endif