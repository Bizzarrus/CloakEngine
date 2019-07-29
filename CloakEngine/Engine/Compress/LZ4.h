#pragma once
#ifndef CE_E_COMPRESS_LZ4_H
#define CE_E_COMPRESS_LZ4_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Compressor.h"
#include "Implementation/Files/Buffer.h"

#define LZ4_MEMORY_USAGE 20
#include <lz4.h> //from vcpkg

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace LZ4 {
				class LZ4WriteBuffer : public virtual API::Files::Buffer_v1::IWriteBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr size_t MAX_BLOCK_SIZE = 4 KB;
						static constexpr size_t BUFFER_SIZE = 64 KB + MAX_BLOCK_SIZE;
						static constexpr size_t COMPRESS_BUFFER_SIZE = LZ4_COMPRESSBOUND(MAX_BLOCK_SIZE);

						CLOAK_CALL LZ4WriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~LZ4WriteBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
						virtual void CLOAK_CALL_THIS Save() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						void CLOAK_CALL_THIS iWrite(In bool finalWrite);

						CE::RefPointer<API::Files::IWriteBuffer> m_output;
						LZ4_stream_t* m_stream;
						uint8_t m_buffer[BUFFER_SIZE];
						uint8_t m_compBuffer[COMPRESS_BUFFER_SIZE];
						size_t m_start;
						size_t m_count;
						uint64_t m_writeCount;
				};
				class LZ4ReadBuffer : public virtual API::Files::Buffer_v1::IReadBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr size_t MAX_BLOCK_SIZE = LZ4WriteBuffer::MAX_BLOCK_SIZE;
						static constexpr size_t BUFFER_SIZE = LZ4_DECODER_RING_BUFFER_SIZE(MAX_BLOCK_SIZE);
						static constexpr size_t INPUT_BUFFER_SIZE = LZ4WriteBuffer::COMPRESS_BUFFER_SIZE;

						CLOAK_CALL LZ4ReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~LZ4ReadBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						void CLOAK_CALL iRead();

						CE::RefPointer<API::Files::IReadBuffer> m_input;
						LZ4_streamDecode_t* m_stream;
						uint8_t m_inputBuffer[INPUT_BUFFER_SIZE];
						uint8_t m_buffer[BUFFER_SIZE];
						size_t m_start;
						size_t m_count;
						uint64_t m_readCount;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif