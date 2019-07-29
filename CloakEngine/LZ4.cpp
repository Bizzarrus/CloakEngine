#include "stdafx.h"
#include "Engine/Compress/LZ4.h"
#include "CloakEngine/Files/Buffer.h"

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace LZ4 {
				constexpr int COMPRESSION_SPEED = 1;

				CLOAK_CALL LZ4WriteBuffer::LZ4WriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type)
				{
					DEBUG_NAME(LZ4WriteBuffer);
					m_output = API::Files::CreateCompressWrite(output, type);
					m_stream = LZ4_createStream();
					m_count = 0;
					m_start = 0;
					m_writeCount = 0;
				}
				CLOAK_CALL LZ4WriteBuffer::~LZ4WriteBuffer()
				{
					LZ4_freeStream(m_stream);
				}
				unsigned long long CLOAK_CALL_THIS LZ4WriteBuffer::GetPosition() const
				{
					return m_writeCount;
				}
				void CLOAK_CALL_THIS LZ4WriteBuffer::WriteByte(In uint8_t b)
				{
					m_writeCount++;
					CLOAK_ASSUME(m_count < MAX_BLOCK_SIZE);
					CLOAK_ASSUME(m_start + MAX_BLOCK_SIZE <= BUFFER_SIZE);
					m_buffer[m_start + (m_count++)] = b;
					if (m_count == MAX_BLOCK_SIZE) { iWrite(false); }
				}
				void CLOAK_CALL_THIS LZ4WriteBuffer::Save()
				{
					if (m_count > 0) { iWrite(true); }
					m_output->Save();
				}
				void* CLOAK_CALL LZ4WriteBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
				void CLOAK_CALL LZ4WriteBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS LZ4WriteBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IWriteBuffer)) { *ptr = (API::Files::Buffer_v1::IWriteBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ptr = (API::Files::Buffer_v1::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS LZ4WriteBuffer::iWrite(In bool finalWrite)
				{
					const int nb = LZ4_compress_fast_continue(m_stream, reinterpret_cast<const char*>(&m_buffer[m_start]), reinterpret_cast<char*>(m_compBuffer), static_cast<int>(m_count), static_cast<int>(COMPRESS_BUFFER_SIZE), COMPRESSION_SPEED);
					CLOAK_ASSUME(nb != 0);
					m_output->WriteByte(static_cast<uint8_t>((nb >> 24) & 0xFF));
					m_output->WriteByte(static_cast<uint8_t>((nb >> 16) & 0xFF));
					m_output->WriteByte(static_cast<uint8_t>((nb >> 8) & 0xFF));
					m_output->WriteByte(static_cast<uint8_t>((nb >> 0) & 0xFF));
					for (int a = 0; a < nb; a++) { m_output->WriteByte(m_compBuffer[a]); }
					m_start += m_count;
					if (m_start + MAX_BLOCK_SIZE > BUFFER_SIZE) { m_start = 0; }
					m_count = 0;
					if (finalWrite == true) { LZ4_resetStream(m_stream); }
				}

				CLOAK_CALL LZ4ReadBuffer::LZ4ReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type)
				{
					DEBUG_NAME(LZ4ReadBuffer);
					m_input = API::Files::CreateCompressRead(input, type);
					m_stream = LZ4_createStreamDecode();
					m_start = 0;
					m_count = 0;
					m_readCount = 0;
					iRead();
				}
				CLOAK_CALL LZ4ReadBuffer::~LZ4ReadBuffer()
				{
					LZ4_freeStreamDecode(m_stream);
				}
				unsigned long long CLOAK_CALL_THIS LZ4ReadBuffer::GetPosition() const
				{
					return m_readCount;
				}
				uint8_t CLOAK_CALL_THIS LZ4ReadBuffer::ReadByte()
				{
					m_readCount++;
					if (m_count > 0)
					{
						uint8_t r = m_buffer[m_start++];
						if (--m_count == 0) { iRead(); }
						return r;
					}
					return 0;
				}
				bool CLOAK_CALL_THIS LZ4ReadBuffer::HasNextByte() { return m_count > 0; }
				void* CLOAK_CALL LZ4ReadBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
				void CLOAK_CALL LZ4ReadBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS LZ4ReadBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IReadBuffer)) { *ptr = (API::Files::Buffer_v1::IReadBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ptr = (API::Files::Buffer_v1::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL LZ4ReadBuffer::iRead()
				{
					if (m_input->HasNextByte() == false) { return; }
					CLOAK_ASSUME(m_count == 0);
					if (m_start + MAX_BLOCK_SIZE > BUFFER_SIZE) { m_start = 0; }
					uint32_t count = 0;
					count |= m_input->ReadByte() << 24;
					count |= m_input->ReadByte() << 16;
					count |= m_input->ReadByte() << 8;
					count |= m_input->ReadByte() << 0;
					if (count > 0)
					{
						CLOAK_ASSUME(count <= INPUT_BUFFER_SIZE);
						for (uint32_t a = 0; a < count; a++) { m_inputBuffer[a] = m_input->ReadByte(); }
						CLOAK_ASSUME(m_start < BUFFER_SIZE);
						int nb = LZ4_decompress_safe_continue(m_stream, reinterpret_cast<const char*>(m_inputBuffer), reinterpret_cast<char*>(&m_buffer[m_start]), count, static_cast<uint32_t>(BUFFER_SIZE - m_start));
						CLOAK_ASSUME(m_start < BUFFER_SIZE);
						CLOAK_ASSUME(nb > 0);
						m_count = static_cast<size_t>(nb);
					}
				}
			}
		}
	}
}