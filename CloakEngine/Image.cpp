#include "stdafx.h"
#include "Implementation/Files/Image.h"
#include "Implementation/Rendering/Manager.h"
#include "Implementation/Global/Graphic.h"

#include "Engine/Graphic/Core.h"

#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Global/Math.h"

#include <unordered_map>

#ifdef _DEBUG
#include <sstream>
#include <iomanip>
#endif

#define V1008_DECODE_FUNC(name) void name(In API::Files::IReader* read, In API::Files::FileVersion version, Inout FileData::BitBuffer* buffer, In const FileData::BitBuffer& prevMip, In const FileData::BitData& bits, In uint8_t channels, In const FileData::Meta& meta, In UINT W, In UINT H, In bool byteAligned)
#define V1008_NBF_FILTER(name) BitColorData name(In BitColorData* line, In BitColorData* prevLine, In UINT x, In UINT y)
#define V1012_DECODE_NBHRF(name) int64_t name(In const int64_t* line, In const int64_t* prevLine, In UINT x, In UINT y, In UINT c, In const FileData::BitBuffer& prevMip, In uint64_t maxValue)

//#define PRINT_DECODING
//#define READ_CHECKMARKS

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace Image_v2 {
				constexpr API::Files::FileType g_fileType{ "InteliImage","CEII",1014 };
				const API::Global::Math::Matrix g_ToRGB(1.000000f, -0.000001f, 1.402000f, -0.700999f, 1.000000f, -0.344136f, -0.714136f, 0.529136f, 1.000000f, 1.772000f, 0.000000f, -0.886000f, -0.000000f, 0.000000f, -0.000000f, 1.000000f);
				constexpr uint8_t TYPE_IMAGE = 0;
				constexpr uint8_t TYPE_ARRAY = 1;
				constexpr uint8_t TYPE_CUBE = 2;
				constexpr uint8_t TYPE_ANIMATION = 3;
				constexpr uint8_t TYPE_MATERIAL = 4;
				constexpr uint8_t TYPE_SKYBOX = 5;

				typedef uint64_t BitColorData;

				inline bool CLOAK_CALL operator<(In API::Global::Graphic::TextureQuality a, In API::Global::Graphic::TextureQuality b)
				{
					return static_cast<uint8_t>(a) < static_cast<uint8_t>(b);
				}
				inline bool CLOAK_CALL operator>(In API::Global::Graphic::TextureQuality a, In API::Global::Graphic::TextureQuality b) { return b < a; }
				inline bool CLOAK_CALL operator<=(In API::Global::Graphic::TextureQuality a, In API::Global::Graphic::TextureQuality b) { return !(b < a); }
				inline bool CLOAK_CALL operator>=(In API::Global::Graphic::TextureQuality a, In API::Global::Graphic::TextureQuality b) { return !(a < b); }
				inline uint8_t CLOAK_CALL operator-(In API::Global::Graphic::TextureQuality a, In API::Global::Graphic::TextureQuality b)
				{
					const uint8_t ia = static_cast<uint8_t>(a);
					const uint8_t ib = static_cast<uint8_t>(b);
					return ia < ib ? 0 : ia - ib;
				}
				inline uint8_t CLOAK_CALL BitLen(In uint64_t val)
				{
					return val == 0 ? 0 : (API::Global::Math::bitScanReverse(val) + 1);
				}

				namespace Half {
					float CLOAK_CALL FromHalf(In uint16_t h)
					{
						const bool s = (h & 32768) != 0;
						const uint64_t e = (h >> 10) & 31;
						const uint64_t m = h & 1023;
						if (e == 31)
						{
							if (m == 0) { return std::numeric_limits<float>::infinity(); }
							return std::numeric_limits<float>::quiet_NaN();
						}
						const float x = (static_cast<float>(m) / 1024) + (e == 0 ? 0 : 1);
						const int shift = static_cast<int>(e - 15);
						return std::copysign(std::ldexpf(x, shift), s ? -1.0f : 1.0f);
					}
					uint16_t CLOAK_CALL ToHalf(In float f)
					{
						int64_t e;
						int64_t s;
						if (f != f)//NaN
						{
							e = 31;
							s = 1023;
						}
						else if (std::abs(f) == std::numeric_limits<float>::infinity())//Infinity
						{
							e = 31;
							s = 0;
						}
						else if (f == 0)
						{
							e = 0;
							s = 0;
						}
						else
						{
							int ex;
							float m = std::frexp(std::abs(f), &ex) * 2;
							e = ex + 14;
							if (e <= 0)//Denormalized
							{
								m /= (1ULL << (-e));
								e = 0;
								s = static_cast<int64_t>(1024 * m);
							}
							else if (e >= 31)//Overflow
							{
								e = 31;
								s = 0;
							}
							else
							{
								s = static_cast<int64_t>(1024 * (m - 1));
							}
						}
						const uint16_t sr = static_cast<uint16_t>((std::copysign(1.0f, f) < 0) ? (1ULL << 15) : 0);
						const uint16_t er = static_cast<uint16_t>(e << 10);
						const uint16_t res = static_cast<uint16_t>(sr | er | s);
						return res;
					}
				}

				namespace v1008 {
					typedef V1008_DECODE_FUNC((*DecodeFilter));

					namespace BlockDecoding {
						struct FixedBlockLength {
							uint8_t fbl[4];
							CLOAK_CALL FixedBlockLength() 
							{
								for (size_t a = 0; a < ARRAYSIZE(fbl); a++) { fbl[a] = 0; }
							}
						};
						void CLOAK_CALL ReadLine(In API::Files::IReader* read, In API::Files::FileVersion version, In const FileData::BitData& bits, In UINT W, In UINT y, In size_t c, In uint8_t sl, In const FixedBlockLength& fbl, Out BitColorData* line, In bool byteAligned)
						{
							UINT x = 0;
							const UINT fby = (y % 0x1) << 1;
							do {
								const UINT l = static_cast<UINT>(read->ReadBits(sl));
								const BitColorData dMin = static_cast<BitColorData>(read->ReadBits(bits.Bits[c]));
								const uint8_t s = static_cast<uint8_t>(read->ReadBits(bits.Len[c]));
								if (byteAligned == true) { read->MoveToNextByte(); }
								for (UINT a = 0; a < l; a++, x++)
								{
									const UINT fbx = x & 0x1;
									if (fbl.fbl[fbx + fby] == 0) { line[x] = static_cast<BitColorData>(read->ReadBits(s)) + dMin; }
									else { line[c] = static_cast<BitColorData>(read->ReadBits(fbl.fbl[c])); }
								}
							} while (x < W);
							if (x > W) { CloakError(API::Global::Debug::Error::FREADER_FILE_CORRUPTED, true); }
						}
					}

					namespace NBF {
						typedef V1008_NBF_FILTER((*FilterMode));

						V1008_NBF_FILTER(NBF0) { return 0; }
						V1008_NBF_FILTER(NBF1)
						{
							if (x == 0) { return 0; }
							return line[x - 1];
						}
						V1008_NBF_FILTER(NBF2)
						{
							if (y == 0) { return 0; }
							return prevLine[x];
						}
						V1008_NBF_FILTER(NBF3)
						{
							const BitColorData a = NBF1(line, prevLine, x, y);
							const BitColorData b = NBF2(line, prevLine, x, y);
							return (a + b) >> 1;
						}
						V1008_NBF_FILTER(NBF4)
						{
							if (y == 0) { return NBF1(line, prevLine, x, y); }
							if (x == 0) { return NBF2(line, prevLine, x, y); }
							const BitColorData ad = line[x - 1];
							const BitColorData bd = prevLine[x];
							const BitColorData cd = prevLine[x - 1];
							const BitColorData p = (ad + bd) - cd;
							const BitColorData pa = p > ad ? (p - ad) : (ad - p);
							const BitColorData pb = p > bd ? (p - bd) : (bd - p);
							const BitColorData pc = p > cd ? (p - cd) : (cd - p);
							if (pa < pb && pa < pc) { return ad; }
							else if (pb < pc) { return bd; }
							return cd;
						}
						V1008_NBF_FILTER(NBF5)
						{
							if (y == 0) { return NBF1(line, prevLine, x, y); }
							if (x == 0) { return NBF2(line, prevLine, x, y); }
							const BitColorData ad = line[x - 1];
							const BitColorData bd = prevLine[x];
							const BitColorData cd = prevLine[x - 1];
							return (ad + bd) - cd;
						}

						constexpr FilterMode MODES[] = { NBF0, NBF1, NBF2, NBF3, NBF4, NBF5 };

						V1008_DECODE_FUNC(Decode)
						{
							BitColorData* lineData = NewArray(BitColorData, W << 1);
							BitColorData* lines[2] = { &lineData[0],&lineData[W] };
							BlockDecoding::FixedBlockLength fbl;
							uint8_t modes[4];
							for (size_t a = 0; a < 4; a++) 
							{ 
								if ((channels & (1 << a)) != 0)
								{
									modes[a] = static_cast<uint8_t>(read->ReadBits(3));
									assert(modes[a] < ARRAYSIZE(MODES));
								}
							}
							if (byteAligned == true) { read->MoveToNextByte(); }
							const uint8_t sl = BitLen(W);
							for (size_t c = 0; c < 4; c++)
							{
								if ((channels & (1 << c)) != 0)
								{
									for (UINT y = 0; y < H; y++)
									{
										BlockDecoding::ReadLine(read, version, bits, W, y, c, sl, fbl, lines[0], byteAligned);
										for (UINT x = 0; x < W; x++)
										{
											BitColorData pv = MODES[modes[c]](lines[0], lines[1], x, y);
											pv = min(bits.MaxValue[c], max(0, pv));
											lines[0][x] = (lines[0][x] + pv) % (bits.MaxValue[c] + 1);
										}
										for (UINT x = 0; x < W; x++)
										{
											buffer->Get(x, y).Set(static_cast<float>(lines[0][x]) / bits.MaxValue[c], c);
										}
										BitColorData* t = lines[0];
										lines[0] = lines[1];
										lines[1] = t;
									}
								}
							}
							DeleteArray(lineData);
						}
					}
					namespace QTF {
						V1008_DECODE_FUNC(Decode)
						{
							BitColorData* lineData = NewArray(BitColorData, W << 1);
							BitColorData* lines[2] = { &lineData[0],&lineData[W] };
							BlockDecoding::FixedBlockLength fbl;
							fbl.fbl[3] = 3;
							const uint8_t sl = BitLen(W);
							for (size_t c = 0; c < 4; c++)
							{
								if ((channels & (1 << c)) != 0)
								{
									for (UINT y = 0; y < H; y++)
									{
										BlockDecoding::ReadLine(read, version, bits, W, y, c, sl, fbl, lines[0], byteAligned);
										if ((y & 0x1) == 1)
										{
											const UINT py = y >> 1;
											const UINT pW = W >> 1;
											for (UINT x = 0; x < pW; x++)
											{
												const UINT dx = x << 1;
												const BitColorData d = (static_cast<BitColorData>(prevMip.Get(x, py).Get(c)*bits.MaxValue[c]) << 2) - (lines[1][dx] + lines[1][dx + 1] + lines[0][dx]);
												lines[0][dx + 1] = (lines[0][dx + 1] + d - 4) % (bits.MaxValue[c] + 1);
											}
										}
										for (UINT x = 0; x < W; x++)
										{
											buffer->Get(x, y).Set(static_cast<float>(lines[0][x]) / bits.MaxValue[c], c);
										}
										BitColorData* t = lines[0];
										lines[0] = lines[1];
										lines[1] = t;
									}
								}
							}
							DeleteArray(lineData);
						}
					}
					namespace HRF {
						inline void CLOAK_CALL DecodeColor(In API::Files::IReader* read, In API::Files::FileVersion version, Inout FileData::BitBuffer* buffer, In const FileData::BitData& bits, In uint8_t channels, In UINT x, In UINT y, In uint64_t exp, In uint8_t edl, In bool byteAligned)
						{
							const uint64_t mex = read->ReadBits(edl) + exp;
							const int e = static_cast<int>(mex) - static_cast<int>(bits.MaxValue[3] >> 1);
							for (size_t c = 0; c < 3; c++)
							{
								if ((channels & (1 << c)) != 0)
								{
									const bool neg = read->ReadBits(1) == 1;
									const BitColorData d = static_cast<BitColorData>(read->ReadBits(bits.Bits[c]));
									if (mex == bits.MaxValue[3]) 
									{
										buffer->Get(x, y).Set(d == 0 ? std::numeric_limits<float>::infinity() : std::numeric_limits<float>::quiet_NaN(), c);
									}
									else
									{
										float cv = (neg ? -1 : 1)*std::ldexp(static_cast<float>(d) / bits.MaxValue[c], e);
										buffer->Get(x, y).Set(cv, c);
									}
									if (byteAligned == true) { read->MoveToNextByte(); }
								}
							}
						}

						V1008_DECODE_FUNC(Decode)
						{
							const uint8_t sl = BitLen(W);
							for (UINT y = 0; y < H; y++)
							{
								UINT x = 0;
								do {
									const UINT l = static_cast<UINT>(read->ReadBits(sl));
									const uint64_t exp = read->ReadBits(bits.Bits[3]);
									const uint8_t exl = static_cast<uint8_t>(read->ReadBits(bits.Len[3]));
									if (byteAligned == true) { read->MoveToNextByte(); }
									for (UINT n = 0; n < l; n++, x++) { DecodeColor(read, version, buffer, bits, channels, x, y, exp, exl, byteAligned); }
								} while (x < W);
							}
						}
					}
					namespace NBHRF {
						typedef V1012_DECODE_NBHRF((*FilterMode));

						//No Filter
						V1012_DECODE_NBHRF(NBHRF0) { return 0; }
						//Assumes same as left color (Same as PNG's Sub)
						V1012_DECODE_NBHRF(NBHRF1)
						{
							if (x == 0) { return 0; }
							return line[x - 1];
						}
						//Assumes same as top color (Same as PNG's Up)
						V1012_DECODE_NBHRF(NBHRF2)
						{
							if (y == 0) { return 0; }
							return prevLine[x];
						}
						//Assumes average of left and top color (Same as PNG's Average)
						V1012_DECODE_NBHRF(NBHRF3)
						{
							const int64_t a = NBHRF1(line, prevLine, x, y, c, prevMip, maxValue);
							const int64_t b = NBHRF2(line, prevLine, x, y, c, prevMip, maxValue);
							return (a + b) / 2;
						}
						//Assumes same es top left color
						V1012_DECODE_NBHRF(NBHRF4)
						{
							if (y == 0) { return NBHRF1(line, prevLine, x, y, c, prevMip, maxValue); }
							if (x == 0) { return NBHRF2(line, prevLine, x, y, c, prevMip, maxValue); }
							return prevLine[x - 1];
						}
						//Assumes best of left, top and top left color based on derivation (Same as PNG's Paeth)
						V1012_DECODE_NBHRF(NBHRF5)
						{
							if (y == 0) { return NBHRF1(line, prevLine, x, y, c, prevMip, maxValue); }
							if (x == 0) { return NBHRF2(line, prevLine, x, y, c, prevMip, maxValue); }
							const int64_t ad = line[x - 1];
							const int64_t bd = prevLine[x];
							const int64_t cd = prevLine[x - 1];
							const int64_t p = (ad + bd) - cd;
							const int64_t pa = std::abs(p - ad);
							const int64_t pb = std::abs(p - bd);
							const int64_t pc = std::abs(p - cd);
							if (pa < pb && pa < pc) { return ad; }
							else if (pb < pc) { return bd; }
							return cd;
						}
						//Assumes derivation
						V1012_DECODE_NBHRF(NBHRF6)
						{
							if (y == 0) { return NBHRF1(line, prevLine, x, y, c, prevMip, maxValue); }
							if (x == 0) { return NBHRF2(line, prevLine, x, y, c, prevMip, maxValue); }
							const int64_t ad = line[x - 1];
							const int64_t bd = prevLine[x];
							const int64_t cd = prevLine[x - 1];
							return (ad + bd) - cd;
						}
						//Assumes same as in previous mip map
						V1012_DECODE_NBHRF(NBHRF7)
						{
							x >>= 1;
							y >>= 1;
							if (x >= prevMip.GetWidth() || y >= prevMip.GetHeight()) { return 0; }
							return static_cast<int64_t>((prevMip.Get(x, y).Get(c) * maxValue) + 0.5f);
						}
						//Quad tree calculation
						V1012_DECODE_NBHRF(NBHRF8)
						{
							const int64_t m = NBHRF7(line, prevLine, x, y, c, prevMip, maxValue);
							const int64_t ad = NBHRF1(line, prevLine, x, y, c, prevMip, maxValue);
							const int64_t bd = NBHRF2(line, prevLine, x, y, c, prevMip, maxValue);
							const int64_t cd = NBHRF4(line, prevLine, x, y, c, prevMip, maxValue);
							return (4 * m) - (ad + bd + cd);
						}
						//Assumes best of left, top, top left and quad tree, based on derivation
						V1012_DECODE_NBHRF(NBHRF9)
						{
							const int64_t m = NBHRF7(line, prevLine, x, y, c, prevMip, maxValue);
							const int64_t ad = NBHRF1(line, prevLine, x, y, c, prevMip, maxValue);
							const int64_t bd = NBHRF2(line, prevLine, x, y, c, prevMip, maxValue);
							const int64_t cd = NBHRF4(line, prevLine, x, y, c, prevMip, maxValue);
							const int64_t p = (ad + bd) - cd;
							const int64_t pa = std::abs(p - ad);
							const int64_t pb = std::abs(p - bd);
							const int64_t pc = std::abs(p - cd);
							const int64_t pm = std::abs(p - m);
							if (pa < pb && pa < pc && pa < pm) { return ad; }
							else if (pb < pc && pb < pm) { return bd; }
							else if (pc < pm) { return cd; }
							return m;
						}
						
						constexpr FilterMode MODES[] = { NBHRF0, NBHRF1, NBHRF2, NBHRF3, NBHRF4, NBHRF5, NBHRF6, NBHRF7, NBHRF8, NBHRF9 };

						int64_t CLOAK_CALL ReadHRValue(In API::Files::IReader* read, In UINT exponentLen, In UINT valueLen, In_opt bool canBeNegative = false)
						{
							bool neg = false;
							if (canBeNegative == true) { neg = read->ReadBits(1) == 1; }
							uint8_t exponent = static_cast<uint8_t>(read->ReadBits(exponentLen));
							uint64_t value = read->ReadBits(valueLen);
							int64_t res = static_cast<int64_t>(value << exponent);
#if defined(_DEBUG) && defined(PRINT_DECODING)
							CloakDebugLog("ReadHR: e = " + std::to_string(exponent) + " v = " + (neg == true ? "-" : "") + std::to_string(value) + " (is " + (neg == true ? "-" : "") + std::to_string(res) + ")");
#endif
							return neg == true ? -res : res;
						}
						int64_t CLOAK_CALL ReadLRValue(In API::Files::IReader* read, In UINT valueLen, In_opt bool canBeNegative = false)
						{
							bool neg = false;
							if (canBeNegative == true) { neg = read->ReadBits(1) == 1; }
							int64_t value = static_cast<int64_t>(read->ReadBits(valueLen));
#if defined(_DEBUG) && defined(PRINT_DECODING)
							CloakDebugLog("ReadLR: v = " + std::string(neg == true ? "-" : "") + std::to_string(value));
#endif
							return neg == true ? -value : value;
						}
						int64_t CLOAK_CALL ReadValue(In API::Files::IReader* read, In int64_t minValue, In UINT exponentLen, In UINT valueLen, In bool useExponent)
						{
							return (useExponent == true ? ReadHRValue(read, exponentLen, valueLen) : ReadLRValue(read, valueLen)) + minValue;
						}

						V1008_DECODE_FUNC(Decode)
						{
							const UINT tileSize = static_cast<UINT>(read->ReadDynamic());
							const UINT tileW = (W + tileSize - 1) / tileSize;
							const UINT tileH = (H + tileSize - 1) / tileSize;
							const UINT tileCount = tileW * tileH;
							const uint8_t blockBits = BitLen(W * H);

							uint8_t* bt = NewArray(uint8_t, tileCount * 4);
							int64_t* lines = NewArray(int64_t, W * 2);
							int64_t* line[2] = { &lines[0], &lines[W] };
							//Read modes:
							for (UINT a = 0; a < 4; a++)
							{
								if ((static_cast<uint8_t>(channels) & (1 << a)) != 0)
								{
									for (UINT y = 0, tid = a; y < H; y += tileSize)
									{
										for (UINT x = 0; x < W; tid += 4, x += tileSize)
										{
											bt[tid] = static_cast<uint8_t>(read->ReadBits(4));
#if defined(_DEBUG) && defined(PRINT_DECODING)
											CloakDebugLog("NBHRF Mode[" + std::to_string(tid) + "] = " + std::to_string(bt[tid]));
#endif
										}
									}
								}
							}
							if (byteAligned == true) { read->MoveToNextByte(); }
							//Read values:
							for (UINT a = 0; a < 4; a++)
							{
								if ((static_cast<uint8_t>(channels) & (1 << a)) != 0)
								{
									size_t remValues = 0;
									bool expValues = false;
									UINT valueLen = 0;
									UINT expLen = 0;
									int64_t minValue = 0;

									for (UINT y = 0; y < H; y++)
									{
										for (UINT x = 0; x < W; x++)
										{
											if (remValues == 0)
											{
												//Read Header
												expValues = read->ReadBits(1) == 1;
												const bool expMin = read->ReadBits(1) == 1;
												remValues = static_cast<size_t>(read->ReadBits(blockBits) + 1);
												valueLen = static_cast<UINT>(read->ReadBits(bits.Len[a]));
												if (expValues == true || expMin == true) { expLen = static_cast<UINT>(read->ReadBits(5)); }
												if (expMin == true) { minValue = ReadHRValue(read, expLen, bits.Bits[a], true); }
												else { minValue = ReadLRValue(read, bits.Bits[a], true); }
												if (byteAligned == true) { read->MoveToNextByte(); }
#if defined(_DEBUG) && defined(PRINT_DECODING)
												CloakDebugLog("Read[" + std::to_string(a) + "] Min Value = " + std::to_string(minValue) + " values = " + std::to_string(remValues) + " valueBits = " + std::to_string(valueLen) + " exponentBits = " + std::to_string(expLen));
#endif
											}

											//Read color
											const size_t ntid = ((((y / tileSize) * tileW) + (x / tileSize)) * 4) + a;
											CLOAK_ASSUME(ntid < (tileCount * 4));
											const int64_t v = ReadValue(read, minValue, expLen, valueLen, expValues);
											const int64_t m = MODES[bt[ntid]](line[0], line[1], x, y, a, prevMip, bits.MaxValue[a]);
											line[0][x] = v + m;
											buffer->Get(x, y).Set(static_cast<float>(static_cast<long double>(v + m) / bits.MaxValue[a]), a);
											remValues--;
#if defined(_DEBUG) && defined(PRINT_DECODING)
											CloakDebugLog("Read[" + std::to_string(a) + "][" + std::to_string(x) + "][" + std::to_string(y) + "] = " + std::to_string(buffer->Get(x, y).Get(a)) + " as Integer = " + std::to_string(v + m) + " (P = " + std::to_string(m) + ") Read: " + std::to_string(v - minValue));
#endif
										}
										int64_t* t = line[0];
										line[0] = line[1];
										line[1] = t;
									}
									CLOAK_ASSUME(remValues == 0);
								}
							}
							DeleteArray(lines);
							DeleteArray(bt);
						}
					}
					enum FILTER_REQUIREMENT { FR_NONE, FR_RGBE, FR_RGBA, FR_YCCA, FR_MATERIAL };

					constexpr DecodeFilter FILTER[] = {			NBF::Decode,	QTF::Decode,	HRF::Decode,	NBHRF::Decode };
					constexpr FILTER_REQUIREMENT FILREQ[] = {	FR_NONE,		FR_NONE,		FR_RGBE,		FR_NONE };
					static_assert(ARRAYSIZE(FILTER) <= ARRAYSIZE(FILREQ), "Each filter require a filter requirement setting!");

					inline void CLOAK_CALL ConvertFromYCCA(Inout FileData::BitBuffer* data, In UINT W, In UINT H)
					{
						API::Global::Math::Vector vc;
						API::Global::Math::Point pc;
						for (UINT y = 0; y < H; y++)
						{
							for (UINT x = 0; x < W; x++)
							{
								FileData::BitBufferPos c = data->Get(x, y);
								vc.Set(c.Get(0), c.Get(1), c.Get(2));
								vc *= g_ToRGB;
								pc = static_cast<API::Global::Math::Point>(vc);
								c.Set(pc.Data[0], 0);
								c.Set(pc.Data[1], 1);
								c.Set(pc.Data[2], 2);
							}
						}
					}
					inline void CLOAK_CALL AddPrevTexture(Inout FileData::BufferData* buffer, In const FileData::BufferData* prevTex, In const size_t size)
					{
						//TODO: Currently deactivated!
					}
					inline size_t CLOAK_CALL ResetBitBuffer(In API::Files::IReader* read, In API::Files::FileVersion version, Inout FileData::BitBuffer* layer, In uint8_t channels, In const FileData::BitList& bits, In FileData::ColorSpace colors, In bool hr, Out UINT* oW, Out UINT* oH)
					{
						const UINT W = static_cast<UINT>(read->ReadDynamic());
						const UINT H = static_cast<UINT>(read->ReadDynamic());
						switch (colors)
						{
							case CloakEngine::Impl::Files::Image_v2::FileData::ColorSpace::RGBA:
								layer->Reset(W, H, channels, bits.RGBA, hr);
								break;
							case CloakEngine::Impl::Files::Image_v2::FileData::ColorSpace::YCCA:
								layer->Reset(W, H, channels, bits.YCCA, hr);
								break;
							case CloakEngine::Impl::Files::Image_v2::FileData::ColorSpace::Material:
								layer->Reset(W, H, channels, bits.Material, hr);
								break;
							default:
								break;
						}
						*oW = W;
						*oH = H;
						return static_cast<size_t>(W)*H;
					}
					inline void CLOAK_CALL DecodeLayerFilter(In API::Files::IReader* read, In API::Files::FileVersion version, In FileData::BitBuffer* buffer, In const FileData::BitBuffer& lastLayer, In uint8_t channels, In const FileData::BitList& bits, In const FileData::Meta& meta, In UINT W, In UINT H)
					{
						const uint8_t mode = static_cast<uint8_t>(read->ReadBits(8));
#if defined(_DEBUG) && defined(PRINT_DECODING)
						CloakDebugLog("Start decoding image layer with mode " + std::to_string(mode));
#endif
						if (CloakCheckOK(mode < ARRAYSIZE(FILTER), API::Global::Debug::Error::FREADER_FILE_CORRUPTED, false))
						{
							switch (FILREQ[mode])
							{
								case FR_NONE:
									switch (meta.ColorSpace)
									{
										case FileData::ColorSpace::RGBA:
											FILTER[mode](read, version, buffer, lastLayer, bits.RGBA, channels, meta, W, H, false);
											break;
										case FileData::ColorSpace::YCCA:
											FILTER[mode](read, version, buffer, lastLayer, bits.YCCA, channels, meta, W, H, false);
											break;
										case FileData::ColorSpace::Material:
											FILTER[mode](read, version, buffer, lastLayer, bits.Material, channels, meta, W, H, false);
											break;
										default:
											break;
									}
									break;
								case FR_RGBE:
									FILTER[mode](read, version, buffer, lastLayer, bits.RGBE, channels, meta, W, H, false);
									break;
								case FR_RGBA:
									FILTER[mode](read, version, buffer, lastLayer, bits.RGBA, channels, meta, W, H, false);
									break;
								case FR_YCCA:
									FILTER[mode](read, version, buffer, lastLayer, bits.YCCA, channels, meta, W, H, false);
									break;
								case FR_MATERIAL:
									FILTER[mode](read, version, buffer, lastLayer, bits.Material, channels, meta, W, H, false);
									break;
								default:
									break;
							}
						}
					}
					void CLOAK_CALL DecodeTextureLayerShort(In API::Files::IReader* read, In API::Files::FileVersion version, Inout FileData::BitBuffer* buffer, In uint8_t channels, In const FileData::BitData& bits, In const FileData::Meta& meta, In UINT W, In UINT H, In const FileData::BitBuffer& lastMip, In_opt FileData::BitBuffer* prevTex = nullptr)
					{
						const bool useDiff = read->ReadBits(1) == 1;
						const uint8_t mode = static_cast<uint8_t>(read->ReadBits(8));
						FILTER[mode](read, version, buffer, lastMip, bits, channels, meta, W, H, false);
					}
					void CLOAK_CALL DecodeTextureLayer(In API::Files::IReader* read, In API::Files::FileVersion version, Inout FileData::BufferData* buffer, In uint8_t channels, In const FileData::BitList& bits, In const FileData::Meta& meta, In_opt FileData::BitBuffer* prevTex = nullptr)
					{
						UINT W = 0;
						UINT H = 0;
						FileData::BitBuffer lastLayer = buffer->LastLayer;
						const size_t size = ResetBitBuffer(read, version, &buffer->LastLayer, channels, bits, meta.ColorSpace, meta.HR, &W, &H);
						const bool useDiff = read->ReadBits(1) == 1;
						DecodeLayerFilter(read, version, &buffer->LastLayer, lastLayer, channels, bits, meta, W, H);
						//if (useDiff) { AddPrevTexture(buffer, prevTex, size); } //TODO: Currently deactivated!
					}
					void CLOAK_CALL DecodeArrayLayer(In API::Files::IReader* read, In API::Files::FileVersion version, Inout FileData::ArrayData* buffer, In size_t index, In uint8_t channels, In const FileData::BitList& bits, In const FileData::Meta& meta, In FileData::ColorSpace space, In_opt FileData::BitBuffer* prevTex = nullptr)
					{
						UINT W = 0;
						UINT H = 0;
						FileData::BitBuffer lastLayer = buffer->LastLayers[index];
						const size_t size = ResetBitBuffer(read, version, &buffer->LastLayers[index], channels, bits, space, meta.HR, &W, &H);
						const bool useDiff = read->ReadBits(1) == 1;
						DecodeLayerFilter(read, version, &buffer->LastLayers[index], lastLayer, channels, bits, meta, W, H);
						//if (useDiff) { AddPrevTexture(buffer, prevTex, size); } //TODO: Currently deactivated!
					}

					API::Rendering::DescriptorPageType CLOAK_CALL ReadHeap(In API::Files::IReader* read)
					{
						const uint8_t h = static_cast<uint8_t>(read->ReadBits(2));
						if (h == 0) { return API::Rendering::DescriptorPageType::GUI; }
						else if (h == 1) { return API::Rendering::DescriptorPageType::WORLD; }
						else if (h == 2) { return API::Rendering::DescriptorPageType::UTILITY; }
						CloakError(API::Global::Debug::Error::FREADER_FILE_CORRUPTED, true);
						return API::Rendering::DescriptorPageType::GUI;
					}
					FileData::ColorSpace ReadColorSpace(In API::Files::IReader* read)
					{
						const uint8_t c = static_cast<uint8_t>(read->ReadBits(3));
						if (c == 0) { return FileData::ColorSpace::RGBA; }
						else if (c == 1) { return FileData::ColorSpace::YCCA;}
						else if (c == 2) { return FileData::ColorSpace::Material; }
						CloakError(API::Global::Debug::Error::FREADER_FILE_CORRUPTED, true);
						return FileData::ColorSpace::RGBA;
					}
					API::Files::Image_v2::MaterialType CLOAK_CALL ReadMaterialType(In API::Files::IReader* read)
					{
						const uint8_t t = static_cast<uint8_t>(read->ReadBits(2));
						if (t == 0) { return API::Files::Image_v2::MaterialType::Opaque; }
						else if (t == 1) { return API::Files::Image_v2::MaterialType::AlphaMapped; }
						else if (t == 2) { return API::Files::Image_v2::MaterialType::Transparent; }
						return API::Files::Image_v2::MaterialType::Unknown;
					}
					void CLOAK_CALL ReadMeta(In API::Files::IReader* read, In API::Files::FileVersion version, Out FileData::Meta* meta)
					{
						meta->ColorSpace = ReadColorSpace(read);
						if (version <= 1010) { static_cast<uint16_t>(read->ReadBits(16)); }
						meta->Flags = static_cast<uint8_t>(read->ReadBits(8));
						if (version >= 1010) { meta->HR = read->ReadBits(1) == 1; }
						else { meta->HR = false; }
					}
					uint8_t CLOAK_CALL FindChannels(In API::Files::Image_v2::MaterialType type, In size_t imgNum)
					{
						if (imgNum >= 3) { return 0x1; }
						if (imgNum == 0 && type == API::Files::Image_v2::MaterialType::Opaque) { return 0x7; }
						return 0xF;
					}
					FileData::ColorSpace CLOAK_CALL FindBiggestColorSpace(In FileData::Meta* metas, In size_t metaCount, In const FileData::BitList& bits)
					{
						uint8_t mbw = 0;
						size_t bf = 0;
						for (size_t a = 0; a < metaCount; a++)
						{
							const FileData::BitData* d = nullptr;
							const FileData::ColorSpace space = metas[a].ColorSpace;
							if (space == FileData::ColorSpace::RGBA) { d = &bits.RGBA; }
							else if (space == FileData::ColorSpace::YCCA) { d = &bits.YCCA; }
							else if (space == FileData::ColorSpace::Material) { d = &bits.Material; }
							for (size_t b = 0; b < 4; b++)
							{
								if (d->Bits[b] > mbw)
								{
									bf = a;
									mbw = d->Bits[b];
								}
							}
						}
						return metas[bf].ColorSpace;
					}
					API::Rendering::Format CLOAK_CALL FindFormat(In uint8_t channels, In const FileData::BitData& bits, In bool hr)
					{
						uint8_t nc = 0;
						uint8_t mbw = 0;
						for (size_t a = 0; a < 4; a++) { mbw = max(mbw, bits.Bits[a]); }
						if ((channels & 0x1) != 0) { nc++; }
						if ((channels & 0x2) != 0) { nc++; }
						if ((channels & 0x4) != 0) { nc++; }
						if ((channels & 0x8) != 0) { nc++; }
						assert(nc > 0);
						assert(mbw > 0);
						assert(mbw <= 32);
						if (nc == 1)
						{
							if (mbw <= 8) { return hr ? API::Rendering::Format::R16_FLOAT : API::Rendering::Format::R8_UNORM; }
							else if (mbw <= 16) { return hr ? API::Rendering::Format::R16_FLOAT : API::Rendering::Format::R16_UNORM; }
							return API::Rendering::Format::R32_FLOAT;
						}
						else if (nc == 2)
						{
							if (mbw <= 8) { return hr ? API::Rendering::Format::R16G16_FLOAT : API::Rendering::Format::R8G8_UNORM; }
							else if (mbw <= 16) { return hr ? API::Rendering::Format::R16G16_FLOAT : API::Rendering::Format::R16G16_UNORM; }
							return API::Rendering::Format::R32G32_FLOAT;
						}
						else
						{
							if (mbw <= 8) { return hr ? API::Rendering::Format::R16G16B16A16_FLOAT : API::Rendering::Format::R8G8B8A8_UNORM; }
							else if (mbw <= 16) { return  hr ? API::Rendering::Format::R16G16B16A16_FLOAT : API::Rendering::Format::R16G16B16A16_UNORM; }
							return API::Rendering::Format::R32G32B32A32_FLOAT;
						}
					}
					API::Rendering::Format CLOAK_CALL FindFormat(In uint8_t channels, In const FileData::BitList& bits, In FileData::ColorSpace space, In bool hr)
					{
						const FileData::BitData* d = nullptr;
						if (space == FileData::ColorSpace::RGBA) { d = &bits.RGBA; }
						else if (space == FileData::ColorSpace::YCCA) { d = &bits.YCCA; }
						else if (space == FileData::ColorSpace::Material) { d = &bits.Material; }
						assert(d != nullptr);
						return FindFormat(channels, *d, hr);
					}
				}
				namespace Proxys {
					CLOAK_CALL EntryProxy::EntryProxy(In Image* parent, In size_t entry) : m_parent(parent), m_entry(entry), m_lod(API::Files::LOD::LOW)
					{
						DEBUG_NAME(EntryProxy);
						if (m_parent != nullptr) { m_parent->AddRef(); }
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
						INIT_EVENT(onLoad, m_sync);
						INIT_EVENT(onUnload, m_sync);
					}
					CLOAK_CALL EntryProxy::~EntryProxy()
					{
						m_parent->UnregisterLOD(m_lod);
						SAVE_RELEASE(m_parent);
						SAVE_RELEASE(m_sync);
					}
					ULONG CLOAK_CALL_THIS EntryProxy::load(In_opt API::Files::Priority prio) { return m_parent->load(prio); }
					ULONG CLOAK_CALL_THIS EntryProxy::unload() { return m_parent->unload(); }
					bool CLOAK_CALL_THIS EntryProxy::isLoaded(In API::Files::Priority prio) { return m_parent->isLoaded(prio); }
					bool CLOAK_CALL_THIS EntryProxy::isLoaded() const { return m_parent->isLoaded(); }

					void CLOAK_CALL_THIS EntryProxy::RequestLOD(In API::Files::LOD lod) 
					{ 
						API::Files::LOD l = m_lod.exchange(lod);
						if (l != lod)
						{
							m_parent->RegisterLOD(lod);
							m_parent->UnregisterLOD(l);
						}
					}

					Success(return == true) bool CLOAK_CALL_THIS EntryProxy::iQueryInterface(In REFIID riid, Outptr void** ptr) 
					{
						if (riid == __uuidof(API::Files::FileHandler_v1::IFileHandler)) { *ptr = (API::Files::FileHandler_v1::IFileHandler*)this; return true; }
						else if (riid == __uuidof(API::Files::ILODFile)) { *ptr = (API::Files::ILODFile*)this; return true; }
						return SavePtr::iQueryInterface(riid, ptr);
					}

					CLOAK_CALL TextureProxy::TextureProxy(In Image* parent, In size_t entry, In_opt size_t buffer) : EntryProxy(parent, entry), m_buffer(buffer)
					{
						DEBUG_NAME(TextureProxy);
					}
					CLOAK_CALL TextureProxy::~TextureProxy()
					{

					}
					size_t CLOAK_CALL_THIS TextureProxy::GetBufferCount() const
					{
						FileData::Texture* tex = m_parent->GetTextureEntry(m_entry);
						if (tex != nullptr) { return tex->GetBufferCount(); }
						return 0;
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS TextureProxy::GetBuffer(In_opt size_t buffer) const
					{
						FileData::Texture* tex = m_parent->GetTextureEntry(m_entry);
						if (tex != nullptr) { return tex->GetBuffer(m_buffer + buffer); }
						return nullptr;
					}
					bool CLOAK_CALL_THIS TextureProxy::IsTransparentAt(In float X, In float Y, In_opt size_t buffer) const
					{
						FileData::Texture* tex = m_parent->GetTextureEntry(m_entry);
						if (tex != nullptr) { return tex->IsTransparentAt(X, Y, m_buffer + buffer); }
						return true;
					}
					bool CLOAK_CALL_THIS TextureProxy::IsBorderedImage(In_opt size_t buffer) const
					{
						FileData::Texture* tex = m_parent->GetTextureEntry(m_entry);
						if (tex != nullptr) { return tex->IsBorderedImage(m_buffer + buffer); }
						return false;
					}

					Success(return == true) bool CLOAK_CALL_THIS TextureProxy::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Files::Image_v2::ITexture)) { *ptr = (API::Files::Image_v2::ITexture*)this; return true; }
						else if (riid == __uuidof(Impl::Files::Image_v2::Proxys::TextureProxy)) { *ptr = (Impl::Files::Image_v2::Proxys::TextureProxy*)this; return true; }
						return EntryProxy::iQueryInterface(riid, ptr);
					}

					CLOAK_CALL TextureArrayProxy::TextureArrayProxy(In Image* parent, In size_t entry) : EntryProxy(parent, entry)
					{
						DEBUG_NAME(TextureArrayProxy);
					}
					CLOAK_CALL TextureArrayProxy::~TextureArrayProxy()
					{

					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS TextureArrayProxy::GetBuffer() const
					{
						FileData::TextureArray* tex = m_parent->GetArrayEntry(m_entry);
						if (tex != nullptr) { return tex->GetBuffer(); }
						return nullptr;
					}
					size_t CLOAK_CALL_THIS TextureArrayProxy::GetSize() const
					{
						FileData::TextureArray* tex = m_parent->GetArrayEntry(m_entry);
						if (tex != nullptr) { return tex->GetSize(); }
						return 0;
					}

					Success(return == true) bool CLOAK_CALL_THIS TextureArrayProxy::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Files::Image_v2::ITextureArray)) { *ptr = (API::Files::Image_v2::ITextureArray*)this; return true; }
						else if (riid == __uuidof(Impl::Files::Image_v2::Proxys::TextureArrayProxy)) { *ptr = (Impl::Files::Image_v2::Proxys::TextureArrayProxy*)this; return true; }
						return EntryProxy::iQueryInterface(riid, ptr);
					}

					CLOAK_CALL CubeMapProxy::CubeMapProxy(In Image* parent, In size_t entry) : TextureArrayProxy(parent, entry)
					{
						DEBUG_NAME(CubeMapProxy);
					}
					CLOAK_CALL CubeMapProxy::~CubeMapProxy()
					{

					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS CubeMapProxy::GetBuffer() const
					{
						FileData::CubeMap* tex = m_parent->GetCubeMapEntry(m_entry);
						if (tex != nullptr) { return tex->GetBuffer(); }
						return nullptr;
					}
					size_t CLOAK_CALL_THIS CubeMapProxy::GetSize() const { return 6; }

					Success(return == true) bool CLOAK_CALL_THIS CubeMapProxy::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Files::Image_v2::ICubeMap)) { *ptr = (API::Files::Image_v2::ICubeMap*)this; return true; }
						else if (riid == __uuidof(Impl::Files::Image_v2::Proxys::CubeMapProxy)) { *ptr = (Impl::Files::Image_v2::Proxys::CubeMapProxy*)this; return true; }
						return TextureArrayProxy::iQueryInterface(riid, ptr);
					}

					CLOAK_CALL SkyboxProxy::SkyboxProxy(In Image* parent, In size_t entry) : CubeMapProxy(parent, entry)
					{
						DEBUG_NAME(SkyboxProxy);
					}
					CLOAK_CALL SkyboxProxy::~SkyboxProxy()
					{

					}
					API::Rendering::IColorBuffer* CLOAK_CALL SkyboxProxy::GetLUT() const
					{
						FileData::Skybox* tex = m_parent->GetSkyboxEntry(m_entry);
						if (tex != nullptr) { return tex->GetLUT(); }
						return nullptr;
					}
					API::Rendering::IColorBuffer* CLOAK_CALL SkyboxProxy::GetRadianceMap() const
					{
						FileData::Skybox* tex = m_parent->GetSkyboxEntry(m_entry);
						if (tex != nullptr) { return tex->GetRadiance(); }
						return nullptr;
					}
					API::Rendering::IConstantBuffer* CLOAK_CALL SkyboxProxy::GetIrradiance() const
					{
						FileData::Skybox* tex = m_parent->GetSkyboxEntry(m_entry);
						if (tex != nullptr) { return tex->GetIrradiance(); }
						return nullptr;
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS SkyboxProxy::GetBuffer() const
					{
						FileData::Skybox* tex = m_parent->GetSkyboxEntry(m_entry);
						if (tex != nullptr) { return tex->GetBuffer(); }
						return nullptr;
					}

					Success(return == true) bool CLOAK_CALL_THIS SkyboxProxy::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Files::Image_v2::ISkybox)) { *ptr = (API::Files::Image_v2::ISkybox*)this; return true; }
						else if (riid == __uuidof(Impl::Files::Image_v2::Proxys::SkyboxProxy)) { *ptr = (Impl::Files::Image_v2::Proxys::SkyboxProxy*)this; return true; }
						return CubeMapProxy::iQueryInterface(riid, ptr);
					}

					CLOAK_CALL TextureAnimationProxy::TextureAnimationProxy(In Image* parent, In size_t entry) : TextureProxy(parent, entry, 0)
					{
						DEBUG_NAME(TextureAnimationProxy);
						m_started = 0;
						m_stoped = 0;
						m_offset = 0;
						m_autoRepeat = false;
						m_state = AnimState::STOPED;
					}
					CLOAK_CALL TextureAnimationProxy::~TextureAnimationProxy()
					{
						
					}
					size_t CLOAK_CALL_THIS TextureAnimationProxy::GetBufferCount() const { return TextureProxy::GetBufferCount() + 1; }
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS TextureAnimationProxy::GetBuffer(In_opt size_t buffer) const
					{
						API::Helper::Lock lock(m_sync);
						if (buffer == 0 && m_state != AnimState::STOPED)
						{
							FileData::Texture* tex = m_parent->GetTextureEntry(m_entry);
							if (tex != nullptr)
							{
								const API::Global::Time l = tex->GetAnimationLength();
								API::Global::Time t = 0;
								if (m_state == AnimState::RUNNING) { t = API::Global::Game::GetTimeStamp() - (m_started + m_offset); }
								else { t = m_stoped - (m_started + m_offset); }
								if (m_autoRepeat && l > 0) { t %= l; }
								return tex->GetBuffer(tex->GetBufferByTime(t));
							}
							return nullptr;
						}
						else { return TextureProxy::GetBuffer(buffer > 0 ? (buffer - 1) : 0); }
					}
					bool CLOAK_CALL_THIS TextureAnimationProxy::IsTransparentAt(In float X, In float Y, In_opt size_t buffer) const
					{
						API::Helper::Lock lock(m_sync);
						if (buffer == 0 && m_state != AnimState::STOPED)
						{
							FileData::Texture* tex = m_parent->GetTextureEntry(m_entry);
							if (tex != nullptr)
							{
								const API::Global::Time l = tex->GetAnimationLength();
								API::Global::Time t = 0;
								if (m_state == AnimState::RUNNING) { t = API::Global::Game::GetTimeStamp() - (m_started + m_offset); }
								else { t = m_stoped - (m_started + m_offset); }
								if (m_autoRepeat && l > 0) { t %= l; }
								return tex->IsTransparentAt(X, Y, tex->GetBufferByTime(t));
							}
							return true;
						}
						else { return TextureProxy::IsTransparentAt(X, Y, buffer > 0 ? (buffer - 1) : 0); }
					}
					bool CLOAK_CALL_THIS TextureAnimationProxy::IsBorderedImage(In_opt size_t buffer) const
					{
						API::Helper::Lock lock(m_sync);
						if (buffer == 0 && m_state != AnimState::STOPED)
						{
							FileData::Texture* tex = m_parent->GetTextureEntry(m_entry);
							if (tex != nullptr)
							{
								const API::Global::Time l = tex->GetAnimationLength();
								API::Global::Time t = 0;
								if (m_state == AnimState::RUNNING) { t = API::Global::Game::GetTimeStamp() - (m_started + m_offset); }
								else { t = m_stoped - (m_started + m_offset); }
								if (m_autoRepeat && l > 0) { t %= l; }
								return tex->IsBorderedImage(tex->GetBufferByTime(t));
							}
							return false;
						}
						else { return TextureProxy::IsBorderedImage(buffer > 0 ? (buffer - 1) : 0); }
					}
					API::Global::Time CLOAK_CALL_THIS TextureAnimationProxy::GetLength() const
					{
						FileData::Texture* tex = m_parent->GetTextureEntry(m_entry);
						if (tex != nullptr) { return tex->GetAnimationLength(); }
						return 0;
					}
					void CLOAK_CALL_THIS TextureAnimationProxy::Start()
					{
						API::Helper::Lock lock(m_sync);
						if (m_state == AnimState::STOPED)
						{
							m_started = API::Global::Game::GetTimeStamp();
						}
						else if (m_state == AnimState::PAUSED)
						{
							m_offset += API::Global::Game::GetTimeStamp() - m_stoped;
						}
						m_state = AnimState::RUNNING;
					}
					void CLOAK_CALL_THIS TextureAnimationProxy::Pause()
					{
						API::Helper::Lock lock(m_sync);
						if (m_state == AnimState::RUNNING)
						{
							m_stoped = API::Global::Game::GetTimeStamp();
							m_state = AnimState::PAUSED;
						}
					}
					void CLOAK_CALL_THIS TextureAnimationProxy::Resume()
					{
						API::Helper::Lock lock(m_sync);
						if (m_state == AnimState::PAUSED)
						{
							m_offset += API::Global::Game::GetTimeStamp() - m_stoped;
							m_state = AnimState::RUNNING;
						}
					}
					void CLOAK_CALL_THIS TextureAnimationProxy::Stop()
					{
						API::Helper::Lock lock(m_sync);
						m_offset = 0;
						m_state = AnimState::STOPED;
					}
					void CLOAK_CALL_THIS TextureAnimationProxy::SetAutoRepeat(In bool repeat)
					{
						API::Helper::Lock lock(m_sync);
						m_autoRepeat = repeat;
					}
					bool CLOAK_CALL_THIS TextureAnimationProxy::GetAutoRepeat() const
					{
						API::Helper::Lock lock(m_sync);
						return m_autoRepeat;
					}
					bool CLOAK_CALL_THIS TextureAnimationProxy::IsPaused()
					{
						API::Helper::Lock lock(m_sync);
						return m_state == AnimState::PAUSED;
					}
					bool CLOAK_CALL_THIS TextureAnimationProxy::IsRunning()
					{
						API::Helper::Lock lock(m_sync);
						return m_state == AnimState::RUNNING;
					}
					
					Success(return == true) bool CLOAK_CALL_THIS TextureAnimationProxy::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Files::Image_v2::ITextureAnmiation)) { *ptr = (API::Files::Image_v2::ITextureAnmiation*)this; return true; }
						else if (riid == __uuidof(API::Files::Animation_v1::IAnimation)) { *ptr = (API::Files::Animation_v1::IAnimation*)this; return true; }
						else if (riid == __uuidof(Impl::Files::Image_v2::Proxys::TextureAnimationProxy)) { *ptr = (Impl::Files::Image_v2::Proxys::TextureAnimationProxy*)this; return true; }
						return TextureProxy::iQueryInterface(riid, ptr);
					}

					CLOAK_CALL MaterialProxy::MaterialProxy(In Image* parent, In size_t entry) : EntryProxy(parent, entry)
					{
						DEBUG_NAME(MaterialProxy);
					}
					CLOAK_CALL MaterialProxy::~MaterialProxy()
					{

					}
					API::Files::Image_v2::MaterialType CLOAK_CALL_THIS MaterialProxy::GetType() const
					{
						FileData::Material* tex = m_parent->GetMaterialEntry(m_entry);
						if (tex != nullptr) { return tex->GetMaterialType(); }
						return API::Files::Image_v2::MaterialType::Unknown;
					}
					bool CLOAK_CALL_THIS MaterialProxy::SupportTesslation() const
					{
						FileData::Material* tex = m_parent->GetMaterialEntry(m_entry);
						if (tex != nullptr) { return tex->SupportTesslation(); }
						return false;
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS MaterialProxy::GetAlbedo() const
					{
						FileData::Material* tex = m_parent->GetMaterialEntry(m_entry);
						if (tex != nullptr) { return tex->GetBuffer(0); }
						return nullptr;
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS MaterialProxy::GetNormalMap() const
					{
						FileData::Material* tex = m_parent->GetMaterialEntry(m_entry);
						if (tex != nullptr) { return tex->GetBuffer(1); }
						return nullptr;
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS MaterialProxy::GetSpecularMap() const
					{
						FileData::Material* tex = m_parent->GetMaterialEntry(m_entry);
						if (tex != nullptr) { return tex->GetBuffer(2); }
						return nullptr;
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS MaterialProxy::GetDisplacementMap() const
					{
						FileData::Material* tex = m_parent->GetMaterialEntry(m_entry);
						if (tex != nullptr) { return tex->GetBuffer(3); }
						return nullptr;
					}
					API::Rendering::ResourceView CLOAK_CALL_THIS MaterialProxy::GetSRVMaterial() const
					{
						FileData::Material* tex = m_parent->GetMaterialEntry(m_entry);
						if (tex != nullptr) { return tex->GetBuffer(0)->GetSRV(); }
						return API::Rendering::ResourceView();
					}
					API::Rendering::ResourceView CLOAK_CALL_THIS MaterialProxy::GetSRVDisplacement() const
					{
						FileData::Material* tex = m_parent->GetMaterialEntry(m_entry);
						if (tex != nullptr) 
						{ 
							API::Rendering::IColorBuffer* buf = tex->GetBuffer(3);
							if (buf != nullptr) { return buf->GetSRV(); }
						}
						return API::Rendering::ResourceView();
					}

					Success(return == true) bool CLOAK_CALL_THIS MaterialProxy::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Files::Image_v2::IMaterial)) { *ptr = (API::Files::Image_v2::IMaterial*)this; return true; }
						else if (riid == __uuidof(Impl::Files::Image_v2::Proxys::MaterialProxy)) { *ptr = (Impl::Files::Image_v2::Proxys::MaterialProxy*)this; return true; }
						return EntryProxy::iQueryInterface(riid, ptr);
					}
				}
				namespace FileData {
					static constexpr uint8_t U8_MAX = static_cast<uint8_t>((1 << 8) - 1);
					static constexpr uint16_t U16_MAX = static_cast<uint16_t>((1 << 16) - 1);

					struct {
						CE::RefPointer<API::Helper::ISyncSection> Sync = nullptr;
						CE::RefPointer<Impl::Rendering::IColorBuffer> Texture = nullptr;
						uint32_t W = 0;
						uint32_t H = 0;
					} g_IBLLUT;
					//------------------------------------------------------------
					//------------------------BitBufferPos------------------------
					//------------------------------------------------------------
					CLOAK_CALL BitBufferPos::BitBufferPos(In const BitBufferPos& p)
					{
						m_mbw = p.m_mbw;
						m_ptr = p.m_ptr;
						for (size_t a = 0; a < 4; a++) { m_off[a] = p.m_off[a]; }
					}
					BitBufferPos& CLOAK_CALL_THIS BitBufferPos::operator=(In const BitBufferPos& p)
					{
						m_mbw = p.m_mbw;
						m_ptr = p.m_ptr;
						for (size_t a = 0; a < 4; a++) { m_off[a] = p.m_off[a]; }
						return *this;
					}
					float CLOAK_CALL_THIS BitBufferPos::Get(In size_t c) const
					{
						if (c < 4 && m_off[c] > 0)
						{
							if (m_mbw == 1)
							{
								uint8_t* p = reinterpret_cast<uint8_t*>(m_ptr + m_off[c] - 1);
								return static_cast<float>(*p) / U8_MAX;
							}
							else if (m_mbw == 2)
							{
								uint16_t* p = reinterpret_cast<uint16_t*>(m_ptr + m_off[c] - 1);
								if (m_hr == true)
								{
									return Half::FromHalf(*p);
								}
								else
								{
									return static_cast<float>(*p) / U16_MAX;
								}
							}
							else if (m_mbw == 4)
							{
								float* p = reinterpret_cast<float*>(m_ptr + m_off[c] - 1);
								return *p;
							}
						}
						return c < 3 ? 0.0f : 1.0f;
					}
					void CLOAK_CALL_THIS BitBufferPos::Set(In float v, In size_t c)
					{
						if (c < 4 && m_off[c] > 0)
						{
							if (m_mbw == 1)
							{
								uint8_t* p = reinterpret_cast<uint8_t*>(m_ptr + m_off[c] - 1);
								*p = static_cast<uint8_t>(U8_MAX*v);
							}
							else if (m_mbw == 2)
							{
								uint16_t* p = reinterpret_cast<uint16_t*>(m_ptr + m_off[c] - 1);
								if (m_hr == true)
								{
									*p = Half::ToHalf(v);
								}
								else
								{
									*p = static_cast<uint16_t>(U16_MAX*v);
								}
							}
							else if (m_mbw == 4)
							{
								float* p = reinterpret_cast<float*>(m_ptr + m_off[c] - 1);
								*p = v;
							}
						}
					}
					CLOAK_CALL BitBufferPos::BitBufferPos(In void* ptr, In uint8_t channels, In uint8_t mbw, In bool hr)
					{
						m_ptr = reinterpret_cast<uintptr_t>(ptr);
						m_mbw = mbw;
						m_hr = hr;
						if (ptr != nullptr)
						{
							for (uint8_t a = 0, o = 0; a < 4; a++)
							{
								if ((channels & (1 << a)) != 0)
								{
									m_off[a] = o + 1;
									o += m_mbw;
								}
								else { m_off[a] = 0; }
							}
						}
						else
						{
							for (size_t a = 0; a < 4; a++) { m_off[a] = 0; }
						}
					}


					//------------------------------------------------------------
					//--------------------------BitBuffer-------------------------
					//------------------------------------------------------------
					CLOAK_CALL BitBuffer::BitBuffer()
					{
						m_W = 0;
						m_H = 0;
						m_data = nullptr;
						m_channels = 0;
						m_mbw = 0;
						m_boff = 0;
					}
					CLOAK_CALL BitBuffer::BitBuffer(In const BitBuffer& o)
					{
						m_W = o.m_W;
						m_H = o.m_H;
						m_channels = o.m_channels;
						m_mbw = o.m_mbw;
						m_boff = o.m_boff;
						size_t s = static_cast<size_t>(m_W)*m_H*m_boff;
						m_data = NewArray(uint8_t, s);
						for (size_t a = 0; a < s; a++) { m_data[a] = o.m_data[a]; }
					}
					CLOAK_CALL BitBuffer::~BitBuffer()
					{
						DeleteArray(m_data);
					}
					BitBuffer& CLOAK_CALL_THIS BitBuffer::operator=(In const BitBuffer& o)
					{
						m_W = o.m_W;
						m_H = o.m_H;
						m_channels = o.m_channels;
						m_mbw = o.m_mbw;
						m_boff = o.m_boff;
						size_t s = static_cast<size_t>(m_W)*m_H*m_boff;
						m_data = NewArray(uint8_t, s);
						for (size_t a = 0; a < s; a++) { m_data[a] = o.m_data[a]; }
						return *this;
					}
					void CLOAK_CALL_THIS BitBuffer::Reset(In UINT W, In UINT H, In uint8_t channels, In const BitData& bits, In bool hr)
					{
						DeleteArray(m_data);
						if (W > 0 && H > 0 && channels != 0)
						{
							m_W = W;
							m_H = H;
							m_hr = hr;
							m_channels = channels;
							size_t mbw = 0;
							for (size_t a = 0; a < 4; a++) { mbw = max(mbw, bits.Bits[a]); }
							size_t cc = 0;
							if ((m_channels & 0x1) != 0) { cc++; }
							if ((m_channels & 0x2) != 0) { cc++; }
							if ((m_channels & 0x4) != 0) { cc++; }
							if ((m_channels & 0x8) != 0) { cc++; }
							if (cc == 3) { cc = 4; }
							if (mbw <= 8 && hr == false) { m_mbw = 1; }
							else if (mbw <= 16) { m_mbw = 2; }
							else if (mbw <= 32) { m_mbw = 4; }
							m_boff = m_mbw*cc;
							size_t s = static_cast<size_t>(m_W)*m_H*m_boff;
							m_data = NewArray(uint8_t, s);

							if (cc == 4 && (m_channels & 0x8) == 0)
							{
								const uintptr_t dp = reinterpret_cast<uintptr_t>(m_data);
								const size_t dof = 3 * m_mbw;
								for (UINT y = 0; y < m_H; y++)
								{
									for (UINT x = 0; x < m_W; x++)
									{
										const size_t p = (((static_cast<size_t>(y)*m_W) + x)*m_boff) + dof;
										if (m_mbw == 1)
										{
											uint8_t* d = reinterpret_cast<uint8_t*>(dp + p);
											*d = U8_MAX;
										}
										else if (m_mbw == 2)
										{
											uint16_t* d = reinterpret_cast<uint16_t*>(dp + p);
											if (m_hr == true)
											{
												*d = Half::ToHalf(1.0f);
											}
											else
											{
												*d = U16_MAX;
											}
										}
										else if (m_mbw == 4)
										{
											float* d = reinterpret_cast<float*>(dp + p);
											*d = 1.0f;
										}
									}
								}
							}
						}
						else
						{
							m_W = 0;
							m_H = 0;
							m_channels = 0;
							m_mbw = 0;
							m_boff = 0;
							m_hr = false;
							m_data = nullptr;
						}
					}
					void CLOAK_CALL_THIS BitBuffer::Reset()
					{
						DeleteArray(m_data);
						m_W = 0;
						m_H = 0;
						m_channels = 0;
						m_mbw = 0;
						m_boff = 0;
						m_hr = false;
						m_data = nullptr;
					}
					void* CLOAK_CALL_THIS BitBuffer::GetBinaryData() const { return m_data; }
					LONG_PTR CLOAK_CALL_THIS BitBuffer::GetRowPitch() const { return static_cast<LONG_PTR>(m_W)*m_boff; }
					LONG_PTR CLOAK_CALL_THIS BitBuffer::GetSlicePitch() const { return static_cast<LONG_PTR>(m_W)*m_boff*m_H; }
					BitBufferPos CLOAK_CALL_THIS BitBuffer::Get(In UINT x, In UINT y)
					{
						if (m_data == nullptr) { return BitBufferPos(nullptr, m_channels, m_mbw, m_hr); }
						size_t off = m_boff * ((y*m_W) + x);
						return BitBufferPos(&m_data[off], m_channels, m_mbw, m_hr);
					}
					const BitBufferPos CLOAK_CALL_THIS BitBuffer::Get(In UINT x, In UINT y) const
					{
						if (m_data == nullptr) { return BitBufferPos(nullptr, m_channels, m_mbw, m_hr); }
						size_t off = m_boff * ((y*m_W) + x);
						return BitBufferPos(&m_data[off], m_channels, m_mbw, m_hr);
					}
					UINT CLOAK_CALL_THIS BitBuffer::GetWidth() const { return m_W; }
					UINT CLOAK_CALL_THIS BitBuffer::GetHeight() const { return m_H; }


					//------------------------------------------------------------
					//--------------------------AlphaMap--------------------------
					//------------------------------------------------------------
					CLOAK_CALL AlphaMap::AlphaMap()
					{
						m_W = 0;
						m_H = 0;
						m_S = 0;
						m_map = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					}
					CLOAK_CALL AlphaMap::~AlphaMap()
					{
						delete m_map;
						SAVE_RELEASE(m_sync);
					}
					void CLOAK_CALL_THIS AlphaMap::Resize(In UINT W, In UINT H, In API::Files::IReader* read, In API::Files::FileVersion version)
					{
						API::Helper::Lock lock(m_sync);
						const size_t s = static_cast<size_t>(W)*H;
						if (W != m_W || H != m_H || s != m_S)
						{
							delete m_map;
							m_S = s;
							m_W = W;
							m_H = H;
							m_map = new Engine::BitMap(s);
						}
						for (size_t a = 0; a < s; a++)
						{
							m_map->set(a, read->ReadBits(1) == 1);
						}
					}
					bool CLOAK_CALL_THIS AlphaMap::IsTransparentAt(In float X, In float Y) const
					{
						API::Helper::Lock lock(m_sync);
						if (X < 0 || X>1 || Y < 0 || Y>1) { return true; }
						const long double xfr = (static_cast<long double>(X)*m_W) - 0.5L;
						const long double yfr = (static_cast<long double>(Y)*m_H) - 0.5L;
						const long double xf = max(xfr, 0);
						const long double yf = max(yfr, 0);
						bool isT[4];
						const size_t ix = static_cast<size_t>(xf);
						const size_t iy = static_cast<size_t>(yf);
						size_t p[4];
						p[0] = ix + (iy*m_W);
						p[1] = p[0] + 1;
						p[2] = p[0] + m_W;
						p[3] = p[2] + 1;
						if (p[0] >= m_S) { return true; }
						const long double xr = xf - static_cast<long long>(xf);
						const long double yr = yf - static_cast<long long>(yf);
						if (xr == 0 || p[1] >= m_S)
						{
							if (yr == 0 || p[2] >= m_S) { isT[0] = isT[1] = isT[2] = isT[3] = m_map->isAt(p[0]); }
							else
							{
								isT[0] = isT[1] = m_map->isAt(p[0]);
								isT[2] = isT[3] = m_map->isAt(p[2]);
							}
						}
						else
						{
							if (yr == 0 || p[2] >= m_S) 
							{ 
								isT[0] = isT[2] = m_map->isAt(p[0]);
								isT[1] = isT[3] = m_map->isAt(p[1]);
							}
							else
							{
								for (size_t a = 0; a < 4; a++) { isT[a] = m_map->isAt(p[1]); }
							}
						}
						const long double x0 = ((isT[0] ? 1 : 0)*xr) + ((isT[1] ? 1 : 0)*(1 - xr));
						const long double x1 = ((isT[2] ? 1 : 0)*xr) + ((isT[2] ? 1 : 0)*(1 - xr));
						const long double r = (x0 * yr) + (x1 * (1 - yr));
						return r > 0.5L;
					}

					inline Impl::Rendering::IColorBuffer* CLOAK_CALL CreateTexture(In Impl::Rendering::IManager* man, In API::Rendering::CONTEXT_TYPE ct, In size_t nodeID, In const API::Rendering::TEXTURE_DESC& desc)
					{
						API::Rendering::IColorBuffer* c = man->CreateColorBuffer(ct, static_cast<uint32_t>(nodeID), desc);
						Impl::Rendering::IColorBuffer* r = nullptr;
						HRESULT hRet=c->QueryInterface(CE_QUERY_ARGS(&r));
						CLOAK_ASSUME(SUCCEEDED(hRet));
						SAVE_RELEASE(c);
						return r;
					}

					//------------------------------------------------------------
					//----------------------------Entry---------------------------
					//------------------------------------------------------------
					CLOAK_CALL Entry::Entry() {}
					CLOAK_CALL Entry::~Entry() {}
					void* CLOAK_CALL Entry::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
					void CLOAK_CALL Entry::operator delete(In void* ptr) { return API::Global::Memory::MemoryPool::Free(ptr); }


					//------------------------------------------------------------
					//---------------------------Texture--------------------------
					//------------------------------------------------------------
					CLOAK_CALL Texture::Texture()
					{
						m_buffer = nullptr;
						m_alpha = nullptr;
						m_meta = nullptr;
						m_bufferSize = 0;
					}
					CLOAK_CALL Texture::~Texture()
					{
						for (size_t a = 0; a < m_bufferSize; a++) { SAVE_RELEASE(m_buffer[a].Buffer); }
						DeleteArray(m_buffer);
						DeleteArray(m_alpha);
						DeleteArray(m_meta);
					}
					size_t CLOAK_CALL_THIS Texture::GetBufferCount() const { return m_bufferSize; }
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS Texture::GetBuffer(In_opt size_t buffer) const
					{
						if (buffer < m_bufferSize) { return m_buffer[buffer].Buffer; }
						return nullptr;
					}
					bool CLOAK_CALL_THIS Texture::IsTransparentAt(In float X, In float Y, In_opt size_t buffer) const 
					{
						if (buffer < m_bufferSize && m_alpha != nullptr) { return m_alpha[buffer].IsTransparentAt(X, Y); }
						return false;
					}
					bool CLOAK_CALL_THIS Texture::IsBorderedImage(In_opt size_t buffer) const 
					{
						if (buffer < m_bufferSize) { return (m_meta[buffer].Flags & 0x2) != 0; }
						return false;
					}
					size_t CLOAK_CALL_THIS Texture::GetBufferByTime(In API::Global::Time time) const { return 0; }
					API::Global::Time CLOAK_CALL_THIS Texture::GetAnimationLength() const { return 0; }
					void CLOAK_CALL_THIS Texture::readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQS)
					{
						if (version >= 1008)
						{
							m_bufferSize = static_cast<size_t>(read->ReadDynamic());
							API::Global::Graphic::TextureQuality maxQuality = static_cast<API::Global::Graphic::TextureQuality>(read->ReadBits(2));
							*maxQS = maxQuality;
							const bool useAlpha = read->ReadBits(1) == 1;
							m_channels = static_cast<uint8_t>(read->ReadBits(4));
							API::Rendering::DescriptorPageType heap = v1008::ReadHeap(read);
							const bool dynamicSize = read->ReadBits(1) == 1;
							m_buffer = NewArray(ColorBufferData<BufferData>, m_bufferSize);
							m_meta = NewArray(Meta, m_bufferSize);
							if (useAlpha) { m_alpha = NewArray(AlphaMap, m_bufferSize); }
							else { m_alpha = nullptr; }
							UINT lW = 0;
							UINT lH = 0;
							UINT oW = 0;
							UINT oH = 0;
							//Notice: Substraction of qualitys is capped at 0 automaticly
							const size_t levelOffset = maxQuality - quality;
							size_t levelCount = 0;
							API::Rendering::TEXTURE_DESC desc;
							Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
							for (size_t a = 0; a < m_bufferSize; a++)
							{
								v1008::ReadMeta(read, version, &m_meta[a]);
								if (a == 0 || dynamicSize)
								{
									levelCount = static_cast<size_t>(read->ReadBits(10));
									oW = static_cast<UINT>(read->ReadDynamic());
									oH = static_cast<UINT>(read->ReadDynamic());
									if (levelCount > levelOffset) { levelCount -= levelOffset; }
									else { levelCount = 1; }
									lW = oW >> levelOffset;
									lH = oH >> levelOffset;
									if (lW == 0) { lW = 1; }
									if (lH == 0) { lH = 1; }
								}
								if (useAlpha) { m_alpha[a].Resize(oW, oH, read, version); }
								m_buffer[a].MaxLayerCount = levelCount;
								m_buffer[a].LoadedLayers = 0;
								m_buffer[a].LastLayer.Reset();
								desc.Width = lW;
								desc.Height = lH;
								desc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE;
								desc.Format = v1008::FindFormat(m_channels, bits, m_meta[a].ColorSpace, m_meta[a].HR);
								desc.AccessType = API::Rendering::VIEW_TYPE::SRV;
								desc.clearValue = API::Helper::Color::Black;
								desc.Texture.MipMaps = static_cast<uint32_t>(levelCount);
								desc.Texture.SampleCount = 1;
								m_buffer[a].Buffer = CreateTexture(man, API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, desc);
								man->GenerateViews(&m_buffer[a].Buffer, 1, API::Rendering::VIEW_TYPE::SRV, heap);
							}
						}
					}
					bool CLOAK_CALL_THIS Texture::RequireLoad(In API::Files::LOD reqLOD)
					{
						size_t mrl = 0;
						size_t mll = 0;
						for (size_t a = 0; a < m_bufferSize; a++)
						{
							const size_t rl = m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers;
							mrl = max(mrl, rl);
							if (a == 0 || mll > m_buffer[a].LoadedLayers) { mll = m_buffer[a].LoadedLayers; }
						}
						if (mrl <= static_cast<size_t>(reqLOD) && mll > 0) { return false; }
						return true;
					}
					bool CLOAK_CALL_THIS Texture::readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD)
					{
						size_t mrl = 0;
						for (size_t a = 0; a < m_bufferSize; a++)
						{
							const size_t rl = m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers;
							mrl = max(mrl, rl);
						}
						const bool hasLayer = read->ReadBits(1) == 1;
						if (hasLayer == false) { return false; }
						size_t l = m_bufferSize;
						API::Rendering::SUBRESOURCE_DATA srd;
						Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
						const size_t nrl = mrl - 1;
						for (size_t a = 0; a < m_bufferSize; a++)
						{
							if (m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers == mrl)
							{
								FileData::BitBuffer* prevTex = nullptr;
								if (l < m_bufferSize) { prevTex = &m_buffer[l].LastLayer; }
								v1008::DecodeTextureLayer(read, version, &m_buffer[a], m_channels, bits, m_meta[a], prevTex);
								srd.pData = m_buffer[a].LastLayer.GetBinaryData();
								srd.RowPitch = m_buffer[a].LastLayer.GetRowPitch();
								srd.SlicePitch = m_buffer[a].LastLayer.GetSlicePitch();
								man->UpdateTextureSubresource(m_buffer[a].Buffer, m_buffer[a].Buffer->GetSubresourceIndex(static_cast<UINT>(nrl)), 1, &srd, true);
								m_buffer[a].LoadedLayers++;
								l = a;
							}
						}
						for (size_t a = 0; a < m_bufferSize; a++)
						{
							if (m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers == nrl) { m_buffer[a].Buffer->SetDetailLevel(static_cast<uint32_t>(nrl)); }
						}
						if (mrl == 1)
						{
							for (size_t a = 0; a < m_bufferSize; a++) { m_buffer[a].LastLayer.Reset(); }
						}
						return nrl > static_cast<size_t>(reqLOD);
					}


					//------------------------------------------------------------
					//------------------------TextureArray------------------------
					//------------------------------------------------------------
					CLOAK_CALL TextureArray::TextureArray()
					{
						m_channels = 0;
						m_meta = nullptr;
					}
					CLOAK_CALL TextureArray::~TextureArray()
					{
						SAVE_RELEASE(m_buffer.Buffer);
						DeleteArray(m_buffer.LastLayers);
						DeleteArray(m_meta);
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS TextureArray::GetBuffer() const { return m_buffer.Buffer; }
					size_t CLOAK_CALL_THIS TextureArray::GetSize() const { return m_buffer.ArraySize; }
					void CLOAK_CALL_THIS TextureArray::readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQS)
					{
						if (version >= 1008)
						{
							m_buffer.ArraySize = static_cast<size_t>(read->ReadDynamic());
							API::Global::Graphic::TextureQuality maxQuality = static_cast<API::Global::Graphic::TextureQuality>(read->ReadBits(2));
							*maxQS = maxQuality;
							m_channels = static_cast<uint8_t>(read->ReadBits(4));
							API::Rendering::DescriptorPageType heap = v1008::ReadHeap(read);
							uint16_t levelCount = static_cast<uint16_t>(read->ReadBits(10));
							UINT lW = static_cast<UINT>(read->ReadDynamic());
							UINT lH = static_cast<UINT>(read->ReadDynamic());
							const uint16_t levelOffset = maxQuality - quality;
							if (levelCount > levelOffset) { levelCount -= levelOffset; }
							else { levelCount = 1; }
							lW >>= levelOffset;
							lH >>= levelOffset;
							if (lW == 0) { lW = 1; }
							if (lH == 0) { lH = 1; }

							m_meta = NewArray(Meta, m_buffer.ArraySize);
							for (size_t a = 0; a < m_buffer.ArraySize; a++) { v1008::ReadMeta(read, version, &m_meta[a]); }

							API::Rendering::TEXTURE_DESC desc;
							Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
							m_buffer.MaxLayerCount = levelCount;
							m_buffer.LoadedLayers = 0;
							m_buffer.LastLayers = NewArray(FileData::BitBuffer, m_buffer.ArraySize);
							for (size_t a = 0; a < 6; a++) { m_buffer.LastLayers[a].Reset(); }
							desc.Width = lW;
							desc.Height = lH;
							desc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE_ARRAY;
							desc.Format = v1008::FindFormat(m_channels, bits, v1008::FindBiggestColorSpace(m_meta, m_buffer.ArraySize, bits), m_meta[0].HR);
							desc.AccessType = API::Rendering::VIEW_TYPE::SRV;
							desc.clearValue = API::Helper::Color::Black;
							desc.TextureArray.Images = static_cast<uint32_t>(m_buffer.ArraySize);
							desc.TextureArray.MipMaps = levelCount;
							m_buffer.Buffer = CreateTexture(man, API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, desc);
							man->GenerateViews(&m_buffer.Buffer, 1, API::Rendering::VIEW_TYPE::SRV, heap);
						}
					}
					bool CLOAK_CALL_THIS TextureArray::RequireLoad(In API::Files::LOD reqLOD)
					{
						const size_t mrl = m_buffer.MaxLayerCount - m_buffer.LoadedLayers;
						if (mrl <= static_cast<size_t>(reqLOD) && m_buffer.LastLayers > 0) { return false; }
						return true;
					}
					bool CLOAK_CALL_THIS TextureArray::readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD)
					{
						const size_t mrl = m_buffer.MaxLayerCount - m_buffer.LoadedLayers;
						const bool hasLayer = read->ReadBits(1) == 1;
						if (hasLayer == false) { return false; }
						API::Rendering::SUBRESOURCE_DATA srd;
						Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
						const FileData::ColorSpace space = v1008::FindBiggestColorSpace(m_meta, m_buffer.ArraySize, bits);
						const size_t nrl = mrl - 1;
						for (size_t a = 0; a < m_buffer.ArraySize; a++)
						{
							FileData::BitBuffer* prevTex = nullptr;
							if (a > 0) { prevTex = &m_buffer.LastLayers[a - 1]; }
							v1008::DecodeArrayLayer(read, version, &m_buffer, a, m_channels, bits, m_meta[a], space, prevTex);
							srd.pData = m_buffer.LastLayers[a].GetBinaryData();
							srd.RowPitch = m_buffer.LastLayers[a].GetRowPitch();
							srd.SlicePitch = m_buffer.LastLayers[a].GetSlicePitch();
							man->UpdateTextureSubresource(m_buffer.Buffer, m_buffer.Buffer->GetSubresourceIndex(static_cast<UINT>(nrl), static_cast<UINT>(a)), 1, &srd, true);
						}
						m_buffer.Buffer->SetDetailLevel(static_cast<uint32_t>(nrl));
						m_buffer.LoadedLayers++;
						return nrl > static_cast<size_t>(reqLOD);
					}


					//------------------------------------------------------------
					//--------------------------CubeMap---------------------------
					//------------------------------------------------------------
					CLOAK_CALL CubeMap::CubeMap()
					{
						m_channels = 0;
					}
					CLOAK_CALL CubeMap::~CubeMap()
					{
						SAVE_RELEASE(m_buffer.Buffer);
						DeleteArray(m_buffer.LastLayers);
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS CubeMap::GetBuffer() const { return m_buffer.Buffer; }
					void CLOAK_CALL_THIS CubeMap::readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQS)
					{
						if (version >= 1008)
						{
							API::Global::Graphic::TextureQuality maxQuality = static_cast<API::Global::Graphic::TextureQuality>(read->ReadBits(2));
							*maxQS = maxQuality;
							m_channels = static_cast<uint8_t>(read->ReadBits(4));
							API::Rendering::DescriptorPageType heap = v1008::ReadHeap(read);
							uint16_t levelCount = static_cast<uint16_t>(read->ReadBits(10));
							UINT lW = static_cast<UINT>(read->ReadDynamic());
							const uint16_t levelOffset = maxQuality - quality;
							if (levelCount > levelOffset) { levelCount -= levelOffset; }
							else { levelCount = 1; }
							lW >>= levelOffset;
							if (lW == 0) { lW = 1; }
							const UINT lH = lW;
							for (size_t a = 0; a < 6; a++) { v1008::ReadMeta(read, version, &m_meta[a]); }


							API::Rendering::TEXTURE_DESC desc;
							Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
							m_buffer.MaxLayerCount = levelCount;
							m_buffer.LoadedLayers = 0;
							m_buffer.ArraySize = 6;
							m_buffer.LastLayers = NewArray(FileData::BitBuffer, 6);
							for (size_t a = 0; a < 6; a++) { m_buffer.LastLayers[a].Reset(); }
							desc.Width = lW;
							desc.Height = lH;
							desc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE_CUBE;
							desc.Format = v1008::FindFormat(m_channels, bits, v1008::FindBiggestColorSpace(m_meta, 6, bits), m_meta[0].HR);
							desc.AccessType = API::Rendering::VIEW_TYPE::SRV;
							desc.clearValue = API::Helper::Color::Black;
							desc.TextureCube.MipMaps = levelCount;
							m_buffer.Buffer = CreateTexture(man, API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, desc);
							man->GenerateViews(&m_buffer.Buffer, 1, API::Rendering::VIEW_TYPE::SRV, heap);
						}
					}
					bool CLOAK_CALL_THIS CubeMap::RequireLoad(In API::Files::LOD reqLOD)
					{
						const size_t mrl = m_buffer.MaxLayerCount - m_buffer.LoadedLayers;
						if (mrl <= static_cast<size_t>(reqLOD) && m_buffer.LastLayers > 0) { return false; }
						return true;
					}
					bool CLOAK_CALL_THIS CubeMap::readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD)
					{
						const size_t mrl = m_buffer.MaxLayerCount - m_buffer.LoadedLayers;
						const bool hasLayer = read->ReadBits(1) == 1;
						if (hasLayer == false) { return false; }
						API::Rendering::SUBRESOURCE_DATA srd;
						Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
						const FileData::ColorSpace space = v1008::FindBiggestColorSpace(m_meta, m_buffer.ArraySize, bits);
						const size_t nrl = mrl - 1;
						for (size_t a = 0; a < 6; a++)
						{
							FileData::BitBuffer* prevTex = nullptr;
							if (a > 0) { prevTex = &m_buffer.LastLayers[a - 1]; }
							v1008::DecodeArrayLayer(read, version, &m_buffer, a, m_channels, bits, m_meta[a], space, prevTex);
							srd.pData = m_buffer.LastLayers[a].GetBinaryData();
							srd.RowPitch = m_buffer.LastLayers[a].GetRowPitch();
							srd.SlicePitch = m_buffer.LastLayers[a].GetSlicePitch();
							man->UpdateTextureSubresource(m_buffer.Buffer, m_buffer.Buffer->GetSubresourceIndex(static_cast<UINT>(nrl), static_cast<UINT>(a)), 1, &srd, true);
						}
						m_buffer.Buffer->SetDetailLevel(static_cast<uint32_t>(nrl));
						m_buffer.LoadedLayers++;
						return nrl > static_cast<size_t>(reqLOD);
					}


					//------------------------------------------------------------
					//---------------------------Skybox---------------------------
					//------------------------------------------------------------
					struct CLOAK_ALIGN IrradianceData {
						API::Rendering::MATRIX M[3];
						UINT FirstSumMipLevels;
						float RadianceScale;
						float IrradianceScale;
					};

					CLOAK_CALL Skybox::Skybox()
					{
						m_channels = 0;
					}
					CLOAK_CALL Skybox::~Skybox()
					{
						SAVE_RELEASE(m_radiance.Buffer);
						SAVE_RELEASE(m_irradiance);
						DeleteArray(m_radiance.LastLayers);
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS Skybox::GetBuffer() const
					{
						return m_buffer.Buffer;
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS Skybox::GetRadiance() const
					{
						return m_radiance.Buffer;
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS Skybox::GetLUT() const
					{
						API::Helper::ReadLock lock(g_IBLLUT.Sync);
						return g_IBLLUT.Texture.Get();
					}
					API::Rendering::IConstantBuffer* CLOAK_CALL_THIS Skybox::GetIrradiance() const
					{
						return m_irradiance;
					}
					void CLOAK_CALL_THIS Skybox::readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQS)
					{
						if (version >= 1011)
						{
							API::Global::Graphic::TextureQuality maxQuality = static_cast<API::Global::Graphic::TextureQuality>(read->ReadBits(2));
							*maxQS = maxQuality;
							m_channels = static_cast<uint8_t>(read->ReadBits(4));
							API::Rendering::DescriptorPageType heap = v1008::ReadHeap(read);
							uint16_t levelCount = static_cast<uint16_t>(read->ReadBits(10));
							UINT lW = static_cast<UINT>(read->ReadDynamic());
							const uint16_t levelOffset = maxQuality - quality;
							if (levelCount > levelOffset) { levelCount -= levelOffset; }
							else { levelCount = 1; }
							lW >>= levelOffset;
							if (lW == 0) { lW = 1; }
							const UINT lH = lW;
							for (size_t a = 0; a < 6; a++) { v1008::ReadMeta(read, version, &m_meta[a]); }


							API::Rendering::TEXTURE_DESC desc;
							Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
							m_buffer.MaxLayerCount = levelCount;
							m_buffer.LoadedLayers = 0;
							m_buffer.ArraySize = 6;
							m_buffer.LastLayers = NewArray(FileData::BitBuffer, 6);
							for (size_t a = 0; a < 6; a++) { m_buffer.LastLayers[a].Reset(); }
							desc.Width = lW;
							desc.Height = lH;
							desc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE_CUBE;
							desc.Format = v1008::FindFormat(m_channels, bits, v1008::FindBiggestColorSpace(m_meta, 6, bits), m_meta[0].HR);
							desc.AccessType = API::Rendering::VIEW_TYPE::SRV;
							desc.clearValue = API::Helper::Color::Black;
							desc.TextureCube.MipMaps = levelCount;
							m_buffer.Buffer = CreateTexture(man, API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, desc);
							man->GenerateViews(&m_buffer.Buffer, 1, API::Rendering::VIEW_TYPE::SRV, heap);

							CloakDebugLog("Skybox: Start Irradiance decoding");

							//Irradiance:
							IrradianceData irrData;
							for (size_t a = 0; a < 3; a++)
							{
								if ((m_channels & (1 << a)) != 0)
								{
									float irr[9];
									for (size_t b = 0; b < 9; b++) { irr[b] = static_cast<float>(read->ReadDouble(32)); }
									API::Global::Math::Matrix m(irr[0], irr[1], irr[2], irr[3], irr[1], -irr[0], irr[4], irr[5], irr[2], irr[4], irr[6], irr[7], irr[3], irr[5], irr[7], irr[8]);
									irrData.M[a] = m;
								}
								else
								{
									API::Global::Math::Matrix m(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
									irrData.M[a] = m;
								}
							}

							CloakDebugLog("Skybox: Start Radiance decoding");

							//Radiance - LUT
							FileData::Meta lutM;
							lutM.ColorSpace = ColorSpace::RGBA;
							lutM.Flags = 0;
							lutM.HR = false;

							v1008::DecodeTextureLayer(read, version, &m_lut, 0x3, bits, lutM);
							
							CloakDebugLog("Skybox: LUT decoded");

							desc.Width = m_lut.LastLayer.GetWidth();
							desc.Height = m_lut.LastLayer.GetHeight();
							desc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE;
							desc.Format = v1008::FindFormat(0x3, bits, lutM.ColorSpace, lutM.HR);
							desc.clearValue = API::Helper::Color::Black;
							desc.AccessType = API::Rendering::VIEW_TYPE::SRV | API::Rendering::VIEW_TYPE::SRV_DYNAMIC;
							desc.Texture.SampleCount = 1;
							desc.Texture.MipMaps = 1;

							API::Rendering::SUBRESOURCE_DATA srd;
							API::Helper::ReadLock lutLock(g_IBLLUT.Sync);
							if (static_cast<uint64_t>(g_IBLLUT.W) * g_IBLLUT.H < static_cast<uint64_t>(desc.Width) * desc.Height)
							{
								Impl::Rendering::IColorBuffer* lut = CreateTexture(man, API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, desc);

								srd.pData = m_lut.LastLayer.GetBinaryData();
								srd.RowPitch = m_lut.LastLayer.GetRowPitch();
								srd.SlicePitch = m_lut.LastLayer.GetSlicePitch();
								man->UpdateTextureSubresource(lut, 0, 1, &srd, true);

								man->GenerateViews(&lut, 1, API::Rendering::VIEW_TYPE::ALL, heap);
								API::Helper::WriteLock lLockW(g_IBLLUT.Sync);
								if (static_cast<uint64_t>(g_IBLLUT.W) * g_IBLLUT.H < static_cast<uint64_t>(desc.Width) * desc.Height)
								{
									g_IBLLUT.W = desc.Width;
									g_IBLLUT.H = desc.Height;
									g_IBLLUT.Texture = lut;
								}
								lLockW.unlock();
								SAVE_RELEASE(lut);
							}
							lutLock.unlock();

							//Radiance - First Sum
							FileData::Meta radM;
							radM.ColorSpace = ColorSpace::RGBA;
							radM.Flags = 0;
							radM.HR = m_meta[0].HR;

							const uint16_t radMipCount = static_cast<uint16_t>(read->ReadBits(10));
							const UINT radSize = static_cast<UINT>(read->ReadDynamic());

							irrData.FirstSumMipLevels = radMipCount;
							irrData.RadianceScale = 1;
							irrData.IrradianceScale = 1;

							desc.Width = radSize;
							desc.Height = radSize;
							desc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE_CUBE;
							desc.Format = v1008::FindFormat(0x7, bits, radM.ColorSpace, radM.HR);
							desc.clearValue = API::Helper::Color::Black;
							desc.AccessType = API::Rendering::VIEW_TYPE::SRV | API::Rendering::VIEW_TYPE::SRV_DYNAMIC;
							desc.TextureCube.MipMaps = radMipCount;

							m_radiance.MaxLayerCount = radMipCount;
							m_radiance.LoadedLayers = 0;
							m_radiance.ArraySize = 6;
							m_radiance.LastLayers = NewArray(FileData::BitBuffer, 6);
							for (size_t a = 0; a < 6; a++) { m_radiance.LastLayers[a].Reset(); }
							m_radiance.Buffer = CreateTexture(man, API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, desc);

							for (size_t a = 0; a < 6; a++)
							{
								for (size_t b = radMipCount; b > 0; b--)
								{
									FileData::BitBuffer* prevTex = nullptr;
									if (a > 0) { prevTex = &m_radiance.LastLayers[a - 1]; }
									v1008::DecodeArrayLayer(read, version, &m_radiance, a, 0x7, bits, radM, radM.ColorSpace, prevTex);

#if defined(_DEBUG) && defined(PRINT_DECODING)
									CloakDebugLog("Skybox: First sum cube side " + std::to_string(a) + " mip map " + std::to_string(b) + " decoded");
#endif

									srd.pData = m_radiance.LastLayers[a].GetBinaryData();
									srd.RowPitch = m_radiance.LastLayers[a].GetRowPitch();
									srd.SlicePitch = m_radiance.LastLayers[a].GetSlicePitch();
									man->UpdateTextureSubresource(m_radiance.Buffer, m_radiance.Buffer->GetSubresourceIndex(static_cast<UINT>(b - 1), static_cast<UINT>(a)), 1, &srd, true);
								}
							}

							m_irradiance = man->CreateConstantBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, sizeof(IrradianceData), &irrData);

							//Generate views
							API::Rendering::IResource* rsc[] = { m_irradiance, m_radiance.Buffer };
							man->GenerateViews(rsc, ARRAYSIZE(rsc), API::Rendering::VIEW_TYPE::ALL, heap);
						}
					}
					bool CLOAK_CALL_THIS Skybox::RequireLoad(In API::Files::LOD reqLOD)
					{
						const size_t mrl = m_buffer.MaxLayerCount - m_buffer.LoadedLayers;
						if (mrl <= static_cast<size_t>(reqLOD) && m_buffer.LastLayers > 0) { return false; }
						return true;
					}
					bool CLOAK_CALL_THIS Skybox::readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD)
					{
						const size_t mrl = m_buffer.MaxLayerCount - m_buffer.LoadedLayers;
						const bool hasLayer = read->ReadBits(1) == 1;
						if (hasLayer == false) { return false; }
						API::Rendering::SUBRESOURCE_DATA srd;
						Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
						const FileData::ColorSpace space = v1008::FindBiggestColorSpace(m_meta, m_buffer.ArraySize, bits);
						const size_t nrl = mrl - 1;
						for (size_t a = 0; a < 6; a++)
						{
							FileData::BitBuffer* prevTex = nullptr;
							if (a > 0) { prevTex = &m_buffer.LastLayers[a - 1]; }
							v1008::DecodeArrayLayer(read, version, &m_buffer, a, m_channels, bits, m_meta[a], space, prevTex);
							srd.pData = m_buffer.LastLayers[a].GetBinaryData();
							srd.RowPitch = m_buffer.LastLayers[a].GetRowPitch();
							srd.SlicePitch = m_buffer.LastLayers[a].GetSlicePitch();
							man->UpdateTextureSubresource(m_buffer.Buffer, m_buffer.Buffer->GetSubresourceIndex(static_cast<UINT>(nrl), static_cast<UINT>(a)), 1, &srd, true);
						}
						m_buffer.Buffer->SetDetailLevel(static_cast<uint32_t>(nrl));
						m_buffer.LoadedLayers++;
						return nrl > static_cast<size_t>(reqLOD);
					}


					//------------------------------------------------------------
					//----------------------TextureAnimation----------------------
					//------------------------------------------------------------
					CLOAK_CALL TextureAnimation::TextureAnimation()
					{

					}
					CLOAK_CALL TextureAnimation::~TextureAnimation()
					{

					}
					size_t CLOAK_CALL_THIS TextureAnimation::GetBufferByTime(In API::Global::Time time) const { return m_frames.Calculate(time); }
					API::Global::Time CLOAK_CALL_THIS TextureAnimation::GetAnimationLength() const { return m_frames.GetMax().X; }
					void CLOAK_CALL_THIS TextureAnimation::readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQS)
					{
						if (version >= 1008)
						{
							m_bufferSize = static_cast<size_t>(read->ReadDynamic());
							API::Global::Graphic::TextureQuality maxQuality = static_cast<API::Global::Graphic::TextureQuality>(read->ReadBits(2));
							*maxQS = maxQuality;
							const bool useAlpha = read->ReadBits(1) == 1;
							m_channels = static_cast<uint8_t>(read->ReadBits(4));
							API::Rendering::DescriptorPageType heap = v1008::ReadHeap(read);
							uint16_t levelCount = static_cast<uint16_t>(read->ReadBits(10));
							const UINT oW = static_cast<UINT>(read->ReadDynamic());
							const UINT oH = static_cast<UINT>(read->ReadDynamic());

							size_t usedFrames = 0;
							KeyFrame* frames = NewArray(KeyFrame, m_bufferSize + 1);
							m_buffer = NewArray(ColorBufferData<BufferData>, m_bufferSize);
							m_meta = NewArray(Meta, m_bufferSize);
							Impl::Rendering::IColorBuffer** buffer = NewArray(Impl::Rendering::IColorBuffer*, m_bufferSize);
							if (useAlpha) { m_alpha = NewArray(AlphaMap, m_bufferSize); }
							else { m_alpha = nullptr; }
							const uint16_t levelOffset = maxQuality - quality;
							if (levelCount > levelOffset) { levelCount -= levelOffset; }
							else { levelCount = 1; }
							UINT lW = oW >> levelOffset;
							UINT lH = oH >> levelOffset;
							if (lW == 0) { lW = 1; }
							if (lH == 0) { lH = 1; }
							API::Global::Time lastShowTime = 0;
							API::Global::Time fullTime = 0;
							API::Rendering::TEXTURE_DESC desc;
							Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
							for (size_t a = 0; a < m_bufferSize; a++)
							{
								v1008::ReadMeta(read, version, &m_meta[a]);
								if (useAlpha) { m_alpha[a].Resize(oW, oH, read, version); }
								API::Global::Time showTime = static_cast<API::Global::Time>(read->ReadDynamic());
								if (showTime != lastShowTime)
								{
									lastShowTime = showTime;
									frames[usedFrames].X = fullTime;
									frames[usedFrames].Y = a;
									usedFrames++;
								}
								fullTime += showTime;

								m_buffer[a].MaxLayerCount = levelCount;
								m_buffer[a].LoadedLayers = 0;
								m_buffer[a].LastLayer.Reset();
								desc.Width = lW;
								desc.Height = lH;
								desc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE;
								desc.Format = v1008::FindFormat(m_channels, bits, m_meta[a].ColorSpace, m_meta[a].HR);
								desc.AccessType = API::Rendering::VIEW_TYPE::SRV;
								desc.clearValue = API::Helper::Color::Black;
								desc.Texture.MipMaps = levelCount;
								desc.Texture.SampleCount = 1;
								m_buffer[a].Buffer = CreateTexture(man, API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, desc);
								buffer[a] = m_buffer[a].Buffer;
							}
							man->GenerateViews(buffer, m_bufferSize, API::Rendering::VIEW_TYPE::SRV, heap);
							frames[usedFrames].X = fullTime;
							frames[usedFrames].Y = m_bufferSize - 1;
							m_frames.Reset(frames, usedFrames + 1);
							DeleteArray(frames);
							DeleteArray(buffer);
						}
					}
					bool CLOAK_CALL_THIS TextureAnimation::RequireLoad(In API::Files::LOD reqLOD)
					{
						//TODO: Since all buffers have the same max layer count, mrl calculations should not be required...
						size_t mrl = 0;
						size_t mll = 0;
						for (size_t a = 0; a < m_bufferSize; a++)
						{
							const size_t rl = m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers;
							mrl = max(mrl, rl);
							if (a == 0 || mll > m_buffer[a].LoadedLayers) { mll = m_buffer[a].LoadedLayers; }
						}
						if (mrl <= static_cast<size_t>(reqLOD) && mll > 0) { return false; }
						return true;
					}
					bool CLOAK_CALL_THIS TextureAnimation::readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD)
					{
						//TODO: Since all buffers have the same max layer count, mrl calculations should not be required...
						size_t mrl = 0;
						for (size_t a = 0; a < m_bufferSize; a++)
						{
							const size_t rl = m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers;
							mrl = max(mrl, rl);
						}
						const bool hasLayer = read->ReadBits(1) == 1;
						if (hasLayer == false) { return false; }
						size_t l = m_bufferSize;
						API::Rendering::SUBRESOURCE_DATA srd;
						Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
						const size_t nrl = mrl - 1;
						for (size_t a = 0; a < m_bufferSize; a++)
						{
							if (m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers == mrl)
							{
								FileData::BitBuffer* prevTex = nullptr;
								if (l < m_bufferSize) { prevTex = &m_buffer[l].LastLayer; }
								v1008::DecodeTextureLayer(read, version, &m_buffer[a], m_channels, bits, m_meta[a], prevTex);
								srd.pData = m_buffer[a].LastLayer.GetBinaryData();
								srd.RowPitch = m_buffer[a].LastLayer.GetRowPitch();
								srd.SlicePitch = m_buffer[a].LastLayer.GetSlicePitch();
								man->UpdateTextureSubresource(m_buffer[a].Buffer, m_buffer[a].Buffer->GetSubresourceIndex(static_cast<UINT>(nrl)), 1, &srd, true);
								m_buffer[a].LoadedLayers++;
								l = a;
							}
						}
						for (size_t a = 0; a < m_bufferSize; a++)
						{
							if (m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers == nrl) { m_buffer[a].Buffer->SetDetailLevel(static_cast<uint32_t>(nrl)); }
						}
						if (mrl == 1)
						{
							for (size_t a = 0; a < m_bufferSize; a++) { m_buffer[a].LastLayer.Reset(); }
						}
						return nrl > static_cast<size_t>(reqLOD);
					}


					//------------------------------------------------------------
					//--------------------------Material--------------------------
					//------------------------------------------------------------
					CLOAK_CALL Material::Material()
					{
						m_type = API::Files::Image_v2::MaterialType::Unknown;
					}
					CLOAK_CALL Material::~Material()
					{
						for (size_t a = 0; a < 4; a++) { SAVE_RELEASE(m_buffer[a].Buffer); }
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS Material::GetBuffer(In size_t num) const { return m_buffer[num].Buffer; }
					API::Files::Image_v2::MaterialType CLOAK_CALL_THIS Material::GetMaterialType() const { return m_type; }
					bool CLOAK_CALL_THIS Material::SupportTesslation() const { return m_useDisplacement; }
					void CLOAK_CALL_THIS Material::readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQS)
					{
						if (version >= 1008)
						{
							m_type = v1008::ReadMaterialType(read);
							API::Global::Graphic::TextureQuality maxQuality = static_cast<API::Global::Graphic::TextureQuality>(read->ReadBits(2));
							*maxQS = maxQuality;
							m_useDisplacement = read->ReadBits(1) == 1;
							API::Rendering::DescriptorPageType heap = v1008::ReadHeap(read);

							API::Rendering::TEXTURE_DESC desc;
							Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
							const uint16_t levelOffset = maxQuality - quality;
							Impl::Rendering::IColorBuffer* buffer[4];
							for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
							{
								v1008::ReadMeta(read, version, &m_meta[a]);
								uint16_t levelCount = 1;
								UINT lW = 1;
								UINT lH = 1;
								if (version >= 1009 && read->ReadBits(1) == 1)
								{
									m_constant[a] = true;
								}
								else
								{
									m_constant[a] = false;
									levelCount = static_cast<uint16_t>(read->ReadBits(10));
									lW = static_cast<UINT>(read->ReadDynamic());
									lH = static_cast<UINT>(read->ReadDynamic());
									if (levelCount > levelOffset) { levelCount -= levelOffset; }
									else { levelCount = 1; }
									lW >>= levelOffset;
									lH >>= levelOffset;
									if (lW == 0) { lW = 1; }
									if (lH == 0) { lH = 1; }
								}

								m_buffer[a].MaxLayerCount = levelCount;
								m_buffer[a].LoadedLayers = 0;
								m_buffer[a].LastLayer.Reset();
								desc.Width = lW;
								desc.Height = lH;
								desc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE;
								desc.Format = v1008::FindFormat(v1008::FindChannels(m_type, a), bits, m_meta[a].ColorSpace, m_meta[a].HR);
								desc.AccessType = API::Rendering::VIEW_TYPE::SRV;
								desc.clearValue = API::Helper::Color::Black;
								desc.Texture.MipMaps = levelCount;
								desc.Texture.SampleCount = 1;
								m_buffer[a].Buffer = CreateTexture(man, API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, desc);
								buffer[a] = m_buffer[a].Buffer;
							}
							if (m_useDisplacement == false)
							{
								m_buffer[3].MaxLayerCount = 0;
								m_buffer[3].LoadedLayers = 0;
								m_buffer[3].LastLayer.Reset();
								m_buffer[3].Buffer = nullptr;
							}
							man->GenerateViews(buffer, m_useDisplacement ? 4 : 3, API::Rendering::VIEW_TYPE::SRV, heap);
							for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
							{
								if (m_constant[a] == true)
								{
									API::Rendering::SUBRESOURCE_DATA srd;
									v1008::DecodeTextureLayer(read, version, &m_buffer[a], v1008::FindChannels(m_type, a), bits, m_meta[a]);
									srd.pData = m_buffer[a].LastLayer.GetBinaryData();
									srd.RowPitch = m_buffer[a].LastLayer.GetRowPitch();
									srd.SlicePitch = m_buffer[a].LastLayer.GetSlicePitch();
									man->UpdateTextureSubresource(m_buffer[a].Buffer, m_buffer[a].Buffer->GetSubresourceIndex(static_cast<UINT>(0)), 1, &srd, true);
									m_buffer[a].LoadedLayers++;
									m_buffer[a].LastLayer.Reset();
									m_buffer[a].Buffer->SetDetailLevel(0);
								}
							}
						}
					}
					bool CLOAK_CALL_THIS Material::RequireLoad(In API::Files::LOD reqLOD)
					{
						size_t mrl = 0;
						size_t mll = 0;
						bool loadConstants = false;
						for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
						{
							if (m_constant[a] == false)
							{
								const size_t rl = m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers;
								mrl = max(mrl, rl);
								if (a == 0 || mll > m_buffer[a].LoadedLayers) { mll = m_buffer[a].LoadedLayers; }
							}
						}
						if (mrl <= static_cast<size_t>(reqLOD) && mll > 0) { return false; }
						return true;
					}
					bool CLOAK_CALL_THIS Material::readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD)
					{
						size_t mrl = 0;
						size_t mll = 0;
						bool loadConstants = false;
						for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
						{
							if (m_constant[a] == false)
							{
								const size_t rl = m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers;
								mrl = max(mrl, rl);
								if (a == 0 || mll > m_buffer[a].LoadedLayers) { mll = m_buffer[a].LoadedLayers; }
							}
						}
						const bool hasLayer = read->ReadBits(1) == 1;
						if (hasLayer == false) { return false; }
						API::Rendering::SUBRESOURCE_DATA srd;
						Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
						const size_t nrl = mrl - 1;
						for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
						{
							if (m_constant[a] == false && m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers == mrl)
							{
								v1008::DecodeTextureLayer(read, version, &m_buffer[a], v1008::FindChannels(m_type, a), bits, m_meta[a]);
								srd.pData = m_buffer[a].LastLayer.GetBinaryData();
								srd.RowPitch = m_buffer[a].LastLayer.GetRowPitch();
								srd.SlicePitch = m_buffer[a].LastLayer.GetSlicePitch();
								const UINT sri = m_buffer[a].Buffer->GetSubresourceIndex(static_cast<UINT>(nrl));
								man->UpdateTextureSubresource(m_buffer[a].Buffer, sri, 1, &srd, true);
								m_buffer[a].LoadedLayers++;
							}
						}
						for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
						{
							if (m_constant[a] == false && m_buffer[a].MaxLayerCount - m_buffer[a].LoadedLayers == nrl) { m_buffer[a].Buffer->SetDetailLevel(static_cast<uint32_t>(nrl)); }
						}
						if (mll == 0)
						{
							mll = 1;
							for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
							{
								if (m_constant[a] == false && m_buffer[a].LoadedLayers == 0) 
								{ 
									mll = 0; 
									break;
								}
							}
						}
						if (mrl == 1)
						{
							for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++) 
							{
								if (m_constant[a] == false)
								{
									m_buffer[a].LastLayer.Reset();
								}
							}
						}
						return nrl > static_cast<size_t>(reqLOD) || mll == 0;
					}
				}

				void CLOAK_CALL Image::Initialize()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&FileData::g_IBLLUT.Sync));
				}
				void CLOAK_CALL Image::Shutdown()
				{
					FileData::g_IBLLUT.Texture = nullptr;
					FileData::g_IBLLUT.Sync = nullptr;
				}

				CLOAK_CALL Image::Image()
				{
					DEBUG_NAME(Image);
					m_entSize = 0;
					m_texSize = 0;
					m_arrSize = 0;
					m_cubSize = 0;
					m_matSize = 0;
					m_skySize = 0;
					m_requestedLOD = API::Files::LOD::LOW;
					m_curLOD = API::Files::LOD::LOW;
					m_entries = nullptr;
					m_textures = nullptr;
					m_arrays = nullptr;
					m_cubes = nullptr;
					m_materials = nullptr;
					m_skybox = nullptr;
					for (size_t a = 0; a < ARRAYSIZE(m_lodReg); a++) { m_lodReg[a] = 0; }
				}
				CLOAK_CALL Image::~Image()
				{
					for (size_t a = 0; a < m_entSize; a++) { delete m_entries[a]; }
					DeleteArray(m_entries);
					DeleteArray(m_textures);
					DeleteArray(m_arrays);
					DeleteArray(m_cubes);
					DeleteArray(m_materials);
					DeleteArray(m_skybox);
				}
				API::RefPointer<API::Files::Image_v2::ITexture> CLOAK_CALL_THIS Image::GetTexture(In size_t id, In_opt size_t buffer)
				{
					API::Files::Image_v2::ITexture* r = new Proxys::TextureProxy(this, id, buffer);
					API::RefPointer<API::Files::Image_v2::ITexture> res = r;
					r->Release();
					return res;
				}
				API::RefPointer<API::Files::Image_v2::ITextureAnmiation> CLOAK_CALL_THIS Image::GetAnimation(In size_t id)
				{
					API::Files::Image_v2::ITextureAnmiation* r = new Proxys::TextureAnimationProxy(this, id);
					API::RefPointer<API::Files::Image_v2::ITextureAnmiation> res = r;
					r->Release();
					return res;
				}
				API::RefPointer<API::Files::Image_v2::ITextureArray> CLOAK_CALL_THIS Image::GetTextureArray(In size_t id)
				{
					API::Files::Image_v2::ITextureArray* r = new Proxys::TextureArrayProxy(this, id);
					API::RefPointer<API::Files::Image_v2::ITextureArray> res = r;
					r->Release();
					return res;
				}
				API::RefPointer<API::Files::Image_v2::ICubeMap> CLOAK_CALL_THIS Image::GetCubeMap(In size_t id)
				{
					API::Files::Image_v2::ICubeMap* r = new Proxys::CubeMapProxy(this, id);
					API::RefPointer<API::Files::Image_v2::ICubeMap> res = r;
					r->Release();
					return res;
				}
				API::RefPointer<API::Files::Image_v2::IMaterial> CLOAK_CALL_THIS Image::GetMaterial(In size_t id)
				{
					API::Files::Image_v2::IMaterial* r = new Proxys::MaterialProxy(this, id);
					API::RefPointer<API::Files::Image_v2::IMaterial> res = r;
					r->Release();
					return res;
				}
				API::RefPointer<API::Files::Image_v2::ISkybox> CLOAK_CALL_THIS Image::GetSkybox(In size_t id)
				{
					API::Files::Image_v2::ISkybox* r = new Proxys::SkyboxProxy(this, id);
					API::RefPointer<API::Files::Image_v2::ISkybox> res = r;
					r->Release();
					return res;
				}
			
				Success(return == true) bool CLOAK_CALL_THIS Image::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, RessourceHandler, Files::Image_v2, Image);
				}
				bool CLOAK_CALL_THIS Image::iCheckSetting(In const API::Global::Graphic::Settings& nset) const
				{
					API::Helper::ReadLock lock(m_sync);
					const API::Global::Graphic::TextureQuality ns = min(m_maxQuality, nset.TextureQuality);
					return ns != m_curQuality;
				}
				LoadResult CLOAK_CALL_THIS Image::iLoad(In API::Files::IReader* read, In API::Files::FileVersion version)
				{
					API::Helper::WriteLock lock(m_sync);
					API::Global::Graphic::Settings gset;
					Impl::Global::Graphic::GetModifedSettings(&gset);
					m_curQuality = gset.TextureQuality;
					m_noBackgroundLoading = false;
					if (version >= 1008)
					{
						m_bits.RGBA.Bits[0] = static_cast<uint8_t>(read->ReadBits(6) + 1);
						m_bits.RGBA.Bits[1] = static_cast<uint8_t>(read->ReadBits(6) + 1);
						m_bits.RGBA.Bits[2] = static_cast<uint8_t>(read->ReadBits(6) + 1);
						m_bits.RGBA.Bits[3] = static_cast<uint8_t>(read->ReadBits(6) + 1);
						if (version >= 1010)
						{
							m_bits.RGBE.Bits[0] = static_cast<uint8_t>(read->ReadBits(6) + 1);
							m_bits.RGBE.Bits[1] = static_cast<uint8_t>(read->ReadBits(6) + 1);
							m_bits.RGBE.Bits[2] = static_cast<uint8_t>(read->ReadBits(6) + 1);
							m_bits.RGBE.Bits[3] = static_cast<uint8_t>(read->ReadBits(6) + 1);
						}
						m_bits.YCCA.Bits[0] = static_cast<uint8_t>(read->ReadBits(6) + 1);
						m_bits.YCCA.Bits[1] = static_cast<uint8_t>(read->ReadBits(6) + 1);
						m_bits.YCCA.Bits[2] = m_bits.YCCA.Bits[1];
						m_bits.YCCA.Bits[3] = static_cast<uint8_t>(read->ReadBits(6) + 1);
						m_bits.Material.Bits[0] = static_cast<uint8_t>(read->ReadBits(6) + 1);
						m_bits.Material.Bits[1] = m_bits.Material.Bits[2] = m_bits.Material.Bits[3] = m_bits.Material.Bits[0];
						for (size_t a = 0; a < 4; a++) { m_bits.RGBA.MaxValue[a] = (1ULL << m_bits.RGBA.Bits[a]) - 1; m_bits.RGBA.Len[a] = BitLen(m_bits.RGBA.Bits[a]); }
						for (size_t a = 0; a < 4; a++) { m_bits.RGBE.MaxValue[a] = (1ULL << m_bits.RGBE.Bits[a]) - 1; m_bits.RGBE.Len[a] = BitLen(m_bits.RGBE.Bits[a]); }
						for (size_t a = 0; a < 4; a++) { m_bits.YCCA.MaxValue[a] = (1ULL << m_bits.YCCA.Bits[a]) - 1; m_bits.YCCA.Len[a] = BitLen(m_bits.YCCA.Bits[a]); }
						for (size_t a = 0; a < 4; a++) { m_bits.Material.MaxValue[a] = (1ULL << m_bits.Material.Bits[a]) - 1;  m_bits.Material.Len[a] = BitLen(m_bits.Material.Bits[a]); }
						if (version >= 1012) { m_noBackgroundLoading = (read->ReadBits(1) == 1); }
						m_entSize = static_cast<size_t>(read->ReadDynamic());
						m_entries = NewArray(FileData::Entry*, m_entSize);
						m_texSize = 0;
						m_cubSize = 0;
						m_arrSize = 0;
						m_matSize = 0;
						m_skySize = 0;

						m_maxQuality = API::Global::Graphic::TextureQuality::LOW;
						for (size_t a = 0; a < m_entSize; a++)
						{
							size_t pos = a;
							if (version >= 1014) 
							{
								read->MoveToNextByte();
								pos = static_cast<size_t>(read->ReadDynamic()); 
							}
							CLOAK_ASSUME(pos < m_entSize);
							const uint8_t et = static_cast<uint8_t>(read->ReadBits(3));
							switch (et)
							{
								case TYPE_IMAGE: m_entries[pos] = new FileData::Texture(); m_texSize++; break;
								case TYPE_ARRAY: m_entries[pos] = new FileData::TextureArray(); m_arrSize++; break;
								case TYPE_CUBE: m_entries[pos] = new FileData::CubeMap(); m_cubSize++; break;
								case TYPE_ANIMATION: m_entries[pos] = new FileData::TextureAnimation(); m_texSize++; break;
								case TYPE_MATERIAL: m_entries[pos] = new FileData::Material(); m_matSize++; break;
								case TYPE_SKYBOX: m_entries[pos] = new FileData::Skybox(); m_skySize++; break;
								default:
									CloakError(API::Global::Debug::Error::FREADER_FILE_CORRUPTED, true);
									return LoadResult::Finished;
							}
							API::Global::Graphic::TextureQuality maxQS;
							m_entries[pos]->readHeader(read, version, m_bits, m_curQuality, &maxQS);
							m_maxQuality = max(m_maxQuality, maxQS);
						}
						m_curQuality = min(m_maxQuality, gset.TextureQuality);
						CloakDebugLog("Started to load Image: " + std::to_string(m_texSize) + " Textures, " + std::to_string(m_arrSize) + " Arrays, " + std::to_string(m_cubSize) + " Cubes, "+std::to_string(m_skySize)+" Skyboxes and " + std::to_string(m_matSize) + " Materials");
						m_textures = NewArray(FileData::Texture*, m_texSize);
						m_arrays = NewArray(FileData::TextureArray*, m_arrSize);
						m_cubes = NewArray(FileData::CubeMap*, m_cubSize);
						m_materials = NewArray(FileData::Material*, m_matSize);
						m_skybox = NewArray(FileData::Skybox*, m_skySize);
						for (size_t a = 0, t = 0, r = 0, c = 0, m = 0, s=0; a < m_entSize; a++)
						{
							const FileData::EntryType type = m_entries[a]->GetType();
							switch (type)
							{
								case FileData::EntryType::Texture: m_textures[t++] = dynamic_cast<FileData::Texture*>(m_entries[a]); break;
								case FileData::EntryType::Array: m_arrays[r++] = dynamic_cast<FileData::TextureArray*>(m_entries[a]); break;
								case FileData::EntryType::Cube: m_cubes[c++] = dynamic_cast<FileData::CubeMap*>(m_entries[a]); break;
								case FileData::EntryType::Material: m_materials[m++] = dynamic_cast<FileData::Material*>(m_entries[a]); break;
								case FileData::EntryType::Skybox: m_skybox[s++] = dynamic_cast<FileData::Skybox*>(m_entries[a]); break;
								default:
									break;
							}
						}
						lock.unlock();
						return LoadLayer(read, version);
					}
					return LoadResult::Finished;
				}
				LoadResult CLOAK_CALL_THIS Image::iLoadDelayed(In API::Files::IReader* read, In API::Files::FileVersion version)
				{
					API::Helper::WriteLock lock(m_sync);
					if (m_curLOD > m_requestedLOD) { m_curLOD = static_cast<API::Files::LOD>(static_cast<uint32_t>(m_curLOD) - 1); }
					lock.unlock();
					return LoadLayer(read, version);
				}
				void CLOAK_CALL_THIS Image::iUnload()
				{
					API::Helper::WriteLock lock(m_sync);
					for (size_t a = 0; a < m_entSize; a++) { delete m_entries[a]; }
					DeleteArray(m_entries);
					DeleteArray(m_textures);
					DeleteArray(m_arrays);
					DeleteArray(m_cubes);
					DeleteArray(m_materials);
					DeleteArray(m_skybox);
					m_entSize = 0;
					m_texSize = 0;
					m_arrSize = 0;
					m_cubSize = 0;
					m_matSize = 0;
					m_skySize = 0;
					m_entries = nullptr;
					m_textures = nullptr;
					m_arrays = nullptr;
					m_cubes = nullptr;
					m_materials = nullptr;
					m_skybox = nullptr;
					m_curLOD = API::Files::LOD::LOW;
				}

				LoadResult CLOAK_CALL_THIS Image::LoadLayer(In API::Files::IReader* read, In API::Files::FileVersion version)
				{
					API::Helper::WriteLock lock(m_sync);
					bool reqLoad = false;
					if (m_noBackgroundLoading == true) { m_curLOD = m_requestedLOD; }
#ifdef _DEBUG
					size_t dbgA = 0;
#endif
					do {
						reqLoad = false;
						for (size_t a = 0; a < m_entSize; a++) { reqLoad = reqLoad || m_entries[a]->RequireLoad(m_curLOD); }
						if (reqLoad == true)
						{
							for (size_t a = 0; a < m_entSize; a++)
							{
								size_t pos = a;
								if (version >= 1014)
								{
									read->MoveToNextByte();
									pos = static_cast<size_t>(read->ReadDynamic());
								}
								CLOAK_ASSUME(pos < m_entSize);
								const bool rl = m_entries[pos]->readLayer(read, version, m_bits, m_curLOD);
								reqLoad = reqLoad || rl;
							}
						}
#ifdef _DEBUG
						dbgA++;
#endif
					} while (reqLoad == true);
#ifdef _DEBUG
					if (m_curLOD == API::Files::LOD::LOW || m_noBackgroundLoading == true) { CloakDebugLog("Loaded Image: " + std::to_string(m_texSize) + " Textures, " + std::to_string(m_arrSize) + " Arrays, " + std::to_string(m_cubSize) + " Cubes, " + std::to_string(m_skySize) + " Skyboxes and " + std::to_string(m_matSize) + " Materials"); }
#endif
					return m_curLOD != m_requestedLOD ? LoadResult::KeepLoading : (m_requestedLOD == API::Files::LOD::ULTRA ? LoadResult::Finished : LoadResult::Waiting);
				}

				FileData::Texture* CLOAK_CALL_THIS Image::GetTextureEntry(In size_t entry)
				{
					API::Helper::ReadLock(m_sync);
					if (entry < m_texSize) { return m_textures[entry]; }
					return nullptr;
				}
				FileData::TextureArray* CLOAK_CALL_THIS Image::GetArrayEntry(In size_t entry)
				{
					API::Helper::ReadLock(m_sync);
					if (entry < m_arrSize) { return m_arrays[entry]; }
					return nullptr;
				}
				FileData::CubeMap* CLOAK_CALL_THIS Image::GetCubeMapEntry(In size_t entry)
				{
					API::Helper::ReadLock(m_sync);
					if (entry < m_cubSize) { return m_cubes[entry]; }
					else if (entry - m_cubSize < m_skySize) { return m_skybox[entry]; }
					return nullptr;
				}
				FileData::Material* CLOAK_CALL_THIS Image::GetMaterialEntry(In size_t entry)
				{
					API::Helper::ReadLock(m_sync);
					if (entry < m_matSize) { return m_materials[entry]; }
					return nullptr;
				}
				FileData::Skybox* CLOAK_CALL_THIS Image::GetSkyboxEntry(In size_t entry)
				{
					API::Helper::ReadLock(m_sync);
					if (entry < m_skySize) { return m_skybox[entry]; }
					return nullptr;
				}
				void CLOAK_CALL_THIS Image::RegisterLOD(In API::Files::LOD lod)
				{
					API::Helper::WriteLock(m_sync);
					const size_t l = static_cast<size_t>(lod);
					if (l < ARRAYSIZE(m_lodReg)) { m_lodReg[l]++; }
					if (l < static_cast<size_t>(m_requestedLOD))
					{
						m_requestedLOD = lod;
						if (isLoaded()) { loadDelayed(); }
					}
				}
				void CLOAK_CALL_THIS Image::UnregisterLOD(In API::Files::LOD lod)
				{
					API::Helper::WriteLock(m_sync);
					const size_t l = static_cast<size_t>(lod);
					if (l < ARRAYSIZE(m_lodReg))
					{
						CLOAK_ASSUME(m_lodReg[l] > 0);
						m_lodReg[l]--;
						if (m_lodReg[l] == 0 && lod == m_requestedLOD && isLoaded() == false)
						{
							m_requestedLOD = API::Files::LOD::LOW;
							for (size_t a = l + 1; a < ARRAYSIZE(m_lodReg); a++)
							{
								if (m_lodReg[a] > 0)
								{
									m_requestedLOD = static_cast<API::Files::LOD>(a);
									break;
								}
							}
						}
					}
				}

				IMPLEMENT_FILE_HANDLER(Image);
			}
		}
	}
}