#include "stdafx.h"
#include "Engine/Compress/NetBuffer/SecureNet.h"
#include "CloakEngine/Global/Memory.h"

#include "CloakEngine/Files/Hash/SHA256.h"
#include "CloakEngine/Files/Hash/Blake2.h"

#include <random>

//See https://de.wikipedia.org/wiki/Diffie-Hellman-Schl%C3%BCsselaustausch for algorithm
//See https://datatracker.ietf.org/doc\rfc3526/?include_text=1 for prime/generator (2048-bit prefered)
//Consider AES ( https://de.wikipedia.org/wiki/Advanced_Encryption_Standard ) as decryption algorithm
	// See https://github.com/weidai11/cryptopp/blob/master\rijndael.cpp for an implementation
//Also: Consider switching to unified diffie-hellman (DH2), also found in crytopp
//shorten generated key by using SHA-256 before using it for AES

#define ID_SERVER_OFFSET 13
#define ID_CLIENT_OFFSET 5

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace NetBuffer {
				constexpr size_t SBOX_SIZE = 256;
				constexpr uint8_t SBOX[SBOX_SIZE*2] = {
					0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
					0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
					0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
					0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
					0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
					0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
					0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
					0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
					0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
					0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
					0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
					0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
					0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
					0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
					0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
					0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
					0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
					0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
					0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
					0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
					0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
					0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
					0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
					0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
					0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
					0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
					0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
					0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
					0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
					0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
					0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
					0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d,
				};
				constexpr uint8_t RCON[] = {
					0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a,
					0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39,
				};
				constexpr uint8_t LOG_TABLE[] = {
					0x00,  0x00,  0x19,  0x01,  0x32,  0x02,  0x1a,  0xc6,  0x4b,  0xc7,  0x1b,  0x68,  0x33,  0xee,  0xdf,  0x03,
					0x64,  0x04,  0xe0,  0x0e,  0x34,  0x8d,  0x81,  0xef,  0x4c,  0x71,  0x08,  0xc8,  0xf8,  0x69,  0x1c,  0xc1,
					0x7d,  0xc2,  0x1d,  0xb5,  0xf9,  0xb9,  0x27,  0x6a,  0x4d,  0xe4,  0xa6,  0x72,  0x9a,  0xc9,  0x09,  0x78,
					0x65,  0x2f,  0x8a,  0x05,  0x21,  0x0f,  0xe1,  0x24,  0x12,  0xf0,  0x82,  0x45,  0x35,  0x93,  0xda,  0x8e,
					0x96,  0x8f,  0xdb,  0xbd,  0x36,  0xd0,  0xce,  0x94,  0x13,  0x5c,  0xd2,  0xf1,  0x40,  0x46,  0x83,  0x38,
					0x66,  0xdd,  0xfd,  0x30,  0xbf,  0x06,  0x8b,  0x62,  0xb3,  0x25,  0xe2,  0x98,  0x22,  0x88,  0x91,  0x10,
					0x7e,  0x6e,  0x48,  0xc3,  0xa3,  0xb6,  0x1e,  0x42,  0x3a,  0x6b,  0x28,  0x54,  0xfa,  0x85,  0x3d,  0xba,
					0x2b,  0x79,  0x0a,  0x15,  0x9b,  0x9f,  0x5e,  0xca,  0x4e,  0xd4,  0xac,  0xe5,  0xf3,  0x73,  0xa7,  0x57,
					0xaf,  0x58,  0xa8,  0x50,  0xf4,  0xea,  0xd6,  0x74,  0x4f,  0xae,  0xe9,  0xd5,  0xe7,  0xe6,  0xad,  0xe8,
					0x2c,  0xd7,  0x75,  0x7a,  0xeb,  0x16,  0x0b,  0xf5,  0x59,  0xcb,  0x5f,  0xb0,  0x9c,  0xa9,  0x51,  0xa0,
					0x7f,  0x0c,  0xf6,  0x6f,  0x17,  0xc4,  0x49,  0xec,  0xd8,  0x43,  0x1f,  0x2d,  0xa4,  0x76,  0x7b,  0xb7,
					0xcc,  0xbb,  0x3e,  0x5a,  0xfb,  0x60,  0xb1,  0x86,  0x3b,  0x52,  0xa1,  0x6c,  0xaa,  0x55,  0x29,  0x9d,
					0x97,  0xb2,  0x87,  0x90,  0x61,  0xbe,  0xdc,  0xfc,  0xbc,  0x95,  0xcf,  0xcd,  0x37,  0x3f,  0x5b,  0xd1,
					0x53,  0x39,  0x84,  0x3c,  0x41,  0xa2,  0x6d,  0x47,  0x14,  0x2a,  0x9e,  0x5d,  0x56,  0xf2,  0xd3,  0xab,
					0x44,  0x11,  0x92,  0xd9,  0x23,  0x20,  0x2e,  0x89,  0xb4,  0x7c,  0xb8,  0x26,  0x77,  0x99,  0xe3,  0xa5,
					0x67,  0x4a,  0xed,  0xde,  0xc5,  0x31,  0xfe,  0x18,  0x0d,  0x63,  0x8c,  0x80,  0xc0,  0xf7,  0x70,  0x07,
				};
				constexpr uint8_t EXP_TABLE[] = {
					0x01,  0x03,  0x05,  0x0f,  0x11,  0x33,  0x55,  0xff,  0x1a,  0x2e,  0x72,  0x96,  0xa1,  0xf8,  0x13,  0x35,
					0x5f,  0xe1,  0x38,  0x48,  0xd8,  0x73,  0x95,  0xa4,  0xf7,  0x02,  0x06,  0x0a,  0x1e,  0x22,  0x66,  0xaa,
					0xe5,  0x34,  0x5c,  0xe4,  0x37,  0x59,  0xeb,  0x26,  0x6a,  0xbe,  0xd9,  0x70,  0x90,  0xab,  0xe6,  0x31,
					0x53,  0xf5,  0x04,  0x0c,  0x14,  0x3c,  0x44,  0xcc,  0x4f,  0xd1,  0x68,  0xb8,  0xd3,  0x6e,  0xb2,  0xcd,
					0x4c,  0xd4,  0x67,  0xa9,  0xe0,  0x3b,  0x4d,  0xd7,  0x62,  0xa6,  0xf1,  0x08,  0x18,  0x28,  0x78,  0x88,
					0x83,  0x9e,  0xb9,  0xd0,  0x6b,  0xbd,  0xdc,  0x7f,  0x81,  0x98,  0xb3,  0xce,  0x49,  0xdb,  0x76,  0x9a,
					0xb5,  0xc4,  0x57,  0xf9,  0x10,  0x30,  0x50,  0xf0,  0x0b,  0x1d,  0x27,  0x69,  0xbb,  0xd6,  0x61,  0xa3,
					0xfe,  0x19,  0x2b,  0x7d,  0x87,  0x92,  0xad,  0xec,  0x2f,  0x71,  0x93,  0xae,  0xe9,  0x20,  0x60,  0xa0,
					0xfb,  0x16,  0x3a,  0x4e,  0xd2,  0x6d,  0xb7,  0xc2,  0x5d,  0xe7,  0x32,  0x56,  0xfa,  0x15,  0x3f,  0x41,
					0xc3,  0x5e,  0xe2,  0x3d,  0x47,  0xc9,  0x40,  0xc0,  0x5b,  0xed,  0x2c,  0x74,  0x9c,  0xbf,  0xda,  0x75,
					0x9f,  0xba,  0xd5,  0x64,  0xac,  0xef,  0x2a,  0x7e,  0x82,  0x9d,  0xbc,  0xdf,  0x7a,  0x8e,  0x89,  0x80,
					0x9b,  0xb6,  0xc1,  0x58,  0xe8,  0x23,  0x65,  0xaf,  0xea,  0x25,  0x6f,  0xb1,  0xc8,  0x43,  0xc5,  0x54,
					0xfc,  0x1f,  0x21,  0x63,  0xa5,  0xf4,  0x07,  0x09,  0x1b,  0x2d,  0x77,  0x99,  0xb0,  0xcb,  0x46,  0xca,
					0x45,  0xcf,  0x4a,  0xde,  0x79,  0x8b,  0x86,  0x91,  0xa8,  0xe3,  0x3e,  0x42,  0xc6,  0x51,  0xf3,  0x0e,
					0x12,  0x36,  0x5a,  0xee,  0x29,  0x7b,  0x8d,  0x8c,  0x8f,  0x8a,  0x85,  0x94,  0xa7,  0xf2,  0x0d,  0x17,
					0x39,  0x4b,  0xdd,  0x7c,  0x84,  0x97,  0xa2,  0xfd,  0x1c,  0x24,  0x6c,  0xb4,  0xc7,  0x52,  0xf6,  0x01,
				};

				constexpr size_t BINARY_SIZE = SecureNetBuffer::PRIME_BIT_LEN / (8 * sizeof(uint32_t));
				constexpr uint32_t PRIME_BINARY[BINARY_SIZE] = {
					0xFFFFFFFF, 0xFFFFFFFF, 0xC90FDAA2, 0x2168C234, 0xC4C6628B, 0x80DC1CD1,
					0x29024E08, 0x8A67CC74, 0x020BBEA6, 0x3B139B22, 0x514A0879, 0x8E3404DD,
					0xEF9519B3, 0xCD3A431B, 0x302B0A6D, 0xF25F1437, 0x4FE1356D, 0x6D51C245,
					0xE485B576, 0x625E7EC6, 0xF44C42E9, 0xA637ED6B, 0x0BFF5CB6, 0xF406B7ED,
					0xEE386BFB, 0x5A899FA5, 0xAE9F2411, 0x7C4B1FE6, 0x49286651, 0xECE45B3D,
					0xC2007CB8, 0xA163BF05, 0x98DA4836, 0x1C55D39A, 0x69163FA8, 0xFD24CF5F,
					0x83655D23, 0xDCA3AD96, 0x1C62F356, 0x208552BB, 0x9ED52907, 0x7096966D,
					0x670C354E, 0x4ABC9804, 0xF1746C08, 0xCA18217C, 0x32905E46, 0x2E36CE3B,
					0xE39E772C, 0x180E8603, 0x9B2783A2, 0xEC07A28F, 0xB5C55DF0, 0x6F4C52C9,
					0xDE2BCBF6, 0x95581718, 0x3995497C, 0xEA956AE5, 0x15D22618, 0x98FA0510,
					0x15728E5A, 0x8AACAA68, 0xFFFFFFFF, 0xFFFFFFFF,
				};
				typedef SecureNetBuffer::KeyInt KeyInt;

				constexpr inline uint8_t CLOAK_CALL MulRijndael(In uint8_t a, In uint8_t b)
				{
					if (a != 0 && b != 0) 
					{ 
						return EXP_TABLE[(LOG_TABLE[a] + LOG_TABLE[b]) % 0xFF];
					}
					return 0;
				}
				struct PrimeGen {
					KeyInt Prime;
					KeyInt Gen;
					CLOAK_CALL PrimeGen() : Prime(PRIME_BINARY), Gen(2) {}
					void* CLOAK_CALL operator new(In size_t s) { return API::Global::Memory::MemoryHeap::Allocate(s); }
					void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }
				};
				PrimeGen* g_primeGen;
				
				struct MulStack {
					KeyInt B;
					bool Before = true;
				};
				inline KeyInt CLOAK_CALL generateRandomInt()
				{
					constexpr size_t binSize = SecureNetBuffer::SECRET_BYTE_LEN / sizeof(uint32_t);
					uint32_t bin[binSize];
					std::random_device RDevice;
					std::default_random_engine REngine(RDevice());
					std::uniform_int_distribution<uint32_t> RDist(0);
					for (size_t a = 0; a < binSize; a++) { bin[a] = RDist(REngine); }
					return bin;
				}
				inline KeyInt CLOAK_CALL generateIntFromCGID(In const API::Files::UGID& cgid)
				{
					uint32_t bin[BINARY_SIZE];
					for (size_t a = 0; a < ARRAYSIZE(bin); a++) { bin[a] = cgid.ID[a % ARRAYSIZE(cgid.ID)]; }
					for (size_t a = 0; a < 16; a++)
					{
						uint8_t vr[API::Files::Hash::Blake2::DIGEST_SIZE];
						API::Files::Hash::Blake2 vh;
						vh.Update(reinterpret_cast<uint8_t*>(bin), ARRAYSIZE(bin) * sizeof(uint32_t));
						vh.Finish(vr);
						for (size_t b = 0; b < max(ARRAYSIZE(bin), ARRAYSIZE(vr)); b++)
						{
							bin[b % ARRAYSIZE(bin)] ^= static_cast<uint32_t>(vr[(b + (a >> 2)) % ARRAYSIZE(vr)]) << (((a + 1) % 4) << 3);
						}
					}
					return bin;
				}
				inline KeyInt CLOAK_CALL generateKey(In const KeyInt& src)
				{
					API::Files::Hash::SHA256 hash;
					constexpr size_t ms = (BINARY_SIZE * sizeof(uint32_t)) / sizeof(KeyInt::type);
					KeyInt::type s[ms];
					for (size_t a = 0; a < ms; a++) { s[a] = src[a]; }
					hash.Update(reinterpret_cast<uint8_t*>(s), ms * sizeof(KeyInt::type));
					uint8_t k[API::Files::Hash::SHA256::DIGEST_SIZE];
					hash.Finish(k);
					return k;
				}


				void* CLOAK_CALL SecureNetBuffer::KeyBuilding::operator new(In size_t s) { return API::Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL SecureNetBuffer::KeyBuilding::operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }


				void CLOAK_CALL SecureNetBuffer::Initialize()
				{
					g_primeGen = new PrimeGen();
				}
				void CLOAK_CALL SecureNetBuffer::Terminate()
				{
					delete g_primeGen;
				}
				/*
				1. Client: Send key part (decrypted with default key) to server
				2. Server: Send key part (decrypted with default key) & ID (decrypted with new key + server ID key) to client
				3. Client: Send ID (decrypted with new key + client ID key) to server
				*/
				CLOAK_CALL SecureNetBuffer::SecureNetBuffer(In Impl::Global::Connection_v1::INetBuffer* prev, In const API::Files::UGID& cgid, In bool lobby) : m_writeBuffer(this), m_readBuffer(this), m_lobby(lobby)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					DEBUG_NAME(SecureNetBuffer);
					m_prev = prev;
					m_callback = nullptr;
					m_keyBuild = new KeyBuilding();
					m_keyBuild->Exponent = generateRandomInt();

					m_keyBuild->Source = g_primeGen->Gen.pow_mod(m_keyBuild->Exponent, g_primeGen->Prime);
					m_write.posOffset = 0;
					m_read.posOffset = 0;
					m_state = lobby ? STATE_SERVER_START : STATE_CLIENT_START;
					m_key = generateKey(generateIntFromCGID(cgid));
					m_write.bufPos = 0;
					m_read.bufPos = AES_BUFFER_SIZE;
					m_read.bufFill = 0;
					m_read.maxFill = AES_BUFFER_SIZE;
					for (size_t a = 0; a < API::Files::UGID::ID_SIZE; a++) { m_keyBuild->ID[a] = cgid.ID[a]; }
					m_prev->SetCallback([=](Impl::Global::Connection_v1::CallbackType t) {
						if (t == Impl::Global::Connection_v1::CallbackType::Connected) { OnParConnect(); }
						else if (t == Impl::Global::Connection_v1::CallbackType::Disconnected) { OnParDisconnect(); }
						else if (t == Impl::Global::Connection_v1::CallbackType::ReceivedData) { OnParRecData(); }
					});
				}
				CLOAK_CALL SecureNetBuffer::~SecureNetBuffer()
				{
					delete m_keyBuild;
					SAVE_RELEASE(m_prev);
					SAVE_RELEASE(m_sync);
				}
				void CLOAK_CALL_THIS SecureNetBuffer::OnWrite()
				{
					API::Helper::Lock lock(m_sync);
					m_prev->OnWrite();
				}
				void CLOAK_CALL_THIS SecureNetBuffer::OnRead(In_opt uint8_t* data, In_opt size_t dataSize)
				{
					API::Helper::Lock lock(m_sync);
					m_prev->OnRead(data, dataSize);
				}
				void CLOAK_CALL_THIS SecureNetBuffer::OnUpdate()
				{
					API::Helper::Lock lock(m_sync);
					m_prev->OnUpdate();
				}
				void CLOAK_CALL_THIS SecureNetBuffer::OnConnect()
				{
					API::Helper::Lock lock(m_sync);
					m_prev->OnConnect();
				}
				void CLOAK_CALL_THIS SecureNetBuffer::SetCallback(In Impl::Global::Connection_v1::ConnectionCallback cb)
				{
					API::Helper::Lock lock(m_sync);
					m_callback = cb;
				}
				bool CLOAK_CALL_THIS SecureNetBuffer::IsConnected()
				{
					API::Helper::Lock lock(m_sync);
					return m_state == STATE_OK && m_prev->IsConnected();
				}
				API::Files::Buffer_v1::IWriteBuffer* CLOAK_CALL_THIS SecureNetBuffer::GetWriteBuffer()
				{
					return &m_writeBuffer;
				}
				API::Files::Buffer_v1::IReadBuffer* CLOAK_CALL_THIS SecureNetBuffer::GetReadBuffer()
				{
					return &m_readBuffer;
				}
				unsigned long long CLOAK_CALL_THIS SecureNetBuffer::GetWritePos() const
				{
					API::Helper::Lock lock(m_sync);
					if (m_keyBuild == nullptr) { return m_prev->GetWritePos() - m_write.posOffset; }
					return 0;
				}
				unsigned long long CLOAK_CALL_THIS SecureNetBuffer::GetReadPos() const
				{
					API::Helper::Lock lock(m_sync);
					if (m_keyBuild == nullptr) { return m_prev->GetReadPos() - m_read.posOffset; }
					return 0;
				}
				void CLOAK_CALL_THIS SecureNetBuffer::WriteByte(In uint8_t b)
				{
					API::Helper::Lock lock(m_sync);
					m_write.buffer[m_write.bufPos] = b;
					m_write.bufPos++;
					if (m_write.bufPos == AES_BUFFER_SIZE) { iEncrypt(); }
				}
				uint8_t CLOAK_CALL_THIS SecureNetBuffer::ReadByte()
				{
					API::Helper::Lock lock(m_sync);
					if (m_read.bufPos >= m_read.maxFill) 
					{
						if (m_prev->HasData() == false) { return 0; }
						if (m_read.bufFill == 0) 
						{ 
							m_read.maxFill = m_prev->ReadByte(); 
						}
						for (size_t a = m_read.bufFill; a < AES_BUFFER_SIZE; a++)
						{
							if (m_prev->HasData()) 
							{
								m_read.buffer[a] = m_prev->ReadByte();
								m_read.bufFill++;
							}
							else { return 0; }
						}
						iDecrypt();
						m_read.bufFill = 0;
						m_read.bufPos = 0;
					}
					uint8_t b = m_read.buffer[m_read.bufPos];
					m_read.bufPos++;
					return b;
				}
				void CLOAK_CALL_THIS SecureNetBuffer::SendData(In_opt bool FragRef)
				{
					API::Helper::Lock lock(m_sync);
					iEncrypt();
					m_prev->SendData(FragRef);
				}
				bool CLOAK_CALL_THIS SecureNetBuffer::HasData()
				{
					API::Helper::Lock lock(m_sync);
					return m_read.bufPos < AES_BUFFER_SIZE || m_prev->HasData();
				}

				void CLOAK_CALL_THIS SecureNetBuffer::OnParConnect()
				{
					API::Helper::Lock lock(m_sync);
					if (m_state == STATE_CLIENT_START)
					{
						m_state = STATE_CLIENT_BLOCK;
						uint8_t srcD[INT_BYTE_SIZE];
						for (size_t a = INT_BYTE_SIZE, b = 0; a >= sizeof(KeyInt::type); a -= sizeof(KeyInt::type), b++)
						{
							KeyInt::type blk = m_keyBuild->Source[b];
							for (size_t c = 0; c < sizeof(KeyInt::type); c++)
							{
								srcD[a - (c + 1)] = static_cast<uint8_t>((blk >> (c * 8)) & 0xFF);
							}
						}
						for (size_t a = 0; a < INT_BYTE_SIZE; a++) { WriteByte(srcD[a]); }
						SendData();
						m_state = STATE_CLIENT_WAIT;
					}
				}
				void CLOAK_CALL_THIS SecureNetBuffer::OnParDisconnect()
				{
					API::Helper::Lock lock(m_sync);
					if (m_state != STATE_OK) { m_state = STATE_ERROR; }
					if (m_callback) { m_callback(Impl::Global::CallbackType::Disconnected); }
				}
				void CLOAK_CALL_THIS SecureNetBuffer::OnParRecData()
				{
					API::Helper::Lock lock(m_sync);
					if (m_state == STATE_CLIENT_WAIT)
					{
						m_state = STATE_CLIENT_BLOCK;
						uint8_t data[INT_BYTE_SIZE];
						for (size_t a = 0; a < INT_BYTE_SIZE; a++) { data[a] = ReadByte(); }
						KeyInt n = data;
						m_key = generateKey(n.pow_mod(m_keyBuild->Exponent, g_primeGen->Prime));
						uint8_t rID[API::Files::UGID::ID_SIZE];
						for (size_t a = 0; a < API::Files::UGID::ID_SIZE; a++)
						{
							size_t p = (a + ID_SERVER_OFFSET) % API::Files::UGID::ID_SIZE;
							rID[p] = ReadByte();
						}
						bool suc = true;
						for (size_t a = 0; a < API::Files::UGID::ID_SIZE; a++)
						{
							if (rID[a] != m_keyBuild->ID[a]) 
							{ 
								suc = false; 
								break; 
							}
						}
						for (size_t a = 0; a < API::Files::UGID::ID_SIZE; a++)
						{
							size_t p = (a + ID_CLIENT_OFFSET) % API::Files::UGID::ID_SIZE;
							WriteByte(m_keyBuild->ID[p]);
						}
						delete m_keyBuild;
						m_keyBuild = nullptr;
						SendData();
						m_write.posOffset = static_cast<unsigned int>(m_prev->GetWritePos());
						m_read.posOffset = static_cast<unsigned int>(m_prev->GetReadPos());
						if (suc)
						{
							m_state = STATE_OK;
							if (m_callback) { m_callback(Impl::Global::CallbackType::Connected); }
						}
						else
						{
							m_state = STATE_ERROR;
							if (m_callback) { m_callback(Impl::Global::CallbackType::Disconnected); }
						}
					}
					else if (m_state == STATE_SERVER_START)
					{
						m_state = STATE_SERVER_BLOCK;
						uint8_t data[INT_BYTE_SIZE];
						for (size_t a = 0; a < INT_BYTE_SIZE; a++) { data[a] = ReadByte(); }
						KeyInt n = data;
						uint8_t srcD[INT_BYTE_SIZE];
						for (size_t a = INT_BYTE_SIZE, b = 0; a >= sizeof(KeyInt::type); a -= sizeof(KeyInt::type), b++)
						{
							KeyInt::type blk = m_keyBuild->Source[b];
							for (size_t c = 0; c < sizeof(KeyInt::type); c++)
							{
								srcD[a - (c + 1)] = static_cast<uint8_t>((blk >> (c * 8)) & 0xFF);
							}
						}
						for (size_t a = 0; a < INT_BYTE_SIZE; a++) { WriteByte(srcD[a]); }
						m_key = generateKey(n.pow_mod(m_keyBuild->Exponent, g_primeGen->Prime));
						for (size_t a = 0; a < API::Files::UGID::ID_SIZE; a++)
						{
							size_t p = (a + ID_SERVER_OFFSET) % API::Files::UGID::ID_SIZE;
							WriteByte(m_keyBuild->ID[p]);
						}
						SendData();
						m_state = STATE_SERVER_WAIT;
					}
					else if (m_state == STATE_SERVER_WAIT)
					{
						m_state = STATE_SERVER_BLOCK;
						uint8_t rID[API::Files::UGID::ID_SIZE];
						for (size_t a = 0; a < API::Files::UGID::ID_SIZE; a++)
						{
							size_t p = (a + ID_CLIENT_OFFSET) % API::Files::UGID::ID_SIZE;
							rID[p] = ReadByte();
						}
						bool suc = true;
						for (size_t a = 0; a < API::Files::UGID::ID_SIZE && suc; a++)
						{
							if (rID[a] != m_keyBuild->ID[a]) { suc = false; }
						}
						delete m_keyBuild;
						m_keyBuild = nullptr;
						m_write.posOffset = static_cast<unsigned int>(m_prev->GetWritePos());
						m_read.posOffset = static_cast<unsigned int>(m_prev->GetReadPos());
						if (suc)
						{
							m_state = STATE_OK;
							if (m_callback) { m_callback(Impl::Global::CallbackType::Connected); }
						}
						else
						{
							m_state = STATE_ERROR;
							if (m_callback) { m_callback(Impl::Global::CallbackType::Disconnected); }
						}
					}
					else if (m_state == STATE_OK)
					{
						if (m_callback) { m_callback(Impl::Global::CallbackType::ReceivedData); }
					}
				}

				Success(return == true) bool CLOAK_CALL_THIS SecureNetBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS SecureNetBuffer::iEncrypt()
				{
					//TODO: Use hardware-accelerated encryption (SSE AES instruction set)
					API::Helper::Lock lock(m_sync);
					if (m_write.bufPos > 0)
					{
						//Expand key:
						uint8_t srcD[AES_BUFFER_SIZE];
						for (size_t a = AES_BUFFER_SIZE, b = 0; a >= sizeof(KeyInt::type); a -= sizeof(KeyInt::type), b++)
						{
							KeyInt::type blk = m_key[b];
							for (size_t c = 0; c < sizeof(KeyInt::type); c++)
							{
								srcD[a - (c + 1)] = static_cast<uint8_t>((blk >> (c * 8)) & 0xFF);
							}
						}
						uint8_t RK[AES_ROUNDS + 1][AES_COLUMS][AES_ROWS];
						for (size_t a = 0, c = 0; a < AES_COLUMS; a++)
						{
							for (size_t b = 0; b < AES_ROWS; b++, c++)
							{
								RK[0][a][b] = srcD[c];
							}
						}
						for (size_t r = 1; r <= AES_ROUNDS; r++)
						{
							for (size_t a = 0; a < AES_COLUMS; a++)
							{
								if (a % 8 == 0)
								{
									for (size_t b = 0; b < AES_ROWS; b++) { RK[r][a][b] = SBOX[RK[r - 1][(a + AES_COLUMS - 1) % AES_COLUMS][(b + 1) % AES_ROWS]]; }
									RK[r][a][0] ^= RCON[r];
									for (size_t b = 0; b < AES_ROWS; b++) { RK[r][a][b] ^= RK[r - 1][a][b]; }
								}
								else
								{
									for (size_t b = 0; b < AES_ROWS; b++) { RK[r][a][b] = RK[r - 1][(a + AES_COLUMS - 1) % AES_COLUMS][b]; }
									if (a % 4 == 0) 
									{ 
										for (size_t b = 0; b < AES_ROWS; b++) { RK[r][a][b] = SBOX[RK[r][a][b]]; }
									}
									for (size_t b = 0; b < AES_ROWS; b++) { RK[r][a][b] ^= RK[r - 1][a][b]; }
								}
							}
						}
						//Fill space
						for (size_t a = m_write.bufPos; a < AES_BUFFER_SIZE; a++) { m_write.buffer[a] = 0; }
						//Encrypt
						for (size_t r = 0; r < AES_ROUNDS; r++)
						{
							//AddRoundKey
							for (size_t a = 0; a < AES_COLUMS; a++)
							{
								for (size_t b = 0; b < AES_ROWS; b++)
								{
									size_t p = a + (b*AES_COLUMS);
									m_write.buffer[p] ^= RK[r][a][b];
								}
							}
							//SubBytes
							for (size_t a = 0; a < AES_BUFFER_SIZE; a++) { m_write.buffer[a] = SBOX[m_write.buffer[a]]; }
							//ShiftRows
							for (size_t a = 1; a < AES_ROWS; a++)
							{
								uint8_t t[AES_COLUMS];
								for (size_t b = 0; b < AES_COLUMS; b++) 
								{
									size_t p = b + (a*AES_COLUMS);
									t[b] = m_write.buffer[p];
								}
								for (size_t b = 0; b < AES_COLUMS; b++)
								{
									size_t p = b + (a*AES_COLUMS);
									m_write.buffer[p] = t[(b + a) % AES_COLUMS];
								}
							}
							
							//MixColumns
							for (size_t a = 0; a < AES_COLUMS; a++)
							{
								uint8_t ac[AES_ROWS];
								for (size_t b = 0; b < AES_ROWS; b++)
								{
									uint8_t ar[AES_ROWS];
									for (size_t c = 0; c < AES_ROWS; c++)
									{
										ar[c] = m_write.buffer[a + (((c + b) % AES_ROWS)*AES_COLUMS)];
									}
									ar[0] = MulRijndael(ar[0], 0x2);
									ar[1] = MulRijndael(ar[1], 0x3);
									ar[2] = MulRijndael(ar[2], 0x1);
									ar[3] = MulRijndael(ar[3], 0x1);
									ac[b] = 0;
									for (size_t c = 0; c < AES_ROWS; c++)
									{
										ac[b] ^= ar[c];
									}
								}
								for (size_t b = 0; b < AES_ROWS; b++) 
								{
									size_t bp = a + (b*AES_COLUMS);
									m_write.buffer[bp] = ac[b];
								}
							}
							
						}
						//SubBytes
						for (size_t a = 0; a < AES_BUFFER_SIZE; a++) { m_write.buffer[a] = SBOX[m_write.buffer[a]]; }
						//ShiftRows
						for (size_t a = 1; a < AES_ROWS; a++)
						{
							uint8_t t[AES_COLUMS];
							for (size_t b = 0; b < AES_COLUMS; b++)
							{
								size_t p = b + (a*AES_COLUMS);
								t[b] = m_write.buffer[p];
							}
							for (size_t b = 0; b < AES_COLUMS; b++)
							{
								size_t p = b + (a*AES_COLUMS);
								m_write.buffer[p] = t[(b + a) % AES_COLUMS];
							}
						}
						//AddRoundKey
						for (size_t a = 0; a < AES_COLUMS; a++)
						{
							for (size_t b = 0; b < AES_ROWS; b++)
							{
								size_t p = a + (b*AES_COLUMS);
								m_write.buffer[p] ^= RK[AES_ROUNDS][a][b];
							}
						}
						//Write data
						m_prev->WriteByte(m_write.bufPos);
						for (size_t a = 0; a < AES_BUFFER_SIZE; a++) { m_prev->WriteByte(m_write.buffer[a]); }
						m_write.bufPos = 0;
					}
				}
				void CLOAK_CALL_THIS SecureNetBuffer::iDecrypt()
				{
					//TODO: Use hardware-accelerated encryption (SSE AES instruction set)
					//Expand key:
					uint8_t srcD[AES_BUFFER_SIZE];
					for (size_t a = AES_BUFFER_SIZE, b = 0; a >= sizeof(KeyInt::type); a -= sizeof(KeyInt::type), b++)
					{
						KeyInt::type blk = m_key[b];
						for (size_t c = 0; c < sizeof(KeyInt::type); c++)
						{
							srcD[a - (c + 1)] = static_cast<uint8_t>((blk >> (c * 8)) & 0xFF);
						}
					}
					uint8_t RK[AES_ROUNDS + 1][AES_COLUMS][AES_ROWS];
					for (size_t a = 0, c = 0; a < AES_COLUMS; a++)
					{
						for (size_t b = 0; b < AES_ROWS; b++, c++)
						{
							RK[0][a][b] = srcD[c];
						}
					}
					for (size_t r = 1; r <= AES_ROUNDS; r++)
					{
						for (size_t a = 0; a < AES_COLUMS; a++)
						{
							if (a % 8 == 0)
							{
								for (size_t b = 0; b < AES_ROWS; b++) { RK[r][a][b] = SBOX[RK[r - 1][(a + AES_COLUMS - 1) % AES_COLUMS][(b + 1) % AES_ROWS]]; }
								RK[r][a][0] ^= RCON[r];
								for (size_t b = 0; b < AES_ROWS; b++) { RK[r][a][b] ^= RK[r - 1][a][b]; }
							}
							else
							{
								for (size_t b = 0; b < AES_ROWS; b++) { RK[r][a][b] = RK[r - 1][(a + AES_COLUMS - 1) % AES_COLUMS][b]; }
								if (a % 4 == 0)
								{
									for (size_t b = 0; b < AES_ROWS; b++) { RK[r][a][b] = SBOX[RK[r][a][b]]; }
								}
								for (size_t b = 0; b < AES_ROWS; b++) { RK[r][a][b] ^= RK[r - 1][a][b]; }
							}
						}
					}
					//Decrypt
					//AddRoundKey
					for (size_t a = 0; a < AES_COLUMS; a++)
					{
						for (size_t b = 0; b < AES_ROWS; b++)
						{
							size_t p = a + (b*AES_COLUMS);
							m_read.buffer[p] ^= RK[AES_ROUNDS][a][b];
						}
					}
					//ShiftRows
					for (size_t a = 1; a < AES_ROWS; a++)
					{
						uint8_t t[AES_COLUMS];
						for (size_t b = 0; b < AES_COLUMS; b++)
						{
							size_t p = b + (a*AES_COLUMS);
							t[b] = m_read.buffer[p];
						}
						for (size_t b = 0; b < AES_COLUMS; b++)
						{
							size_t p = b + (a*AES_COLUMS);
							m_read.buffer[p] = t[(b + AES_COLUMS - a) % AES_COLUMS];
						}
					}
					//SubBytes
					for (size_t a = 0; a < AES_BUFFER_SIZE; a++) { m_read.buffer[a] = SBOX[m_read.buffer[a] + SBOX_SIZE]; }
					for (size_t r = AES_ROUNDS; r > 0; r--)
					{
						//MixColumns
						for (size_t a = 0; a < AES_COLUMS; a++)
						{
							uint8_t ac[AES_COLUMS];
							for (size_t b = 0; b < AES_ROWS; b++)
							{
								uint8_t ar[AES_ROWS];
								for (size_t c = 0; c < AES_ROWS; c++)
								{
									ar[c] = m_read.buffer[a + (((c + b) % AES_ROWS)*AES_COLUMS)];
								}
								ar[0] = MulRijndael(ar[0], 0xE);
								ar[1] = MulRijndael(ar[1], 0xB);
								ar[2] = MulRijndael(ar[2], 0xD);
								ar[3] = MulRijndael(ar[3], 0x9);
								ac[b] = 0;
								for (size_t c = 0; c < AES_ROWS; c++)
								{
									ac[b] ^= ar[c];
								}
							}
							for (size_t b = 0; b < AES_ROWS; b++)
							{
								size_t bp = a + (b*AES_COLUMS);
								m_read.buffer[bp] = ac[b];
							}
						}
						//ShiftRows
						for (size_t a = 1; a < AES_ROWS; a++)
						{
							uint8_t t[AES_COLUMS];
							for (size_t b = 0; b < AES_COLUMS; b++)
							{
								size_t p = b + (a*AES_COLUMS);
								t[b] = m_read.buffer[p];
							}
							for (size_t b = 0; b < AES_COLUMS; b++)
							{
								size_t p = b + (a*AES_COLUMS);
								m_read.buffer[p] = t[(b + AES_COLUMS - a) % AES_COLUMS];
							}
						}
						//SubBytes
						for (size_t a = 0; a < AES_BUFFER_SIZE; a++) { m_read.buffer[a] = SBOX[m_read.buffer[a] + SBOX_SIZE]; }
						//AddRoundKey
						for (size_t a = 0; a < AES_COLUMS; a++)
						{
							for (size_t b = 0; b < AES_ROWS; b++)
							{
								size_t p = a + (b*AES_COLUMS);
								m_read.buffer[p] ^= RK[r - 1][a][b];
							}
						}
					}
				}
			}
		}
	}
}