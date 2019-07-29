#pragma once
#ifndef CE_E_COMPRESS_AAC_H
#define CE_E_COMPRESS_AAC_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Compressor.h"
#include "Implementation/Files/Buffer.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			//Adaptive Arithmetic Compression
			//TODO: Currently not working!
			namespace AAC {
				class AACWriteBuffer : public virtual API::Files::Buffer_v1::IWriteBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL AACWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~AACWriteBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
						virtual void CLOAK_CALL_THIS Save() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						CE::RefPointer<API::Files::Compressor_v2::IWriter> m_output;
						uint64_t m_histogram[256];
						uint64_t m_histoTotal;
						uint64_t m_low; //Low end of current range
						uint64_t m_high; //High end of current range
						uint64_t m_underflow;
				};
				class AACReadBuffer : public virtual API::Files::Buffer_v1::IReadBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL AACReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type);
						virtual CLOAK_CALL ~AACReadBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						CE::RefPointer<API::Files::Compressor_v2::IReader> m_input;
						uint64_t m_histogram[256];
						uint64_t m_histoTotal;
						uint64_t m_low;
						uint64_t m_high;
						uint64_t m_code;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif