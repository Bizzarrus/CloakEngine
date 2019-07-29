#include "stdafx.h"
#include "Engine/Compress/Huffman.h"
#include "CloakEngine/Global/Memory.h"
#include "Implementation/Files/Compressor.h"

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace Huffman {
				struct GraphNode {
					bool IsBranch = false;
					bool InUse = false;
					uint32_t Chance = 0;
					Leaf Code;
					union {
						struct { uint32_t Value; } Leaf;
						struct {
							uint16_t LeafLeft;
							uint16_t LeafRight;
						} Branch;
					};
					CLOAK_CALL GraphNode() { Leaf.Value = 0; IsBranch = false; }
				};

				CLOAK_CALL HuffmanWriteBuffer::HuffmanWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type)
				{
					DEBUG_NAME(HuffmanWriteBuffer);
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_output));
					m_output->SetTarget(output, type);
					m_bytePos = 0;
					m_writtenCount = 0;
					m_hasTree = false;
					for (size_t a = 0; a < TREE_SIZE; a++) { m_count[a] = 0; }
				}
				CLOAK_CALL HuffmanWriteBuffer::~HuffmanWriteBuffer()
				{
					
				}
				unsigned long long CLOAK_CALL_THIS HuffmanWriteBuffer::GetPosition() const
				{
					return m_writtenCount;
				}
				void CLOAK_CALL_THIS HuffmanWriteBuffer::WriteByte(In uint8_t b)
				{
					m_writtenCount++;
					m_count[b]++;
					m_data[m_bytePos++] = b;
					if (m_bytePos >= DATA_SIZE)
					{ 
						writeData(m_bytePos); 
						m_bytePos = 0;
						for (size_t a = 0; a < TREE_SIZE; a++) { m_count[a] = 0; }
					}
				}
				void CLOAK_CALL_THIS HuffmanWriteBuffer::Save()
				{
					writeData(m_bytePos);
					m_bytePos = 0;
					for (size_t a = 0; a < TREE_SIZE; a++) { m_count[a] = 0; }
					writeData(0);
					m_output->Save();
				}

				Success(return == true) bool CLOAK_CALL_THIS HuffmanWriteBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::IWriteBuffer)) { *ptr = (API::Files::IWriteBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::IBuffer)) { *ptr = (API::Files::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS HuffmanWriteBuffer::writeData(In size_t bytes)
				{
					if (m_output != nullptr)
					{
						GraphNode Tree[TREE_GRAPH_SIZE];
						if (bytes > 0)
						{
							uint64_t resultLen = 0;
							uint32_t treeSize = 0;
							for (uint32_t a = 0; a < TREE_SIZE; a++)
							{
								if (m_count[a] > 0)
								{
									Tree[treeSize].IsBranch = false;
									Tree[treeSize].InUse = false;
									Tree[treeSize].Chance = m_count[a];
									Tree[treeSize].Leaf.Value = a;
									treeSize++;
								}
							}
							for (uint32_t a = treeSize; a < TREE_GRAPH_SIZE; a++)
							{
								Tree[a].IsBranch = true;
								Tree[a].InUse = false;
							}
							const uint32_t valueSize = treeSize;
							//Calculate tree:
							for (uint32_t a = treeSize; a > 1; a--, treeSize++)
							{
								uint32_t chance[2] = { 0,0 };
								GraphNode* n = &Tree[treeSize];
								n->Branch.LeafLeft = 0;
								n->Branch.LeafRight = 0;
								for (uint32_t b = 0; b < treeSize; b++)
								{
									GraphNode* t = &Tree[b];
									if (t->InUse == false)
									{
										if (chance[0] == 0 || t->Chance < chance[0])
										{
											chance[1] = chance[0];
											n->Branch.LeafRight = n->Branch.LeafLeft;
											n->Branch.LeafLeft = b;
											chance[0] = t->Chance;
										}
										else if (chance[1] == 0 || t->Chance < chance[1])
										{
											n->Branch.LeafRight = b;
											chance[1] = t->Chance;
										}
									}
								}
								n->Chance = chance[0] + chance[1];
								Tree[n->Branch.LeafLeft].InUse = true;
								Tree[n->Branch.LeafRight].InUse = true;
							}
							//Calculate code words:
							uint32_t stack[TREE_SIZE] = { treeSize - 1 };
							uint32_t stackSize = 1;
							do {
								const uint32_t p = stack[stackSize - 1];
								stackSize--;
								const GraphNode* n = &Tree[p];
								if (n->IsBranch)
								{
									const Leaf l = n->Code << 1;
									Tree[n->Branch.LeafLeft].Code = l | false;
									Tree[n->Branch.LeafRight].Code = l | true;
									stack[stackSize] = n->Branch.LeafLeft;
									stack[stackSize + 1] = n->Branch.LeafRight;
									stackSize += 2;
								}
								else
								{
									m_tree[n->Leaf.Value] = n->Code;
									resultLen += static_cast<uint64_t>(n->Chance)*n->Code.Len;
								}
							} while (stackSize > 0);
							//Write tree:
							m_output->WriteBits(1, 1);
							m_output->WriteBits(9, treeSize);
							uint32_t writeStack[TREE_GRAPH_SIZE] = { treeSize - 1 };
							uint32_t writePos = 0;
							uint32_t writeSize = 1;
							do {
								const GraphNode* n = &Tree[writeStack[writePos++]];
								if (n->IsBranch)
								{
									m_output->WriteBits(1, 1);
									writeStack[writeSize] = n->Branch.LeafLeft;
									writeStack[writeSize + 1] = n->Branch.LeafRight;
									writeSize += 2;
								}
								else
								{
									m_output->WriteBits(1, 0);
									m_output->WriteBits(8, n->Leaf.Value);
								}
							} while (writePos < writeSize);
							//Write data:
							m_output->WriteBits(17, bytes);
							for (size_t a = 0; a < bytes; a++) { writeCodeByte(m_data[a]); }
						}
					}
				}
				void CLOAK_CALL_THIS HuffmanWriteBuffer::writeCodeByte(In uint8_t byte)
				{
					const Leaf& l = m_tree[byte];
					for (size_t a = 0, b = 192; a < 4; a++, b -= 64)
					{
						if (l.Len > b) { m_output->WriteBits(min(64, l.Len - b), l.Data[a]); }
					}
				}
				void* CLOAK_CALL HuffmanWriteBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL HuffmanWriteBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

				CLOAK_CALL HuffmanReadBuffer::HuffmanReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type)
				{
					DEBUG_NAME(HuffmanReadBuffer);
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_input));
					m_input->SetTarget(input, type);
					m_bytePos = 0;
					m_pagePos = 0;
					m_readCount = 0;
					m_isAtEnd = false;
					m_treeDur = 0;
					for (size_t a = 0; a < PAGE_SIZE; a++) { readData(a); }
				}
				CLOAK_CALL HuffmanReadBuffer::~HuffmanReadBuffer()
				{
					
				}
				unsigned long long CLOAK_CALL_THIS HuffmanReadBuffer::GetPosition() const
				{
					return m_readCount;
				}
				uint8_t CLOAK_CALL_THIS HuffmanReadBuffer::ReadByte()
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
				bool CLOAK_CALL_THIS HuffmanReadBuffer::HasNextByte()
				{
					return m_bytePos < m_dataLen[m_pagePos] || m_isAtEnd;
				}

				Success(return == true) bool CLOAK_CALL_THIS HuffmanReadBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IReadBuffer)) { *ptr = (API::Files::Buffer_v1::IReadBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ptr = (API::Files::Buffer_v1::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS HuffmanReadBuffer::readData(In size_t page)
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
				bool CLOAK_CALL_THIS HuffmanReadBuffer::readCodeByte(Out uint8_t* res)
				{
					if (m_treeDur == 0)
					{
						if (m_input->ReadBits(1) == 1)
						{
							//Read tree:
							const uint32_t treeSize = static_cast<uint32_t>(m_input->ReadBits(9));
							uint32_t stack[TREE_GRAPH_SIZE] = { 0 };
							uint32_t stackPos = 0;
							uint32_t stackSize = 0;
							bool stackState = false;
							for (uint32_t a = 0; a < treeSize; a++)
							{
								if (stackSize > stackPos)
								{
									m_tree[stack[stackPos]].Childs[stackState == false ? 0 : 1] = &m_tree[a];
									stackState = !stackState;
									if (stackState == false) { stackPos++; }
								}
								if (m_input->ReadBits(1) == 1)
								{
									m_tree[a].Type = TreeNode::TYPE::NODE;
									stack[stackSize++] = a;
								}
								else
								{
									m_tree[a].Type = TreeNode::TYPE::DATA;
									m_tree[a].Data = static_cast<uint8_t>(m_input->ReadBits(8));
								}
							}
						}
						m_treeDur = static_cast<uint32_t>(m_input->ReadBits(17));
					}
					if (m_treeDur > 0)
					{
						m_treeDur--;
						const TreeNode* cur = &m_tree[0];
						while (cur->Type == TreeNode::TYPE::NODE)
						{
							if (m_input->ReadBits(1) == 0)
							{
								cur = cur->Childs[0];
							}
							else
							{
								cur = cur->Childs[1];
							}
							if (CloakCheckError(cur != nullptr, API::Global::Debug::Error::FREADER_FILE_CORRUPTED, false)) { return false; }
						}
						*res = cur->Data;
						return true;
					}
					return false;
				}
				void* CLOAK_CALL HuffmanReadBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL HuffmanReadBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }
			}
		}
	}
}
