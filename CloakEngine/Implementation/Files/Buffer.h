#pragma once
#ifndef CE_IMPL_FILES_BUFFER_H
#define CE_IMPL_FILES_BUFFER_H

#include "CloakEngine/Files/Buffer.h"
#include "CloakEngine/Files/ExtendedBuffers.h"
#include "Implementation/Helper/SavePtr.h"
#include "CloakEngine/Files/Compressor.h"
#include "CloakEngine/Global/Task.h"

#include <functional>
#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			inline namespace Buffer_v1 {
				void CLOAK_CALL Initialize();
				void CLOAK_CALL Release();

				template<size_t N> CLOAK_FORCEINLINE constexpr uint16_t CLOAK_CALL ASCIIToPort(In const char(&c)[N])
				{
					uint16_t r = 0;
					for (size_t a = 0; a < N; a++)
					{
						const size_t o = (N - (1 + a)) % 16;
						r ^= static_cast<uint16_t>(static_cast<uint16_t>(c[a]) << o);
					}
					return (r | 0x0400) & 0xBFFF;
				}

				constexpr uint16_t DEFAULT_PORT_SERVER = ASCIIToPort("CloakEngine");
				constexpr uint16_t DEFAULT_PORT_CLIENT = ASCIIToPort("CloakLobby");

				class VirtualReaderBuffer;
				class VirtualWriterBuffer : public virtual API::Files::Buffer_v1::IVirtualWriteBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					friend class VirtualReaderBuffer;
					public:
						CLOAK_CALL VirtualWriterBuffer();
						virtual CLOAK_CALL ~VirtualWriterBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
						virtual void CLOAK_CALL_THIS Save() override;
						virtual CE::RefPointer<API::Files::Buffer_v1::IReadBuffer> CLOAK_CALL_THIS GetReadBuffer() override;
						virtual void CLOAK_CALL_THIS Clear() override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						struct PAGE {
							PAGE* Next;
						};

						static constexpr size_t FULL_PAGE_SIZE = 2 MB;
						static constexpr size_t PAGE_SIZE = FULL_PAGE_SIZE - sizeof(PAGE);

						PAGE* m_start;
						PAGE* m_cur;
						uint64_t m_pos;
						uint8_t m_rbCID;
				};
				class VirtualReaderBuffer : public virtual API::Files::Buffer_v1::IReadBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL VirtualReaderBuffer(In VirtualWriterBuffer* src);
						virtual CLOAK_CALL ~VirtualReaderBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						VirtualWriterBuffer::PAGE* m_page;
						VirtualWriterBuffer* m_src;
						uint64_t m_pos;
						uint8_t m_lrbCID;
				};
				class MultithreadedWriteBuffer : public virtual API::Files::Buffer_v1::IMultithreadedWriteBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL MultithreadedWriteBuffer(In const CE::RefPointer<API::Files::IWriteBuffer>& dst, In_opt std::function<void(In void* userData, In uint64_t size)> onWrite = nullptr, In_opt API::Global::Threading::Flag flags = API::Global::Threading::Flag::None);
						virtual CLOAK_CALL ~MultithreadedWriteBuffer();
						virtual CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer> CLOAK_CALL_THIS CreateLocalWriteBuffer(In_opt void* userData = nullptr) override;
						virtual CE::RefPointer<API::Files::Compressor_v2::IWriter> CLOAK_CALL_THIS CreateLocalWriter(In_opt void* userData = nullptr) override;
						virtual void CLOAK_CALL_THIS SetFinishTask(In const API::Global::Task& task) override;
						virtual void CLOAK_CALL_THIS OnWrite();
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						class InternWriteBuffer;
						struct Page {
							Page* Next;
						};
						struct PageList {
							PageList* Next;
							Page* Page;
							void* UserData;
							uint64_t Size;
						};

						static constexpr uint64_t FULL_PAGE_SIZE = 32 KB;
						static constexpr uint64_t PAGE_LIST_SIZE = FULL_PAGE_SIZE - sizeof(PageList);
						static constexpr uint64_t PAGE_SIZE = FULL_PAGE_SIZE - sizeof(Page);

						class InternWriteBuffer : public API::Files::Buffer_v1::IWriteBuffer {
							public:
								CLOAK_CALL InternWriteBuffer(In MultithreadedWriteBuffer* parent, In void* userData);
								virtual CLOAK_CALL ~InternWriteBuffer();

								uint64_t CLOAK_CALL_THIS AddRef() override;
								uint64_t CLOAK_CALL_THIS Release() override;

								uint64_t CLOAK_CALL_THIS GetRefCount() override;
								API::Global::Debug::Error CLOAK_CALL_THIS Delete() override;
								Success(return == S_OK) Post_satisfies(*ppvObject != nullptr) HRESULT STDMETHODCALLTYPE QueryInterface(In REFIID riid, Outptr void** ppvObject) override;
								void CLOAK_CALL_THIS SetDebugName(In const std::string& s) override;
								void CLOAK_CALL_THIS SetSourceFile(In const std::string& s) override;
								std::string CLOAK_CALL_THIS GetDebugName() const override;
								std::string CLOAK_CALL_THIS GetSourceFile() const override;

								unsigned long long CLOAK_CALL_THIS GetPosition() const override;

								void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
								void CLOAK_CALL_THIS Save() override;

								void* CLOAK_CALL operator new(In size_t size);
								void CLOAK_CALL operator delete(In void* ptr);
							private:
								MultithreadedWriteBuffer* const m_parent;
								void* const m_userData;
								uint64_t m_bytePos;
								uint64_t m_writePos;
								uint64_t m_writeSize;
								uint64_t m_refCount;
								uint8_t* m_dst;
								Page* m_curPage;
								PageList* m_resList;
						};

						const std::function<void(In void* userData, In uint64_t size)> m_onWrite;
						const API::Global::Threading::Flag m_writeFlags;
						CE::RefPointer<API::Helper::ISyncSection> m_sync;
						CE::RefPointer<API::Files::IWriteBuffer> m_target;
						PageList* m_readyToWriteBegin;
						PageList* m_readyToWriteEnd;
						std::atomic<bool> m_hasTask;
						API::Global::Task m_task;
						API::Global::Task m_finish;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif