#pragma once
#ifndef CE_API_FILES_HASH_FNV_H
#define CE_API_FILES_HASH_FNV_H

#include "CloakEngine/Defines.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			namespace Hash {
				struct FNV {
					private:
						template<size_t S> struct FNV_DATA {};
						template<> struct FNV_DATA<4> {
							static constexpr uint32_t Prime = (1Ui32 << 24) + (1Ui32 << 8) + 0x93Ui32;
							static constexpr uint32_t Offset = 0x811c9dc5;
						};
						template<> struct FNV_DATA<8> {
							static constexpr uint64_t Prime = (1Ui64 << 40) + (1Ui64 << 8) + 0xb3Ui64;
							static constexpr uint64_t Offset = 0xcbf29ce484222325;
						};

						CLOAK_CALL FNV() {}
					public:
						template<typename T> static CLOAK_FORCEINLINE T CLOAK_CALL Calculate(In size_t length, In_reads(length) const uint8_t* data)
						{
							T h = static_cast<T>(FNV_DATA<sizeof(T)>::Offset);
							for (size_t a = 0; a < length; a++)
							{
								T l = h;
								h ^= data[a];
								if (h == 0) { h = (l << 1) ^ (data[a] + 1); }
								h *= static_cast<T>(FNV_DATA<sizeof(T)>::Prime);
							}
							return h;
						}
				};
			}
		}
	}
}

#endif