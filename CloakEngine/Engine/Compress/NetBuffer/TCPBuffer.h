#pragma once
#ifndef CE_E_COMPRESS_NETBUFFER_TCPBUFFER_H
#define CE_E_COMPRESS_NETBUFFER_TCPBUFFER_H

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Memory.h"

#include "Implementation/Global/Connection.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace NetBuffer {
				class TCPBuffer : public virtual Impl::Global::Connection_v1::INetBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr size_t WRITE_BUFFER_SIZE = 2 KB;
						static constexpr size_t READ_BUFFER_SIZE = 2 KB;
						static constexpr size_t READ_PAGE_COUNT = 2;

						CLOAK_CALL TCPBuffer(In SOCKET sock);
						virtual CLOAK_CALL ~TCPBuffer();
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
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						void CLOAK_CALL_THIS iTrySend();

						struct WriteBuffer {
							private:
								size_t capacity;
							public:
								uint8_t* Data;
								size_t Len;
								CLOAK_CALL WriteBuffer() { Data = nullptr; Len = 0; capacity = 0; }
								CLOAK_CALL WriteBuffer(In const WriteBuffer& b) 
								{
									if (b.Len > 0) 
									{ 
										Data = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryHeap::Allocate(b.Len * sizeof(uint8_t)));
										memcpy(Data, b.Data, b.Len * sizeof(uint8_t)); 
									}
									else { Data = nullptr; }
									Len = b.Len;
									capacity = b.Len;
								}
								CLOAK_CALL WriteBuffer(In uint8_t* b, In size_t l)
								{
									Len = l;
									if (Len > 0) 
									{ 
										Data = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryHeap::Allocate(l * sizeof(uint8_t)));
										memcpy(Data, b, l * sizeof(uint8_t)); 
									}
									else { Data = nullptr; }
									capacity = l;
								}
								CLOAK_CALL ~WriteBuffer() { API::Global::Memory::MemoryHeap::Free(Data); }
								void CLOAK_CALL_THIS Set(In uint8_t* b, In size_t l)
								{
									if (l > capacity) {
										API::Global::Memory::MemoryHeap::Free(Data);
										Data = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryHeap::Allocate(l * sizeof(uint8_t)));
										capacity = l;
									}
									if (l > 0) { memcpy(Data, b, l * sizeof(uint8_t)); }
									Len = l;
								}
								WriteBuffer& CLOAK_CALL_THIS operator=(In const WriteBuffer& b) 
								{
									if (b.Len > capacity) {
										API::Global::Memory::MemoryHeap::Free(Data);
										Data = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryHeap::Allocate(b.Len * sizeof(uint8_t)));
										capacity = b.Len;
									}
									if (b.Len > 0) { memcpy(Data, b.Data, b.Len * sizeof(uint8_t)); }
									Len = b.Len;
									return *this;
								}
						};
						struct {
							struct {
								size_t Len;
								uint8_t Data[READ_BUFFER_SIZE];
							} Buffer[READ_PAGE_COUNT];
							size_t Page;
							size_t Pos;
							unsigned long long Offset;
						} m_read;
						struct {
							uint8_t Buffer[WRITE_BUFFER_SIZE];
							size_t Pos;
							unsigned long long Offset;
							API::Queue<WriteBuffer> Waiting;
							bool CanSend;
							size_t WaitingPartPos;
						} m_write;
						Impl::Global::Connection_v1::NetWriteBuffer m_writeHandler;
						Impl::Global::Connection_v1::NetReadBuffer m_readHandler;
						bool m_connected;
						bool m_conError;
						API::Helper::ISyncSection* m_sync;
						SOCKET m_sock;
						Impl::Global::Connection_v1::ConnectionCallback m_callback;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif