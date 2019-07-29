#include "stdafx.h"
#include "CloakEngine/Files/Hash/SHA256.h"
#include "CloakEngine/Global/Debug.h"

#define INIT_SHA() m_h[0]=0x6a09e667; m_h[1]=0xbb67ae85; m_h[2]=0x3c6ef372; m_h[3]=0xa54ff53a; m_h[4]=0x510e527f; m_h[5]=0x9b05688c; m_h[6]=0x1f83d9ab; m_h[7]=0x5be0cd19
#define SHA_SHFR(a,b) ((a)>>(b))
#define SHA_ROTR(a,b) (((a)>>(b)) | ((a)<<(32-(b))))
#define SHA_ROTL(a,b) (((a)<<(b)) | ((a)>>(32-(b))))
#define SHA_CH(a,b,c) (((a) & (b))^(~(a) & (c)))
#define SHA_MAJ(a,b,c) (((a) & (b))^((a) & (c))^((b) & (c)))
#define SHA_F1(a) (SHA_ROTR(a,2)^SHA_ROTR(a,13)^SHA_ROTR(a,22))
#define SHA_F2(a) (SHA_ROTR(a,6)^SHA_ROTR(a,11)^SHA_ROTR(a,25))
#define SHA_F3(a) (SHA_ROTR(a,7)^SHA_ROTR(a,18)^SHA_SHFR(a,3))
#define SHA_F4(a) (SHA_ROTR(a,17)^SHA_ROTR(a,19)^SHA_SHFR(a,10))
#define SHA_UNPACK8(a,b,c,d) *(reinterpret_cast<uint8_t*>(b)+(d))=static_cast<uint8_t>((a)>>(c))
#define SHA_UNPACK32(a,b) SHA_UNPACK8(a,b,0,3); SHA_UNPACK8(a,b,8,2); SHA_UNPACK8(a,b,16,1); SHA_UNPACK8(a,b,24,0)
#define SHA_PACK8(b,c) (static_cast<uint32_t>(b)<<(c))
#define SHA_PACK32(a,b) *(b)=SHA_PACK8((a)[3],0) | SHA_PACK8((a)[2],8) | SHA_PACK8((a)[1],16) | SHA_PACK8((a)[0],24)

//TODO: Use hardware-accelerated calculation (SSE SHA instruction set)

namespace CloakEngine {
	namespace API {
		namespace Files {
			namespace Hash {
				static constexpr uint32_t TABLE[] = {
					0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
					0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
					0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
					0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
					0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
					0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
					0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
					0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
					0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
					0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
					0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
					0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
					0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
					0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
					0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
					0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
				};

				CLOAK_CALL SHA256::SHA256()
				{
					INIT_SHA();
					m_len = 0; 
					m_tlen = 0; 
					memset(m_block, 0, BLOCK_SIZE << 1);
				}
				void CLOAK_CALL_THIS SHA256::Update(In const uint8_t* data, In size_t len)
				{
					size_t tlen = BLOCK_SIZE - m_len;
					size_t rlen = min(len, tlen);
					memcpy(&m_block[m_len], data, rlen);
					if (m_len + len < BLOCK_SIZE) { m_len += len; }
					else
					{
						size_t nlen = len - rlen;
						size_t bc = nlen / BLOCK_SIZE;
						const uint8_t* sd = &data[rlen];
						Insert(m_block, 1);
						Insert(sd, bc);
						rlen = nlen % BLOCK_SIZE;
						memcpy(m_block, &sd[bc << 6], rlen);
						m_len = rlen;
						m_tlen += (bc + 1) << 6;
					}
				}
				void CLOAK_CALL_THIS SHA256::Finish(Out uint8_t digest[DIGEST_SIZE])
				{
					size_t bc = 1;
					if (BLOCK_SIZE - 9 < m_len % BLOCK_SIZE) { bc++; }
					size_t blen = (m_tlen + m_len) << 3;
					memset(&m_block[m_len], 0, BLOCK_SIZE - m_len);
					m_block[m_len] = 0x80;
					SHA_UNPACK32(blen, &m_block[BLOCK_SIZE - 4]);
					Insert(m_block, bc);
					for (size_t a = 0; a < 8; a++) { SHA_UNPACK32(m_h[a], &digest[a << 2]); }
					INIT_SHA();
				}

				void CLOAK_CALL_THIS SHA256::Insert(In const uint8_t* data, In size_t len)
				{
					uint32_t w[BLOCK_SIZE];
					uint32_t h[ARRAYSIZE(m_h)];
					uint32_t t[2];
					const uint8_t* block;
					for (size_t a = 0; a < len; a++)
					{
						block = &data[a << 6];
						for (size_t b = 0; b < 16; b++) { SHA_PACK32(&block[b << 2], &w[b]); }
						for (size_t b = 16; b < BLOCK_SIZE; b++) { w[b] = SHA_F4(w[b - 2]) + w[b - 7] + SHA_F3(w[b - 15]) + w[b - 16]; }
						for (size_t b = 0; b < 8; b++) { h[b] = m_h[b]; }
						for (size_t b = 0; b < BLOCK_SIZE; b++)
						{
							t[0] = h[7] + SHA_F2(h[4]) + SHA_CH(h[4], h[5], h[6]) + TABLE[b] + w[b];
							t[1] = SHA_F1(h[0]) + SHA_MAJ(h[0], h[1], h[2]);
							h[7] = h[6];
							h[6] = h[5];
							h[5] = h[4];
							h[4] = h[3] + t[0];
							h[3] = h[2];
							h[2] = h[1];
							h[1] = h[0];
							h[0] = t[0] + t[1];
						}
						for (size_t b = 0; b < ARRAYSIZE(m_h); b++) { m_h[b] += h[b]; }
					}
				}
			}
		}
	}
}