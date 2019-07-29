#pragma once
#ifndef CE_E_HASH_H
#define CE_E_HASH_H

#include "CloakEngine/Defines.h"

namespace CloakEngine {
	namespace Engine {
		namespace Hash {
			inline size_t CLOAK_CALL_FAST Iterate(In size_t next, In_opt size_t current = 2166136261U)
			{
				return 16777619U * current ^ next;
			}
			template<typename T> inline size_t CLOAK_CALL_FAST Range(In const T* begin, In const T* end, In_opt size_t init = 2166136261U)
			{
				while (begin < end) { init = Iterate(static_cast<size_t>(*begin++), init); }
				return init;
			}
			template<typename T> inline size_t CLOAK_CALL_FAST StateArray(In const T* state, In size_t count, In_opt size_t init = 2166136261U)
			{
				static_assert((sizeof(T) & 0x3) == 0, "Object must be word-aligned!");
				return Range((UINT*)state, (UINT*)state + count, init);
			}
			template<typename T> inline size_t CLOAK_CALL_FAST State(In const T* state, In_opt size_t init = 2166136261U)
			{
				static_assert((sizeof(T) & 0x3) == 0, "Object must be word-aligned!");
				return Range((UINT*)state, (UINT*)state + 1, init);
			}
		}
	}
}

#endif
