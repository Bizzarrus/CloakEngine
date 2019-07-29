#include "stdafx.h"
#include "Engine/Compress/AAC.h"

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace AAC {
				constexpr uint64_t STATE_SIZE = 32;
				constexpr uint64_t MAX_RANGE = 1ULL << STATE_SIZE;
				constexpr uint64_t MIN_RANGE = (MAX_RANGE >> 2) + 2;
				constexpr uint64_t MAX_TOTAL = min(MIN_RANGE, static_cast<uint64_t>(-1) / MAX_RANGE);
				constexpr uint64_t RANGE_MASK = MAX_RANGE - 1;
				constexpr uint64_t BIT1_MASK = MAX_RANGE >> 1;
				constexpr uint64_t BIT2_MASK = MAX_RANGE >> 2;

				CLOAK_CALL AACWriteBuffer::AACWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& output, In API::Files::Buffer_v1::CompressType type)
				{
					DEBUG_NAME(AACWriteBuffer);
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_output));
					m_output->SetTarget(output, type);
					for (size_t a = 0; a < ARRAYSIZE(m_histogram); a++) { m_histogram[a] = 1; }
					m_low = 0;
					m_high = RANGE_MASK;
					m_histoTotal = ARRAYSIZE(m_histogram);
					m_underflow = 0;
				}
				CLOAK_CALL AACWriteBuffer::~AACWriteBuffer()
				{
					
				}
				unsigned long long CLOAK_CALL_THIS AACWriteBuffer::GetPosition() const
				{
					return m_output->GetPosition() + ((m_output->GetBitPosition() + 7) / 8);
				}
				void CLOAK_CALL_THIS AACWriteBuffer::WriteByte(In uint8_t b)
				{
					CLOAK_ASSUME(m_low < m_high);
					CLOAK_ASSUME((m_low & RANGE_MASK) == m_low);
					CLOAK_ASSUME((m_high & RANGE_MASK) == m_high);
					
					const uint64_t range = 1 + m_high - m_low;
					CLOAK_ASSUME(range >= MIN_RANGE);
					CLOAK_ASSUME(range <= MAX_RANGE);
					uint64_t symLow = 0;
					for (uint8_t a = 0; a < b; a++) { symLow += m_histogram[a]; }
					const uint64_t symHigh = m_histogram[b] + symLow;
					CLOAK_ASSUME(symLow < symHigh);
					CLOAK_ASSUME(m_histoTotal <= MAX_TOTAL);

					uint64_t nLow = m_low + ((symLow * range) / m_histoTotal);
					uint64_t nHigh = m_low + ((symHigh * range) / m_histoTotal) - 1;
					m_low = nLow;
					m_high = nHigh;

					//First bits are equal
					while (((m_low ^ m_high) & BIT1_MASK) == 0)
					{
						const bool bit = (m_low >> (STATE_SIZE - 1));
						m_output->WriteBits(1, bit ? 1 : 0);
						for (; m_underflow > 0; m_underflow--) { m_output->WriteBits(1, bit ? 0 : 1); }

						m_low = (m_low << 1) & RANGE_MASK;
						m_high = ((m_high << 1) & RANGE_MASK) | 0x1;
					}

					while ((m_low & m_high & BIT2_MASK) != 0)
					{
						m_underflow++;

						m_low = (m_low << 1) & (RANGE_MASK >> 1);
						m_high = ((m_high << 1) & (RANGE_MASK >> 1)) | BIT1_MASK | 1;
					}

					m_histogram[b]++;
					m_histoTotal++;
					if (m_histoTotal > MAX_TOTAL)
					{
						m_output->WriteBits(1, 1);
						for (size_t a = 0; a < ARRAYSIZE(m_histogram); a++) { m_histogram[a] = 1; }
						m_low = 0;
						m_high = RANGE_MASK;
						m_histoTotal = ARRAYSIZE(m_histogram);
						m_underflow = 0;
					}
				}
				void CLOAK_CALL_THIS AACWriteBuffer::Save()
				{
					if (m_histoTotal > ARRAYSIZE(m_histogram)) { m_output->WriteBits(1, 1); }
				}
				void* CLOAK_CALL AACWriteBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL AACWriteBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS AACWriteBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::IWriteBuffer)) { *ptr = (API::Files::IWriteBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::IBuffer)) { *ptr = (API::Files::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}

				CLOAK_CALL AACReadBuffer::AACReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input, In API::Files::Buffer_v1::CompressType type)
				{
					DEBUG_NAME(AACReadBuffer);
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_input));
					m_input->SetTarget(input, type);
					for (size_t a = 0; a < ARRAYSIZE(m_histogram); a++) { m_histogram[a] = 1; }
					m_low = 0;
					m_high = RANGE_MASK;
					m_histoTotal = ARRAYSIZE(m_histogram);
					m_code = 0;
					for (size_t a = 0; a < STATE_SIZE; a++) { m_code = (m_code << 1) | m_input->ReadBits(1); }
				}
				CLOAK_CALL AACReadBuffer::~AACReadBuffer()
				{
					
				}
				unsigned long long CLOAK_CALL_THIS AACReadBuffer::GetPosition() const
				{
					return m_input->GetPosition() + ((m_input->GetBitPosition() + 7) / 8);
				}
				uint8_t CLOAK_CALL_THIS AACReadBuffer::ReadByte()
				{
					CLOAK_ASSUME(m_histoTotal <= MAX_TOTAL);

					const uint64_t range = 1 + m_high - m_low;
					CLOAK_ASSUME(range >= MIN_RANGE);
					CLOAK_ASSUME(range <= MAX_RANGE);

					const uint64_t offset = m_code - m_low;
					const uint64_t value = (((offset + 1)*m_histoTotal) - 1) / range;
					CLOAK_ASSUME((value * range) / m_histoTotal <= offset);
					CLOAK_ASSUME(value < m_histoTotal);

					size_t result = 0;
					uint64_t symHigh = 0;
					for (; result < ARRAYSIZE(m_histogram) && symHigh <= value; result++) { symHigh += m_histogram[result]; }
					const uint64_t symLow = symHigh - m_histogram[result];
					CLOAK_ASSUME(symLow < symHigh);

					uint64_t nLow = m_low + ((symLow * range) / m_histoTotal);
					uint64_t nHigh = m_low + ((symHigh * range) / m_histoTotal) - 1;
					m_low = nLow;
					m_high = nHigh;

					//First bits are equal
					while (((m_low ^ m_high) & BIT1_MASK) == 0)
					{
						m_code = ((m_code << 1) & RANGE_MASK) | m_input->ReadBits(1);

						m_low = (m_low << 1) & RANGE_MASK;
						m_high = ((m_high << 1) & RANGE_MASK) | 0x1;
					}

					while ((m_low & m_high & BIT2_MASK) != 0)
					{
						m_code = (m_code & BIT1_MASK) | ((m_code << 1) & (RANGE_MASK >> 1)) | m_input->ReadBits(1);

						m_low = (m_low << 1) & (RANGE_MASK >> 1);
						m_high = ((m_high << 1) & (RANGE_MASK >> 1)) | BIT1_MASK | 1;
					}

					m_histogram[result]++;
					m_histoTotal++;
					if (m_histoTotal > MAX_TOTAL)
					{
						for (size_t a = 0; a < ARRAYSIZE(m_histogram); a++) { m_histogram[a] = 1; }
						m_low = 0;
						m_high = RANGE_MASK;
						m_histoTotal = ARRAYSIZE(m_histogram);
						m_code = 0;
						for (size_t a = 0; a < STATE_SIZE; a++) { m_code = (m_code << 1) | m_input->ReadBits(1); }
					}
					return static_cast<uint8_t>(result);
				}
				bool CLOAK_CALL_THIS AACReadBuffer::HasNextByte()
				{
					return m_code > 0 || m_input->HasNextByte();
				}
				void* CLOAK_CALL AACReadBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL AACReadBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS AACReadBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IReadBuffer)) { *ptr = (API::Files::Buffer_v1::IReadBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ptr = (API::Files::Buffer_v1::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
			}
		}
	}
}