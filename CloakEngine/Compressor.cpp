#include "stdafx.h"
#include "Implementation/Files/Compressor.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Global/Log.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/StringConvert.h"
#include "CloakEngine/Helper/SyncSection.h"

#include "Engine/Compress/Huffman.h"
#include "Engine/Compress/LZC.h"
#include "CloakEngine/Global/Memory.h"

#include <limits>
#include <atomic>
#include <math.h>
#include <codecvt>
#include <algorithm>
#include <random>
#include <assert.h>

#define WAIT_TIME 128
#define MAX_FILE_BUFFER (128 KB)

//#define DBG_OUTPUT

#if defined(DBG_OUTPUT) && defined(_DEBUG)
#define DBG_WRITE(bits, data) CloakDebugLog("Write "+std::to_string(bits)+": "+std::to_string(data))
#define DBG_READ(bits, data) CloakDebugLog("Read "+std::to_string(bits)+": "+std::to_string(data))
#define DBG_WBUFS() CloakDebugLog("---START WRITE BUFFER---");
#define DBG_WBUFE() CloakDebugLog("---END WRITE BUFFER---");
#else
#define DBG_WRITE(bits,data)
#define DBG_READ(bits,data)
#define DBG_WBUFS()
#define DBG_WBUFE()
#endif

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace Compressor_v2 {
				constexpr unsigned int VERSION = 2000;
				API::Files::UGID g_defaultGameID;
				std::atomic<bool> g_hasDefaultGameID = false;
				API::Helper::ISyncSection* g_syncDefaultGameID = nullptr;

				template<typename T> inline T AlignLength(T v) { return (v + 0x7) & ~static_cast<T>(0x7); }
				template<typename T> inline bool IsAlignedLength(T v) { return (v & 0x7) == 0; }

				CLOAK_CALL_THIS Writer::Writer()
				{
					DEBUG_NAME(Writer);
					m_output = nullptr;
					m_data = 0;
					m_bitPos = 0;
				}
				CLOAK_CALL_THIS Writer::~Writer()
				{
					Save();
				}
				unsigned long long CLOAK_CALL_THIS Writer::GetPosition() const
				{
					if (m_output != nullptr) { return m_output->GetPosition(); }
					return 0;
				}
				unsigned int CLOAK_CALL_THIS Writer::GetBitPosition() const
				{
					if (m_output != nullptr) { return m_bitPos; }
					return 0;
				}
				void CLOAK_CALL_THIS Writer::Move(In unsigned long long byte, In_opt unsigned int bit)
				{
					for (unsigned long long a = 0; a < byte; a++) { WriteBits(8, 0); }
					WriteBits(bit, 0);
				}
				void CLOAK_CALL_THIS Writer::MoveTo(In unsigned long long byte, In_opt unsigned int bit)
				{
					const unsigned long long cBy = GetPosition();
					const unsigned int cBi = GetBitPosition();
					assert(byte > cBy || (byte == cBy && bit >= cBi));
					if (byte > cBy || (byte == cBy && bit > cBi))
					{
						const unsigned int mBi = bit + 8 - cBi;
						const unsigned long long mBy = (mBi >> 3) + byte - (cBy + 1);
						Move(mBy, mBi & 0x07);
					}
				}
				void CLOAK_CALL_THIS Writer::MoveToNextByte()
				{
					if (m_output != nullptr)
					{
						m_bitPos = (m_bitPos + 7) & ~7Ui8;
						checkPos();
					}
				}
				void CLOAK_CALL_THIS Writer::SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& buffer, In_opt API::Files::Buffer_v1::CompressType Type, In_opt bool txt)
				{
					Save();
					m_output = buffer;
					if (m_output != nullptr)
					{
						m_bitPos = 0;
						m_compress = Type;
						m_output = API::Files::Buffer_v1::CreateCompressWrite(m_output, Type);
					}
				}
				void CLOAK_CALL_THIS Writer::SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& buffer, In const API::Files::Compressor_v2::FileType& type, In API::Files::Buffer_v1::CompressType Type, In_opt bool txt)
				{
					if (g_hasDefaultGameID == false || txt == true) 
					{ 
						SetTarget(buffer, type, Type, nullptr, txt); 
					}
					else 
					{
						API::Helper::Lock lock(g_syncDefaultGameID);
						SetTarget(buffer, type, Type, &g_defaultGameID, txt);
					}
				}
				void CLOAK_CALL_THIS Writer::SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& buffer, In const API::Files::Compressor_v2::FileType& type, In API::Files::Buffer_v1::CompressType Type, In const API::Files::Buffer_v1::UGID& cgid, In_opt bool txt)
				{
					SetTarget(buffer, type, Type, &cgid, txt);
				}
				void CLOAK_CALL_THIS Writer::SetTarget(In const API::Files::Buffer_v1::UFI& ufi, In_opt API::Files::Buffer_v1::CompressType Type, In_opt bool txt)
				{
					SetTarget(ufi.OpenWriter(false), Type, txt);
				}
				void CLOAK_CALL_THIS Writer::SetTarget(In const API::Files::Buffer_v1::UFI& ufi, In const API::Files::Compressor_v2::FileType& type, In API::Files::Buffer_v1::CompressType Type, In_opt bool txt)
				{
					if (g_hasDefaultGameID == false || txt == true)
					{ 
						SetTarget(ufi.OpenWriter(false, type.Type), type, Type, nullptr, txt);
					}
					else
					{
						API::Helper::Lock lock(g_syncDefaultGameID);
						SetTarget(ufi.OpenWriter(false, type.Type), type, Type, &g_defaultGameID, txt);
					}
				}
				void CLOAK_CALL_THIS Writer::SetTarget(In const API::Files::Buffer_v1::UFI& ufi, In const API::Files::Compressor_v2::FileType& type, In API::Files::Buffer_v1::CompressType Type, In const API::Files::Buffer_v1::UGID& cgid, In_opt bool txt)
				{
					SetTarget(ufi.OpenWriter(false, type.Type), type, Type, &cgid, txt);
				}
				void CLOAK_CALL_THIS Writer::WriteByte(In uint8_t b) { WriteBits(8, b); }
				void CLOAK_CALL_THIS Writer::WriteBits(In size_t len, In uint64_t data)
				{
					if (m_output != nullptr)
					{
						if (len > 0)
						{
							DBG_WRITE(len, data);
							checkPos();
							data = data & ((1ULL << len) - 1);
							size_t space = 8 - m_bitPos;
							long long Move = ((long long)space) - len;
							do {
								uint64_t d;
								if (Move == 0) { d = data; }
								else if (Move < 0) { d = data >> (-Move); }
								else { d = data << Move; }
								const uint8_t b = static_cast<uint8_t>(d & 0xff);
								m_data |= b;
								size_t delta = min(space, len);
								len -= delta;
								m_bitPos += static_cast<uint8_t>(delta);
								checkPos();
								space = 8;
								Move += 8;
							} while (len > 0);
						}
					}
				}
				void CLOAK_CALL_THIS Writer::WriteBytes(In const API::List<uint8_t>& data, In_opt size_t len, In_opt size_t offset)
				{
					if (len == 0) { len = data.size() - offset; }
					for (size_t a = offset; a < (len + offset); a++)
					{
						if (a < data.size()) { WriteBits(8, data[a]); }
						else { WriteBits(8, 0); }
					}
				}
				void CLOAK_CALL_THIS Writer::WriteBytes(In_reads(dataSize) const uint8_t* data, In size_t dataSize, In_opt size_t len, In_opt size_t offset)
				{
					if (len == 0) { len = dataSize - offset; }
					for (size_t a = offset; a < (len + offset); a++)
					{
						if (a < dataSize) { WriteBits(8, data[a]); }
						else { WriteBits(8, 0); }
					}
				}
				void CLOAK_CALL_THIS Writer::WriteDouble(In size_t len, In long double data, In_opt size_t expLen)
				{
					if (CloakDebugCheckOK(len >= expLen, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						if (data == 0) { WriteBits(len, 0); }
						else
						{
							if (expLen == 0)
							{
								if (len < 16) { expLen = len >> 1; }
								else if (len == 16) { expLen = 5; }
								else if (len < 64) { expLen = 8; }
								else if (len < 128) { expLen = 11; }
								else if (len < 256) { expLen = 15; }
								else { expLen = 19; }
							}
							unsigned int sigBit = static_cast<unsigned int>(len - (expLen + 1));
							unsigned long long sig = std::copysign(1.0f, data) < 0 ? (1ULL << (len - 1)) : 0;
							if (data != data)
							{
								unsigned long long e = (1ULL << expLen) - 1;
								unsigned long long s = (1ULL << (len - (expLen + 1))) - 1;
								WriteBits(len, sig | (e << sigBit) | s);
							}
							else if (std::abs(data) == std::numeric_limits<long double>::infinity())
							{
								unsigned long long e = (1ULL << expLen) - 1;
								unsigned long long s = 0;
								WriteBits(len, sig | (e << sigBit) | s);
							}
							else
							{
								int ex;
								long double m = std::frexp(std::abs(data), &ex) * 2;
								long long e = (ex - 1) + ((1ULL << (expLen - 1)) - 1);
								if (e <= 0)
								{
									m /= (1ULL << (-e));
									e = 0;
									unsigned long long s = static_cast<unsigned long long>((1 << (len - (expLen + 1)))*m);
									WriteBits(len, sig | (e << sigBit) | s);
								}
								else if (e >= (1LL << expLen) - 1)
								{
									e = (1ULL << expLen) - 1;
									unsigned long long s = 0;
									WriteBits(len, sig | (e << sigBit) | s);
								}
								else
								{
									unsigned long long s = static_cast<unsigned long long>((1 << (len - (expLen + 1)))*(m - 1));
									WriteBits(len, sig | (e << sigBit) | s);
								}
							}
							
						}
					}
				}
				void CLOAK_CALL_THIS Writer::WriteString(In const std::string& str, In_opt size_t len, In_opt size_t offset)
				{
					WriteBytes(reinterpret_cast<const uint8_t*>(str.c_str()), str.length(), len, offset);
				}
				void CLOAK_CALL_THIS Writer::WriteString(In const std::u16string& str, In_opt size_t len, In_opt size_t offset)
				{
					std::string s = API::Helper::StringConvert::ConvertToU8(str);
					const size_t l16 = str.length();
					const size_t l8 = s.length();
					if (l8 > l16)
					{
						WriteBits(1, 1);
						size_t l = l8 - l16;
						while (l >= 16) { WriteBits(1, 1); l -= 16; }
						WriteBits(1, 0);
						WriteBits(4, l);
					}
					else { WriteBits(1, 0); }
					WriteString(s, len, offset);
					if (l8 < l16)
					{
						for (size_t l = l16 - l8; l > 0; l--) { WriteBits(8, 0); }
					}
				}
				void CLOAK_CALL_THIS Writer::WriteStringWithLength(In const std::string& str, In_opt size_t lenSize, In_opt size_t len, In_opt size_t offset)
				{
					const std::string subS = len > 0 || offset > 0 ? str.substr(offset, len > 0 ? len : str.npos) : str;
					size_t l = subS.length();
					const size_t ls = static_cast<size_t>(1) << lenSize;
					while (l >= ls) { WriteBits(1, 1); l -= ls; }
					WriteBits(1, 0);
					WriteBits(lenSize, l);
					WriteString(subS, subS.length());
				}
				void CLOAK_CALL_THIS Writer::WriteStringWithLength(In const std::u16string& str, In_opt size_t lenSize, In_opt size_t len, In_opt size_t offset)
				{
					const std::u16string subS = len > 0 || offset > 0 ? str.substr(offset, len > 0 ? len : str.npos) : str;
					const std::string s = API::Helper::StringConvert::ConvertToU8(subS);
					WriteStringWithLength(s, lenSize);
				}
				void CLOAK_CALL_THIS Writer::WriteGameID(In const API::Files::Buffer_v1::UGID& cgid)
				{
					for (size_t a = 0; a < API::Files::Buffer_v1::UGID::ID_SIZE; a++) { WriteBits(8, cgid.ID[a]); }
				}
				void CLOAK_CALL_THIS Writer::WriteBuffer(In API::Files::Buffer_v1::IReadBuffer* buffer, In_opt unsigned long long byte, In_opt unsigned int bit)
				{
					if (CloakDebugCheckOK(buffer != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						const unsigned long long mb = byte + ((bit + 7) / 8);
						byte += bit / 8;
						bit %= 8;
						unsigned long long p = 0;
						DBG_WBUFS();
						while ((mb == 0 && buffer->HasNextByte()) || p < mb)
						{
							uint8_t b = buffer->ReadByte();
							if (mb == 0 || p < byte) { WriteBits(8, b); }
							else { WriteBits(bit, b >> (8 - bit)); }
							p++;
						}
						DBG_WBUFE();
					}
				}
				void CLOAK_CALL_THIS Writer::WriteBuffer(In API::Files::Buffer_v1::IVirtualWriteBuffer* buffer, In_opt unsigned long long byte, In_opt unsigned int bit)
				{
					API::Files::Buffer_v1::IReadBuffer* read = buffer->GetReadBuffer();
					WriteBuffer(read, byte, bit);
					SAVE_RELEASE(read);
				}
				void CLOAK_CALL_THIS Writer::WriteDynamic(In uint64_t data)
				{
					size_t l = 0;
					uint64_t d = data;
					do {
						l++;
						d >>= 8;
					} while (d != 0);
					WriteBits(3, l - 1);
					do {
						WriteBits(8, data & 0xFF);
						data >>= 8;
					} while (data != 0);
				}
				void CLOAK_CALL_THIS Writer::Save()
				{
					if (m_output != nullptr)
					{
						MoveToNextByte();
						checkPos();
						if (m_bitPos > 0) { m_output->WriteByte(m_data); }
						m_output->Save();
						m_bitPos = 0;
					}
				}
				void* CLOAK_CALL Writer::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL Writer::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS Writer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Compressor_v2::ICompressor)) { *ptr = (API::Files::Compressor_v2::ICompressor*)this; return true; }
					else if (riid == __uuidof(API::Files::Compressor_v2::IWriter)) { *ptr = (API::Files::Compressor_v2::IWriter*)this; return true; }
					else if (riid == __uuidof(Writer)) { *ptr = (Writer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS Writer::SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& buffer, In const API::Files::Compressor_v2::FileType& fileT, In API::Files::Buffer_v1::CompressType Type, In_opt const API::Files::Buffer_v1::UGID* cgid, In bool txt)
				{
					Save();
					m_output = buffer;
					if (m_output != nullptr)
					{
						std::string ft = fileT.Type;
						std::transform(ft.begin(), ft.end(), ft.begin(), toupper);
						m_bitPos = 0;
						m_compress = Type;
						if (txt == false)
						{
							WriteBits(17, VERSION);
							switch (Type)
							{
								case CloakEngine::API::Files::Buffer_v1::CompressType::NONE:
									WriteBits(6, 0);
									break;
								case CloakEngine::API::Files::Buffer_v1::CompressType::HUFFMAN:
									WriteBits(6, 1);
									break;
								case CloakEngine::API::Files::Buffer_v1::CompressType::LZC:
									WriteBits(6, 2);
									break;
								case CloakEngine::API::Files::Buffer_v1::CompressType::LZW:
									WriteBits(6, 3);
									break;
								case CloakEngine::API::Files::Buffer_v1::CompressType::LZH:
									WriteBits(6, 4);
									break;
								case CloakEngine::API::Files::Buffer_v1::CompressType::ARITHMETIC:
									WriteBits(6, 5);
									break;
								case CloakEngine::API::Files::Buffer_v1::CompressType::LZ4:
									WriteBits(6, 6);
									break;
								case CloakEngine::API::Files::Buffer_v1::CompressType::ZLIB:
									WriteBits(6, 7);
									break;
								default:
									API::Global::Log::WriteToLog("Unkown compressor type!", API::Global::Log::Type::Warning);
									Type = API::Files::Buffer_v1::CompressType::NONE;
									WriteBits(6, 0);
									break;
							}
							WriteBits(6, ft.length());
							MoveToNextByte();//Padding: unused bits

							WriteBits(16, 0x0D0A); //writing line break
							WriteString(ft);
							WriteBits(16, 0x0D0A); //writing line break

							if (cgid != nullptr)
							{
								WriteBits(1, 1);
								Save();
							}
							else 
							{ 
								WriteBits(1, 0); 
								Save();
							}
							SetTarget(m_output, Type);
							if (cgid != nullptr) { WriteGameID(*cgid); }
							std::string k = fileT.Key;
							WriteBits(6, k.length());
							WriteBits(16, fileT.Version);
							WriteString(k);
							WriteBits(32, 0);//Additional information space(not used)
						}
					}
				}
				void CLOAK_CALL_THIS Writer::checkPos()
				{
					size_t tw = m_bitPos >> 3;
					m_bitPos &= 0x07;
					while (tw > 0)
					{
						m_output->WriteByte(m_data);
						m_data = 0;
						tw--;
					}
				}

				CLOAK_CALL_THIS Reader::Reader()
				{
					DEBUG_NAME(Reader);
					m_bitPos = 0;
					m_data = 0;
					m_input = nullptr;
					m_validSize = 0;
				}
				CLOAK_CALL_THIS Reader::~Reader()
				{
					
				}
				unsigned long long CLOAK_CALL_THIS Reader::GetPosition() const
				{
					if (m_input != nullptr) { return m_bytePos; }
					return 0;
				}
				unsigned int CLOAK_CALL_THIS Reader::GetBitPosition() const
				{
					if (m_input != nullptr) { return m_bitPos; }
					return 0;
				}
				void CLOAK_CALL_THIS Reader::Move(In unsigned long long byte, In_opt unsigned int bit)
				{
					for (size_t a = 0; a < byte; a++) { ReadBits(8); }
					ReadBits(bit);
				}
				void CLOAK_CALL_THIS Reader::MoveTo(In unsigned long long byte, In_opt unsigned int bit)
				{
					const unsigned long long cBy = GetPosition();
					const unsigned int cBi = GetBitPosition();
					assert(byte > cBy || (byte == cBy && bit >= cBi));
					if (byte > cBy || (byte == cBy && bit > cBi))
					{
						const unsigned int mBi = bit + 8 - cBi;
						const unsigned long long mBy = (mBi >> 3) + byte - (cBy + 1);
						Move(mBy, mBi & 0x07);
#ifdef _DEBUG
						const unsigned long long aBy = GetPosition();
						const unsigned int aBi = GetBitPosition();
						assert(aBy == byte);
						assert(aBi == bit);
#endif
					}
				}
				void CLOAK_CALL_THIS Reader::MoveToNextByte()
				{
					if (m_input != nullptr)
					{
						m_bitPos = (m_bitPos + 7) & ~7Ui8;
						checkPos();
					}
				}
				API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS Reader::SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& buffer, In_opt API::Files::Buffer_v1::CompressType Type, In_opt bool txt)
				{
					m_input = buffer;
					if (m_input != nullptr)
					{
						m_bitPos = 8;
						m_input = API::Files::Buffer_v1::CreateCompressRead(m_input, Type);
						if (txt == true) { m_input = API::Files::Buffer_v1::CreateUTFRead(m_input); }
						checkPos();
						return 1;
					}
					return 0;
				}
				API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS Reader::SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& buffer, In const API::Files::Compressor_v2::FileType& type, In_opt bool txt)
				{
					if (g_hasDefaultGameID == false || txt == true) { return SetTarget(buffer, type, true, nullptr, txt); }
					else
					{
						API::Helper::Lock lock(g_syncDefaultGameID);
						return SetTarget(buffer, type, true, &g_defaultGameID, txt);
					}
				}
				API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS Reader::SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& buffer, In const API::Files::Compressor_v2::FileType& type, In const API::Files::Buffer_v1::UGID& cgid, In_opt bool txt)
				{
					return SetTarget(buffer, type, true, &cgid, txt);
				}
				API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS Reader::SetTarget(In const API::Files::Buffer_v1::UFI& ufi, In_opt API::Files::Buffer_v1::CompressType Type, In_opt bool txt, In_opt bool silentTry)
				{
					return SetTarget(ufi.OpenReader(silentTry), Type, txt);
				}
				API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS Reader::SetTarget(In const API::Files::Buffer_v1::UFI& ufi, In const API::Files::Compressor_v2::FileType& type, In_opt bool txt, In_opt bool silentTry)
				{
					if (g_hasDefaultGameID == false || txt == true)
					{
						return SetTarget(ufi.OpenReader(silentTry, type.Type), type, !silentTry, nullptr, txt);
					}
					else
					{
						API::Helper::Lock lock(g_syncDefaultGameID);
						return SetTarget(ufi.OpenReader(silentTry, type.Type), type, !silentTry, &g_defaultGameID, txt);
					}
				}
				API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS Reader::SetTarget(In const API::Files::Buffer_v1::UFI& ufi, In const API::Files::Compressor_v2::FileType& type, In const API::Files::Buffer_v1::UGID& cgid, In_opt bool txt, In_opt bool silentTry)
				{
					return SetTarget(ufi.OpenReader(silentTry, type.Type), type, !silentTry, &cgid, txt);
				}
				uint64_t CLOAK_CALL_THIS Reader::ReadBits(In size_t len)
				{
					if (m_input != nullptr && len > 0)
					{
						uint64_t res = 0;
						size_t ro = 0;
						if (m_bitPos != 0)
						{
							checkPos();
							ro = min(static_cast<size_t>(8 - m_bitPos), len);
							const uint8_t dat = m_data >> (8 - (m_bitPos + ro));
							const uint8_t mask = (1 << ro) - 1;
							res |= static_cast<uint64_t>(dat & mask) << (len - ro);
							m_bitPos += static_cast<uint8_t>(ro);
						}
						for (size_t a = 8; a <= len - ro; a += 8)
						{
							checkPos();
							res |= static_cast<uint64_t>(m_data) << (len - (a + ro));
							m_bitPos += 8;
						}
						if (len > ro)
						{
							size_t bl = (len - ro) & 0x7;
							if (bl > 0)
							{
								checkPos();
								res |= (static_cast<uint64_t>(m_data)) >> (8 - bl);
								m_bitPos += static_cast<uint8_t>(bl);
							}
						}
						checkPos();
						DBG_READ(len, res);
						return res;
					}
					return 0;
				}
				void CLOAK_CALL_THIS Reader::ReadBytes(In size_t len, Inout API::List<uint8_t>& data)
				{
					data.clear();
					data.reserve(len);
					for (size_t a = 0; a < len; a++) { data.push_back(static_cast<uint8_t>(ReadBits(8))); }
				}
				void CLOAK_CALL_THIS Reader::ReadBytes(In size_t len, Out_writes(dataSize) uint8_t* data, In size_t dataSize)
				{
					if (CloakDebugCheckOK(data != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						for (size_t a = 0; a < len || a < dataSize; a++)
						{
							uint8_t r = 0;
							if (a < len) { r = static_cast<uint8_t>(ReadBits(8)); }
							if (a < dataSize) { data[a] = r; }
						}
					}
				}
				long double CLOAK_CALL_THIS Reader::ReadDouble(In size_t len, In_opt size_t expLen)
				{
					if (CloakDebugCheckOK(len >= expLen || expLen == 0, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						if (expLen == 0)
						{
							if (len < 16) { expLen = len / 2; }
							else if (len == 16) { expLen = 5; }
							else if (len < 64) { expLen = 8; }
							else if (len < 128) { expLen = 11; }
							else if (len < 256) { expLen = 15; }
							else { expLen = 19; }
						}
						uint64_t d = static_cast<uint64_t>(ReadBits(len));
						bool sign = (d & (1ULL << (len - 1))) != 0;
						unsigned int sigB = static_cast<unsigned int>(len - (expLen + 1));
						long double bias = static_cast<long double>(1ULL << (expLen - 1)) - 1;
						unsigned long long si = 1ULL << sigB;
						unsigned long long e = (d >> sigB) & ((1ULL << expLen) - 1);
						unsigned long long m = (d & (si - 1));
						if (e == 0 && m == 0) { return 0; }
						if (e == (1ULL << expLen) - 1)
						{
							if (m == 0) { return std::numeric_limits<long double>::infinity(); }
							return std::numeric_limits<long double>::quiet_NaN();
						}
						int shift = static_cast<int>(e - bias);
						long double x = (static_cast<long double>(m) / si) + (e == 0 ? 0 : 1);
						return std::ldexp(x, shift)*(sign ? -1 : 1);
					}
					return 0;
				}
				std::string CLOAK_CALL_THIS Reader::ReadString(In size_t len)
				{
					API::List<BYTE> data;
					ReadBytes(len, data);
					return std::string(data.begin(), data.end());
				}
				std::string CLOAK_CALL_THIS Reader::ReadStringWithLength(In_opt size_t lenSize)
				{
					size_t l = 0;
					const size_t ls = static_cast<size_t>(1) << lenSize;
					while (ReadBits(1) == 1) { l += ls; }
					l += static_cast<size_t>(ReadBits(lenSize));
					const std::string s = ReadString(l);
					return s;
				}
				bool CLOAK_CALL_THIS Reader::ReadGameID(In const API::Files::Buffer_v1::UGID& cgid)
				{
					if (m_input != nullptr)
					{
						for (size_t a = 0; a < 24; a++)
						{
							const uint8_t b = static_cast<uint8_t>(ReadBits(8));
							if (cgid.ID[a] != b) { return false; }
						}
						return true;
					}
					return false;
				}
				bool CLOAK_CALL_THIS Reader::ReadLine(Out std::string* line, In_opt char32_t delChar)
				{
					bool r = false;
					if (CloakDebugCheckOK(line != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						line->clear();
						while (m_input != nullptr && m_input->HasNextByte())
						{
							char32_t c = static_cast<char32_t>(ReadBits(32));
							if (c == U'\n' || c == U'\0' || c == delChar) { return true; }
							else if (static_cast<uint32_t>(c) >= 0x20) 
							{ 
								r = true;
								if ((c & 0x1F0000) != 0)
								{
									line->push_back(0xF0 | ((c >> 18) & 0x07));
									line->push_back(0x80 | ((c >> 12) & 0x3F));
									line->push_back(0x80 | ((c >> 6) & 0x3F));
									line->push_back(0x80 | ((c >> 0) & 0x3F));
								}
								else if ((c & 0x00F800) != 0)
								{
									line->push_back(0xE0 | ((c >> 12) & 0x0F));
									line->push_back(0x80 | ((c >> 6) & 0x3F));
									line->push_back(0x80 | ((c >> 0) & 0x3F));
								}
								else if ((c & 0x000780) != 0)
								{
									line->push_back(0xC0 | ((c >> 6) & 0x1F));
									line->push_back(0x80 | ((c >> 0) & 0x3F));
								}
								else
								{
									line->push_back(c & 0x7F);
								}
							}
						}
					}
					return r;
				}
				void CLOAK_CALL_THIS Reader::ReadBuffer(In API::Files::Buffer_v1::IWriteBuffer* buffer, In size_t byte, In_opt size_t bit)
				{
					if (CloakDebugCheckOK(buffer != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						byte += bit / 8;
						bit %= 8;
						for (size_t a = 0; a < byte; a++) { buffer->WriteByte(static_cast<uint8_t>(ReadBits(8))); }
						if (bit > 0) { buffer->WriteByte(static_cast<uint8_t>(ReadBits(bit))); }
					}
				}
				uint64_t CLOAK_CALL_THIS Reader::ReadDynamic()
				{
					const size_t l = static_cast<size_t>(ReadBits(3) + 1);
					uint64_t r = 0;
					for (size_t a = 0; a < l; a++) { r |= ReadBits(8) << (a * 8); }
					return r;
				}
				bool CLOAK_CALL_THIS Reader::IsAtEnd() const
				{
					if (m_input != nullptr) { return !m_input->HasNextByte(); }
					return true;
				}
				uint8_t CLOAK_CALL_THIS Reader::ReadByte() { return static_cast<uint8_t>(ReadBits(8)); }
				bool CLOAK_CALL_THIS Reader::HasNextByte() { return !IsAtEnd(); };
				
				Success(return == true) bool CLOAK_CALL_THIS Reader::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Compressor_v2::ICompressor)) { *ptr = (API::Files::Compressor_v2::ICompressor*)this; return true; }
					else if (riid == __uuidof(API::Files::Compressor_v2::IReader)) { *ptr = (API::Files::Compressor_v2::IReader*)this; return true; }
					else if (riid == __uuidof(Reader)) { *ptr = (Reader*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				API::Files::Compressor_v2::FileVersion CLOAK_CALL_THIS Reader::SetTarget(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& buffer, In const API::Files::Compressor_v2::FileType& fileT, In_opt bool forceExists, In_opt const API::Files::Buffer_v1::UGID* cgid, In_opt bool txt)
				{
					m_input = buffer;
					if (m_input != nullptr)
					{
						const bool hasSrc = m_input->HasNextByte();
						if (CloakCheckOK(forceExists == false || hasSrc, API::Global::Debug::Error::FREADER_NO_FILE, false) && hasSrc)
						{
							std::string ft = fileT.Type;
							std::transform(ft.begin(), ft.end(), ft.begin(), toupper);
							m_bitPos = 8;
							m_bytePos = 0;
							if (txt == false)
							{
								checkPos();
								const unsigned int v = static_cast<unsigned int>(ReadBits(17));
								if (CloakCheckError(v <= VERSION, API::Global::Debug::Error::FREADER_OLD_VERSION, false)) { return 0; }
								const unsigned int ct = static_cast<unsigned int>(ReadBits(6));
								const unsigned int tyL = static_cast<unsigned int>(ReadBits(6));
								MoveToNextByte();
								ReadBits(16);
								const std::string type = ReadString(tyL);
								if (CloakCheckError(type.compare(ft) == 0, API::Global::Debug::Error::FREADER_FAILED_FILETYPE, false)) { return 0; }
								ReadBits(16);

								API::Files::Buffer_v1::CompressType Type;
								switch (ct)
								{
									case 0:
										Type = API::Files::Buffer_v1::CompressType::NONE;
										break;
									case 1:
										Type = API::Files::Buffer_v1::CompressType::HUFFMAN;
										break;
									case 2:
										Type = API::Files::Buffer_v1::CompressType::LZC;
										break;
									case 3:
										Type = API::Files::Buffer_v1::CompressType::LZW;
										break;
									case 4:
										Type = API::Files::Buffer_v1::CompressType::LZH;
										break;
									case 5:
										Type = API::Files::Buffer_v1::CompressType::ARITHMETIC;
										break;
									case 6:
										Type = API::Files::Buffer_v1::CompressType::LZ4;
										break;
									case 7:
										Type = API::Files::Buffer_v1::CompressType::ZLIB;
										break;
									default:
										break;
								}

								bool trCGID = false;
								if (ReadBits(1) == 1)
								{
									if (CloakCheckError(cgid != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false)) { return 0; }
									trCGID = true;
								}
								SetTarget(m_input, Type);
								if (trCGID)
								{
									if (CloakCheckError(ReadGameID(*cgid), API::Global::Debug::Error::FREADER_FAILED_CRYPTID, false)) { return 0; }
								}
								const unsigned int fkl = static_cast<unsigned int>(ReadBits(6));
								API::Files::Compressor_v2::FileVersion res = (API::Files::Compressor_v2::FileVersion)ReadBits(16);
								const std::string fk = ReadString(fkl);
								if (CloakCheckError(fk.compare(fileT.Key) == 0, API::Global::Debug::Error::FREADER_FAILED_FILEKEY, false)) { return 0; }
								if (res > fileT.Version) 
								{ 
									API::Global::Log::WriteToLog("Filetype '" + ft + "' is out of date! Reader-Version: " + std::to_string(fileT.Version) + " File-Version: " + std::to_string(res), API::Global::Log::Type::Warning); 
								}
								const uint32_t offset = static_cast<uint32_t>(ReadBits(32));
								for (size_t a = 0; a < offset; a++) { ReadBits(8); }
								return res;
							}
							else
							{
								m_input = API::Files::Buffer_v1::CreateUTFRead(m_input);
								return 1;
							}
						}
					}
					return 0;
				}
				void CLOAK_CALL_THIS Reader::checkPos()
				{
					size_t tr = m_bitPos >> 3;
					m_bitPos &= 0x07;
					while (tr > 0)
					{
						if (m_input->HasNextByte()) { m_data = m_input->ReadByte(); }
						else { m_data = 0; }
						m_bytePos++;
						tr--;
					}
				}
				void* CLOAK_CALL Reader::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL Reader::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

				void CLOAK_CALL Initialize()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncDefaultGameID));
				}
				void CLOAK_CALL Release()
				{
					SAVE_RELEASE(g_syncDefaultGameID);
				}
			}
		}
	}
	namespace API {
		namespace Files {
			namespace Compressor_v2 {
				CLOAKENGINE_API void CLOAK_CALL SetDefaultGameID(In const Buffer_v1::UGID& gameID)
				{
					API::Helper::Lock lock(Impl::Files::g_syncDefaultGameID);
					Impl::Files::g_defaultGameID = gameID;
					Impl::Files::g_hasDefaultGameID = true;
				}
			}
		}
	}
}