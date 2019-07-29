#include "stdafx.h"
#include "Engine/Compress/LZW.h"
#include "CloakEngine/Global/Memory.h"

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace LZW {
				struct LibLink {
					size_t Next = 0;
					bool HasNext = false;
				};
				struct LibEntry {
					static constexpr uint32_t LINK_NUM = 256;
					LibLink Link[LINK_NUM];
				};

				constexpr size_t MAXBITSIZE = 12;

				void* CLOAK_CALL LibReverse::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				void CLOAK_CALL LibReverse::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

				CLOAK_CALL LZWWriteBuffer::LZWWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type)
				{
					DEBUG_NAME(HuffmanWriteBuffer);
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_output));
					m_output->SetTarget(output, type);
					m_bytePos = 0;
					m_writeCount = 0;
				}
				CLOAK_CALL LZWWriteBuffer::~LZWWriteBuffer()
				{
					
				}
				unsigned long long CLOAK_CALL_THIS LZWWriteBuffer::GetPosition() const
				{
					return m_writeCount;
				}
				void CLOAK_CALL_THIS LZWWriteBuffer::WriteByte(In uint8_t b)
				{
					m_writeCount++;
					m_data[m_bytePos++] = b;
					if (m_bytePos >= DATA_SIZE)
					{
						writeData(m_bytePos);
						m_bytePos = 0;
					}
				}
				void CLOAK_CALL_THIS LZWWriteBuffer::Save()
				{
					writeData(m_bytePos);
					m_bytePos = 0;
					m_output->Save();
				}
				void* CLOAK_CALL LZWWriteBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL LZWWriteBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS LZWWriteBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::IWriteBuffer)) { *ptr = (API::Files::IWriteBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::IBuffer)) { *ptr = (API::Files::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS LZWWriteBuffer::writeData(In size_t bytes)
				{
					if (m_output != nullptr)
					{
						m_output->WriteBits(17, bytes);
						//init lib:
						LibEntry libRoot;
						API::List<LibEntry> lib;
						for (uint32_t a = 0; a < LibEntry::LINK_NUM; a++)
						{
							libRoot.Link[a].HasNext = true;
							libRoot.Link[a].Next = a;
							lib.push_back(LibEntry());
						}
						LibEntry* cur = &libRoot;
						size_t curIndex = 0;
						size_t bitSize = 9;
						//write data:
						for (size_t a = 0; a < bytes; a++)
						{
							const uint8_t b = m_data[a];
							if (cur->Link[b].HasNext)
							{
								curIndex = cur->Link[b].Next;
								cur = &lib[curIndex];
							}
							else
							{
								cur->Link[b].HasNext = true;
								cur->Link[b].Next = lib.size();
								lib.push_back(LibEntry());
								m_output->WriteBits(bitSize, curIndex);
								if (lib.size() > (1U << bitSize) - 1) { bitSize++; }
								curIndex = libRoot.Link[b].Next;
								cur = &lib[curIndex];
							}
							if (bitSize > MAXBITSIZE)
							{
								if (cur != &libRoot) { m_output->WriteBits(MAXBITSIZE, curIndex); }
								lib.clear();
								for (uint32_t a = 0; a < LibEntry::LINK_NUM; a++)
								{
									libRoot.Link[a].HasNext = true;
									libRoot.Link[a].Next = a;
									lib.push_back(LibEntry());
								}
								cur = &libRoot;
								curIndex = 0;
								bitSize = 9;
							}
						}
						if (cur != &libRoot)
						{
							m_output->WriteBits(bitSize, curIndex);
						}
					}
				}
				CLOAK_CALL LZWReadBuffer::LZWReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type)
				{
					DEBUG_NAME(HuffmanReadBuffer);
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_input));
					m_input->SetTarget(input, type);
					m_bytePos = 0;
					m_pagePos = 0;
					m_readCount = 0;
					m_isAtEnd = false;
					for (size_t a = 0; a < PAGE_SIZE; a++) { readData(a); }
				}
				CLOAK_CALL LZWReadBuffer::~LZWReadBuffer()
				{
					for (size_t a = 0; a < m_lib.size(); a++) { delete m_lib[a]; }
				}
				unsigned long long CLOAK_CALL_THIS LZWReadBuffer::GetPosition() const
				{
					return m_readCount;
				}
				uint8_t CLOAK_CALL_THIS LZWReadBuffer::ReadByte()
				{
					if (m_input != nullptr && m_bytePos < m_dataLen[m_pagePos])
					{
						m_readCount++;
						uint8_t r = m_data[m_pagePos][m_bytePos++];
						if (m_bytePos >= m_dataLen[m_pagePos])
						{
							readData(m_pagePos);
							m_pagePos = (m_pagePos + 1) % PAGE_SIZE;
							m_bytePos = 0;
						}
						return r;
					}
					return 0;
				}
				bool CLOAK_CALL_THIS LZWReadBuffer::HasNextByte()
				{
					return m_bytePos < m_dataLen[m_pagePos] || m_isAtEnd;
				}

				Success(return == true) bool CLOAK_CALL_THIS LZWReadBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IReadBuffer)) { *ptr = (API::Files::Buffer_v1::IReadBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ptr = (API::Files::Buffer_v1::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS LZWReadBuffer::readData(In size_t page)
				{
					uint32_t rl = 0;
					uint8_t b;
					bool hasNext = false;
					while (rl < PAGE_DATA_SIZE && m_isAtEnd == false && (hasNext = readCodeByte(&b)) == true)
					{
						m_data[page][rl++] = b;
					}
					if (hasNext == false) { m_isAtEnd = true; }
					m_dataLen[page] = rl;
				}
				bool CLOAK_CALL_THIS LZWReadBuffer::readCodeByte(Out uint8_t* res)
				{
					if (m_dataBuffer.size() == 0 && m_input != nullptr && !m_input->IsAtEnd())
					{
						if (m_libDur == 0)
						{
							m_libDur = static_cast<size_t>(m_input->ReadBits(17));
							resetLib();
						}
						if (m_libDur > 0)
						{
							bool rsL = false;
							if (m_bitSize > MAXBITSIZE)
							{
								rsL = true;
								m_bitSize = MAXBITSIZE;
							}
							const size_t next = static_cast<size_t>(m_input->ReadBits(m_bitSize));
							LibReverse* r = nullptr;
							if (m_last != nullptr)
							{
								if (next < m_lib.size())
								{
									r = m_lib[next];
									LibReverse* b = r;
									while (b->prev != nullptr) { b = b->prev; }
									LibReverse* nr = new LibReverse();
									nr->prev = m_last;
									nr->Byte = b->Byte;
									m_lib.push_back(nr);
								}
								else
								{
									LibReverse* b = m_last;
									while (b->prev != nullptr) { b = b->prev; }
									LibReverse* nr = new LibReverse();
									nr->prev = m_last;
									nr->Byte = b->Byte;
									m_lib.push_back(nr);
									r = m_lib[next];
								}
								if (m_lib.size() > (1U << m_bitSize) - 2) { m_bitSize++; }
							}
							else { r = m_lib[next]; }
							m_last = r;
							do {
								m_dataBuffer.push(r->Byte);
								r = r->prev;
								m_libDur--;
							} while (r != nullptr);

							if (rsL) { resetLib(); }
						}
					}
					if (m_dataBuffer.size() > 0)
					{
						*res = m_dataBuffer.top();
						m_dataBuffer.pop();
						return true;
					}
					return false;
				}
				void CLOAK_CALL_THIS LZWReadBuffer::resetLib()
				{
					for (size_t a = 0; a < m_lib.size(); a++) { delete m_lib[a]; }
					m_lib.clear();
					for (size_t a = 0; a < LibEntry::LINK_NUM; a++)
					{
						LibReverse* r = new LibReverse();
						r->prev = nullptr;
						r->Byte = static_cast<uint8_t>(a);
						m_lib.push_back(r);
					}
					m_bitSize = 9;
					m_last = nullptr;
				}
				void* CLOAK_CALL LZWReadBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL LZWReadBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }
			}
		}
	}
}