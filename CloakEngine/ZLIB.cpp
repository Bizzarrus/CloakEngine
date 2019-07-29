#include "stdafx.h"
#include "Engine/Compress/ZLIB.h"

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace ZLIB {
				voidpf alloc_zlib(In voidpf op, In uInt items, In uInt size)
				{
					return API::Global::Memory::MemoryPool::Allocate(items*size);
				}
				void free_zlib(In voidpf op, In voidpf ptr)
				{
					API::Global::Memory::MemoryPool::Free(ptr);
				}

				CLOAK_CALL ZLIBWriteBuffer::ZLIBWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type)
				{
					m_bufferPos = 0;
					m_writeCount = 0;
					m_init = false;

					m_output = API::Files::CreateCompressWrite(output, type);
				}
				CLOAK_CALL ZLIBWriteBuffer::~ZLIBWriteBuffer()
				{
					Save();
				}
				unsigned long long CLOAK_CALL_THIS ZLIBWriteBuffer::GetPosition() const
				{
					return m_writeCount;
				}
				void CLOAK_CALL_THIS ZLIBWriteBuffer::WriteByte(In uint8_t b)
				{
					m_writeCount++;
					m_inBuffer[m_bufferPos++] = b;
					if (m_bufferPos == BUFFER_IN_SIZE) { iWrite(false); }
				}
				void CLOAK_CALL_THIS ZLIBWriteBuffer::Save()
				{
					iWrite(true);
					m_output->Save();
				}
				void* CLOAK_CALL ZLIBWriteBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
				void CLOAK_CALL ZLIBWriteBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS ZLIBWriteBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IWriteBuffer)) { *ptr = (API::Files::Buffer_v1::IWriteBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ptr = (API::Files::Buffer_v1::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS ZLIBWriteBuffer::iWrite(In bool finish)
				{
					if (m_init == false)
					{
						if (m_bufferPos == 0) { return; }
						m_init = true;
						memset(&m_stream, 0, sizeof(z_stream));

						m_stream.zalloc = alloc_zlib;
						m_stream.zfree = free_zlib;
						m_stream.opaque = nullptr;
						deflateInit(&m_stream, Z_DEFAULT_COMPRESSION);
					}
					int ret = Z_OK;
					const size_t inSize = min(m_bufferPos, BUFFER_IN_SIZE);
					m_stream.next_in = m_inBuffer;
					m_stream.avail_in = static_cast<uInt>(inSize);
					do {
						m_stream.next_out = m_outBuffer;
						m_stream.avail_out = static_cast<uInt>(BUFFER_OUT_SIZE);
						ret = deflate(&m_stream, finish == true ? Z_FINISH : Z_NO_FLUSH);
						const size_t outSize = BUFFER_OUT_SIZE - m_stream.avail_out;
						for (size_t a = 0; a < outSize; a++) { m_output->WriteByte(m_outBuffer[a]); }
					} while (m_stream.avail_out == 0 && (ret == Z_OK || (ret == Z_BUF_ERROR && finish == true)));
					CLOAK_ASSUME(ret != Z_STREAM_ERROR);
					CLOAK_ASSUME(m_stream.avail_in <= inSize);
					if (finish == false)
					{
						for (size_t a = inSize - m_stream.avail_in, b = 0; a < inSize; a++, b++) { m_inBuffer[b] = m_inBuffer[a]; }
						m_bufferPos = m_stream.avail_in;
					}
					else 
					{ 
						m_bufferPos = 0;
						deflateEnd(&m_stream); 
						m_init = false;
					}
				}

				CLOAK_CALL ZLIBReadBuffer::ZLIBReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type)
				{
					m_bufferPos = 0;
					m_bufferSize = 0;
					m_readCount = 0;
					m_inPos = 0;
					memset(&m_stream, 0, sizeof(z_stream));

					m_stream.zalloc = alloc_zlib;
					m_stream.zfree = free_zlib;
					m_stream.opaque = nullptr;
					inflateInit(&m_stream);

					m_input = API::Files::CreateCompressRead(input, type);
					iRead();
				}
				CLOAK_CALL ZLIBReadBuffer::~ZLIBReadBuffer()
				{
					inflateEnd(&m_stream);
				}
				unsigned long long CLOAK_CALL_THIS ZLIBReadBuffer::GetPosition() const
				{
					return m_readCount;
				}
				uint8_t CLOAK_CALL_THIS ZLIBReadBuffer::ReadByte()
				{
					m_readCount++;
					if (m_bufferPos < m_bufferSize)
					{
						const uint8_t r = m_outBuffer[m_bufferPos++];
						if (m_bufferPos == m_bufferSize) { iRead(); }
						return r;
					}
					return 0;
				}
				bool CLOAK_CALL_THIS ZLIBReadBuffer::HasNextByte()
				{
					return m_bufferPos < m_bufferSize;
				}
				void* CLOAK_CALL ZLIBReadBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
				void CLOAK_CALL ZLIBReadBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS ZLIBReadBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IReadBuffer)) { *ptr = (API::Files::Buffer_v1::IReadBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ptr = (API::Files::Buffer_v1::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS ZLIBReadBuffer::iRead()
				{
					size_t inCount = m_inPos;
					for (; inCount < BUFFER_IN_SIZE && m_input->HasNextByte(); inCount++) { m_inBuffer[inCount] = m_input->ReadByte(); }
					if (inCount > 0)
					{
						m_stream.avail_in = static_cast<uInt>(inCount);
						m_stream.next_in = m_inBuffer;
						m_stream.avail_out = static_cast<uInt>(BUFFER_OUT_SIZE);
						m_stream.next_out = m_outBuffer;
						const int ret = inflate(&m_stream, Z_NO_FLUSH);
						CLOAK_ASSUME(ret != Z_STREAM_ERROR && ret != Z_MEM_ERROR);
						if (ret == Z_DATA_ERROR) 
						{ 
							API::Global::Log::WriteToLog(static_cast<std::string>("ZLib read error: ") + m_stream.msg, API::Global::Log::Type::Warning); 
							m_bufferSize = 0;
						}
						else
						{
							m_bufferSize = BUFFER_OUT_SIZE - m_stream.avail_out;
							m_inPos = m_stream.avail_in;
							if (inCount > 0)
							{
								CLOAK_ASSUME(m_inPos < inCount);
								for (size_t a = inCount - m_inPos, b = 0; a < inCount; a++, b++) { m_inBuffer[b] = m_inBuffer[a]; }
							}
						}
					}
					m_bufferPos = 0;
				}
			}
		}
	}
}