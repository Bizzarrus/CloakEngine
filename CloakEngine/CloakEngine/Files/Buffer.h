#pragma once
#ifndef CE_API_FILES_BUFFER_H
#define CE_API_FILES_BUFFER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Global/Task.h"

#include <string>
#include <string_view>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			inline namespace Buffer_v1 {
				//Unique Game ID
				struct CLOAKENGINE_API UGID {
					public:
						static constexpr size_t ID_SIZE = 24;
						uint8_t ID[ID_SIZE];

						CLOAK_CALL UGID();
						CLOAK_CALL UGID(In const UGID& x);
						CLOAK_CALL UGID(In const GUID& x);
						UGID& CLOAK_CALL_THIS operator=(In const UGID& x);
						UGID& CLOAK_CALL_THIS operator=(In const GUID& x);
				};
				enum class HTTPMethod {
					GET,
					POST
				};
				enum class SeekOrigin {
					START,
					CURRENT_POS,
					END,
				};
				typedef uint64_t RequestID;

				CLOAK_INTERFACE_ID("{1F118E93-3288-4F47-8C45-387347AD3D20}") IBuffer : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const = 0;
				};
				CLOAK_INTERFACE_ID("{91C12B35-5984-40C0-85F1-3B4992B14524}") IFileBuffer : public virtual Buffer_v1::IBuffer{
					public:
						virtual void CLOAK_CALL_THIS Seek(In SeekOrigin origin, In long long byteOffset) = 0;
				};
				CLOAK_INTERFACE_ID("{9F4676CC-EFFD-446E-9B70-16E730523700}") IReadBuffer : public virtual Buffer_v1::IBuffer{
					public:
						virtual uint8_t CLOAK_CALL_THIS ReadByte() = 0;
						virtual bool CLOAK_CALL_THIS HasNextByte() = 0;
				};
				CLOAK_INTERFACE_ID("{AD565D9B-8605-427B-A1C3-083584175F38}") IWriteBuffer : public virtual Buffer_v1::IBuffer{
					public:
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) = 0;
						virtual void CLOAK_CALL_THIS Save() = 0;
				};
				CLOAK_INTERFACE_ID("{0F43C996-C50A-475C-BC93-4CA04687CDA9}") IVirtualWriteBuffer : public virtual Buffer_v1::IWriteBuffer{
					public:
						virtual CE::RefPointer<IReadBuffer> CLOAK_CALL_THIS GetReadBuffer() = 0;
						virtual void CLOAK_CALL_THIS Clear() = 0;
				};

				enum class CompressType 
				{
					NONE,
					LZC,
					LZW,
					HUFFMAN,
					ARITHMETIC,
					LZH, //LZC + HUFFMAN
					LZ4,
					ZLIB, //using deflate of zlib dll
				};
				//Unique File Identifier
				class CLOAKENGINE_API UFI {
					public:
						CLOAK_CALL UFI();

						CLOAK_CALL UFI(In const std::string& filePath);
						CLOAK_CALL UFI(In const std::u16string& filePath);
						CLOAK_CALL UFI(In const std::wstring& filePath);
						CLOAK_CALL UFI(In const std::string_view& filePath);
						CLOAK_CALL UFI(In const std::u16string_view& filePath);
						CLOAK_CALL UFI(In const std::wstring_view& filePath);
						CLOAK_CALL UFI(In const char* filePath);
						CLOAK_CALL UFI(In const char16_t* filePath);
						CLOAK_CALL UFI(In const wchar_t* filePath);

						CLOAK_CALL UFI(In const UFI& u, In const std::string& filePath);
						CLOAK_CALL UFI(In const UFI& u, In const std::u16string& filePath);
						CLOAK_CALL UFI(In const UFI& u, In const std::wstring& filePath);
						CLOAK_CALL UFI(In const UFI& u, In const std::string_view& filePath);
						CLOAK_CALL UFI(In const UFI& u, In const std::u16string_view& filePath);
						CLOAK_CALL UFI(In const UFI& u, In const std::wstring_view& filePath);
						CLOAK_CALL UFI(In const UFI& u, In const char* filePath);
						CLOAK_CALL UFI(In const UFI& u, In const char16_t* filePath);
						CLOAK_CALL UFI(In const UFI& u, In const wchar_t* filePath);

						CLOAK_CALL UFI(In const UFI& u, In uint32_t offset);

						CLOAK_CALL UFI(In const UFI& o);
						CLOAK_CALL UFI(In UFI&& o);
						CLOAK_CALL ~UFI();
						UFI& CLOAK_CALL_THIS operator=(In const UFI& o);
						bool CLOAK_CALL_THIS operator==(In const UFI& o) const;
						bool CLOAK_CALL_THIS operator!=(In const UFI& o) const;
						CE::RefPointer<IReadBuffer> CLOAK_CALL_THIS OpenReader(In_opt bool silent = false, In_opt std::string fileType = "") const;
						CE::RefPointer<IWriteBuffer> CLOAK_CALL_THIS OpenWriter(In_opt bool silent = false, In_opt std::string fileType = "") const;
						size_t CLOAK_CALL_THIS GetHash() const;
						std::string CLOAK_CALL_THIS GetRoot() const;
						std::string CLOAK_CALL_THIS GetPath() const;
						std::string CLOAK_CALL_THIS GetName() const;
						std::string CLOAK_CALL_THIS ToString() const;

						UFI CLOAK_CALL_THIS Append(In const std::string& filePath) const;
						UFI CLOAK_CALL_THIS Append(In const std::u16string& filePath) const;
						UFI CLOAK_CALL_THIS Append(In const std::wstring& filePath) const;
						UFI CLOAK_CALL_THIS Append(In const std::string_view& filePath) const;
						UFI CLOAK_CALL_THIS Append(In const std::u16string_view& filePath) const;
						UFI CLOAK_CALL_THIS Append(In const std::wstring_view& filePath) const;
						UFI CLOAK_CALL_THIS Append(In const char* filePath) const;
						UFI CLOAK_CALL_THIS Append(In const char16_t* filePath) const;
						UFI CLOAK_CALL_THIS Append(In const wchar_t* filePath) const;
					private:
						uint32_t m_type;
						size_t m_hash;
						void* m_data;
				};

				CLOAKENGINE_API CE::RefPointer<IVirtualWriteBuffer> CLOAK_CALL CreateVirtualWriteBuffer();
				CLOAKENGINE_API CE::RefPointer<IVirtualWriteBuffer> CLOAK_CALL CreateTempWriteBuffer();
				CLOAKENGINE_API CE::RefPointer<IReadBuffer> CLOAK_CALL CreateUTFRead(In const CE::RefPointer<IReadBuffer>& input);
				CLOAKENGINE_API CE::RefPointer<IReadBuffer> CLOAK_CALL CreateCompressRead(In const CE::RefPointer<IReadBuffer>& input, In CompressType type);
				CLOAKENGINE_API CE::RefPointer<IWriteBuffer> CLOAK_CALL CreateCompressWrite(In const CE::RefPointer<IWriteBuffer>& output, In CompressType type);
				CLOAKENGINE_API std::string CLOAK_CALL GetFullFilePath(In const std::string& path);
				CLOAKENGINE_API std::u16string CLOAK_CALL GetFullFilePath(In const std::u16string& path);
			}
		}
	}
}

#endif