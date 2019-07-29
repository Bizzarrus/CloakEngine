#pragma once
#ifndef CE_API_FILES_HASH_SHA256_H
#define CE_API_FILES_HASH_SHA256_H

#include "CloakEngine/Defines.h"

#include <stdint.h>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			namespace Hash {
				class CLOAKENGINE_API SHA256 {
					public:
						static constexpr size_t DIGEST_SIZE = 256 / 8;
						CLOAK_CALL SHA256();
						void CLOAK_CALL_THIS Update(In const uint8_t* data, In size_t len);
						void CLOAK_CALL_THIS Finish(Out uint8_t digest[DIGEST_SIZE]);
					protected:
						static constexpr size_t BLOCK_SIZE = DIGEST_SIZE << 1;
						void CLOAK_CALL_THIS Insert(In const uint8_t* data, In size_t len);
						uint8_t m_block[BLOCK_SIZE << 1];
						uint32_t m_h[8];
						size_t m_len;
						size_t m_tlen;
				};
			}
		}
	}
}

#endif