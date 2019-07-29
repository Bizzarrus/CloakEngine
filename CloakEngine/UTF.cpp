#include "stdafx.h"
#include "Engine/Compress/UTF.h"
#include "CloakEngine/Global/Memory.h"

#ifdef _DEBUG
#include <sstream>
#include <iomanip>
#endif

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace UTF {
				constexpr char32_t ALLOWED_UTF8_RANGES[] = { 0x000000,0x000080,0x000800,0x010000 };

				CLOAK_CALL UTFReadBuffer::UTFReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input)
				{
					DEBUG_NAME(UTFReadBuffer);
					m_read = input;
					m_state = RDY_U8_C;
					m_curChar = 0;
					m_charPos = 0;
				}
				CLOAK_CALL UTFReadBuffer::~UTFReadBuffer()
				{

				}
				unsigned long long CLOAK_CALL_THIS UTFReadBuffer::GetPosition() const
				{
					return m_read->GetPosition() - m_charPos;
				}
				uint8_t CLOAK_CALL_THIS UTFReadBuffer::ReadByte()
				{
					uint32_t buffer[4];
					size_t bufferSize = 0;
					if (m_charPos == 0)
					{
						if (m_read->HasNextByte() == false) { return 0; }
						m_curChar = 0;
						while (m_state != UNKNOWN)
						{
							const uint8_t b = m_read->ReadByte();
							Input type = Input::INVALID;
							if (b == 0) { type = Input::BOM_00; }
							else if (b == 0xFF) { type = Input::FF; }
							else if (b == 0xFE) { type = Input::FE; }
							else if (b == 0xEF) { type = Input::EF; }
							else if (b == 0xBB) { type = Input::BB; }
							else if (b == 0xBF) { type = Input::BF; }
							else if ((b & 0x80) == 0) { type = Input::ASCII; }
							else if ((b & 0xFC) == 0xD8) { type = Input::U16_HS; }
							else if ((b & 0xFC) == 0xDC) { type = Input::U16_LS; }
							else if ((b & 0xC0) == 0x80) { type = Input::U8_PART; }
							else if ((b & 0xE0) == 0xC0) { type = Input::U8_1; }
							else if ((b & 0xF0) == 0xE0) { type = Input::U8_2; }
							else if ((b & 0xF8) == 0xF0) { type = Input::U8_3; }

							State lUTF = m_state;
							if (type != Input::INVALID) { m_state = g_machine[m_state][static_cast<size_t>(type)]; }
							else { m_state = UNKNOWN; }

							if (bufferSize + 1 > 4) { m_state = UNKNOWN; }

							if (m_state != UNKNOWN)
							{
								buffer[bufferSize++] = b;
								bool rdy = false;
								if (m_state == RDY_U8)
								{
									CLOAK_ASSUME(bufferSize <= 4 && bufferSize >= 1);
									if (bufferSize == 1)
									{
										m_curChar = buffer[0] & 0x7F;
									}
									else if (bufferSize == 2)
									{
										m_curChar = ((buffer[0] & 0x1F) << 6) | (buffer[1] & 0x3F);
									}
									else if (bufferSize == 3)
									{
										m_curChar = ((buffer[0] & 0x0F) << 12) | ((buffer[1] & 0x3F) << 6) | (buffer[2] & 0x3F);
									}
									else if (bufferSize == 4)
									{
										m_curChar = ((buffer[0] & 0x07) << 18) | ((buffer[1] & 0x3F) << 12) | ((buffer[2] & 0x3F) << 6) | (buffer[3] & 0x3F);
									}

									//Check for non-shortest-form:
									if (m_curChar < ALLOWED_UTF8_RANGES[bufferSize - 1]) { m_state = UNKNOWN; }
									break;
								}
								else if (m_state == RDY_U16_BE)
								{
									CLOAK_ASSUME(bufferSize == 2 || bufferSize == 4);
									if (bufferSize == 2)
									{
										m_curChar = (buffer[0] << 8) | buffer[1];
									}
									else if (bufferSize == 4)
									{
										m_curChar = (((buffer[0] & 0x03) << 18) | (buffer[1] << 10) | ((buffer[2] & 0x03) << 8) | buffer[3]) + 0x10000;
									}
									break;
								}
								else if (m_state == RDY_U16_LE)
								{
									CLOAK_ASSUME(bufferSize == 2 || bufferSize == 4);
									if (bufferSize == 2)
									{
										m_curChar = (buffer[1] << 8) | buffer[0];
									}
									else if (bufferSize == 4)
									{
										m_curChar = (((buffer[1] & 0x03) << 18) | (buffer[0] << 10) | ((buffer[3] & 0x03) << 8) | buffer[2]) + 0x10000;
									}
									break;
								}
								else if (m_state == RDY_U32_BE)
								{
									CLOAK_ASSUME(bufferSize == 4);
									m_curChar = (buffer[0]) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
									break;
								}
								else if (m_state == RDY_U32_LE)
								{
									CLOAK_ASSUME(bufferSize == 4);
									m_curChar = (buffer[3]) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
									break;
								}
								else if (m_state == RDY_U8_S || m_state == RDY_U16_BE_S || m_state == RDY_U16_LE_S || m_state == RDY_U32_BE_S || m_state == RDY_U32_LE_S) { bufferSize = 0; }
							}
						}
						//Invalid Characters (Surrogate):
						if (m_curChar >= 0xD800 && m_curChar <= 0xDFFF) { m_state = UNKNOWN; }
#ifdef _DEBUG
						if (m_state == UNKNOWN)
						{
							if (bufferSize + 1 > 4)
							{
								CloakDebugLog("UTF Buffer overflow!");
							}
							else
							{
								std::stringstream s;
								s << "Invalid UTF-Char:";
								for (size_t a = 0; a < bufferSize; a++) { s << " 0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<uint32_t>(buffer[a]); }
								CloakDebugLog(s.str());
							}
						}
#endif
						if (m_state == UNKNOWN) { m_curChar = 0; }
						m_charPos = 4;
					}
					CLOAK_ASSUME(m_charPos > 0 && m_charPos <= 4);
					m_charPos--;
					return static_cast<uint8_t>((m_curChar >> (m_charPos * 8)) & 0xFF);
				}
				bool CLOAK_CALL_THIS UTFReadBuffer::HasNextByte()
				{
					return m_state != UNKNOWN && (m_charPos > 0 || m_read->HasNextByte());
				}
				void* CLOAK_CALL UTFReadBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL UTFReadBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS UTFReadBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IReadBuffer)) { *ptr = (API::Files::Buffer_v1::IReadBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ptr = (API::Files::Buffer_v1::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
			}
		}
	}
}