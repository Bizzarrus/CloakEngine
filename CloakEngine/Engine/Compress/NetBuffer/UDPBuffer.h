#pragma once
#ifndef CE_E_COMPRESS_NETBUFFER_UDPBUFFER_H
#define CE_E_COMPRESS_NETBUFFER_UDPBUFFER_H

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Files/Compressor.h"
#include "CloakEngine/Global/Memory.h"

#include "Implementation/Global/Connection.h"

#include "Engine/LinkedList.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace NetBuffer {
				class UDPBuffer : public virtual Impl::Global::Connection_v1::INetBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						struct HeadHelper {
							API::Files::IReader* reader;
							API::Files::IWriter* writer;
							API::Files::IVirtualWriteBuffer* writeBuf;
							API::Files::IReadBuffer* readBuf;
						};
						enum CMD {
							CMD_SEND = 0,
							CMD_RESET = 1,
							CMD_CONFIRM = 2,
						};
						struct HEAD {
							uint16_t Size;
							uint16_t Version;
							uint16_t FrameLen;
							uint32_t FrameID;
							uint8_t FrameOffset;
							uint8_t PartID;
							uint8_t PartCount;
							bool LinkToPrevious;
							CMD Command;
						};

						static constexpr size_t READ_BUFFER_SIZE = 32 KB;
						static constexpr size_t WRITE_BUFFER_SIZE = 32 KB;
						static constexpr size_t FRAG_LEN_BITS = 11;
						static constexpr size_t FRAME_ID_BITS = 32;
						static constexpr size_t FRAG_ID_BITS = 8;
						static constexpr size_t FRAME_OFF_BITS = 6;
						static constexpr size_t FRAME_OFF = (1 << FRAME_OFF_BITS) - 1;
						static constexpr size_t MAX_FRAG_PER_FRAME = (1 << FRAG_ID_BITS) - 1;
						static constexpr uint64_t MAX_FRAME_ID = (1ULL << FRAME_ID_BITS) - 1;
						static constexpr size_t MAX_FRAG_LEN = (1 << FRAG_LEN_BITS) - 1;
						static constexpr unsigned long long TIMEOUT = 5000;
						/*Bits: 11[Version] + 11[Frag Length] + 32[Frame ID] + 6[Frame Off] + 8 [Frag ID] + 8 [Frag Count] + 1 [Ref Frame] + 3 [CMD]*/
						static constexpr size_t HEAD_SIZE = CEIL_DIV(11 + FRAG_LEN_BITS + FRAME_ID_BITS + FRAME_OFF_BITS + FRAG_ID_BITS + FRAG_ID_BITS + 1 + 3, 8);
						static constexpr size_t MAX_PAYLOAD_LEN = MAX_FRAG_LEN - HEAD_SIZE;

						CLOAK_CALL UDPBuffer(In SOCKET sock, In const SOCKADDR_STORAGE& addr);
						virtual CLOAK_CALL ~UDPBuffer();
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
						void CLOAK_CALL_THIS iSendBuffer(In uint8_t* src, In size_t srcLen, In uint32_t FID, In uint8_t FOF, In uint8_t prt, In_opt bool retry = true);
						void CLOAK_CALL_THIS iSendBufferTry();
						void CLOAK_CALL_THIS iSendBufferRetry();
						void CLOAK_CALL_THIS iSendReset();
						void CLOAK_CALL_THIS iSendConfirm(In uint32_t fID, In uint8_t prtID, In uint8_t fOff, In bool fullFrame);

						template<size_t N, typename T = uint8_t> struct RotBuffer {
							size_t Start = 0;
							size_t Len = 0;
							T Buffer[N];
							static constexpr size_t MaxSize = N;
						};
						struct Fragment {
							uint8_t* Data;
							size_t Size;
							size_t Capacity;
							uint32_t FID;
							uint8_t FOF;
							uint8_t PrtID;
							unsigned long long Time;
							CLOAK_CALL Fragment() { Data = nullptr; Size = 0; Capacity = 0; Time = 0; }
							CLOAK_CALL Fragment(In const Fragment& f) 
							{
								if (f.Size > 0) 
								{ 
									Data = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryHeap::Allocate(f.Size * sizeof(uint8_t)));
									Size = f.Size; 
									Capacity = f.Size; 
									memcpy(Data, f.Data, sizeof(uint8_t)*Size);
								}
								else { Data = nullptr; Size = 0; Capacity = 0; }
								FID = f.FID;
								FOF = f.FOF;
								PrtID = f.PrtID;
								Time = f.Time;
							}
							CLOAK_CALL Fragment(In Fragment&& f)
							{
								Data = f.Data;
								Size = f.Size;
								Capacity = f.Capacity;
								FID = f.FID;
								FOF = f.FOF;
								PrtID = f.PrtID;
								Time = f.Time;
							}
							CLOAK_CALL ~Fragment() { API::Global::Memory::MemoryHeap::Free(Data); }
							Fragment& CLOAK_CALL_THIS operator=(In const Fragment& f)
							{
								API::Global::Memory::MemoryHeap::Free(Data);
								if (f.Size > 0) 
								{ 
									Data = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryHeap::Allocate(f.Size * sizeof(uint8_t)));
									Size = f.Size; 
									Capacity = f.Size; 
									memcpy(Data, f.Data, sizeof(uint8_t)*Size);
								}
								else { Data = nullptr; Size = 0; Capacity = 0; }
								FID = f.FID;
								FOF = f.FOF;
								PrtID = f.PrtID;
								Time = f.Time;
								return *this;
							}
							Fragment& CLOAK_CALL_THIS operator=(In Fragment&& f)
							{
								Data = f.Data;
								Size = f.Size;
								Capacity = f.Capacity;
								FID = f.FID;
								FOF = f.FOF;
								PrtID = f.PrtID;
								Time = f.Time;
								return *this;
							}
							void CLOAK_CALL_THIS Reset(In size_t s)
							{
								if (s > Capacity) 
								{ 
									API::Global::Memory::MemoryHeap::Free(Data);
									Data = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryHeap::Allocate(s * sizeof(uint8_t)));
									Capacity = s; 
								}
								Size = s;
								if (s > 0) { memset(Data, 0, sizeof(uint8_t)*s); }
								Time = 0;
							}
							void CLOAK_CALL_THIS Free()
							{
								API::Global::Memory::MemoryHeap::Free(Data);
								Data = nullptr;
								Capacity = 0;
								Size = 0;
							}
						};
						struct Frame {
							Fragment Frags[MAX_FRAG_PER_FRAME];
							size_t FragsSize = 0;
							size_t FragCount = 0;
							uint32_t FID = 0;
							uint8_t FOF = 0;
							uint32_t Length = 0;
							bool RefNextFrame = false;
							bool CheckPref = false;
						};

						struct {
							RotBuffer<READ_BUFFER_SIZE> Buffer;
							uint32_t FID;
							uint8_t FOF;
							size_t WaitFrames;
							unsigned long long Pos;
							RotBuffer<8,Frame> FrameBuffer;
							HEAD lastHead;
							bool readHead;
						} m_read;
						struct {
							RotBuffer<WRITE_BUFFER_SIZE> Buffer;
							uint32_t FID;
							uint8_t FOF;
							unsigned long long Pos;
							bool CanSend;
							API::Queue<Fragment> ToSend;
							Engine::LinkedList<Fragment> ToConfirm;
							unsigned long long LastAnswer;
						} m_write;
						
						Impl::Global::Connection_v1::NetWriteBuffer m_writeHandler;
						Impl::Global::Connection_v1::NetReadBuffer m_readHandler;
						const SOCKET m_sock;
						DWORD m_maxFrameLen;
						bool m_connected;
						API::Helper::ISyncSection* m_sync;
						Impl::Global::Connection_v1::ConnectionCallback m_callback;
						const SOCKADDR_STORAGE m_addr;
						HeadHelper m_headHelper;

						unsigned short m_dbgPort;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif