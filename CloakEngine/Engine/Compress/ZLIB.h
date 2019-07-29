#pragma once
#ifndef CE_E_COMPRESS_ZLIB_H
#define CE_E_COMPRESS_ZLIB_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Compressor.h"
#include "Implementation/Files/Buffer.h"

#include <zlib.h> //from vcpkg

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace ZLIB {
				class ZLIBWriteBuffer : public virtual API::Files::Buffer_v1::IWriteBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr size_t BUFFER_IN_SIZE = 32 KB;
						static constexpr size_t BUFFER_OUT_SIZE = 64 KB;

						CLOAK_CALL ZLIBWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~ZLIBWriteBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
						virtual void CLOAK_CALL_THIS Save() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS iWrite(In bool finish);

						uint8_t m_inBuffer[BUFFER_IN_SIZE];
						uint8_t m_outBuffer[BUFFER_OUT_SIZE];
						size_t m_bufferPos;
						uint64_t m_writeCount;
						z_stream m_stream;
						CE::RefPointer<API::Files::IWriteBuffer> m_output;
						bool m_init;
				};
				class ZLIBReadBuffer : public virtual API::Files::Buffer_v1::IReadBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						static constexpr size_t BUFFER_IN_SIZE = 64 KB;
						static constexpr size_t BUFFER_OUT_SIZE = 32 KB;

						CLOAK_CALL ZLIBReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~ZLIBReadBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS iRead();

						uint8_t m_inBuffer[BUFFER_IN_SIZE];
						uint8_t m_outBuffer[BUFFER_OUT_SIZE];
						size_t m_inPos;
						size_t m_bufferPos;
						size_t m_bufferSize;
						uint64_t m_readCount;
						z_stream m_stream;
						CE::RefPointer<API::Files::IReadBuffer> m_input;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif