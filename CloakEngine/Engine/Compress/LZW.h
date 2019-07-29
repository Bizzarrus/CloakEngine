#pragma once
#ifndef CE_E_COMPRESS_LZW_H
#define CE_E_COMPRESS_LZW_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Compressor.h"
#include "Implementation/Files/Buffer.h"
#include "Engine/LinkedList.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace LZW {
				struct LibReverse {
					LibReverse* prev = nullptr;
					uint8_t Byte = 0;
					void* CLOAK_CALL operator new(In size_t s);
					void CLOAK_CALL operator delete(In void* ptr);
				};

				class LZWWriteBuffer : public virtual API::Files::Buffer_v1::IWriteBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr uint32_t DATA_SIZE = 64 KB;

						CLOAK_CALL LZWWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~LZWWriteBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
						virtual void CLOAK_CALL_THIS Save() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS writeData(In size_t bytes);
						
						CE::RefPointer<API::Files::Compressor_v2::IWriter> m_output;
						unsigned int m_bytePos;
						uint64_t m_writeCount;
						uint8_t m_data[DATA_SIZE];
				};
				class LZWReadBuffer : public virtual API::Files::Buffer_v1::IReadBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr uint32_t DATA_SIZE = LZWWriteBuffer::DATA_SIZE;
						static constexpr uint32_t PAGE_SIZE = 1;
						static constexpr uint32_t PAGE_DATA_SIZE = DATA_SIZE / PAGE_SIZE;

						CLOAK_CALL LZWReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~LZWReadBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS readData(In size_t page);
						virtual bool CLOAK_CALL_THIS readCodeByte(Out uint8_t* res);
						virtual void CLOAK_CALL_THIS resetLib();

						API::List<LibReverse*> m_lib;
						LibReverse* m_last;
						API::Stack<uint8_t> m_dataBuffer;
						CE::RefPointer<API::Files::Compressor_v2::IReader> m_input;
						unsigned int m_bytePos;
						unsigned int m_pagePos;
						size_t m_bitSize;
						size_t m_libDur;
						bool m_isAtEnd;
						uint64_t m_readCount;
						uint32_t m_dataLen[PAGE_SIZE];
						uint8_t m_data[PAGE_SIZE][PAGE_DATA_SIZE];
				};
			}
		}
	}
}

#pragma warning(pop)

#endif