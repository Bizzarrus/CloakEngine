#include "stdafx.h"
#include "Engine/Compress/NetBuffer/TCPBuffer.h"

#define SIMPLE_QUERY(riid,ptr,clas) if (riid == __uuidof(clas)) { *ppvObject = (clas*)this; AddRef(); return S_OK; }

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace NetBuffer {
				CLOAK_CALL TCPBuffer::TCPBuffer(In SOCKET sock) : m_writeHandler(this), m_readHandler(this)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					DEBUG_NAME(TCPBuffer);
					m_connected = false;
					m_read.Page = 0;
					m_read.Pos = 0;
					m_read.Offset = 0;
					for (size_t a = 0; a < READ_PAGE_COUNT; a++) { m_read.Buffer[a].Len = 0; }
					m_write.Pos = 0;
					m_write.Offset = 0;
					m_write.CanSend = true;
					m_write.WaitingPartPos = 0;
					m_sock = sock;
					m_callback = nullptr;
					m_conError = false;
				}
				CLOAK_CALL TCPBuffer::~TCPBuffer()
				{
					SAVE_RELEASE(m_sync);
				}
				void CLOAK_CALL_THIS TCPBuffer::OnWrite()
				{
					API::Helper::Lock lock(m_sync);
					m_write.CanSend = true;
					iTrySend();
				}
				void CLOAK_CALL_THIS TCPBuffer::OnRead(In_opt uint8_t* data, In_opt size_t dataSize)
				{
					API::Helper::Lock lock(m_sync);
					if (m_connected && m_conError == false)
					{
						bool readOnce = false;
						for (size_t a = 0; a < READ_PAGE_COUNT; a++)
						{
							const size_t p = (a + m_read.Page) % READ_PAGE_COUNT;
							if (m_read.Buffer[p].Len < READ_BUFFER_SIZE)
							{
								int r = recv(m_sock, (char*)&m_read.Buffer[p].Data[m_read.Buffer[p].Len], static_cast<int>(READ_BUFFER_SIZE - m_read.Buffer[p].Len), 0);
								if (r == SOCKET_ERROR) { goto on_read_failed; }
								else
								{
									m_read.Buffer[p].Len = min(m_read.Buffer[p].Len + r, READ_BUFFER_SIZE);
									readOnce = true;
								}
							}
						}
						if (readOnce && m_callback) { m_callback(Impl::Global::CallbackType::ReceivedData); }
						return;

					on_read_failed:
						if (readOnce && m_callback) { m_callback(Impl::Global::CallbackType::ReceivedData); }
						int r = WSAGetLastError();
						if (r == WSAETIMEDOUT || r == WSAECONNRESET || r == WSAECONNABORTED || r == WSAENETRESET || r == WSAENETDOWN)
						{
							m_conError = true;
							if (m_read.Buffer[m_read.Page].Len <= m_read.Pos) 
							{
								if (m_callback) { m_callback(Impl::Global::CallbackType::Disconnected); }
								m_connected = false;
								m_conError = false;
							}
						}
					}
				}
				void CLOAK_CALL_THIS TCPBuffer::OnUpdate() {}
				void CLOAK_CALL_THIS TCPBuffer::OnConnect()
				{
					API::Helper::Lock lock(m_sync);
					m_connected = true;
					if (m_callback) { m_callback(Impl::Global::CallbackType::Connected); }
				}
				void CLOAK_CALL_THIS TCPBuffer::SetCallback(In Impl::Global::Connection_v1::ConnectionCallback cb)
				{
					API::Helper::Lock lock(m_sync);
					m_callback = cb;
				}
				bool CLOAK_CALL_THIS TCPBuffer::IsConnected()
				{
					API::Helper::Lock lock(m_sync);
					return m_connected;
				}
				API::Files::Buffer_v1::IWriteBuffer* CLOAK_CALL_THIS TCPBuffer::GetWriteBuffer()
				{
					API::Helper::Lock lock(m_sync);
					return &m_writeHandler;
				}
				API::Files::Buffer_v1::IReadBuffer* CLOAK_CALL_THIS TCPBuffer::GetReadBuffer()
				{
					API::Helper::Lock lock(m_sync);
					return &m_readHandler;
				}
				unsigned long long CLOAK_CALL_THIS TCPBuffer::GetWritePos() const
				{
					API::Helper::Lock lock(m_sync);
					return m_write.Offset + m_write.Pos;
				}
				unsigned long long CLOAK_CALL_THIS TCPBuffer::GetReadPos() const
				{
					API::Helper::Lock lock(m_sync);
					return m_read.Offset + m_read.Pos;
				}
				void CLOAK_CALL_THIS TCPBuffer::WriteByte(In uint8_t b)
				{
					API::Helper::Lock lock(m_sync);
					m_write.Buffer[m_write.Pos] = b;
					m_write.Pos++;
					if (m_write.Pos >= WRITE_BUFFER_SIZE)
					{
						SendData();
						m_write.Offset += WRITE_BUFFER_SIZE;
						m_write.Pos = 0;
					}
				}
				uint8_t CLOAK_CALL_THIS TCPBuffer::ReadByte()
				{
					API::Helper::Lock lock(m_sync);
					if (m_connected && m_conError == false)
					{
						for (size_t a = 0; a < READ_PAGE_COUNT; a++)
						{
							const size_t p = (a + m_read.Page) % READ_PAGE_COUNT;
							if (m_read.Buffer[p].Len < READ_BUFFER_SIZE)
							{
								int r = recv(m_sock, (char*)&m_read.Buffer[p].Data[m_read.Buffer[p].Len], static_cast<int>(READ_BUFFER_SIZE - m_read.Buffer[p].Len), 0);
								if (r == SOCKET_ERROR) 
								{ 
									r = WSAGetLastError();
									if (r == WSAETIMEDOUT || r == WSAECONNRESET || r == WSAECONNABORTED || r == WSAENETRESET || r == WSAENETDOWN)
									{
										m_conError = true;
									}
									break;
								}
								else
								{
									m_read.Buffer[p].Len = min(m_read.Buffer[p].Len + r, READ_BUFFER_SIZE);
								}
							}
						}
					}
					if (m_read.Buffer[m_read.Page].Len > m_read.Pos)
					{
						uint8_t r = m_read.Buffer[m_read.Page].Data[m_read.Pos];
						m_read.Pos++;
						if (m_read.Pos >= m_read.Buffer[m_read.Page].Len) 
						{
							m_read.Buffer[m_read.Page].Len = 0;
							m_read.Pos = 0;
							m_read.Page = (m_read.Page + 1) % READ_PAGE_COUNT;
						}
						return r;
					}
					if (m_connected)
					{
						if (m_callback) { m_callback(Impl::Global::CallbackType::Disconnected); }
						m_connected = false;
						m_conError = false;
					}
					return 0;
				}
				void CLOAK_CALL_THIS TCPBuffer::SendData(In_opt bool FragRef)
				{
					API::Helper::Lock lock(m_sync);
					if (m_connected && m_conError == false)
					{
						m_write.Waiting.emplace(m_write.Buffer, min(m_write.Pos, WRITE_BUFFER_SIZE));
						m_write.Pos = 0;
						iTrySend();
					}
				}
				bool CLOAK_CALL_THIS TCPBuffer::HasData()
				{
					API::Helper::Lock lock(m_sync);
					return m_read.Buffer[m_read.Page].Len > m_read.Pos;
				}

				Success(return == true) bool CLOAK_CALL_THIS TCPBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS TCPBuffer::iTrySend()
				{
					API::Helper::Lock lock(m_sync);
					while (m_write.Waiting.size() > 0 && m_write.CanSend && m_connected && m_conError == false)
					{
						const WriteBuffer& b = m_write.Waiting.front();
						int r = send(m_sock, (char*)&b.Data[m_write.WaitingPartPos], static_cast<int>(b.Len - m_write.WaitingPartPos), 0);
						if (r == SOCKET_ERROR || r < 0)
						{
							m_write.CanSend = false;
							if (r == SOCKET_ERROR)
							{
								r = WSAGetLastError();
								if (r == WSAETIMEDOUT || r == WSAECONNRESET || r == WSAECONNABORTED || r == WSAENETRESET || r == WSAENETDOWN)
								{
									m_conError = true;
									if (m_read.Buffer[m_read.Page].Len <= m_read.Pos)
									{
										if (m_callback) { m_callback(Impl::Global::CallbackType::Disconnected); }
										m_connected = false;
										m_conError = false;
									}
								}
							}
						}
						else if (static_cast<size_t>(r) >= b.Len - m_write.WaitingPartPos)
						{
							m_write.Waiting.pop();
							m_write.WaitingPartPos = 0;
						}
						else
						{
							m_write.WaitingPartPos += r;
						}
					}
				}
			}
		}
	}
}