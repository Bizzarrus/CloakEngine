#include "stdafx.h"
#include "Engine/Compress/NetBuffer/UDPBuffer.h"

#define SIMPLE_QUERY(riid,ptr,clas) if (riid == __uuidof(clas)) { *ppvObject = (clas*)this; AddRef(); return S_OK; }

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace NetBuffer {
				constexpr size_t g_version = 100;
				constexpr size_t g_maxDeltaTime = 1000;

				inline void CLOAK_CALL initHeaderHelper(In UDPBuffer::HeadHelper* h)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&h->reader));
					CREATE_INTERFACE(CE_QUERY_ARGS(&h->writer));
					h->writeBuf = API::Files::CreateVirtualWriteBuffer();
					h->readBuf = h->writeBuf->GetReadBuffer();
					h->reader->SetTarget(h->readBuf, API::Files::CompressType::NONE);
					h->writer->SetTarget(h->writeBuf, API::Files::CompressType::NONE);
					h->reader->ReadBits(8);
				}
				inline void CLOAK_CALL releaseHeaderHelper(In UDPBuffer::HeadHelper* h)
				{
					SAVE_RELEASE(h->reader);
					SAVE_RELEASE(h->writer);
					SAVE_RELEASE(h->readBuf);
					SAVE_RELEASE(h->writeBuf);
				}
				/*Bits: 11[Version] + 11[Frag Length] + 32[Frame ID] + 6[Frame Off] + 8 [Frag ID] + 8 [Frag Count] + 1 [Ref Frame] + 3 [CMD]*/
				inline void CLOAK_CALL writeHeader(Inout uint8_t* dst, In const UDPBuffer::HeadHelper& help, In const UDPBuffer::HEAD& header)
				{
					help.writer->WriteBits(11, header.Version);
					help.writer->WriteBits(UDPBuffer::FRAG_LEN_BITS, header.FrameLen);
					help.writer->WriteBits(UDPBuffer::FRAME_ID_BITS, header.FrameID);
					help.writer->WriteBits(UDPBuffer::FRAME_OFF_BITS, header.FrameOffset);
					help.writer->WriteBits(UDPBuffer::FRAG_ID_BITS, header.PartID);
					help.writer->WriteBits(UDPBuffer::FRAG_ID_BITS, header.PartCount - 1);
					help.writer->WriteBits(1, header.LinkToPrevious ? 1 : 0);
					help.writer->WriteBits(3, header.Command);
					help.writer->Save();
					for (size_t a = 0; a < header.Size; a++) { dst[a] = help.readBuf->ReadByte(); }
					help.writer->MoveToNextByte();
					help.reader->MoveToNextByte();
					help.writeBuf->Clear();
				}
				inline void CLOAK_CALL readHeader(In uint8_t* src, In const UDPBuffer::HeadHelper& help, Out UDPBuffer::HEAD* header)
				{
					for (size_t a = 0; a < header->Size; a++) { help.writeBuf->WriteByte(src[a]); }
					help.reader->ReadBits(8);//move to new data
					header->Version = static_cast<uint16_t>(help.reader->ReadBits(11));
					header->FrameLen = static_cast<uint16_t>(help.reader->ReadBits(UDPBuffer::FRAG_LEN_BITS));
					header->FrameID = static_cast<uint32_t>(help.reader->ReadBits(UDPBuffer::FRAME_ID_BITS));
					header->FrameOffset = static_cast<uint8_t>(help.reader->ReadBits(UDPBuffer::FRAME_OFF_BITS));
					header->PartID = static_cast<uint8_t>(help.reader->ReadBits(UDPBuffer::FRAG_ID_BITS));
					header->PartCount = static_cast<uint8_t>(help.reader->ReadBits(UDPBuffer::FRAG_ID_BITS)) + 1;
					header->LinkToPrevious = help.reader->ReadBits(1) == 1;
					header->Command = static_cast<UDPBuffer::CMD>(help.reader->ReadBits(3));
					help.writer->MoveToNextByte();
					help.reader->MoveToNextByte();
					help.writeBuf->Clear();
				}


				CLOAK_CALL UDPBuffer::UDPBuffer(In SOCKET sock, In const SOCKADDR_STORAGE& addr) : m_writeHandler(this), m_readHandler(this), m_sock(sock), m_addr(addr)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					DEBUG_NAME(UDPBuffer);
					initHeaderHelper(&m_headHelper);
					m_write.FID = 0;
					m_write.FOF = 0;
					m_read.FID = 0;
					m_read.FOF = 0;
					m_write.Pos = 0;
					m_read.Pos = 0;
					m_write.CanSend = false;
					m_write.LastAnswer = 0;
					m_read.WaitFrames = 0;
					m_connected = false;
					m_callback = nullptr;
					m_read.readHead = true;
					int s = sizeof(DWORD);
					int r = getsockopt(sock, SOL_SOCKET, SO_MAX_MSG_SIZE, (char*)&m_maxFrameLen, &s);
					if (r != 0 || m_maxFrameLen < HEAD_SIZE) { m_maxFrameLen = 0; }
					m_maxFrameLen = min(m_maxFrameLen, MAX_FRAG_LEN);

					const SOCKADDR_IN* si = reinterpret_cast<const SOCKADDR_IN*>(&addr);
					m_dbgPort = si->sin_port;
				}
				CLOAK_CALL UDPBuffer::~UDPBuffer()
				{
					SAVE_RELEASE(m_sync);
					releaseHeaderHelper(&m_headHelper);
				}
				void CLOAK_CALL_THIS UDPBuffer::OnWrite()
				{
					API::Helper::Lock lock(m_sync);
					m_write.CanSend = true;
					iSendBufferTry();
				}
				void CLOAK_CALL_THIS UDPBuffer::OnRead(In_opt uint8_t* data, In_opt size_t dataSize)
				{
					if (dataSize > 0)
					{
						API::Helper::Lock lock(m_sync);
						HEAD h;
						h.Size = HEAD_SIZE;
						readHeader(data, m_headHelper, &h);
						m_write.LastAnswer = Impl::Global::Game::getCurrentTimeMilliSeconds();
						bool readOnce = false;
						if (h.Command == CMD_CONFIRM)
						{
							for (auto e = m_write.ToConfirm.begin(); e != m_write.ToConfirm.end(); e++)
							{
								const Fragment& f = *e;
								if ((f.FID == h.FrameID && f.FOF == h.FrameOffset && f.PrtID == h.PartID) || (f.FID <= h.FrameID && f.FOF == h.FrameOffset && h.LinkToPrevious == true))
								{
									m_write.ToConfirm.remove(e);
								}
							}
						}
						else if (h.Command == CMD_RESET)
						{
							if (h.FrameOffset == m_read.FOF)
							{
								m_read.FID = 0;
								m_read.FOF = (m_read.FOF + 1) % FRAME_OFF;
							}
							iSendConfirm(h.FrameID, h.PartID, h.FrameOffset, true);
						}
						else if (h.Command == CMD_SEND)
						{
							uint32_t framePos = 0;
							bool fullFrame = false;
							if (dataSize > HEAD_SIZE)
							{
								uint32_t sFID = 0;
								uint8_t sFOF = 0;
								if (m_read.FrameBuffer.Len > 0)
								{
									const Frame& low = m_read.FrameBuffer.Buffer[m_read.FrameBuffer.Start];
									sFID = low.FID;
									sFOF = low.FOF;
									if (h.FrameID >= low.FID)
									{
										if (h.FrameOffset == low.FOF)
										{
											framePos = h.FrameID - low.FID;
											goto on_read_process;
										}
										else if (h.FrameOffset == (low.FOF + 1) % FRAME_OFF)
										{
											framePos = h.FrameID + (MAX_FRAME_ID - low.FID);
											goto on_read_process;
										}
									}
									goto on_read_failed;
								}

							on_read_process:
								if (framePos >= m_read.FrameBuffer.Len)
								{
									for (uint32_t a = static_cast<uint32_t>(m_read.FrameBuffer.Len); a <= framePos; a++)
									{
										if (m_read.FrameBuffer.Len + 1 > m_read.FrameBuffer.MaxSize && m_read.WaitFrames > 0) { goto on_read_failed; }
										else
										{
											size_t p = (m_read.FrameBuffer.Start + a) % m_read.FrameBuffer.MaxSize;
											uint32_t nFID = sFID + a;
											uint8_t nFOF = sFOF;
											if (nFID >= MAX_FRAME_ID) { nFID -= MAX_FRAME_ID; nFOF = (nFOF + 1) % FRAME_OFF; }
											m_read.FrameBuffer.Buffer[p].FID = nFID;
											m_read.FrameBuffer.Buffer[p].FOF = nFOF;
											m_read.FrameBuffer.Buffer[p].FragCount = 0;
											m_read.FrameBuffer.Buffer[p].FragsSize = 0;
											m_read.FrameBuffer.Buffer[p].Length = 0;
											m_read.FrameBuffer.Buffer[p].RefNextFrame = false;
											m_read.FrameBuffer.Buffer[p].CheckPref = false;
											for (size_t b = 0; b < MAX_FRAG_PER_FRAME; b++) { m_read.FrameBuffer.Buffer[p].Frags[b].Reset(0); }
										}
									}
									m_read.FrameBuffer.Len = framePos + 1;
									if (m_read.FrameBuffer.Len > m_read.FrameBuffer.MaxSize)
									{
										m_read.FrameBuffer.Start = (m_read.FrameBuffer.Start + m_read.FrameBuffer.Len) % m_read.FrameBuffer.MaxSize;
										m_read.FrameBuffer.Len = m_read.FrameBuffer.MaxSize;
									}
									framePos = static_cast<uint32_t>(m_read.FrameBuffer.Len - 1);
								}
								const size_t rfp = (m_read.FrameBuffer.Start + framePos) % m_read.FrameBuffer.MaxSize;
								Frame* frame = &m_read.FrameBuffer.Buffer[rfp];
								frame->FragCount = h.PartCount;
								frame->RefNextFrame = h.LinkToPrevious;
								if (frame->Frags[h.PartID].Size == 0)
								{
									frame->FragsSize++;
									frame->Length += static_cast<uint32_t>(dataSize - HEAD_SIZE);
									Fragment* frag = &m_read.FrameBuffer.Buffer[rfp].Frags[h.PartID];
									frag->Reset(dataSize - HEAD_SIZE);
									frag->FID = h.FrameID;
									frag->FOF = h.FrameOffset;
									frag->PrtID = h.PartID;
									frag->Time = 0;
									memcpy(frag->Data, &data[HEAD_SIZE], sizeof(uint8_t)*frag->Size);
									if (frame->FragCount == frame->FragsSize)
									{
										uint32_t refStart = framePos;
										bool suc = true;
										size_t frameCount = 0;
										for (size_t a = refStart; a > 0 && m_read.FrameBuffer.Buffer[(m_read.FrameBuffer.Start + a - 1) % m_read.FrameBuffer.MaxSize].RefNextFrame == true && suc == true; a--)
										{
											refStart = static_cast<uint32_t>(a - 1);
											frameCount++;
											const Frame& f = m_read.FrameBuffer.Buffer[(m_read.FrameBuffer.Start + a - 1) % m_read.FrameBuffer.MaxSize];
											if (f.Length == 0 || f.FragCount != f.FragsSize) { suc = false; }
										}
										bool hnr = true;
										for (size_t a = framePos; a < m_read.FrameBuffer.Len && hnr == true && suc == true; a++)
										{
											const Frame& f = m_read.FrameBuffer.Buffer[(m_read.FrameBuffer.Start + a) % m_read.FrameBuffer.MaxSize];
											hnr = f.RefNextFrame;
											if (f.Length == 0 || f.FragCount != f.FragsSize) { suc = false; }
											frameCount++;
										}
										if (hnr == true) { suc = false; }
										if (suc == true)
										{
											fullFrame = true;
											size_t wrnF = refStart;
											if (m_read.WaitFrames == 0)
											{
												for (size_t a = 0; a < frameCount && m_read.Buffer.Len + m_read.FrameBuffer.Buffer[(m_read.FrameBuffer.Start + refStart + a) % m_read.FrameBuffer.MaxSize].Length <= m_read.Buffer.MaxSize; a++)
												{
													wrnF++;
													const Frame& f = m_read.FrameBuffer.Buffer[(m_read.FrameBuffer.Start + refStart + a) % m_read.FrameBuffer.MaxSize];
													for (size_t b = 1, c = 0; c < f.FragCount; b++)
													{
														m_read.Buffer.Buffer[(m_read.Buffer.Start + m_read.Buffer.Len + b - 1) % m_read.Buffer.MaxSize] = f.Frags[c].Data[b - 1];
														if (b >= f.Frags[c].Size)
														{
															m_read.Buffer.Len += f.Frags[c].Size;
															b = 0;
															c++;
														}
													}
												}
												m_read.FrameBuffer.Len -= wrnF;
												m_read.FrameBuffer.Start = (m_read.FrameBuffer.Start + wrnF) % m_read.FrameBuffer.MaxSize;
												readOnce = true;
											}
											m_read.WaitFrames += frameCount + refStart - wrnF;
										}
									}
								}
								

							on_read_failed:;
							}
							iSendConfirm(h.FrameID, h.PartID, h.FrameOffset, fullFrame);
						}
						if (readOnce && m_callback) 
						{
							m_callback(Impl::Global::CallbackType::ReceivedData); 
						}
					}
				}
				void CLOAK_CALL_THIS UDPBuffer::OnUpdate()
				{
					iSendBufferRetry();
				}
				void CLOAK_CALL_THIS UDPBuffer::OnConnect()
				{
					API::Helper::Lock lock(m_sync);
					if (m_connected == false)
					{
						m_connected = true;
						if (m_callback) { m_callback(Impl::Global::CallbackType::Connected); }
					}
				}
				void CLOAK_CALL_THIS UDPBuffer::SetCallback(In Impl::Global::Connection_v1::ConnectionCallback cb)
				{
					API::Helper::Lock lock(m_sync);
					m_callback = cb;
				}
				bool CLOAK_CALL_THIS UDPBuffer::IsConnected()
				{
					API::Helper::Lock lock(m_sync);
					return m_connected;
				}
				API::Files::Buffer_v1::IWriteBuffer* CLOAK_CALL_THIS UDPBuffer::GetWriteBuffer()
				{
					API::Helper::Lock lock(m_sync);
					return &m_writeHandler;
				}
				API::Files::Buffer_v1::IReadBuffer* CLOAK_CALL_THIS UDPBuffer::GetReadBuffer()
				{
					API::Helper::Lock lock(m_sync);
					return &m_readHandler;
				}
				unsigned long long CLOAK_CALL_THIS UDPBuffer::GetWritePos() const
				{
					API::Helper::Lock lock(m_sync);
					return m_write.Pos;
				}
				unsigned long long CLOAK_CALL_THIS UDPBuffer::GetReadPos() const
				{
					API::Helper::Lock lock(m_sync);
					return m_read.Pos;
				}
				void CLOAK_CALL_THIS UDPBuffer::WriteByte(In uint8_t b)
				{
					API::Helper::Lock lock(m_sync);
					if (m_write.Buffer.Len >= m_write.Buffer.MaxSize) { SendData(true); }
					m_write.Buffer.Buffer[(m_write.Buffer.Start + m_write.Buffer.Len) % m_write.Buffer.MaxSize] = b;
					m_write.Buffer.Len++;
					m_write.Pos++;
				}
				uint8_t CLOAK_CALL_THIS UDPBuffer::ReadByte()
				{
					API::Helper::Lock lock(m_sync);
					if (m_read.Buffer.Len > 0)
					{
						uint8_t r = m_read.Buffer.Buffer[m_read.Buffer.Start];
						m_read.Buffer.Start = (m_read.Buffer.Start + 1) % m_read.Buffer.MaxSize;
						m_read.Buffer.Len--;
						m_read.Pos++;
						if (m_read.WaitFrames > 0 && m_read.FrameBuffer.Buffer[m_read.FrameBuffer.Start].Length + m_read.Buffer.Len <= m_read.Buffer.MaxSize)
						{
							const Frame& f = m_read.FrameBuffer.Buffer[m_read.FrameBuffer.Start];
							for (size_t b = 1, c = 0; c < f.FragCount; b++)
							{
								m_read.Buffer.Buffer[(m_read.Buffer.Start + m_read.Buffer.Len + b - 1) % m_read.Buffer.MaxSize] = f.Frags[c].Data[b - 1];
								if (b >= f.Frags[c].Size)
								{
									m_read.Buffer.Len += f.Frags[c].Size;
									b = 0;
									c++;
								}
							}
							m_read.FrameBuffer.Len--;
							m_read.FrameBuffer.Start = (m_read.FrameBuffer.Start + 1) % m_read.FrameBuffer.MaxSize;
							if (m_read.WaitFrames > 0 && f.RefNextFrame == false)
							{
								size_t ns = 0;
								bool lr = false;
								for (size_t a = 0; a < m_read.FrameBuffer.Len; a++)
								{
									const Frame& frame = m_read.FrameBuffer.Buffer[(m_read.FrameBuffer.Start + a) % m_read.FrameBuffer.MaxSize];
									if (frame.FragCount != frame.FragsSize) { lr = false; }
									else if (lr == false) { lr = true; ns = a; }
									if (lr == true && frame.RefNextFrame == false) 
									{ 
										m_read.FrameBuffer.Len -= ns;
										m_read.FrameBuffer.Start = (m_read.FrameBuffer.Start + ns) % m_read.FrameBuffer.MaxSize;
										if (m_callback) { m_callback(Impl::Global::CallbackType::ReceivedData); }
										goto read_byte_finish;
									}
								}
								m_read.WaitFrames = 0;

							read_byte_finish:;

							}
						}
						return r;
					}
					return 0;
				}
				void CLOAK_CALL_THIS UDPBuffer::SendData(In_opt bool FragRef)
				{
					API::Helper::Lock lock(m_sync);
					if (m_write.Buffer.Len > 0 && m_maxFrameLen > 0)
					{
						size_t resLen = m_write.Buffer.Len;
						size_t fragCount = CEIL_DIV(resLen, m_maxFrameLen - HEAD_SIZE);
						if (fragCount > MAX_FRAG_PER_FRAME + 1) 
						{ 
							fragCount = MAX_FRAG_PER_FRAME + 1; 
							FragRef = true; 
							resLen = fragCount*(m_maxFrameLen - HEAD_SIZE);
						}
						uint8_t* tmp = NewArray(uint8_t,  m_maxFrameLen);
						for (size_t a = 0; a < fragCount; a++)
						{
							const size_t payloadLen = min(m_maxFrameLen - HEAD_SIZE, resLen);
							HEAD h;
							h.Size = HEAD_SIZE;
							h.Version = g_version;
							h.FrameID = m_write.FID;
							h.FrameLen = static_cast<uint16_t>(HEAD_SIZE + payloadLen);
							h.FrameOffset = m_write.FOF;
							h.LinkToPrevious = FragRef;
							h.PartCount = static_cast<uint8_t>(fragCount);
							h.PartID = static_cast<uint8_t>(a);
							h.Command = CMD_SEND;
							writeHeader(tmp, m_headHelper, h);
							for (size_t b = 0; b < payloadLen; b++)
							{
								tmp[HEAD_SIZE + b] = m_write.Buffer.Buffer[(m_write.Buffer.Start + b) % m_write.Buffer.MaxSize];
							}
							m_write.Buffer.Start = (m_write.Buffer.Start + payloadLen) % m_write.Buffer.MaxSize;
							m_write.Buffer.Len -= payloadLen;
							iSendBuffer(tmp, HEAD_SIZE + payloadLen, m_write.FID, m_write.FOF, (uint8_t)a);
							m_write.FID++;
							if (m_write.FID >= MAX_FRAME_ID) { iSendReset(); }
						}
						DeleteArray(tmp);
					}
				}
				bool CLOAK_CALL_THIS UDPBuffer::HasData()
				{
					API::Helper::Lock lock(m_sync);
					return m_read.Buffer.Len > 0;
				}

				Success(return == true) bool CLOAK_CALL_THIS UDPBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS UDPBuffer::iSendBuffer(In uint8_t* src, In size_t srcLen, In uint32_t FID, In uint8_t FOF, In uint8_t prt, In_opt bool retry)
				{
					API::Helper::Lock lock(m_sync);
					Fragment f;
					f.Reset(srcLen);
					memcpy(f.Data, src, sizeof(uint8_t)*srcLen);
					f.FID = FID;
					f.FOF = FOF;
					f.PrtID = prt;
					f.Time = Impl::Global::Game::getCurrentTimeMilliSeconds();
					m_write.ToSend.push(f);
					iSendBufferTry();
				}
				void CLOAK_CALL_THIS UDPBuffer::iSendBufferTry()
				{
					API::Helper::Lock lock(m_sync);
					const SOCKADDR_IN* adr = reinterpret_cast<const SOCKADDR_IN*>(&m_addr);
					while (m_write.CanSend && m_write.ToSend.size() > 0 && m_connected)
					{
						const Fragment f = m_write.ToSend.front();
						int r = sendto(m_sock, reinterpret_cast<char*>(f.Data), static_cast<int>(f.Size), 0, reinterpret_cast<const SOCKADDR*>(&m_addr), sizeof(m_addr));
						if (r == f.Size)
						{
							if (m_write.ToConfirm.size() == 0) { m_write.LastAnswer = Impl::Global::Game::getCurrentTimeMilliSeconds(); }
							m_write.ToConfirm.push(f);
							m_write.ToSend.pop();
						}
						else 
						{ 
							m_write.CanSend = false; 
							if (r == SOCKET_ERROR)
							{
								r = WSAGetLastError();
								if (r == WSAETIMEDOUT || r == WSAECONNRESET || r == WSAECONNABORTED || r == WSAENETRESET || r == WSAENETDOWN)
								{
									if (m_connected && m_callback) { m_callback(Impl::Global::CallbackType::Disconnected); }
									m_connected = false;
								}
							}
						}
					}
					iSendBufferRetry();
				}
				void CLOAK_CALL_THIS UDPBuffer::iSendBufferRetry()
				{
					API::Helper::Lock lock(m_sync);
					unsigned long long t = Impl::Global::Game::getCurrentTimeMilliSeconds();
					for (auto f = m_write.ToConfirm.begin(); f != m_write.ToConfirm.end(); f++)
					{
						if (f->Time<t && t - f->Time>g_maxDeltaTime)
						{
							int r = sendto(m_sock, (char*)f->Data, static_cast<int>(f->Size), 0, reinterpret_cast<const SOCKADDR*>(&m_addr), sizeof(m_addr));
							if (r == f->Size) { f->Time = t; }
							else
							{
								m_write.CanSend = false;
								if (r == SOCKET_ERROR)
								{
									r = WSAGetLastError();
									if (r == WSAETIMEDOUT || r == WSAECONNRESET || r == WSAECONNABORTED || r == WSAENETRESET || r == WSAENETDOWN)
									{
										if (m_connected && m_callback) { m_callback(Impl::Global::CallbackType::Disconnected); }
										m_connected = false;
									}
								}
							}
						}
					}
					if (m_write.ToConfirm.size() > 0 && t > m_write.LastAnswer && t - m_write.LastAnswer > TIMEOUT)
					{
						if (m_connected && m_callback) { m_callback(Impl::Global::CallbackType::Disconnected); }
						m_connected = false;
					}
				}
				void CLOAK_CALL_THIS UDPBuffer::iSendReset()
				{
					uint8_t tmp[HEAD_SIZE];
					HEAD h;
					h.Size = HEAD_SIZE;
					h.Version = g_version;
					h.FrameID = m_write.FID;
					h.FrameLen = HEAD_SIZE;
					h.FrameOffset = m_write.FOF;
					h.LinkToPrevious = false;
					h.PartCount = 1;
					h.PartID = 0;
					h.Command = CMD_RESET;
					writeHeader(tmp, m_headHelper, h);
					API::Helper::Lock lock(m_sync);
					iSendBuffer(tmp, HEAD_SIZE, m_write.FID, m_write.FOF, 0);
					m_write.FID = 0;
					m_write.FOF = (m_write.FOF + 1) % FRAME_OFF;
				}
				void CLOAK_CALL_THIS UDPBuffer::iSendConfirm(In uint32_t fID, In uint8_t prtID, In uint8_t fOff, In bool fullFrame)
				{
					uint8_t tmp[HEAD_SIZE];
					HEAD h;
					h.Size = HEAD_SIZE;
					h.Version = g_version;
					h.FrameID = fID;
					h.FrameLen = HEAD_SIZE;
					h.FrameOffset = fOff;
					h.LinkToPrevious = fullFrame;
					h.PartCount = 1;
					h.PartID = prtID;
					h.Command = CMD_CONFIRM;
					writeHeader(tmp, m_headHelper, h);
					iSendBuffer(tmp, HEAD_SIZE, fID, fOff, prtID, false);
				}
			}
		}
	}
}