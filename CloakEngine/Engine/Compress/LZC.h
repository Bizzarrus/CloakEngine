#pragma once
#ifndef CE_E_COMPRESS_LZC_H
#define CE_E_COMPRESS_LZC_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Compressor.h"
#include "Implementation/Files/Buffer.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace LZC {
				class LZCWriteBuffer : public virtual API::Files::Buffer_v1::IWriteBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr uint32_t BYTE_COUNT = 256;
						static constexpr uint32_t DATA_SIZE = 64 KB;
						static constexpr uint32_t BIT_COUNT_OFFSET = 11;
						static constexpr uint32_t BIT_COUNT_WORD = 6;
						static constexpr uint32_t WINDOW_SIZE = 1U << BIT_COUNT_OFFSET;
						static constexpr uint32_t MIN_WORD_LEN = (BIT_COUNT_OFFSET + BIT_COUNT_WORD + 9) / 9;
						static constexpr uint32_t MAX_WORD_LEN = (1U << BIT_COUNT_WORD) + MIN_WORD_LEN - 2;

						CLOAK_CALL LZCWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~LZCWriteBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
						virtual void CLOAK_CALL_THIS Save() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS writeData(In size_t bytes, In bool flush);

						CE::RefPointer<API::Files::Compressor_v2::IWriter> m_output;
						uint8_t m_data[DATA_SIZE];
						uint8_t m_window[WINDOW_SIZE];
						uint64_t m_writeCount;
						unsigned int m_windowSize;
						unsigned int m_windowStart;
						unsigned int m_bytePos;
				};
				class LZCReadBuffer : public virtual API::Files::Buffer_v1::IReadBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr uint32_t BYTE_COUNT = LZCWriteBuffer::BYTE_COUNT;
						static constexpr uint32_t BIT_COUNT_OFFSET = LZCWriteBuffer::BIT_COUNT_OFFSET;
						static constexpr uint32_t BIT_COUNT_WORD = LZCWriteBuffer::BIT_COUNT_WORD;
						static constexpr uint32_t WINDOW_SIZE = LZCWriteBuffer::WINDOW_SIZE;
						static constexpr uint32_t DATA_SIZE = 64 KB;
						static constexpr uint32_t MIN_WORD_LEN = LZCWriteBuffer::MIN_WORD_LEN;
						static constexpr uint32_t MAX_WORD_LEN = LZCWriteBuffer::MAX_WORD_LEN;
						static constexpr uint32_t PAGE_SIZE = 2;
						static constexpr uint32_t PAGE_DATA_SIZE = DATA_SIZE / PAGE_SIZE;

						CLOAK_CALL LZCReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~LZCReadBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS readData(In size_t page);

						CE::RefPointer<API::Files::Compressor_v2::IReader> m_input;
						uint8_t m_data[PAGE_SIZE][DATA_SIZE];
						uint8_t m_window[WINDOW_SIZE];
						uint8_t m_last[MAX_WORD_LEN];
						uint32_t m_lastPos;
						uint32_t m_lastLen;
						uint64_t m_readCount;
						size_t m_curPage;
						uint32_t m_pageLen[PAGE_SIZE];
						unsigned int m_windowSize;
						unsigned int m_windowStart;
						unsigned int m_bytePos;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif