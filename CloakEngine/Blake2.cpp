#include "stdafx.h"
#include "CloakEngine/Files/Hash/Blake2.h"

//Original algorithm by Jean-Philippe Aumasson, Samuel Neves, Zooko Wilcox-O'Hearn, and Christian Winnerlein.
//Implementation based on https://tools.ietf.org/html/rfc7693

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			namespace Hash {
				constexpr size_t ROUNDS = 12;
				constexpr uint64_t IV[Blake2::DIGEST_SIZE / sizeof(uint64_t)] = { 
					0x6A09E667F3BCC908 ^ 0x01010000 ^ Blake2::DIGEST_SIZE,
					0xBB67AE8584CAA73B,
					0x3C6EF372FE94F82B, 
					0xA54FF53A5F1D36F1,
					0x510E527FADE682D1, 
					0x9B05688C2B3E6C1F,
					0x1F83D9ABFB41BD6B, 
					0x5BE0CD19137E2179 
				};
				constexpr uint8_t SIGMA[12][16] = {
					{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
					{ 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
					{ 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
					{ 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
					{ 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
					{ 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
					{ 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
					{ 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
					{ 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
					{ 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 },
					{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
					{ 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 }
				};

				CLOAK_FORCEINLINE uint64_t CLOAK_CALL Rotate(In uint64_t a, In size_t b) 
				{
					CLOAK_ASSUME(b <= 64);
					return (a >> b) | (a << (64 - b)); 
				}
				CLOAK_FORCEINLINE void CLOAK_CALL Mix(Inout uint64_t* a, Inout uint64_t* b, Inout uint64_t* c, Inout uint64_t* d, In uint64_t x, In uint64_t y)
				{
					*a += (*b) + x;
					*d = Rotate((*d) ^ (*a), 32);
					*d += (*d);
					*b = Rotate((*b) ^ (*c), 24);
					*a += (*b) + y;
					*d = Rotate((*d) ^ (*a), 16);
					*c += (*d);
					*b = Rotate((*b) ^ (*c), 63);
				}

				CLOAK_CALL Blake2::Blake2() : Blake2(nullptr, 0) {}
				CLOAK_CALL Blake2::Blake2(In_reads(keyLen) uint8_t* key, In_range(0, 64) size_t keyLen)
				{
					CLOAK_ASSUME(keyLen <= 64);
					for (size_t a = 0; a < ARRAYSIZE(m_hash); a++) { m_hash[a] = IV[a]; }
					m_hash[0] ^= keyLen << 8;
					m_curInput = 0;
					m_inputCount[0] = m_inputCount[1] = 0;
					for (size_t a = keyLen; a < BLOCK_SIZE; a++) { m_input[a] = 0; }
					if (keyLen > 0)
					{
						Update(key, keyLen);
						m_curInput = BLOCK_SIZE;
					}
				}
				void CLOAK_CALL_THIS Blake2::Update(In const uint8_t* data, In size_t len)
				{
					for (size_t a = 0; a < len; a++)
					{
						if (m_curInput == BLOCK_SIZE)
						{
							m_inputCount[0] += BLOCK_SIZE;
							if (m_inputCount[0] < BLOCK_SIZE) { m_inputCount[1]++; } //overflow
							Compress();
							m_curInput = 0;
						}
						m_input[m_curInput++] = data[a];
					}
				}
				void CLOAK_CALL_THIS Blake2::Finish(Out uint8_t digest[DIGEST_SIZE])
				{
					m_inputCount[0] += m_curInput;
					if (m_inputCount[0] < m_curInput) { m_inputCount[1]++; } //overflow
					while (m_curInput < BLOCK_SIZE) { m_input[m_curInput++] = 0; }
					Compress(true);
					for (size_t a = 0; a < DIGEST_SIZE; a++)
					{
						digest[a] = static_cast<uint8_t>((m_hash[a >> 3] >> ((a & 0x07) << 3)) & 0xFF);
					}
				}

				void CLOAK_CALL_THIS Blake2::Compress(In_opt bool fin)
				{
					uint64_t a[2 * ARRAYSIZE(m_hash)];
					uint64_t b[2 * ARRAYSIZE(m_hash)];

					//Init values:
					for (size_t c = 0; c < ARRAYSIZE(m_hash); c++)
					{
						a[c] = m_hash[c];
						a[c + 8] = IV[c];
					}
					a[12] ^= m_inputCount[0];
					a[13] ^= m_inputCount[1];
					if (fin) { a[14] = ~a[14]; }

					for (size_t c = 0; c < ARRAYSIZE(b); c++)
					{
						b[c] = 0;
						for (size_t d = 0; d < sizeof(uint64_t); d++)
						{
							CLOAK_ASSUME((c * sizeof(uint64_t)) + d < ARRAYSIZE(m_input));
							b[c] |= m_input[(c * sizeof(uint64_t)) + d] << (d << 3);
						}
					}

					//Rotations:
					for (size_t c = 0; c < ROUNDS; c++)
					{
						Mix(&a[0], &a[4], &a[ 8], &a[12], b[SIGMA[c][ 0]], b[SIGMA[c][ 1]]);
						Mix(&a[1], &a[5], &a[ 9], &a[13], b[SIGMA[c][ 2]], b[SIGMA[c][ 3]]);
						Mix(&a[2], &a[6], &a[10], &a[14], b[SIGMA[c][ 4]], b[SIGMA[c][ 5]]);
						Mix(&a[3], &a[7], &a[11], &a[15], b[SIGMA[c][ 6]], b[SIGMA[c][ 7]]);
						Mix(&a[0], &a[5], &a[10], &a[15], b[SIGMA[c][ 8]], b[SIGMA[c][ 9]]);
						Mix(&a[1], &a[6], &a[11], &a[12], b[SIGMA[c][10]], b[SIGMA[c][11]]);
						Mix(&a[2], &a[7], &a[ 8], &a[13], b[SIGMA[c][12]], b[SIGMA[c][13]]);
						Mix(&a[3], &a[4], &a[ 9], &a[14], b[SIGMA[c][14]], b[SIGMA[c][15]]);
					}

					//Store:
					for (size_t c = 0; c < ARRAYSIZE(m_hash); c++)
					{
						m_hash[c] = a[c] ^ a[c + 8];
					}
				}
			}
		}
	}
}