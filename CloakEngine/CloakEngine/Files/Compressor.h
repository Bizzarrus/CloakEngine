#pragma once
#ifndef CE_API_FILES_COMPRESSOR_H
#define CE_API_FILES_COMPRESSOR_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Files/Buffer.h"

#include <Windows.h>
#include <string>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			inline namespace Compressor_v2 {
				typedef unsigned int FileVersion;
				struct FileType {
					CLOAKENGINE_API constexpr CLOAK_CALL FileType(In const char* k, In const char* t, In FileVersion v) : Key(k), Type(t), Version(v) {}
					CLOAKENGINE_API constexpr CLOAK_CALL FileType(In const char* t) : Key(""), Type(t), Version(0) {}
					const char* Key;
					const char* Type;
					FileVersion Version;
				};
				CLOAKENGINE_API void CLOAK_CALL SetDefaultGameID(In const Buffer_v1::UGID& gameID);
				CLOAK_INTERFACE_ID("{1C8BFDE0-3B1A-4196-96ED-E2222C8A4361}") ICompressor : public virtual Buffer_v1::IBuffer{
					public:
						virtual unsigned int CLOAK_CALL_THIS GetBitPosition() const = 0;
						virtual void CLOAK_CALL_THIS MoveToNextByte() = 0;
						virtual void CLOAK_CALL_THIS Move(In unsigned long long byte, In_opt unsigned int bit = 0) = 0;
						virtual void CLOAK_CALL_THIS MoveTo(In unsigned long long byte, In_opt unsigned int bit = 0) = 0;
				};
				CLOAK_INTERFACE_ID("{F454C0B1-73CB-486A-8D17-85374DE69FE2}") IWriter : public virtual Compressor_v2::ICompressor, public virtual Buffer_v1::IWriteBuffer {
					public:
						virtual void CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<Buffer_v1::IWriteBuffer>& buffer, In_opt API::Files::Buffer_v1::CompressType Type = API::Files::Buffer_v1::CompressType::NONE, In_opt bool txt = false) = 0;
						virtual void CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<Buffer_v1::IWriteBuffer>& buffer, In const FileType& type, In API::Files::Buffer_v1::CompressType Type, In_opt bool txt = false) = 0;
						virtual void CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<Buffer_v1::IWriteBuffer>& buffer, In const FileType& type, In API::Files::Buffer_v1::CompressType Type, In const Buffer_v1::UGID& cgid, In_opt bool txt = false) = 0;
						virtual void CLOAK_CALL_THIS SetTarget(In const Buffer_v1::UFI& ufi, In_opt API::Files::Buffer_v1::CompressType Type = API::Files::Buffer_v1::CompressType::NONE, In_opt bool txt = false) = 0;
						virtual void CLOAK_CALL_THIS SetTarget(In const Buffer_v1::UFI& ufi, In const FileType& type, In API::Files::Buffer_v1::CompressType Type, In_opt bool txt = false) = 0;
						virtual void CLOAK_CALL_THIS SetTarget(In const Buffer_v1::UFI& ufi, In const FileType& type, In API::Files::Buffer_v1::CompressType Type, In const Buffer_v1::UGID& cgid, In_opt bool txt = false) = 0;
						
						virtual void CLOAK_CALL_THIS WriteBits(In size_t len, In uint64_t data) = 0;
						virtual void CLOAK_CALL_THIS WriteBytes(In const API::List<uint8_t>& data, In_opt size_t len = 0, In_opt size_t offset = 0) = 0;
						virtual void CLOAK_CALL_THIS WriteBytes(In_reads(dataSize) const uint8_t* data, In size_t dataSize, In_opt size_t len = 0, In_opt size_t offset = 0) = 0;
						virtual void CLOAK_CALL_THIS WriteDouble(In size_t len, In long double data, In_opt size_t expLen = 0) = 0;
						virtual void CLOAK_CALL_THIS WriteString(In const std::string& str, In_opt size_t len = 0, In_opt size_t offset = 0) = 0;
						virtual void CLOAK_CALL_THIS WriteString(In const std::u16string& str, In_opt size_t len = 0, In_opt size_t offset = 0) = 0;
						virtual void CLOAK_CALL_THIS WriteStringWithLength(In const std::string& str, In_opt size_t lenSize = 8, In_opt size_t len = 0, In_opt size_t offset = 0) = 0;
						virtual void CLOAK_CALL_THIS WriteStringWithLength(In const std::u16string& str, In_opt size_t lenSize = 8, In_opt size_t len = 0, In_opt size_t offset = 0) = 0;
						virtual void CLOAK_CALL_THIS WriteGameID(In const Buffer_v1::UGID& cgid) = 0;
						virtual void CLOAK_CALL_THIS WriteBuffer(In Buffer_v1::IReadBuffer* buffer, In_opt unsigned long long byte = 0, In_opt unsigned int bit = 0) = 0;
						virtual void CLOAK_CALL_THIS WriteBuffer(In Buffer_v1::IVirtualWriteBuffer* buffer, In_opt unsigned long long byte = 0, In_opt unsigned int bit = 0) = 0;
						virtual void CLOAK_CALL_THIS WriteDynamic(In uint64_t data) = 0;
				};
				CLOAK_INTERFACE_ID("{B3FFA52B-F111-4180-BE85-19CA5F791174}") IReader : public virtual Compressor_v2::ICompressor, public virtual Buffer_v1::IReadBuffer{
					public:
						virtual FileVersion CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<Buffer_v1::IReadBuffer>& buffer, In_opt API::Files::Buffer_v1::CompressType Type = API::Files::Buffer_v1::CompressType::NONE, In_opt bool txt = false) = 0;
						virtual FileVersion CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<Buffer_v1::IReadBuffer>& buffer, In const FileType& type, In_opt bool txt = false) = 0;
						virtual FileVersion CLOAK_CALL_THIS SetTarget(In const CE::RefPointer<Buffer_v1::IReadBuffer>& buffer, In const FileType& type, In const Buffer_v1::UGID& cgid, In_opt bool txt = false) = 0;
						virtual FileVersion CLOAK_CALL_THIS SetTarget(In const Buffer_v1::UFI& ufi, In_opt API::Files::Buffer_v1::CompressType Type = API::Files::Buffer_v1::CompressType::NONE, In_opt bool txt = false, In_opt bool silentTry = false) = 0;
						virtual FileVersion CLOAK_CALL_THIS SetTarget(In const Buffer_v1::UFI& ufi, In const FileType& type, In_opt bool txt = false, In_opt bool silentTry = false) = 0;
						virtual FileVersion CLOAK_CALL_THIS SetTarget(In const Buffer_v1::UFI& ufi, In const FileType& type, In const Buffer_v1::UGID& cgid, In_opt bool txt = false, In_opt bool silentTry = false) = 0;
						
						virtual uint64_t CLOAK_CALL_THIS ReadBits(In size_t len) = 0;
						virtual void CLOAK_CALL_THIS ReadBytes(In size_t len, Inout API::List<uint8_t>& data) = 0;
						virtual void CLOAK_CALL_THIS ReadBytes(In size_t len, Out_writes(dataSize) uint8_t* data, In size_t dataSize) = 0;
						virtual long double CLOAK_CALL_THIS ReadDouble(In size_t len, In_opt size_t expLen = 0) = 0;
						virtual std::string CLOAK_CALL_THIS ReadString(In size_t len) = 0;
						virtual std::string CLOAK_CALL_THIS ReadStringWithLength(In_opt size_t lenSize = 8) = 0;
						virtual bool CLOAK_CALL_THIS ReadGameID(In const Buffer_v1::UGID& cgid) = 0;
						virtual bool CLOAK_CALL_THIS ReadLine(Out std::string* line, In_opt char32_t delChar = U'\0') = 0;
						virtual void CLOAK_CALL_THIS ReadBuffer(In Buffer_v1::IWriteBuffer* buffer, In size_t byte, In_opt size_t bit = 0) = 0;
						virtual uint64_t CLOAK_CALL_THIS ReadDynamic() = 0;
						virtual bool CLOAK_CALL_THIS IsAtEnd() const = 0;
				};
			}
		}
	}
}

#endif