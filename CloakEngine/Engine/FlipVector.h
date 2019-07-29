#pragma once
#ifndef CE_E_FLIPVECTOR_H
#define CE_E_FLIPVECTOR_H

#include "CloakEngine/Defines.h"

#include <atomic>

namespace CloakEngine {
	namespace Engine {
		template<typename F, typename V = API::List<F>> class FlipVector {
			private:
				V v1;
				V v2;
				std::atomic<bool> uv = true;
			public:
				CLOAK_CALL_THIS FlipVector() {}
				virtual CLOAK_CALL_THIS ~FlipVector() {}
				V& CLOAK_CALL_THIS flip()
				{ 
					const bool luv = uv.exchange(!uv);
					if (luv) { return v1; }
					return v2;
				}
				bool CLOAK_CALL_THIS isEmpty() { return v1.size() == 0 && v2.size() == 0; }
				void CLOAK_CALL_THIS clear() { v1.clear(); v2.clear(); }
				void CLOAK_CALL_THIS resize(In size_t size) { v1.resize(size); v2.resize(size); }
				void CLOAK_CALL_THIS setAll(In F val)
				{
					const size_t v1s = v1.size();
					const size_t v2s = v2.size();
					for (size_t a = 0; a < v1s || a < v2s; a++)
					{
						if (a < v1s) { v1[a] = val; }
						if (a < v2s) { v2[a] = val; }
					}
				}
				Ret_notnull V* CLOAK_CALL_THIS get()
				{
					if (uv) { return &v1; }
					return &v2;
				}
				Ret_notnull V* CLOAK_CALL_THIS operator->() { return get(); }
				F& CLOAK_CALL_THIS operator[](In size_t a)
				{
					if (uv) { return v1[a]; }
					return v2[a];
				}
				const F& CLOAK_CALL_THIS operator[](In size_t a)const
				{
					if (uv) { return v1[a]; }
					return v2[a];
				}
				CLOAK_CALL_THIS operator V&()
				{
					if (uv) { return v1; }
					return v2;
				}
				void* CLOAK_CALL operator new(In size_t s) { return API::Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }
		};
	}
}

#endif