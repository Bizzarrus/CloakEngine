#pragma once
#ifndef CE_E_COMPRESS_SECURENET_H
#define CE_E_COMPRESS_SECURENET_H

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Files/Hash/SHA256.h"
#include "CloakEngine/Global/Math.h"

#include "Implementation/Global/Connection.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace NetBuffer {
				class SecureNetBuffer : public virtual Impl::Global::Connection_v1::INetBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr size_t PRIME_BIT_LEN = 2048;
						static constexpr size_t INT_BYTE_SIZE = PRIME_BIT_LEN / 8;
						static constexpr size_t SECRET_BIT_LEN = 256;
						static constexpr size_t SECRET_BYTE_LEN = SECRET_BIT_LEN / 8;
						static constexpr size_t KEY_BYTE_SIZE = API::Files::Hash::SHA256::DIGEST_SIZE;
						static constexpr size_t AES_ROWS = 4;
						static constexpr size_t AES_COLUMS = 8;
						static constexpr size_t AES_BUFFER_SIZE = AES_ROWS * AES_COLUMS;
						static constexpr size_t AES_ROUNDS = 14;
						typedef int_bit_t KeyInt;

						static void CLOAK_CALL Initialize();
						static void CLOAK_CALL Terminate();

						CLOAK_CALL SecureNetBuffer(In Impl::Global::Connection_v1::INetBuffer* prev, In const API::Files::UGID& cgid, In bool lobby);
						virtual CLOAK_CALL ~SecureNetBuffer();
						virtual void CLOAK_CALL_THIS OnWrite() override;
						virtual void CLOAK_CALL_THIS OnRead(In_opt uint8_t* data = nullptr, In_opt size_t dataSize = 0) override;
						virtual void CLOAK_CALL_THIS OnUpdate() override;
						virtual void CLOAK_CALL_THIS OnConnect() override;
						virtual void CLOAK_CALL_THIS SetCallback(In Impl::Global::Connection_v1::ConnectionCallback cb) override;
						virtual bool CLOAK_CALL_THIS IsConnected() override;
						virtual API::Files::Buffer_v1::IWriteBuffer* CLOAK_CALL_THIS GetWriteBuffer() override;
						virtual API::Files::Buffer_v1::IReadBuffer* CLOAK_CALL_THIS GetReadBuffer() override;
						unsigned long long CLOAK_CALL_THIS GetWritePos() const override;
						unsigned long long CLOAK_CALL_THIS GetReadPos() const override;
						void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
						uint8_t CLOAK_CALL_THIS ReadByte() override;
						void CLOAK_CALL_THIS SendData(In_opt bool FragRef = false) override;
						bool CLOAK_CALL_THIS HasData() override;

						void CLOAK_CALL_THIS OnParConnect();
						void CLOAK_CALL_THIS OnParDisconnect();
						void CLOAK_CALL_THIS OnParRecData();
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						void CLOAK_CALL_THIS iEncrypt();
						void CLOAK_CALL_THIS iDecrypt();

						struct KeyBuilding {
							KeyInt Exponent;
							KeyInt Source;
							uint8_t ID[API::Files::UGID::ID_SIZE];
							void* CLOAK_CALL operator new(In size_t s);
							void CLOAK_CALL operator delete(In void* ptr);
						};
						enum {
							STATE_CLIENT_START,
							STATE_CLIENT_WAIT,
							STATE_CLIENT_BLOCK,
							STATE_SERVER_START,
							STATE_SERVER_WAIT,
							STATE_SERVER_BLOCK,
							STATE_OK,
							STATE_ERROR,
						} m_state;
						API::Helper::ISyncSection* m_sync;
						Impl::Global::Connection_v1::NetWriteBuffer m_writeBuffer;
						Impl::Global::Connection_v1::NetReadBuffer m_readBuffer;
						Impl::Global::Connection_v1::INetBuffer* m_prev;
						const bool m_lobby;
						KeyBuilding* m_keyBuild;
						KeyInt m_key;
						Impl::Global::ConnectionCallback m_callback;
						struct {
							unsigned int posOffset;
							uint8_t bufPos;
							uint8_t buffer[AES_BUFFER_SIZE];
							uint8_t bufFill;
							uint8_t maxFill;
						} m_read;
						struct {
							unsigned int posOffset;
							uint8_t bufPos;
							uint8_t buffer[AES_BUFFER_SIZE];
						} m_write;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif