#pragma once
#ifndef CE_IMPL_FILES_FILEIOBUFFER_H
#define CE_IMPL_FILES_FILEIOBUFFER_H

#include "CloakEngine/Files/Buffer.h"
#include "Implementation/Helper/SavePtr.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			inline namespace FileIO_v1 {
				void CLOAK_CALL Initialize();
				void CLOAK_CALL CloseAllFiles();
				void CLOAK_CALL Release();

				typedef void* FILE_ID;
				typedef uint64_t FILE_FENCE;

				class FileWriteBuffer : public virtual API::Files::Buffer_v1::IWriteBuffer, public virtual API::Files::Buffer_v1::IFileBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL FileWriteBuffer(In const std::string& filePath, In bool silent);
						CLOAK_CALL FileWriteBuffer(In FILE_ID fileID);
						virtual CLOAK_CALL ~FileWriteBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
						virtual void CLOAK_CALL_THIS Save() override;
						virtual void CLOAK_CALL_THIS Seek(In API::Files::Buffer_v1::SeekOrigin origin, In long long byteOffset) override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS WriteData(In uint64_t bytes);

						static constexpr uint64_t STORAGE_SIZE = 128 KB;
						static constexpr uint64_t PAGE_SIZE = 4;
						static constexpr uint64_t DATA_SIZE = STORAGE_SIZE / PAGE_SIZE;
						FILE_ID m_file;
						FILE_FENCE m_pageFence[PAGE_SIZE];
						uint64_t m_bytePos;
						uint64_t m_pagePos;
						unsigned long long m_byteOffset;
						uint8_t m_page[PAGE_SIZE][DATA_SIZE];
				};
				class FileReadBuffer : public virtual API::Files::Buffer_v1::IReadBuffer, public virtual API::Files::Buffer_v1::IFileBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL FileReadBuffer(In const std::string& filePath, In bool silent);
						CLOAK_CALL FileReadBuffer(In FILE_ID fileID);
						virtual CLOAK_CALL ~FileReadBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;
						virtual void CLOAK_CALL_THIS Seek(In API::Files::Buffer_v1::SeekOrigin origin, In long long byteOffset) override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS ReadData();

						static constexpr uint64_t STORAGE_SIZE = 128 KB;
						static constexpr uint64_t PAGE_SIZE = 4;
						static constexpr uint64_t DATA_SIZE = STORAGE_SIZE / PAGE_SIZE;
						FILE_ID m_file;
						FILE_FENCE m_pageFence[PAGE_SIZE];
						uint64_t m_bytePos;
						uint64_t m_pagePos;
						unsigned long long m_byteOffset;
						uint64_t m_pageLength[PAGE_SIZE];
						uint8_t m_page[PAGE_SIZE][DATA_SIZE];
				};
				class TempFileWriteBuffer : public FileWriteBuffer, public virtual API::Files::IVirtualWriteBuffer {
					friend class TempFileReadBuffer;
					public:
						CLOAK_CALL TempFileWriteBuffer();

						virtual CE::RefPointer<API::Files::Buffer_v1::IReadBuffer> CLOAK_CALL_THIS GetReadBuffer() override;
						virtual void CLOAK_CALL_THIS Clear() override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						std::atomic<uint64_t> m_state;
						std::atomic<FILE_ID> m_syncFile;
					};
				class TempFileReadBuffer : public FileReadBuffer {
					public:
						CLOAK_CALL TempFileReadBuffer(In FILE_ID fileID, In TempFileWriteBuffer* parent);
						virtual CLOAK_CALL ~TempFileReadBuffer();

						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;
						virtual void CLOAK_CALL_THIS Seek(In API::Files::Buffer_v1::SeekOrigin origin, In long long byteOffset) override;
					protected:
						virtual void CLOAK_CALL_THIS checkReload();

						TempFileWriteBuffer* const m_parent;
						uint64_t m_state;
				};
			}
		}
	}
}

#endif