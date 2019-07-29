#pragma once
#ifndef CE_E_COMPRESS_HUFFMAN_H
#define CE_E_COMPRESS_HUFFMAN_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Compressor.h"
#include "Implementation/Files/Buffer.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace Huffman {
				struct TreeNode {
					enum class TYPE { DATA, NODE } Type;
					union {
						uint8_t Data;
						const TreeNode* Childs[2];
					};
				};


				struct Leaf {
					uint32_t Len = 0;//Range 0 to 256
					uint64_t Data[4] = { 0,0,0,0 };

					CLOAK_CALL Leaf() {}
					CLOAK_CALL Leaf(In const Leaf& l) { Len = l.Len; for (size_t a = 0; a < 4; a++) { Data[a] = l.Data[a]; } }
					Leaf& CLOAK_CALL_THIS operator=(In const Leaf& l) { Len = l.Len; for (size_t a = 0; a < 4; a++) { Data[a] = l.Data[a]; } return *this; }
					Leaf CLOAK_CALL_THIS operator<<(In uint32_t c) const
					{
						Leaf l = *this;
						return l <<= c;
					}
					Leaf CLOAK_CALL_THIS operator|(In bool c) const
					{
						Leaf l = *this;
						return l |= c;
					}
					Leaf& CLOAK_CALL_THIS operator<<=(In uint32_t c)
					{
						for (size_t a = 0; a < 4; a++)
						{
							Data[a] <<= c;
							if (a < 3) { Data[a] |= Data[a + 1] >> (64 - c); }
						}
						Len = min(Len + c, 256);
						return *this;
					}
					Leaf& CLOAK_CALL_THIS operator|=(In bool c)
					{
						if (c == true) { Data[3] |= 1; }
						return *this;
					}
					bool CLOAK_CALL_THIS operator==(const Leaf& l) const
					{
						return Len == l.Len && Data[0] == l.Data[0] && Data[1] == l.Data[1] && Data[2] == l.Data[2] && Data[3] == l.Data[3];
					}
				};
				struct GraphLeaf {
					union {
						struct {
							size_t Left;
							bool HasLeft;
							size_t Right;
							bool HasRight;
						} Tree;
						uint8_t Value;
					};
					bool isValue;
					CLOAK_CALL GraphLeaf() 
					{
						reset();
					}
					void CLOAK_CALL_THIS reset()
					{
						isValue = false;
						Tree.Left = 0;
						Tree.Right = 0;
						Tree.HasLeft = false;
						Tree.HasRight = false;
					}
				};

				class HuffmanWriteBuffer : public virtual API::Files::Buffer_v1::IWriteBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr uint32_t DATA_SIZE = 64 KB;
						static constexpr uint32_t TREE_SIZE = 256;
						static constexpr uint32_t TREE_ITERATIONS = TREE_SIZE - 1;
						static constexpr uint32_t TREE_GRAPH_SIZE = TREE_SIZE + TREE_ITERATIONS;

						CLOAK_CALL HuffmanWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~HuffmanWriteBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
						virtual void CLOAK_CALL_THIS Save() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS writeData(In size_t bytes);
						virtual void CLOAK_CALL_THIS writeCodeByte(In uint8_t byte);

						CE::RefPointer<API::Files::Compressor_v2::IWriter> m_output;
						uint32_t m_count[TREE_SIZE];
						unsigned int m_bytePos;
						uint64_t m_writtenCount;
						uint8_t m_data[DATA_SIZE];
						Leaf m_tree[TREE_SIZE];
						bool m_hasTree;
				};
				class HuffmanReadBuffer : public virtual API::Files::Buffer_v1::IReadBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr uint32_t DATA_SIZE = HuffmanWriteBuffer::DATA_SIZE;
						static constexpr uint32_t TREE_SIZE = HuffmanWriteBuffer::TREE_SIZE;
						static constexpr uint32_t TREE_GRAPH_SIZE = HuffmanWriteBuffer::TREE_GRAPH_SIZE;
						static constexpr uint32_t PAGE_SIZE = 1;
						static constexpr uint32_t PAGE_DATA_SIZE = DATA_SIZE / PAGE_SIZE;

						CLOAK_CALL HuffmanReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~HuffmanReadBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS readData(In size_t page);
						virtual bool CLOAK_CALL_THIS readCodeByte(Out uint8_t* res);

						CE::RefPointer<API::Files::Compressor_v2::IReader> m_input;
						TreeNode m_tree[TREE_GRAPH_SIZE];
						unsigned int m_bytePos;
						unsigned int m_pagePos;
						uint64_t m_readCount;
						bool m_isAtEnd;
						uint32_t m_treeDur;
						uint32_t m_dataLen[PAGE_SIZE];
						uint8_t m_data[PAGE_SIZE][PAGE_DATA_SIZE];
				};
			}
		}
	}
}

#pragma warning(pop)

#endif