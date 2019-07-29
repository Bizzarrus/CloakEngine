#include "stdafx.h"
#include "Engine/Compress/LZC.h"
#include "CloakEngine/Global/Memory.h"

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace LZC {
				CLOAK_CALL LZCWriteBuffer::LZCWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type)
				{
					DEBUG_NAME(LZCWriteBuffer);
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_output));
					m_output->SetTarget(output, type);
					m_bytePos = 0;
					m_windowSize = 0;
					m_windowStart = 0;
					m_writeCount = 0;
				}
				CLOAK_CALL LZCWriteBuffer::~LZCWriteBuffer()
				{
					
				}
				unsigned long long CLOAK_CALL_THIS LZCWriteBuffer::GetPosition() const
				{
					return m_writeCount;
				}
				void CLOAK_CALL_THIS LZCWriteBuffer::WriteByte(In uint8_t b)
				{
					m_writeCount++;
					m_data[m_bytePos++] = b;
					if (m_bytePos >= DATA_SIZE)
					{
						writeData(m_bytePos, false);
					}
				}
				void CLOAK_CALL_THIS LZCWriteBuffer::Save()
				{
					writeData(m_bytePos, true);
					m_bytePos = 0;
					m_output->Save();
				}
				void* CLOAK_CALL LZCWriteBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL LZCWriteBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS LZCWriteBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::IWriteBuffer)) { *ptr = (API::Files::IWriteBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::IBuffer)) { *ptr = (API::Files::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS LZCWriteBuffer::writeData(In size_t bytes, In bool flush)
				{
					m_bytePos = 0;
					int prefix[MAX_WORD_LEN + 1];
					size_t validPref = 0;
					for (size_t srcPos = 0; srcPos < bytes;)
					{
						size_t len = 1;
						size_t off = 0;
						//Find in window:
						const size_t n = min(MAX_WORD_LEN, bytes - srcPos);
						for (size_t a = validPref; a < MAX_WORD_LEN; a++) { prefix[a + 1] = 0; }
						uint8_t* w = m_data + srcPos;
						prefix[0] = -1;
						int j = prefix[validPref];
						for (size_t a = validPref; a < n; a++)
						{
							if (flush == false && a + srcPos >= bytes) 
							{ 
								//Out of src
								m_bytePos = static_cast<unsigned int>(bytes - srcPos);
								for (size_t a = 0; a < m_bytePos; a++) { m_data[a] = m_data[srcPos + a]; }
								return;
							}
							else
							{
								while (j >= 0 && w[a] != w[j]) { j = prefix[j]; }
								j++;
								prefix[a + 1] = j;
							}
						}
						validPref = MAX_WORD_LEN;
						j = 0;
						for (size_t a = 0; a < m_windowSize && len < n; a++)
						{
							const size_t p = (m_windowStart + a) % WINDOW_SIZE;
							while (j >= 0 && m_window[p] != w[j]) { j = prefix[j]; }
							j++;
							if (static_cast<size_t>(j) > len && static_cast<size_t>(j) <= n)
							{
								len = j;
								off = m_windowSize + j - (a + 2);
							}
						}

						//Write:

						if (len > MIN_WORD_LEN)
						{
							m_output->WriteBits(1, 0);
							m_output->WriteBits(BIT_COUNT_OFFSET, off);
							m_output->WriteBits(BIT_COUNT_WORD, len - MIN_WORD_LEN);
						}
						else
						{
							for (size_t a = 0; a < len; a++)
							{
								m_output->WriteBits(1, 1);
								m_output->WriteBits(8, m_data[srcPos + a]);
							}
						}
						//Move prefix:
						for (size_t a = 1; a < MAX_WORD_LEN + 1 - len; a++)
						{
							int np = prefix[a + len];
							if (np > 0 && static_cast<size_t>(np) > len) { prefix[a] = np - static_cast<int>(len); }
							else { prefix[a] = 0; }
						}
						validPref -= len;
						//Insert in window:
						for (size_t a = 0; a < len; a++)
						{
							const size_t p = (m_windowStart + m_windowSize + a) % WINDOW_SIZE;
							m_window[p] = m_data[srcPos + a];
						}
						m_windowSize += static_cast<unsigned int>(len);
						if (m_windowSize > WINDOW_SIZE)
						{
							m_windowStart = (m_windowStart + m_windowSize) % WINDOW_SIZE;
							m_windowSize = WINDOW_SIZE;
						}
						srcPos += len;
					}
				}

				CLOAK_CALL LZCReadBuffer::LZCReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type)
				{
					DEBUG_NAME(LZCReadBuffer);
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_input));
					m_input->SetTarget(input, type);
					m_windowSize = 0;
					m_windowStart = 0;
					m_bytePos = 0;
					m_curPage = 0;
					m_lastPos = 0;
					m_lastLen = 0;
					m_readCount = 0;
					for (size_t a = 0; a < PAGE_SIZE; a++) { readData(a); }
				}
				CLOAK_CALL LZCReadBuffer::~LZCReadBuffer()
				{
					
				}
				unsigned long long CLOAK_CALL_THIS LZCReadBuffer::GetPosition() const
				{
					return m_readCount;
				}
				uint8_t CLOAK_CALL_THIS LZCReadBuffer::ReadByte()
				{
					if (m_bytePos < m_pageLen[m_curPage])
					{
						m_readCount++;
						uint8_t r = m_data[m_curPage][m_bytePos++];
						if (m_bytePos >= m_pageLen[m_curPage])
						{
							readData(m_curPage);
							m_curPage = (m_curPage + 1) % PAGE_SIZE;
							m_bytePos = 0;
						}
						return r;
					}
					return 0;
				}
				bool CLOAK_CALL_THIS LZCReadBuffer::HasNextByte()
				{
					return m_bytePos < m_pageLen[m_curPage];
				}
				
				Success(return == true) bool CLOAK_CALL_THIS LZCReadBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IReadBuffer)) { *ptr = (API::Files::Buffer_v1::IReadBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ptr = (API::Files::Buffer_v1::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS LZCReadBuffer::readData(In size_t page)
				{
					uint32_t res = 0;
					for (size_t dstPos = 0; dstPos < PAGE_DATA_SIZE && (m_input->IsAtEnd() == false || m_lastPos < m_lastLen); dstPos++)
					{
						//Read new data:
						if (m_lastPos >= m_lastLen)
						{
							if (m_input->ReadBits(1) == 1)
							{
								m_last[0] = static_cast<uint8_t>(m_input->ReadBits(8));
								m_lastLen = 1;
							}
							else
							{
								const size_t off = static_cast<size_t>(m_input->ReadBits(BIT_COUNT_OFFSET));
								const size_t p = m_windowSize - (off + 1);
								m_lastLen = static_cast<uint32_t>(m_input->ReadBits(BIT_COUNT_WORD)) + MIN_WORD_LEN;
								for (size_t a = 0; a < m_lastLen; a++) 
								{ 
									const size_t rp = (m_windowStart + a + p) % WINDOW_SIZE;
									m_last[a] = m_window[rp]; 
								}
							}
							m_lastPos = 0;
							//Insert in window:
							for (size_t a = 0; a < m_lastLen; a++)
							{
								const size_t p = (m_windowStart + m_windowSize + a) % WINDOW_SIZE;
								m_window[p] = m_last[a];
							}
							m_windowSize += m_lastLen;
							if (m_windowSize > WINDOW_SIZE)
							{
								m_windowStart = (m_windowStart + m_windowSize) % WINDOW_SIZE;
								m_windowSize = WINDOW_SIZE;
							}
						}
						//insert in dst:
						m_data[page][dstPos] = m_last[m_lastPos++];
						res++;
					}
					m_pageLen[page] = res;
				}
				void* CLOAK_CALL LZCReadBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL LZCReadBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }
			}
		}
	}
}