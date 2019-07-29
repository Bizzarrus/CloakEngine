#pragma once
#ifndef CE_API_FILES_HASH_BLAKE2_H
#define CE_API_FILES_HASH_BLAKE2_H

#include "CloakEngine/Defines.h"

#include <stdint.h>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			namespace Hash {
				class CLOAKENGINE_API Blake2 {
					public:
						static constexpr size_t DIGEST_SIZE = 64;
						CLOAK_CALL Blake2();
						CLOAK_CALL Blake2(In_reads(keyLen) uint8_t* key, In_range(0, 64) size_t keyLen);
						void CLOAK_CALL_THIS Update(In const uint8_t* data, In size_t len);
						void CLOAK_CALL_THIS Finish(Out uint8_t digest[DIGEST_SIZE]);
					protected:
						void CLOAK_CALL_THIS Compress(In_opt bool fin = false);

						static constexpr size_t BLOCK_SIZE = 128;
						uint8_t m_input[BLOCK_SIZE];
						uint64_t m_hash[DIGEST_SIZE / sizeof(uint64_t)];
						uint64_t m_inputCount[2];
						size_t m_curInput;
				};
			}
		}
	}
}

#endif