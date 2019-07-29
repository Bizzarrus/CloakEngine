#pragma once
#ifndef CE_IMPL_FILES_COMPRESSOR_H
#define CE_IMPL_FILES_COMPRESSOR_H

#include "CloakEngine/Files/Compressor.h"
#include "Implementation/Helper/SavePtr.h"

#ifdef OLD_COMPR
#include "Engine/Compress/Core.h"
#include <fstream>
#endif

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			inline namespace Compressor_v2 {
				class CLOAK_UUID("{9E6833C5-4B30-4C96-8ABB-443E9954C8AB}") Writer : public virtual API::Files::Compressor_v2::IWriter, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL_THIS Writer();
						virtual CLOAK_CALL_THIS ~Writer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual unsigned int CLOAK_CALL_THIS GetBitPosition() const override;
						virtual void CLOAK_CALL_THIS Move(In unsigned long long byte, In_opt unsigned int bit = 0) override;
						virtual void CLOAK_CALL_THIS MoveTo(In unsigned long long byte, In_opt unsigned int bit = 0) override;
						virtual void CLOAK_CALL_THIS MoveToNextByte() override;
						virtual void CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& buffer, In_opt API::Files::Buffer_v1::CompressType Type = API::Files::Buffer_v1::CompressType::NONE, In_opt bool txt = false) override;
						virtual void CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& buffer, In const API::Files::Compressor_v2::FileType& type, In API::Files::Buffer_v1::CompressType Type, In_opt bool txt = false) override;
						virtual void CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& buffer, In const API::Files::Compressor_v2::FileType& type, In API::Files::Buffer_v1::CompressType Type, In const API::Files::Buffer_v1::UGID& cgid, In_opt bool txt = false) override;
						virtual void CLOAK_CALL_THIS SetTarget(In const API::Files::Buffer_v1::UFI & ufi, In_opt API::Files::Buffer_v1::CompressType Type = API::Files::Buffer_v1::CompressType::NONE, In_opt bool txt = false) override;
						virtual void CLOAK_CALL_THIS SetTarget(In const API::Files::Buffer_v1::UFI& ufi, In const API::Files::Compressor_v2::FileType& type, In API::Files::Buffer_v1::CompressType Type, In_opt bool txt = false) override;
						virtual void CLOAK_CALL_THIS SetTarget(In const API::Files::Buffer_v1::UFI& ufi, In const API::Files::Compressor_v2::FileType& type, In API::Files::Buffer_v1::CompressType Type, In const API::Files::Buffer_v1::UGID& cgid, In_opt bool txt = false) override;
						
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
						virtual void CLOAK_CALL_THIS WriteBits(In size_t len, In uint64_t data) override;
						virtual void CLOAK_CALL_THIS WriteBytes(In const API::List<uint8_t>& data, In_opt size_t len = 0, In_opt size_t offset = 0) override;
						virtual void CLOAK_CALL_THIS WriteBytes(In_reads(dataSize) const uint8_t* data, In size_t dataSize, In_opt size_t len = 0, In_opt size_t offset = 0) override;
						virtual void CLOAK_CALL_THIS WriteDouble(In size_t len, In long double data, In_opt size_t expLen = 0) override;
						virtual void CLOAK_CALL_THIS WriteString(In const std::string& str, In_opt size_t len = 0, In_opt size_t offset = 0) override;
						virtual void CLOAK_CALL_THIS WriteString(In const std::u16string& str, In_opt size_t len = 0, In_opt size_t offset = 0) override;
						virtual void CLOAK_CALL_THIS WriteStringWithLength(In const std::string& str, In_opt size_t lenSize = 8, In_opt size_t len = 0, In_opt size_t offset = 0) override;
						virtual void CLOAK_CALL_THIS WriteStringWithLength(In const std::u16string& str, In_opt size_t lenSize = 8, In_opt size_t len = 0, In_opt size_t offset = 0) override;
						virtual void CLOAK_CALL_THIS WriteGameID(In const API::Files::Buffer_v1::UGID& cgid) override;
						virtual void CLOAK_CALL_THIS WriteBuffer(In API::Files::Buffer_v1::IReadBuffer* buffer, In_opt unsigned long long byte = 0, In_opt unsigned int bit = 0) override;
						virtual void CLOAK_CALL_THIS WriteBuffer(In API::Files::Buffer_v1::IVirtualWriteBuffer* buffer, In_opt unsigned long long byte = 0, In_opt unsigned int bit = 0) override;
						virtual void CLOAK_CALL_THIS WriteDynamic(In uint64_t data) override;
						virtual void CLOAK_CALL_THIS Save() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						void CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& buffer, In const API::Files::Compressor_v2::FileType& type, In API::Files::Buffer_v1::CompressType Type, In_opt const API::Files::Buffer_v1::UGID* cgid, In bool txt);
						void CLOAK_CALL_THIS checkPos();

						CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer> m_output;
						uint8_t m_bitPos;
						uint8_t m_data;
						API::Files::Buffer_v1::CompressType m_compress;
				};
				class CLOAK_UUID("{51762D61-7BFD-459D-8894-6A92287E9F2D}") Reader : public virtual API::Files::Compressor_v2::IReader, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL_THIS Reader();
						virtual CLOAK_CALL_THIS ~Reader();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual unsigned int CLOAK_CALL_THIS GetBitPosition() const override;
						virtual void CLOAK_CALL_THIS Move(In unsigned long long byte, In_opt unsigned int bit = 0) override;
						virtual void CLOAK_CALL_THIS MoveTo(In unsigned long long byte, In_opt unsigned int bit = 0) override;
						virtual void CLOAK_CALL_THIS MoveToNextByte() override;
						virtual API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& buffer, In_opt API::Files::Buffer_v1::CompressType Type = API::Files::Buffer_v1::CompressType::NONE, In_opt bool txt = false) override;
						virtual API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& buffer, In const API::Files::Compressor_v2::FileType& type, In_opt bool txt = false) override;
						virtual API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& buffer, In const API::Files::Compressor_v2::FileType& type, In const API::Files::Buffer_v1::UGID& cgid, In_opt bool txt = false) override;
						virtual API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS SetTarget(In const API::Files::Buffer_v1::UFI & ufi, In_opt API::Files::Buffer_v1::CompressType Type = API::Files::Buffer_v1::CompressType::NONE, In_opt bool txt = false, In_opt bool silentTry = false) override;
						virtual API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS SetTarget(In const API::Files::Buffer_v1::UFI& ufi, In const API::Files::Compressor_v2::FileType& type, In_opt bool txt = false, In_opt bool silentTry = false) override;
						virtual API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS SetTarget(In const API::Files::Buffer_v1::UFI& ufi, In const API::Files::Compressor_v2::FileType& type, In const API::Files::Buffer_v1::UGID& cgid, In_opt bool txt = false, In_opt bool silentTry = false) override;
						
						virtual uint64_t CLOAK_CALL_THIS ReadBits(In size_t len) override;
						virtual void CLOAK_CALL_THIS ReadBytes(In size_t len, Inout API::List<uint8_t>& data) override;
						virtual void CLOAK_CALL_THIS ReadBytes(In size_t len, Out_writes(dataSize) uint8_t* data, In size_t dataSize) override;
						virtual long double CLOAK_CALL_THIS ReadDouble(In size_t len, In_opt size_t expLen = 0) override;
						virtual std::string CLOAK_CALL_THIS ReadString(In size_t len) override;
						virtual std::string CLOAK_CALL_THIS ReadStringWithLength(In_opt size_t lenSize = 8) override;
						virtual bool CLOAK_CALL_THIS ReadGameID(In const API::Files::Buffer_v1::UGID& cgid) override;
						virtual bool CLOAK_CALL_THIS ReadLine(Out std::string* line, In_opt char32_t delChar = U'\0') override;
						virtual void CLOAK_CALL_THIS ReadBuffer(In API::Files::Buffer_v1::IWriteBuffer* buffer, In size_t byte, In_opt size_t bit = 0) override;
						virtual uint64_t CLOAK_CALL_THIS ReadDynamic() override;
						virtual bool CLOAK_CALL_THIS IsAtEnd() const override;

						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;

						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& buffer, In const API::Files::Compressor_v2::FileType& ftype, In_opt bool forceExists = true, In_opt const API::Files::Buffer_v1::UGID* cgid = nullptr, In_opt bool txt = false);
						void CLOAK_CALL_THIS checkPos();

						CE::RefPointer<API::Files::Buffer_v1::IReadBuffer> m_input;
						uint8_t m_bitPos;
						uint8_t m_data;
						uint8_t m_validSize;
						unsigned long long m_bytePos;
				};
				void CLOAK_CALL Initialize();
				void CLOAK_CALL Release();
			}
		}
	}
}

#pragma warning(pop)

#endif