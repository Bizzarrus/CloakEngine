#include "stdafx.h"
#include "Engine/Image/TextureEncoding.h"
#include "Engine/LibWriter.h"

#include <tuple>
#include <sstream>

#define V1008_ENCODE_FUNC(name) bool name(In CloakEngine::Files::IWriter* write, In const MipMapLevel& texture, In const BitInfo& bits, In API::Image::v1008::ColorChannel channels, In uint16_t blockSize, In const ResponseCaller& response, In size_t imgNum, In size_t texNum, In bool hr, In bool byteAligned, In_opt const MipMapLevel* prevMip)
#define V1008_ENCODE_NBF(name) BitColorData name(In const BitTexture& orig, In UINT x, In UINT y, In size_t c)
#define V1012_ENCODE_NBHRF(name) int64_t name(In const FixedTexture& texture, In UINT x, In UINT y, In size_t c, In const FixedTexture* prevMip)

//Filter 3 is actually superior to filter 0-2
#define FORCED_FILTER 3

//Debug macros:
//#define WRITE_CHECKMARKS
//#define PRINT_ENCODING

namespace CloakCompiler {
	namespace Engine {
		namespace Image {
			namespace v1008 {
				constexpr float ALPHAMAP_THRESHOLD = 0.5f;

				//0 = auto/disabled
				struct FixedBitLength {
					uint8_t BitLength[4];
					CLOAK_CALL FixedBitLength() { for (size_t a = 0; a < ARRAYSIZE(BitLength); a++) { BitLength[a] = 0; } }
				};
				template<typename T> CLOAK_FORCEINLINE T CLOAK_CALL AlignToByte(In T bits) { return (bits + 7) & ~static_cast<T>(7); }
				inline void CLOAK_CALL EncodeBlockDynamic(In CloakEngine::Files::IWriter* write, In const BitTexture& tex, In const BitInfo& bits, In size_t c, In size_t sl, In const FixedBitLength& length, In bool byteAligned)
				{
					size_t hl = sl + bits.Channel[c].Bits + bits.Channel[c].Len;
					if (byteAligned == true) { hl = AlignToByte(hl); }
					for (UINT y = 0; y < tex.Size.H; y++)
					{
						const UINT fby = (y % 0x1) << 1;

						BitColorData bMin = 0;
						BitColorData bMax = 0;
						BitColorData sMin = 0;
						BitColorData sMax = 0;
						BitColorData lbMin = 0;
						UINT lbx = 0;
						UINT lsx = 0;
						UINT lsc = 0;
						UINT lbc = 0;
						uint8_t bbl = 0;
						bool smm = false;
						for (UINT x = 0; x < tex.Size.W; x++)
						{
							const UINT fbx = x & 0x1;
							if (length.BitLength[fby + fbx] == 0)
							{
								const BitColorData d = tex[y][x][c];
								if (smm == false)
								{
									bMin = bMax = sMin = sMax = d;
									smm = true;
								}
								else
								{
									lsc++;
									lbc++;
									lbMin = bMin;
									if (d < bMin) { bMin = d; sMin = d; sMax = d; lsx = x; lsc = 0; }
									if (d > bMax) { bMax = d; sMin = d; sMax = d; lsx = x; lsc = 0; }
									if (d < sMin) { sMin = d; lsx = x; lsc = 0; }
									if (d > sMax) { sMax = d; lsx = x; lsc = 0; }
									uint8_t nbl = BitLen(bMax - bMin);
									uint8_t nsl = BitLen(sMax - sMin);
									if (byteAligned == true) 
									{ 
										nbl = AlignToByte(nbl); 
										nsl = AlignToByte(nsl);
									}
									if (nbl > bbl)
									{
										if ((nbl - bbl)*lbc > hl)
										{
											write->WriteBits(sl, x - lbx);
											write->WriteBits(bits.Channel[c].Bits, lbMin);
											CLOAK_ASSUME((1 << bits.Channel[c].Len) > bbl);
											write->WriteBits(bits.Channel[c].Len, bbl);
											if (byteAligned == true) { write->MoveToNextByte(); }
											for (UINT dx = lbx; dx < x; dx++)
											{
												const UINT nfbx = dx & 0x1;
												const BitColorData nd = tex[y][dx][c] - lbMin;
												if (length.BitLength[fby + nfbx] == 0) { write->WriteBits(bbl, nd); }
												else { write->WriteBits(length.BitLength[fby + nfbx], tex[y][dx][c]); }
											}
											lbx = x;
											lsx = x;
											bMin = bMax = sMin = sMax = lbMin = d;
											bbl = 0;
											lbc = 0;
											lsc = 0;
										}
										else
										{
											bbl = nbl;
										}
									}
									else if (nsl < bbl && (bbl - nsl)*lsc > hl)
									{
										write->WriteBits(sl, lsx - lbx);
										write->WriteBits(bits.Channel[c].Bits, lbMin);
										CLOAK_ASSUME((1 << bits.Channel[c].Len) > bbl);
										write->WriteBits(bits.Channel[c].Len, bbl);
										if (byteAligned == true) { write->MoveToNextByte(); }
										for (UINT dx = lbx; dx < lsx; dx++)
										{
											const UINT nfbx = dx & 0x1;
											const BitColorData nd = tex[y][dx][c] - lbMin;
											if (length.BitLength[fby + nfbx] == 0) { write->WriteBits(bbl, nd); }
											else { write->WriteBits(length.BitLength[fby + nfbx], tex[y][dx][c]); }
										}
										lbc = lsc;
										lbx = lsx;
										bbl = nsl;
										bMin = sMin;
										bMax = sMax;
										sMin = d;
										sMax = d;
										lsc = 0;
										lsx = x;
									}
								}
							}
							if (x + 1 == tex.Size.W)
							{
								write->WriteBits(sl, tex.Size.W - lbx);
								write->WriteBits(bits.Channel[c].Bits, bMin);
								CLOAK_ASSUME((1 << bits.Channel[c].Len) > bbl);
								write->WriteBits(bits.Channel[c].Len, bbl);
								if (byteAligned == true) { write->MoveToNextByte(); }
								for (UINT dx = lbx; dx < tex.Size.W; dx++)
								{
									const UINT nfbx = dx & 0x1;
									const BitColorData nd = tex[y][dx][c] - bMin;
									if (length.BitLength[fby + nfbx] == 0) { write->WriteBits(bbl, nd); }
									else { write->WriteBits(length.BitLength[fby + nfbx], tex[y][dx][c]); }
								}
							}
						}
					}
				}
				inline void CLOAK_CALL EncodeBlockFixed(In CloakEngine::Files::IWriter* write, In const BitTexture& tex, In const BitInfo& bits, In UINT blockSize, In size_t c, In size_t sl, In const FixedBitLength& length, In bool byteAligned)
				{
					for (UINT y = 0; y < tex.Size.H; y++)
					{
						const UINT fby = (y % 0x1) << 1;
						for (UINT x = 0; x < tex.Size.W; x += blockSize)
						{
							const UINT s = min(blockSize, tex.Size.W - x);
							write->WriteBits(sl, s);
							BitColorData dMin = 0;
							BitColorData dMax = 0;
							bool sd = false;
							for (UINT dx = 0; dx < s; dx++)
							{
								const UINT fbx = (x + dx) & 0x1;
								if (length.BitLength[fby + fbx] == 0)
								{
									const BitColorData d = tex[y][x + dx][c];
									if (sd == false || d < dMin) { dMin = d; }
									if (sd == false || d > dMax) { dMax = d; }
									sd = true;
								}
							}
							write->WriteBits(bits.Channel[c].Bits, dMin);
							uint8_t l = BitLen(dMax - dMin);
							if (byteAligned == true) { l = AlignToByte(l); }
							CLOAK_ASSUME((1 << bits.Channel[c].Len) > l);
							write->WriteBits(bits.Channel[c].Len, l);
							if (byteAligned == true) { write->MoveToNextByte(); }
							for (UINT dx = 0; dx < s; dx++)
							{
								const UINT fbx = (x + dx) & 0x1;
								const BitColorData d = tex[y][x + dx][c] - dMin;
								if (length.BitLength[fby + fbx] == 0) { write->WriteBits(l, d); }
								else { write->WriteBits(length.BitLength[fby + fbx], tex[y][x + dx][c]); }
							}
						}
					}
				}
				inline void CLOAK_CALL EncodeTextureBlocks(In CloakEngine::Files::IWriter* write, In const BitTexture& tex, In const BitInfo& bits, In API::Image::v1008::ColorChannel channels, In uint16_t blockSize, In const FixedBitLength& length, In bool byteAligned)
				{
					const size_t sl = BitLen(tex.Size.W);
					for (size_t c = 0; c < 4; c++)
					{
						if ((static_cast<uint8_t>(channels) & (1 << c)) != 0)
						{
							if (blockSize == 0)
							{
								EncodeBlockDynamic(write, tex, bits, c, sl, length, byteAligned);
							}
							else
							{
								EncodeBlockFixed(write, tex, bits, blockSize, c, sl, length, byteAligned);
							}
						}
					}
				}
				inline void CLOAK_CALL NormalizeTexture(Inout BitTexture* tex, In size_t c, In BitColorData maxVal)
				{
					for (UINT y = 0; y < tex->Size.H; y++)
					{
						for (UINT x = 0; x < tex->Size.W; x++)
						{
							BitColorData d = tex->at(y)[x][c];
							while (d < 0) { d += maxVal + 1; }
							tex->at(y)[x][c] = d % (maxVal + 1);
						}
					}
				}

				typedef V1008_ENCODE_FUNC((*EncodeFilter));

				//NBF (Neighbor Biased Filter) encodes pixels based on their neighbors
				namespace NBF {
					typedef V1008_ENCODE_NBF((*FilterMode));

					V1008_ENCODE_NBF(NBF0) { return 0; }
					V1008_ENCODE_NBF(NBF1)
					{
						if (x == 0) { return 0; }
						return orig[y][x - 1][c];
					}
					V1008_ENCODE_NBF(NBF2)
					{
						if (y == 0) { return 0; }
						return orig[y - 1][x][c];
					}
					V1008_ENCODE_NBF(NBF3)
					{
						const BitColorData a = NBF1(orig, x, y, c);
						const BitColorData b = NBF2(orig, x, y, c);
						return (a + b) >> 1;
					}
					V1008_ENCODE_NBF(NBF4)
					{
						if (y == 0) { return NBF1(orig, x, y, c); }
						if (x == 0) { return NBF2(orig, x, y, c); }
						const BitColorData ad = orig[y][x - 1][c];
						const BitColorData bd = orig[y - 1][x][c];
						const BitColorData cd = orig[y - 1][x - 1][c];
						const BitColorData p = (ad + bd) - cd;
						const BitColorData pa = p > ad ? (p - ad) : (ad - p);
						const BitColorData pb = p > bd ? (p - bd) : (bd - p);
						const BitColorData pc = p > cd ? (p - cd) : (cd - p);
						if (pa < pb && pa < pc) { return ad; }
						else if (pb < pc) { return bd; }
						return cd;
					}
					V1008_ENCODE_NBF(NBF5)
					{
						if (y == 0) { return NBF1(orig, x, y, c); }
						if (x == 0) { return NBF2(orig, x, y, c); }
						const BitColorData ad = orig[y][x - 1][c];
						const BitColorData bd = orig[y - 1][x][c];
						const BitColorData cd = orig[y - 1][x - 1][c];
						return (ad + bd) - cd;
					}

					constexpr FilterMode MODES[] = { NBF0, NBF1, NBF2, NBF3, NBF4, NBF5 };

					inline uint64_t CLOAK_CALL TextureHash(In const BitTexture& t, In size_t c)
					{
						BitColorData dMax = 0;
						BitColorData dMin = 0;
						for (UINT y = 0; y < t.Size.H; y++)
						{
							for (UINT x = 0; x < t.Size.W; x++)
							{
								const BitColorData d = t[y][x][c];
								if (d < dMin || (x == 0 && y == 0)) { dMin = d; }
								if (d > dMax || (x == 0 && y == 0)) { dMax = d; }
							}
						}
						return static_cast<uint64_t>(dMax - dMin);
					}
					inline void CLOAK_CALL ApplyFilter(Inout BitTexture* tex, In const BitTexture& orig, In size_t modeNum, In size_t c, In BitColorData maxVal)
					{
						for (UINT y = 0; y < orig.Size.H; y++)
						{
							for (UINT x = 0; x < orig.Size.W; x++)
							{
								BitColorData pv = MODES[modeNum](orig, x, y, c);
								pv = min(maxVal, max(0, pv));
								BitColorData ov = orig[y][x][c];
								while (ov < pv) { ov += maxVal + 1; }
								tex->at(y)[x][c] = (ov - pv) % (maxVal + 1);
							}
						}
					}

					inline bool CLOAK_CALL CanUse(In uint8_t channels, In bool hr, In const MipMapLevel& a, In_opt const MipMapLevel* b = nullptr)
					{
						if (hr == true) { return false; }
						return true;
					}
					V1008_ENCODE_FUNC(Encode)
					{
						if (CanUse(static_cast<uint8_t>(channels), hr, texture, prevMip) == false) { return false; }
						BitTexture orig(texture, bits);
						size_t bm[4] = { 0,0,0,0 };
						for (size_t a = 0; a < 4; a++)
						{
							if ((static_cast<uint8_t>(channels) & (1 << a)) != 0)
							{
								uint64_t bh = 0;
								for (size_t b = 0; b < ARRAYSIZE(MODES); b++)
								{
									BitTexture tex = orig;
									ApplyFilter(&tex, orig, b, a, bits.Channel[a].MaxVal);
									const uint64_t h = TextureHash(tex, a);
									if (b == 0 || h < bh)
									{
										bh = h;
										bm[a] = b;
									}
								}
								write->WriteBits(3, bm[a]);
							}
						}
						BitTexture tex = orig;
						for (size_t a = 0; a < 4; a++)
						{
							if ((static_cast<uint8_t>(channels) & (1 << a)) != 0)
							{
								ApplyFilter(&tex, orig, bm[a], a, bits.Channel[a].MaxVal);
								NormalizeTexture(&tex, a, bits.Channel[a].MaxVal);
							}
						}
						if (byteAligned == true) { write->MoveToNextByte(); }
						EncodeTextureBlocks(write, tex, bits, channels, blockSize, FixedBitLength(), byteAligned);
						return true;
					}
				}
				//QTF (Quad Tree Filter) encodes pixels based on the previous mip map
				namespace QTF {
					inline bool CLOAK_CALL CanUse(In uint8_t channels, In bool hr, In const MipMapLevel& a, In_opt const MipMapLevel* b = nullptr)
					{
						if (b == nullptr) { return false; }
						if ((a.Size.W & 0x1) != 0 || (a.Size.H & 0x1) != 0) { return false; }
						if (hr == true) { return false; }
						return true;
					}
					V1008_ENCODE_FUNC(Encode)
					{
						if (CanUse(static_cast<uint8_t>(channels), hr, texture, prevMip) == false) { return false; }
						BitTexture tex(texture, bits);
						BitTexture prev(*prevMip, bits);
						for (size_t c = 0; c < 4; c++)
						{
							if ((static_cast<uint8_t>(channels) & (1 << c)) != 0)
							{
								for (UINT y = 0; y < prev.Size.H; y++)
								{
									for (UINT x = 0; x < prev.Size.W; x++)
									{
										const UINT dx = x << 1;
										const UINT dy = y << 1;
										const BitColorData dp = prev[y][x][c] << 2;
										const BitColorData dtl = tex[dy][dx][c];
										const BitColorData dt = tex[dy][dx + 1][c];
										const BitColorData dl = tex[dy + 1][dx][c];
										const BitColorData d = dp - (dtl + dt + dl);
										const BitColorData o = tex[dy + 1][dx + 1][c];
										BitColorData r = 4 + o - d; //original value - calculated value of quad tree should be in range [-4; 4] (rounding error)
										if (r < 0 || r > 8) { return false; } //rounding error outside of expected range => prev is not an actual mip map
										tex[dy + 1][dx + 1][c] = r;
									}
								}
								NormalizeTexture(&tex, c, bits.Channel[c].MaxVal);
							}
						}
						FixedBitLength fbl;
						fbl.BitLength[3] = byteAligned == true ? AlignToByte(3) : 3;
						EncodeTextureBlocks(write, tex, bits, channels, blockSize, fbl, byteAligned);
						return true;
					}
				}
				//HRF (High Range Filter) encodes pixels in an shared-exponent float format. This filter is not loss-less!
				namespace HRF {
					inline void CLOAK_CALL EncodeColor(In CloakEngine::Files::IWriter* write, In const CloakEngine::Helper::Color::RGBA& color, In uint8_t channel, In const BitInfo& bits, In int exMin, In uint8_t edl, In bool byteAligned)
					{
						float f[3];
						int e[3];
						size_t b = 0;
						for (size_t a = 0; a < 3; a++)
						{
							if ((channel & (1 << a)) != 0)
							{
								if (color[a] != color[a]) { e[b] = static_cast<int>(bits.Channel[3].MaxVal); f[b] = 1; } //NaN
								else if (color[a] == std::numeric_limits<float>::infinity()) { e[b] = static_cast<int>(bits.Channel[3].MaxVal); f[b] = 0; } //Infinity
								else
								{
									f[b] = std::frexp(color[a], &e[b]);
									e[b] = static_cast<int>(e[b] + (bits.Channel[3].MaxVal >> 1));
									if (e[b] < 0)
									{
										f[b] /= (1 << (-e[b]));
										e[b] = 0;
									}
									else if (e[b] > bits.Channel[3].MaxVal)
									{
										e[b] = static_cast<int>(bits.Channel[3].MaxVal);
										f[b] = 0;
									}
								}
								b++;
							}
						}
						int mex = 0;
						for (size_t a = 0; a < b; a++) { if (a == 0 || mex < e[a]) { mex = e[a]; } }

						CLOAK_ASSUME(mex >= exMin);
						CLOAK_ASSUME((mex - exMin) < (1 << edl));
						if (edl > 0) { write->WriteBits(edl, mex - exMin); }
						b = 0;
						for (size_t a = 0; a < 3; a++)
						{
							if ((channel & (1 << a)) != 0)
							{
								float x = std::abs(f[b]);
								if (e[b] < mex) { x /= 1 << (mex - e[b]); }
								if (f[b] < 0) { write->WriteBits(1, 1); }
								else { write->WriteBits(1, 0); }
								uint64_t v = static_cast<uint64_t>(bits.Channel[a].MaxVal * x);
								if (mex == bits.Channel[3].MaxVal) { v = f[b] > 0 ? bits.Channel[a].MaxVal : 0; }
								write->WriteBits(bits.Channel[a].Bits, v);
								if (byteAligned == true) { write->MoveToNextByte(); }
								b++;
							}
						}
					}
					inline void CLOAK_CALL EncodeBlockDynamic(In CloakEngine::Files::IWriter* write, In const MipMapLevel& tex, In const BitInfo& bits, In uint8_t channels, In size_t sl, In bool byteAligned)
					{
						size_t hl = sl + 24;
						if (byteAligned == true) { hl = AlignToByte(hl); }
						for (UINT y = 0; y < tex.Size.H; y++)
						{
							int bMin = 0;
							int bMax = 0;
							int sMin = 0;
							int sMax = 0;
							int lbMin = 0;
							UINT lbx = 0;
							UINT lsx = 0;
							UINT lsc = 0;
							UINT lbc = 0;
							uint8_t bbl = 0;
							bool smm = false;
							for (UINT x = 0; x < tex.Size.W; x++)
							{
								const CloakEngine::Helper::Color::RGBA& col = tex.Get(x, y);
								int d = 0;
								bool setMex = false;
								for (size_t c = 0; c < 3; c++)
								{
									if ((channels & (1 << c)) != 0)
									{
										int e = 0;
										if (col[c] != col[c] || col[c] == std::numeric_limits<float>::infinity()) { e = static_cast<int>(bits.Channel[3].MaxVal); }//NaN or Inf
										else 
										{ 
											std::frexp(col[c], &e); 
											e = static_cast<int>(e + (bits.Channel[3].MaxVal >> 1));
											e = static_cast<int>(max(0, min(e, bits.Channel[3].MaxVal)));
										}
										if (setMex == false || e > d) { setMex = true; d = e; }
									}
								}

								if (smm == false)
								{
									bMax = bMin = sMax = sMin = d;
									smm = true;
								}
								else
								{
									lsc++;
									lbc++;
									lbMin = bMin;
									if (d < bMin) { bMin = d; sMin = d; sMax = d; lsx = x; lsc = 0; }
									if (d > bMax) { bMax = d; sMin = d; sMax = d; lsx = x; lsc = 0; }
									if (d < sMin) { sMin = d; lsx = x; lsc = 0; }
									if (d > sMax) { sMax = d; lsx = x; lsc = 0; }
									uint8_t nbl = BitLen(bMax - bMin);
									uint8_t nsl = BitLen(sMax - sMin);
									if (byteAligned == true)
									{
										nbl = AlignToByte(nbl);
										nsl = AlignToByte(nsl);
									}
									if (nbl > bbl)
									{
										if ((nbl - bbl)*lbc > hl)
										{
											write->WriteBits(sl, x - lbx);

											write->WriteBits(bits.Channel[3].Bits, static_cast<unsigned int>(lbMin));
											CLOAK_ASSUME((1 << bits.Channel[3].Len) > bbl);
											write->WriteBits(bits.Channel[3].Len, bbl);
											if (byteAligned == true) { write->MoveToNextByte(); }
											for (UINT dx = lbx; dx < x; dx++) { EncodeColor(write, tex.Get(dx, y), channels, bits, lbMin, bbl, byteAligned); }

											lbx = x;
											lsx = x;
											bMin = bMax = sMin = sMax = lbMin = d;
											bbl = 0;
											lbc = 0;
											lsc = 0;
										}
										else
										{
											bbl = nbl;
										}
									}
									else if (nsl < bbl && (bbl - nsl)*lsc > hl)
									{
										write->WriteBits(sl, lsx - lbx);

										write->WriteBits(bits.Channel[3].Bits, static_cast<unsigned int>(lbMin));
										CLOAK_ASSUME((1 << bits.Channel[3].Len) > bbl);
										write->WriteBits(bits.Channel[3].Len, bbl);
										if (byteAligned == true) { write->MoveToNextByte(); }
										for (UINT dx = lbx; dx < lsx; dx++) { EncodeColor(write, tex.Get(dx, y), channels, bits, lbMin, bbl, byteAligned); }

										lbc = lsc;
										lbx = lsx;
										bbl = nsl;
										bMin = sMin;
										bMax = sMax;
										sMin = d;
										sMax = d;
										lsc = 0;
										lsx = x;
									}
								}
								if (x + 1 == tex.Size.W)
								{
									write->WriteBits(sl, tex.Size.W - lbx);
									write->WriteBits(bits.Channel[3].Bits, static_cast<unsigned int>(bMin));
									CLOAK_ASSUME((1 << bits.Channel[3].Len) > bbl);
									write->WriteBits(bits.Channel[3].Len, bbl);
									if (byteAligned == true) { write->MoveToNextByte(); }
									for (UINT dx = lbx; dx < tex.Size.W; dx++) { EncodeColor(write, tex.Get(dx, y), channels, bits, bMin, bbl, byteAligned); }
								}
							}
						}
					}
					inline void CLOAK_CALL EncodeBlockFixed(In CloakEngine::Files::IWriter* write, In const MipMapLevel& tex, In const BitInfo& bits, In UINT blockSize, In uint8_t channels, In size_t sl, In bool byteAligned)
					{
						for (UINT y = 0; y < tex.Size.H; y++)
						{
							for (UINT x = 0; x < tex.Size.W; x += blockSize)
							{
								const UINT s = min(blockSize, tex.Size.W - x);
								write->WriteBits(sl, s);
								int exMin = 0;
								int exMax = 0;
								for (UINT dx = 0; dx < s; dx++)
								{
									const CloakEngine::Helper::Color::RGBA& col = tex.Get(x + dx, y);
									int mex = 0;
									bool setMex = false;
									for (size_t c = 0; c < 3; c++)
									{
										if ((channels & (1 << c)) != 0)
										{
											int e = 0;
											if (col[c] != col[c] || col[c] == std::numeric_limits<float>::infinity()) { e = static_cast<int>(bits.Channel[3].MaxVal); }//NaN or Inf
											else
											{
												std::frexp(col[c], &e);
												e = static_cast<int>(e + (bits.Channel[3].MaxVal >> 1));
												e = static_cast<int>(max(0, min(e, bits.Channel[3].MaxVal)));
											}
											if (setMex == false || e > mex) { setMex = true; mex = e; }
										}
									}
									if (dx == 0 || mex < exMin) { exMin = mex; }
									else if (dx == 0 || mex > exMax) { exMax = mex; }
								}
								write->WriteBits(bits.Channel[3].Bits, static_cast<unsigned int>(exMin));
								uint8_t edl = BitLen(exMax - exMin);
								if (byteAligned == true) { edl = AlignToByte(edl); }
								CLOAK_ASSUME((1 << bits.Channel[3].Len) > edl);
								write->WriteBits(bits.Channel[3].Len, edl);
								if (byteAligned == true) { write->MoveToNextByte(); }
								for (UINT dx = 0; dx < s; dx++) { EncodeColor(write, tex.Get(x + dx, y), channels, bits, exMin, edl, byteAligned); }
							}
						}
					}
					inline void CLOAK_CALL EncodeTextureBlocks(In CloakEngine::Files::IWriter* write, In const MipMapLevel& tex, In const BitInfo& bits, In UINT blockSize, In uint8_t channels, In bool byteAligned)
					{
						const uint8_t sl = BitLen(tex.Size.W);
						if (blockSize == 0)
						{
							EncodeBlockDynamic(write, tex, bits, channels, sl, byteAligned);
						}
						else
						{
							EncodeBlockFixed(write, tex, bits, blockSize, channels, sl, byteAligned);
						}
					}
					inline bool CLOAK_CALL CanUse(In uint8_t channels, In bool hr, In const MipMapLevel& a, In_opt const MipMapLevel* b = nullptr)
					{
						if (hr == false) { return false; }
						if ((channels & 0x8) != 0) { return false; }
						return true;
					}
					V1008_ENCODE_FUNC(Encode)
					{
						if (CanUse(static_cast<uint8_t>(channels), hr, texture, prevMip) == false) { return false; }
						EncodeTextureBlocks(write, texture, bits, blockSize, static_cast<uint8_t>(channels), byteAligned);
						return true;
					}
				}
				//NFHRF (Neighbor Biased High Range Filter) encodes pixels based on their neighbors and previous mip map, and supports high range encoding
				namespace NBHRF {
					struct NBHRFV {
						public:
							int64_t Value;
							uint8_t Exponent;

							CLOAK_CALL NBHRFV() : Value(0), Exponent(0) {}
							CLOAK_CALL NBHRFV(In int64_t v, In const BitInfo::SingleInfo& bits)
							{
								const uint64_t uv = static_cast<uint64_t>(CloakEngine::Global::Math::abs(v));
								const uint8_t bl = BitLen(uv);
								if (bl > bits.Bits)
								{
									Exponent = bl - bits.Bits;
									const uint64_t sv = uv >> Exponent;
									Value = v < 0 ? -static_cast<int64_t>(sv) : static_cast<int64_t>(sv);
								}
								else
								{
									Exponent = 0;
									Value = v;
								}
							}

							explicit CLOAK_CALL_THIS operator int64_t() const {
								const uint64_t uv = static_cast<uint64_t>(CloakEngine::Global::Math::abs(Value));
								return Value < 0 ? -static_cast<int64_t>(uv << Exponent) : static_cast<int64_t>(uv << Exponent);
							}
						private:
							CLOAK_CALL NBHRFV(In int64_t v) : Value(v), Exponent(0) {}
					};
					class FixedTexture {
						public:
							const Dimensions Size;
						private:
							const BitInfo& m_bits;
							int64_t* m_table;
						public:
							CLOAK_CALL FixedTexture(In Dimensions size, In const BitInfo& bits) : Size(size), m_bits(bits)
							{
								m_table = reinterpret_cast<int64_t*>(CloakEngine::Global::Memory::MemoryHeap::Allocate(sizeof(int64_t) * Size.GetSize() * 4));
							}
							CLOAK_CALL FixedTexture(In UINT W, In UINT H, In const BitInfo& bits) : FixedTexture(Dimensions(W, H), bits) {}
							CLOAK_CALL FixedTexture(In const MipMapLevel* texture, In const BitInfo& bits) : FixedTexture(texture != nullptr ? texture->Size : Dimensions(0, 0), bits)
							{
								if (texture != nullptr)
								{
									for (size_t a = 0; a < 4; a++)
									{
										for (UINT y = 0; y < Size.H; y++)
										{
											for (UINT x = 0; x < Size.W; x++)
											{
												const size_t p = static_cast<size_t>((((y * Size.W) + x) * 4) + a);
												m_table[p] = static_cast<int64_t>((static_cast<long double>(texture->Get(x, y)[a]) * static_cast<int64_t>(bits.Channel[a].MaxVal)) + 0.5f);
											}
										}
									}
								}
							}
							CLOAK_CALL FixedTexture(In const MipMapLevel& texture, In const BitInfo& bits) : FixedTexture(&texture, bits) {}
							CLOAK_CALL ~FixedTexture() { CloakEngine::Global::Memory::MemoryHeap::Free(m_table); }

							int64_t& CLOAK_CALL_THIS Get(In UINT x, In UINT y, In size_t c)
							{
								const size_t p = static_cast<size_t>((((y * Size.W) + x) * 4) + c);
								return m_table[p];
							}
							const int64_t& CLOAK_CALL_THIS Get(In UINT x, In UINT y, In size_t c) const
							{
								const size_t p = static_cast<size_t>((((y * Size.W) + x) * 4) + c);
								return m_table[p];
							}
							int64_t* CLOAK_CALL_THIS Get(In UINT x, In UINT y)
							{
								const size_t p = static_cast<size_t>(((y * Size.W) + x) * 4);
								return &m_table[p];
							}
							const int64_t* CLOAK_CALL_THIS Get(In UINT x, In UINT y) const
							{
								const size_t p = static_cast<size_t>(((y * Size.W) + x) * 4);
								return &m_table[p];
							}
							void CLOAK_CALL_THIS Set(In UINT x, In UINT y, In size_t c, In int64_t v)
							{
								const size_t p = static_cast<size_t>((((y * Size.W) + x) * 4) + c);
								m_table[p] = v;
							}
							NBHRFV CLOAK_CALL_THIS GetFilterValue(In UINT x, In UINT y, In size_t c) const
							{
								return NBHRFV(Get(x, y, c), m_bits.Channel[c]);
							}
					};

					typedef V1012_ENCODE_NBHRF((*FilterMode));
					//No Filter
					V1012_ENCODE_NBHRF(NBHRF0) { return 0; }
					//Assumes same as left color (Same as PNG's Sub)
					V1012_ENCODE_NBHRF(NBHRF1) 
					{
						if (x == 0) { return 0; }
						return texture.Get(x - 1, y)[c];
					}
					//Assumes same as top color (Same as PNG's Up)
					V1012_ENCODE_NBHRF(NBHRF2)
					{
						if (y == 0) { return 0; }
						return texture.Get(x, y - 1)[c];
					}
					//Assumes average of left and top color (Same as PNG's Average)
					V1012_ENCODE_NBHRF(NBHRF3)
					{
						const int64_t a = NBHRF1(texture, x, y, c, prevMip);
						const int64_t b = NBHRF2(texture, x, y, c, prevMip);
						return (a + b) / 2;
					}
					//Assumes same es top left color
					V1012_ENCODE_NBHRF(NBHRF4)
					{
						if (y == 0) { return NBHRF1(texture, x, y, c, prevMip); }
						if (x == 0) { return NBHRF2(texture, x, y, c, prevMip); }
						return texture.Get(x - 1, y - 1)[c];
					}
					//Assumes best of left, top and top left color based on derivation (Same as PNG's Paeth)
					V1012_ENCODE_NBHRF(NBHRF5)
					{
						if (y == 0) { return NBHRF1(texture, x, y, c, prevMip); }
						if (x == 0) { return NBHRF2(texture, x, y, c, prevMip); }
						const int64_t ad = texture.Get(x - 1, y)[c];
						const int64_t bd = texture.Get(x, y - 1)[c];
						const int64_t cd = texture.Get(x - 1, y - 1)[c];
						const int64_t p = (ad + bd) - cd;
						const int64_t pa = std::abs(p - ad);
						const int64_t pb = std::abs(p - bd);
						const int64_t pc = std::abs(p - cd);
						if (pa < pb && pa < pc) { return ad; }
						else if (pb < pc) { return bd; }
						return cd;
					}
					//Assumes derivation
					V1012_ENCODE_NBHRF(NBHRF6)
					{
						if (y == 0) { return NBHRF1(texture, x, y, c, prevMip); }
						if (x == 0) { return NBHRF2(texture, x, y, c, prevMip); }
						const int64_t ad = texture.Get(x - 1, y)[c];
						const int64_t bd = texture.Get(x, y - 1)[c];
						const int64_t cd = texture.Get(x - 1, y - 1)[c];
						return (ad + bd) - cd;
					}
					//Assumes same as in previous mip map
					V1012_ENCODE_NBHRF(NBHRF7)
					{
						if (prevMip == nullptr) { return 0; }
						x >>= 1;
						y >>= 1;
						if (x >= prevMip->Size.W || y >= prevMip->Size.H) { return 0; }
						return prevMip->Get(x, y)[c];
					}
					//Quad tree calculation
					V1012_ENCODE_NBHRF(NBHRF8)
					{
						const int64_t m = NBHRF7(texture, x, y, c, prevMip);
						const int64_t ad = NBHRF1(texture, x, y, c, prevMip);
						const int64_t bd = NBHRF2(texture, x, y, c, prevMip);
						const int64_t cd = NBHRF4(texture, x, y, c, prevMip);
						return (4 * m) - (ad + bd + cd);
					}
					//Assumes best of left, top, top left and quad tree, based on derivation
					V1012_ENCODE_NBHRF(NBHRF9)
					{
						const int64_t m = NBHRF7(texture, x, y, c, prevMip);
						const int64_t ad = NBHRF1(texture, x, y, c, prevMip);
						const int64_t bd = NBHRF2(texture, x, y, c, prevMip);
						const int64_t cd = NBHRF4(texture, x, y, c, prevMip);
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

					constexpr FilterMode MODES[] = { NBHRF0, NBHRF1, NBHRF2, NBHRF3, NBHRF4, NBHRF5, NBHRF6, /*NBHRF7, NBHRF8, NBHRF9*/ }; //TODO: Mip Map based filter don't work correctly for now

					void CLOAK_CALL WriteHRValue(In CloakEngine::Files::IWriter* write, In const NBHRFV& value, In UINT exponentLen, In UINT valueLen, In_opt bool canBeNegative = false)
					{
						CLOAK_ASSUME(value.Value >= 0 || canBeNegative == true);
#if defined(_DEBUG) && defined(PRINT_ENCODING)
						const int64_t rv = static_cast<int64_t>(value);
#endif
						if (canBeNegative == true) { write->WriteBits(1, value.Value < 0 ? 1 : 0); }
						write->WriteBits(exponentLen, value.Exponent);
						write->WriteBits(valueLen, std::abs(value.Value));
#if defined(_DEBUG) && defined(PRINT_ENCODING)
						CloakDebugLog("WriteHR: e = " + std::to_string(value.Exponent) + " v = " + (canBeNegative == true && rv < 0 ? "-" : "") + std::to_string(std::abs(value.Value)) + " (should be " + std::to_string(rv) + ")");
#endif
					}
					void CLOAK_CALL WriteLRValue(In CloakEngine::Files::IWriter* write, In const NBHRFV& value, In UINT valueLen, In_opt bool canBeNegative = false)
					{
						CLOAK_ASSUME(value.Value >= 0 || canBeNegative == true);
						CLOAK_ASSUME(value.Exponent == 0);
#if defined(_DEBUG) && defined(PRINT_ENCODING)
						const int64_t rv = static_cast<int64_t>(value);
#endif
						if (canBeNegative == true) { write->WriteBits(1, value.Value < 0 ? 1 : 0); }
						write->WriteBits(valueLen, std::abs(value.Value));
#if defined(_DEBUG) && defined(PRINT_ENCODING)
						CloakDebugLog("WriteLR: v = " + std::string(canBeNegative == true && rv < 0 ? "-" : "") + std::to_string(std::abs(value.Value)) + " (should be " + std::to_string(rv) + ")");
#endif
					}
					void CLOAK_CALL ApplyFilter(Out FixedTexture* res, In const FixedTexture& texture, In size_t mode, In size_t c, In UINT xs, In UINT ys, In UINT tileSize, In_opt const FixedTexture* prevMip = nullptr)
					{
						CLOAK_ASSUME(res->Size.W == texture.Size.W && res->Size.H == texture.Size.H);
						CLOAK_ASSUME(mode < ARRAYSIZE(MODES));
						for (UINT y = ys, yi = 0; yi < tileSize && y < res->Size.H; y++, yi++)
						{
							for (UINT x = xs, xi = 0; xi < tileSize && x < res->Size.W; x++, xi++)
							{
								res->Set(x, y, c, texture.Get(x, y)[c] - MODES[mode](texture, x, y, c, prevMip));
							}
						}
					}
					//Required per value bits to write values in given range
					UINT CLOAK_CALL RequiredBits(In int64_t valueRange, In const BitInfo::SingleInfo& bits, In bool byteAligned)
					{
						UINT r = 0;
						NBHRFV nv(valueRange, bits);
						if (nv.Exponent > 0)
						{
							r = BitLen(nv.Exponent) + bits.Bits;
						}
						else
						{
							r = BitLen(std::abs(nv.Value));
						}
						if (byteAligned == true) { r = AlignToByte(r); }
						return r;
					}
					UINT CLOAK_CALL RequiredBits(In int64_t valueRange, In UINT exponentLen, In const BitInfo::SingleInfo& bits, In bool byteAligned)
					{
						UINT r = 0;
						NBHRFV nv(valueRange, bits);
						if (nv.Exponent > 0)
						{
							r = exponentLen + bits.Bits;
						}
						else
						{
							r = BitLen(std::abs(nv.Value));
						}
						if (byteAligned == true) { r = AlignToByte(r); }
						return r;
					}
					void CLOAK_CALL WriteHeader(In CloakEngine::Files::IWriter* write, In int64_t minV, In int64_t maxV, In UINT valueCount, In UINT exponentLen, In UINT perValueBits, In uint8_t blockBits, In const BitInfo::SingleInfo& bits, In bool byteAligned)
					{
						CLOAK_ASSUME(maxV >= minV);
						CLOAK_ASSUME(valueCount > 0);
						const NBHRFV mv = NBHRFV(minV, bits);
						const NBHRFV rv = NBHRFV(maxV - minV, bits);

						write->WriteBits(1, rv.Exponent > 0 ? 1 : 0);
						write->WriteBits(1, mv.Exponent > 0 ? 1 : 0);
						write->WriteBits(blockBits, valueCount - 1);
						write->WriteBits(bits.Len, perValueBits);

						if (mv.Exponent > 0 || rv.Exponent > 0)
						{
							CLOAK_ASSUME(exponentLen < (1 << 5));
							write->WriteBits(5, exponentLen);
						}
						if (mv.Exponent > 0)
						{
							WriteHRValue(write, mv, exponentLen, bits.Bits, true);
						}
						else
						{
							WriteLRValue(write, mv, bits.Bits, true);
						}
						if (byteAligned == true) { write->MoveToNextByte(); }
					}
					void CLOAK_CALL WriteValue(In CloakEngine::Files::IWriter* write, In int64_t minV, In int64_t maxV, In int64_t value, In UINT exponentLen, In UINT perValueBits, In const BitInfo::SingleInfo& bits)
					{
						CLOAK_ASSUME(maxV >= minV);
						CLOAK_ASSUME(value >= minV);
						CLOAK_ASSUME(value <= maxV);
						const NBHRFV cv(value - minV, bits);

						if (maxV - minV > bits.MaxVal)
						{
							WriteHRValue(write, cv, exponentLen, perValueBits);
						}
						else
						{
							WriteLRValue(write, cv, perValueBits);
						}
					}
					//Exact calcluation of <Header Len in Bit | Exponent Len in Bit | Per Value Len in Bit>
					std::tuple<UINT,UINT, UINT> CLOAK_CALL BlockHeaderLen(In int64_t minV, In int64_t maxV, In uint8_t blockBits, In const BitInfo::SingleInfo& bits, In bool byteAligned)
					{
						CLOAK_ASSUME(maxV >= minV);
						UINT r = 2; //Block uses high range (1) + min uses high range bits (1)
						r += blockBits; //Block len
						r += bits.Len; //Bits per value

						uint8_t mebl = 0;
						const NBHRFV mv = NBHRFV(minV, bits);
						const NBHRFV rv = NBHRFV(maxV - minV, bits);
						if (mv.Exponent > 0 || rv.Exponent > 0)
						{
							r += 5; //Bits per exponent
							if (mv.Exponent > 0)
							{
								mebl = BitLen(mv.Exponent);
								CLOAK_ASSUME((1Ui64 << mebl) > mv.Exponent);
								CLOAK_ASSUME(mebl > 0);
							}
							if (rv.Exponent > 0)
							{
								const uint8_t m = BitLen(rv.Exponent);
								mebl = max(m, mebl);
								CLOAK_ASSUME((1Ui64 << mebl) > rv.Exponent);
								CLOAK_ASSUME(mebl > 0);
							}
						}
						if (byteAligned == true)
						{
							mebl = AlignToByte(mebl);
						}
						if (std::abs(minV) > 1)
						{
							r += 1; //Min value negative
							r += mebl + 1; //Min exponent
							r += bits.Bits; //Min value
						}
						else
						{
							r += 1; //Min value negative
							r += bits.Bits; //Min value
						}

						UINT pvb = rv.Exponent == 0 ? BitLen(rv.Value) : bits.Bits;
						if (byteAligned == true)
						{
							r = AlignToByte(r);
							pvb = AlignToByte(pvb);
						}
						CLOAK_ASSUME(pvb < (1U << bits.Len));
						return std::make_tuple(r, mebl, pvb);
					}
					//Tries to estimate compression quality with current filter. Lower values are better
					uint64_t CLOAK_CALL FilterQuality(In const FixedTexture& texture, In size_t c, In const BitInfo& bits, In UINT xs, In UINT ys, In UINT tileSize, In uint8_t blockBits, In bool byteAligned)
					{
						uint64_t res = 0;

						int64_t bMin = 0;
						int64_t bMax = 0;
						int64_t sMin = 0;
						int64_t sMax = 0;
						UINT lsc = 0;
						UINT lbc = 0;
						UINT lsx = 0;
						UINT lsy = 0;
						UINT lbx = 0;
						UINT lby = 0;
						UINT bbl = 0;
						UINT nbl = 0;
						UINT nsl = 0;

						for (UINT y = ys, yi = 0; y < texture.Size.H && yi < tileSize; y++, yi++)
						{
							for (UINT x = xs, xi = 0; x < texture.Size.W && xi < tileSize; x++, xi++)
							{
								int64_t cv = texture.Get(x, y)[c];
								if (xi == 0 && yi == 0)
								{
									bMin = bMax = sMin = sMax = cv;
								}
								else
								{
									lsc++;
									lbc++;
									int64_t lbMin = bMin;
									int64_t lbMax = bMax;
									bool upd = false;
									if (cv < bMin) { bMin = cv; sMin = cv; sMax = cv; lsx = x; lsy = y; lsc = 0; upd = true; }
									if (cv > bMax) { bMax = cv; sMin = cv; sMax = cv; lsx = x; lsy = y; lsc = 0; upd = true; }
									if (cv < sMin) { sMin = cv; lsx = x; lsy = y; lsc = 0; upd = true; }
									if (cv > sMax) { sMax = cv; lsx = x; lsy = y; lsc = 0; upd = true; }

									if (upd == true || nsl < bbl)
									{
										//meta.first = Header Len
										//meta.second = Exponent Len
										//meta.third = Per Value Bits
										const std::tuple<UINT, UINT, UINT> meta = BlockHeaderLen(bMin, bMax, blockBits, bits.Channel[c], byteAligned);
										nbl = RequiredBits(bMax - bMin, std::get<1>(meta), bits.Channel[c], byteAligned);
										nsl = RequiredBits(sMax - sMin, std::get<1>(meta), bits.Channel[c], byteAligned);

										if (nbl > bbl)
										{
											if ((nbl - bbl) * lbc > std::get<0>(meta))
											{
												const std::tuple<UINT, UINT, UINT> nm = BlockHeaderLen(lbMin, lbMax, blockBits, bits.Channel[c], byteAligned);
												res += std::get<0>(nm);
												res += lbc * (((lbMax - lbMin) > 0 ? std::get<1>(nm) : 0) + std::get<2>(nm));

												lbx = x;
												lby = y;
												bMin = bMax = sMin = sMax = cv;
												bbl = 0;
												lbc = 0;
												lsc = 0;
											}
											else
											{
												bbl = nbl;
											}
										}
										else if (nsl < bbl && (bbl - nsl)*lsc > std::get<0>(meta))
										{
											const std::tuple<UINT, UINT, UINT> nm = BlockHeaderLen(lbMin, lbMax, blockBits, bits.Channel[c], byteAligned);
											res += std::get<0>(nm);
											res += (lbc - lsc) * (((lbMax - lbMin) > 0 ? std::get<1>(nm) : 0) + std::get<2>(nm));

											lbc = lsc;
											lbx = lsx;
											lby = lsy;
											bbl = nsl;
											bMin = sMin;
											bMax = sMax;
											sMin = cv;
											sMax = cv;
											lsc = 0;
											lsx = x;
											lsy = y;
										}
									}
								}
							}
						}
						const std::tuple<UINT, UINT, UINT> meta = BlockHeaderLen(bMin, bMax, blockBits, bits.Channel[c], byteAligned);
						res += std::get<0>(meta);
						res += (lbc + 1) * (((bMax - bMin) > 0 ? std::get<1>(meta) : 0) + std::get<2>(meta));
						return res;
					}

					V1008_ENCODE_FUNC(Encode)
					{
						FixedTexture tex[2] = { {texture.Size, bits}, {texture.Size, bits} };
						FixedTexture src(texture, bits);
						FixedTexture pMip(prevMip, bits);

						const UINT tileSize = blockSize == 0 ? max(src.Size.W, src.Size.H) : blockSize;
						const UINT tileW = (src.Size.W + tileSize - 1) / tileSize;
						const UINT tileH = (src.Size.H + tileSize - 1) / tileSize;
						const UINT tileCount = tileW * tileH;
						write->WriteDynamic(tileSize);
						const uint8_t blockBits = BitLen(src.Size.W * src.Size.H);

						//saves used tex id per color channel per tile (bt[channelID + (4 * tileID)])
						size_t* bt = NewArray(size_t, tileCount * 4);
						for (size_t a = 0; a < tileCount * 4; a++) { bt[a] = ARRAYSIZE(tex) - 1; }

						//Apply best filter to all tiles:
						for (UINT a = 0; a < 4; a++)
						{
							if ((static_cast<uint8_t>(channels) & (1 << a)) != 0)
							{
								for (UINT y = 0, tid = a; y < src.Size.H; y += tileSize)
								{
									for (UINT x = 0; x < src.Size.W; tid += 4, x += tileSize)
									{
										//Calculate mode for current tile:
										uint64_t bh = 0;
										size_t bm = 0;
										for (size_t m = 0; m < ARRAYSIZE(MODES); m++)
										{
											CLOAK_ASSUME(tid < (tileCount * 4));
											const size_t t = (bt[tid] + 1) % ARRAYSIZE(tex);
											CLOAK_ASSUME(t < ARRAYSIZE(tex));
											ApplyFilter(&tex[t], src, m, a, x, y, tileSize, prevMip == nullptr ? nullptr : &pMip);
											const uint64_t h = FilterQuality(tex[t], a, bits, x, y, tileSize, blockBits, byteAligned);
											if (m == 0 || h < bh)
											{
												bh = h;
												bt[tid] = t;
												bm = m;
											}
										}
										write->WriteBits(4, bm);
									}
								}
							}
						}
						if (byteAligned == true) { write->MoveToNextByte(); }


						//Write color values:
						for (UINT a = 0; a < 4; a++)
						{
							if ((static_cast<uint8_t>(channels) & (1 << a)) != 0)
							{
								struct {
									int64_t Max = 0;
									int64_t Min = 0;
									UINT Count = 0;
									UINT StartX = 0;
									UINT StartY = 0;
									uint8_t ByteLen = 0;
								} lBox;
								struct {
									struct {
										int64_t Max = 0;
										int64_t Min = 0;
										UINT StartX = 0;
										UINT StartY = 0;
										UINT Count = 0;
										uint8_t ByteLen = 0;
										struct {
											int64_t Max = 0;
											int64_t Min = 0;
											uint8_t ByteLen = 0;
										} Previous;
									} Max;
									struct {
										int64_t Max = 0;
										int64_t Min = 0;
										UINT StartX = 0;
										UINT StartY = 0;
										UINT Count = 0;
										uint8_t ByteLen = 0;
										struct {
											int64_t Max = 0;
											int64_t Min = 0;
											uint8_t ByteLen = 0;
										} Previous;
									} Min;
								} sBox;

								for (UINT y = 0; y < src.Size.H; y++)
								{
									for (UINT x = 0; x < src.Size.W; x++)
									{
										const size_t tid = ((y / tileSize) * tileW) + (x / tileSize);
										CLOAK_ASSUME(a + (tid * 4) < (tileCount * 4));
										const size_t t = bt[a + (4 * tid)];
										CLOAK_ASSUME(t < ARRAYSIZE(tex));
										const int64_t cv = tex[t].Get(x, y)[a];
										if (x == 0 && y == 0)
										{
											lBox.Max = lBox.Min = cv;
											sBox.Max.Max = sBox.Max.Min = cv;
											sBox.Min.Max = sBox.Min.Min = cv;
										}
										else
										{
											lBox.Count++;
											sBox.Max.Count++;
											sBox.Min.Count++;
											int64_t llMax = lBox.Max;
											int64_t llMin = lBox.Min;
											bool upd = false;
											if (cv < lBox.Min) { lBox.Min = cv; upd = true; }
											if (cv > lBox.Max) { lBox.Max = cv; upd = true; }
											if (cv < sBox.Min.Min)
											{
												sBox.Min.Previous.Min = min(sBox.Min.Min, sBox.Min.Previous.Min);
												sBox.Min.Previous.Max = max(sBox.Min.Max, sBox.Min.Previous.Max);
												sBox.Min.Max = sBox.Min.Min = cv;
												sBox.Min.StartX = x;
												sBox.Min.StartY = y;
												sBox.Min.Count = 0;
												upd = true;
											}
											if (cv > sBox.Max.Max)
											{
												sBox.Max.Previous.Min = min(sBox.Max.Min, sBox.Max.Previous.Min);
												sBox.Max.Previous.Max = max(sBox.Max.Max, sBox.Max.Previous.Max);
												sBox.Max.Max = sBox.Max.Min = cv;
												sBox.Max.StartX = x;
												sBox.Max.StartY = y;
												sBox.Max.Count = 0;
												upd = true;
											}
											if (cv > sBox.Min.Max) { sBox.Min.Max = cv; upd = true; }
											if (cv < sBox.Max.Min) { sBox.Max.Min = cv; upd = true; }
											if (upd == true || sBox.Max.ByteLen < lBox.ByteLen || sBox.Min.ByteLen < lBox.ByteLen)
											{
												//meta.first = Header Len
												//meta.second = Exponent Len
												//meta.third = Per Value Bits
												const std::tuple<UINT, UINT, UINT> meta = BlockHeaderLen(lBox.Min, lBox.Max, blockBits, bits.Channel[a], byteAligned);
												const uint8_t llBl = lBox.ByteLen;
												lBox.ByteLen = RequiredBits(lBox.Max - lBox.Min, std::get<1>(meta), bits.Channel[a], byteAligned);
												sBox.Max.ByteLen = RequiredBits(sBox.Max.Max - sBox.Max.Min, std::get<1>(meta), bits.Channel[a], byteAligned);
												sBox.Max.Previous.ByteLen = RequiredBits(sBox.Max.Previous.Max - sBox.Max.Previous.Min, std::get<1>(meta), bits.Channel[a], byteAligned);
												sBox.Min.ByteLen = RequiredBits(sBox.Min.Max - sBox.Min.Min, std::get<1>(meta), bits.Channel[a], byteAligned);
												sBox.Min.Previous.ByteLen = RequiredBits(sBox.Min.Previous.Max - sBox.Min.Previous.Min, std::get<1>(meta), bits.Channel[a], byteAligned);
												if (lBox.ByteLen > llBl && (lBox.ByteLen - llBl) * lBox.Count > std::get<0>(meta))
												{
													const std::tuple<UINT, UINT, UINT> nm = BlockHeaderLen(llMin, llMax, blockBits, bits.Channel[a], byteAligned);
													CLOAK_ASSUME(lBox.Count == (x + (y * src.Size.W)) - (lBox.StartX + (lBox.StartY * src.Size.W)));
													WriteHeader(write, llMin, llMax, lBox.Count, std::get<1>(nm), std::get<2>(nm), blockBits, bits.Channel[a], byteAligned);
#if defined(_DEBUG) && defined(PRINT_ENCODING)
													CloakDebugLog("Write[" + std::to_string(a) + "] Min Value = " + std::to_string(llMin) + " values = " + std::to_string(lBox.Count) + " valueBits = " + std::to_string(std::get<2>(nm)) + " exponentBits = " + std::to_string(std::get<1>(nm)));
#endif
													for (UINT dy = lBox.StartY; dy <= y; dy++)
													{
														for (UINT dx = (dy == lBox.StartY ? lBox.StartX : 0); dx < x || (dy < y && dx < src.Size.W); dx++)
														{
															const size_t ntid = ((dy / tileSize) * tileW) + (dx / tileSize);
															CLOAK_ASSUME(a + (ntid * 4) < (tileCount * 4));
															const size_t nt = bt[a + (4 * ntid)];
															const int64_t v = tex[nt].Get(dx, dy)[a];
															WriteValue(write, llMin, llMax, v, std::get<1>(nm), std::get<2>(nm), bits.Channel[a]);
#if defined(_DEBUG) && defined(PRINT_ENCODING)
															CloakDebugLog("Write[" + std::to_string(a) + "][" + std::to_string(dx) + "][" + std::to_string(dy) + "] = " + std::to_string(texture.Get(dx, dy)[a]) + " as Integer = " + std::to_string(src.Get(dx, dy)[a]) + " (P = " + std::to_string(src.Get(dx, dy)[a] - v) + ") Write: " + std::to_string(v - llMin));
#endif
														}
													}
													lBox.StartX = x;
													lBox.StartY = y;
													lBox.Count = 0;
													lBox.ByteLen = 0;
													lBox.Max = cv;
													lBox.Min = cv;

													sBox.Max.Count = 0;
													sBox.Max.StartX = x;
													sBox.Max.StartY = y;
													sBox.Max.Max = sBox.Max.Min = cv;
													sBox.Max.Previous.Max = sBox.Max.Previous.Min = cv;
													sBox.Max.ByteLen = 0;
													
													sBox.Min.Count = 0;
													sBox.Min.StartX = x;
													sBox.Min.StartY = y;
													sBox.Min.Max = sBox.Min.Max = cv;
													sBox.Min.Previous.Max = sBox.Min.Previous.Min = cv;
													sBox.Min.ByteLen = 0;
												}
												else
												{
													if (((lBox.Count - sBox.Max.Count) * sBox.Max.Previous.ByteLen) + (sBox.Max.Count * sBox.Max.ByteLen) < ((lBox.Count - sBox.Min.Count) * sBox.Min.Previous.ByteLen) + (sBox.Min.Count * sBox.Min.ByteLen))
													{
														//try with sBox.max
														if ((sBox.Max.Previous.ByteLen * (lBox.Count - sBox.Max.Count)) + (sBox.Max.Count * sBox.Max.ByteLen) + std::get<0>(meta) < lBox.ByteLen * lBox.Count)
														{
															const std::tuple<UINT, UINT, UINT> nm = BlockHeaderLen(sBox.Max.Previous.Min, sBox.Max.Previous.Max, blockBits, bits.Channel[a], byteAligned);
															CLOAK_ASSUME(lBox.Count - sBox.Max.Count == (sBox.Max.StartX + (sBox.Max.StartY * src.Size.W)) - (lBox.StartX + (lBox.StartY * src.Size.W)));
															WriteHeader(write, sBox.Max.Previous.Min, sBox.Max.Previous.Max, lBox.Count - sBox.Max.Count, std::get<1>(nm), std::get<2>(nm), blockBits, bits.Channel[a], byteAligned);
#if defined(_DEBUG) && defined(PRINT_ENCODING)
															CloakDebugLog("Write[" + std::to_string(a) + "] Min Value = " + std::to_string(sBox.Max.Previous.Min) + " values = " + std::to_string(lBox.Count - sBox.Max.Count) + " valueBits = " + std::to_string(std::get<2>(nm)) + " exponentBits = " + std::to_string(std::get<1>(nm)));
#endif
															for (UINT dy = lBox.StartY; dy <= sBox.Max.StartY; dy++)
															{
																for (UINT dx = (dy == lBox.StartY ? lBox.StartX : 0); dx < sBox.Max.StartX || (dy < sBox.Max.StartY && dx < src.Size.W); dx++)
																{
																	const size_t ntid = ((dy / tileSize) * tileW) + (dx / tileSize);
																	CLOAK_ASSUME(a + (ntid * 4) < (tileCount * 4));
																	const size_t nt = bt[a + (4 * ntid)];
																	const int64_t v = tex[nt].Get(dx, dy)[a];
																	WriteValue(write, sBox.Max.Previous.Min, sBox.Max.Previous.Max, v, std::get<1>(nm), std::get<2>(nm), bits.Channel[a]);
#if defined(_DEBUG) && defined(PRINT_ENCODING)
																	CloakDebugLog("Write[" + std::to_string(a) + "][" + std::to_string(dx) + "][" + std::to_string(dy) + "] = " + std::to_string(texture.Get(dx, dy)[a]) + " as Integer = " + std::to_string(src.Get(dx, dy)[a]) + " (P = " + std::to_string(src.Get(dx, dy)[a] - v) + ") Write: " + std::to_string(v - sBox.Max.Previous.Min));
#endif
																}
															}

															lBox.ByteLen = sBox.Max.ByteLen;
															lBox.Count = sBox.Max.Count;
															lBox.Max = sBox.Max.Max;
															lBox.Min = sBox.Max.Min;
															lBox.StartX = sBox.Max.StartX;
															lBox.StartY = sBox.Max.StartY;

															for (UINT dy = lBox.StartY; dy <= y; dy++)
															{
																for (UINT dx = (dy == lBox.StartY ? lBox.StartX : 0); dx < x || (dy < y && dx < src.Size.W); dx++)
																{
																	const size_t ntid = ((dy / tileSize) * tileW) + (dx / tileSize);
																	CLOAK_ASSUME(a + (ntid * 4) < (tileCount * 4));
																	const size_t nt = bt[a + (4 * ntid)];
																	const int64_t v = tex[nt].Get(dx, dy)[a];
																	if (v > sBox.Max.Previous.Max || (dy == lBox.StartY && dx == lBox.StartX)) { sBox.Max.Previous.Max = v; }
																	if (v < sBox.Max.Previous.Min || (dy == lBox.StartY && dx == lBox.StartX)) { sBox.Max.Previous.Min = v; }
																}
															}

															if (sBox.Min.Count >= sBox.Max.Count)
															{
																sBox.Min.ByteLen = 0;
																sBox.Min.Count = 0;
																sBox.Min.Max = cv;
																sBox.Min.Min = cv;
																sBox.Min.StartX = x;
																sBox.Min.StartY = y;
																sBox.Min.Previous.Max = sBox.Max.Previous.Max;
																sBox.Min.Previous.Min = sBox.Max.Previous.Min;
															}

															sBox.Max.ByteLen = 0;
															sBox.Max.Count = 0;
															sBox.Max.Max = cv;
															sBox.Max.Min = cv;
															sBox.Max.StartX = x;
															sBox.Max.StartY = y;
														}
													}
													else
													{
														//try with sBox.min
														if ((sBox.Min.Previous.ByteLen * (lBox.Count - sBox.Min.Count)) + (sBox.Min.Count * sBox.Min.ByteLen) + std::get<0>(meta) < lBox.ByteLen * lBox.Count)
														{
															const std::tuple<UINT, UINT, UINT> nm = BlockHeaderLen(sBox.Min.Previous.Min, sBox.Min.Previous.Max, blockBits, bits.Channel[a], byteAligned);
															CLOAK_ASSUME(lBox.Count - sBox.Min.Count == (sBox.Min.StartX + (sBox.Min.StartY * src.Size.W)) - (lBox.StartX + (lBox.StartY * src.Size.W)));
															WriteHeader(write, sBox.Min.Previous.Min, sBox.Min.Previous.Max, lBox.Count - sBox.Min.Count, std::get<1>(nm), std::get<2>(nm), blockBits, bits.Channel[a], byteAligned);
#if defined(_DEBUG) && defined(PRINT_ENCODING)
															CloakDebugLog("Write[" + std::to_string(a) + "] Min Value = " + std::to_string(sBox.Min.Previous.Min) + " values = " + std::to_string(lBox.Count - sBox.Min.Count) + " valueBits = " + std::to_string(std::get<2>(nm)) + " exponentBits = " + std::to_string(std::get<1>(nm)));
#endif
															for (UINT dy = lBox.StartY; dy <= sBox.Min.StartY; dy++)
															{
																for (UINT dx = (dy == lBox.StartY ? lBox.StartX : 0); dx < sBox.Min.StartX || (dy < sBox.Min.StartY && dx < src.Size.W); dx++)
																{
																	const size_t ntid = ((dy / tileSize) * tileW) + (dx / tileSize);
																	CLOAK_ASSUME(a + (ntid * 4) < (tileCount * 4));
																	const size_t nt = bt[a + (4 * ntid)];
																	const int64_t v = tex[nt].Get(dx, dy)[a];
																	WriteValue(write, sBox.Min.Previous.Min, sBox.Min.Previous.Max, v, std::get<1>(nm), std::get<2>(nm), bits.Channel[a]);
#if defined(_DEBUG) && defined(PRINT_ENCODING)
																	CloakDebugLog("Write[" + std::to_string(a) + "][" + std::to_string(dx) + "][" + std::to_string(dy) + "] = " + std::to_string(texture.Get(dx, dy)[a]) + " as Integer = " + std::to_string(src.Get(dx, dy)[a]) + " (P = " + std::to_string(src.Get(dx, dy)[a] - v) + ") Write: " + std::to_string(v - sBox.Min.Previous.Min));
#endif
																}
															}

															lBox.ByteLen = sBox.Min.ByteLen;
															lBox.Count = sBox.Min.Count;
															lBox.Max = sBox.Min.Max;
															lBox.Min = sBox.Min.Min;
															lBox.StartX = sBox.Min.StartX;
															lBox.StartY = sBox.Min.StartY;

															for (UINT dy = lBox.StartY; dy <= y; dy++)
															{
																for (UINT dx = (dy == lBox.StartY ? lBox.StartX : 0); dx < x || (dy < y && dx < src.Size.W); dx++)
																{
																	const size_t ntid = ((dy / tileSize) * tileW) + (dx / tileSize);
																	CLOAK_ASSUME(a + (ntid * 4) < (tileCount * 4));
																	const size_t nt = bt[a + (4 * ntid)];
																	const int64_t v = tex[nt].Get(dx, dy)[a];
																	if (v > sBox.Min.Previous.Max || (dy == lBox.StartY && dx == lBox.StartX)) { sBox.Min.Previous.Max = v; }
																	if (v < sBox.Min.Previous.Min || (dy == lBox.StartY && dx == lBox.StartX)) { sBox.Min.Previous.Min = v; }
																}
															}

															if (sBox.Max.Count >= sBox.Min.Count)
															{
																sBox.Max.ByteLen = 0;
																sBox.Max.Count = 0;
																sBox.Max.Max = cv;
																sBox.Max.Min = cv;
																sBox.Max.StartX = x;
																sBox.Max.StartY = y;
																sBox.Max.Previous.Max = sBox.Min.Previous.Max;
																sBox.Max.Previous.Min = sBox.Min.Previous.Min;
															}

															sBox.Min.ByteLen = 0;
															sBox.Min.Count = 0;
															sBox.Min.Max = cv;
															sBox.Min.Min = cv;
															sBox.Min.StartX = x;
															sBox.Min.StartY = y;
														}
													}
												}
											}
										}
									}
								}
								CLOAK_ASSUME(lBox.Count + 1 == (src.Size.W * src.Size.H) - (lBox.StartX + (lBox.StartY * src.Size.W)));
								const std::tuple<UINT, UINT, UINT> nm = BlockHeaderLen(lBox.Min, lBox.Max, blockBits, bits.Channel[a], byteAligned);
								WriteHeader(write, lBox.Min, lBox.Max, lBox.Count + 1, std::get<1>(nm), std::get<2>(nm), blockBits, bits.Channel[a], byteAligned);
#if defined(_DEBUG) && defined(PRINT_ENCODING)
								CloakDebugLog("Write[" + std::to_string(a) + "] Min Value = " + std::to_string(lBox.Min) + " values = " + std::to_string(lBox.Count + 1) + " valueBits = " + std::to_string(std::get<2>(nm)) + " exponentBits = " + std::to_string(std::get<1>(nm)));
#endif
								for (UINT dy = lBox.StartY; dy < src.Size.H; dy++)
								{
									for (UINT dx = (dy == lBox.StartY ? lBox.StartX : 0); dx < src.Size.W; dx++)
									{
										const size_t ntid = ((dy / tileSize) * tileW) + (dx / tileSize);
										CLOAK_ASSUME(a + (ntid * 4) < (tileCount * 4));
										const size_t nt = bt[a + (4 * ntid)];
										const int64_t v = tex[nt].Get(dx, dy)[a];
										WriteValue(write, lBox.Min, lBox.Max, v, std::get<1>(nm), std::get<2>(nm), bits.Channel[a]);
#if defined(_DEBUG) && defined(PRINT_ENCODING)
										CloakDebugLog("Write[" + std::to_string(a) + "][" + std::to_string(dx) + "][" + std::to_string(dy) + "] = " + std::to_string(texture.Get(dx, dy)[a]) + " as Integer = " + std::to_string(src.Get(dx, dy)[a]) + " (P = " + std::to_string(src.Get(dx, dy)[a] - v) + ") Write: " + std::to_string(v - lBox.Min));
#endif
									}
								}
							}
						}
						DeleteArray(bt);
						return true;
					}
				}

				enum RequiredBits { REQ_ANY, REQ_RGBA, REQ_YCCA, REQ_RGBE, REQ_MATERIAL };

				constexpr EncodeFilter FILTER[] = { NBF::Encode,	QTF::Encode,	HRF::Encode,	NBHRF::Encode };
				constexpr RequiredBits FILREQ[] = { REQ_ANY,		REQ_ANY,		REQ_RGBE,		REQ_ANY };
				static_assert(ARRAYSIZE(FILTER) <= ARRAYSIZE(FILREQ), "Every Filter needs to have a requirement setting!");

				inline bool CLOAK_CALL CanUseDiff(In const MipMapLevel& tex, In_opt const MipMapLevel* prevTex = nullptr, In_opt const MipMapLevel* prevMip = nullptr)
				{
					return false; //Deactivated due to clculation issues
					if (prevTex == nullptr) { return false; }
					if (prevTex->Size.W != tex.Size.W || prevTex->Size.H != tex.Size.H) { return false; }
					//if (QTF::CanUse(tex, prevMip) == true) { return false; }
					return true;
				}
				inline bool CLOAK_CALL CalculateTextureDifference(In const MipMapLevel& tex, In const MipMapLevel& prev, Out MipMapLevel* res)
				{
					float oMin[4] = { 1,1,1,1 };
					float oMax[4] = { 0,0,0,0 };
					float nMin[4] = { 1,1,1,1 };
					float nMax[4] = { 0,0,0,0 };
					for (UINT y = 0; y < tex.Size.H; y++)
					{
						for (UINT x = 0; x < tex.Size.W; x++)
						{
							const CloakEngine::Helper::Color::RGBA& c1 = tex.Get(x, y);
							const CloakEngine::Helper::Color::RGBA& c2 = prev.Get(x, y);
							CloakEngine::Helper::Color::RGBA cr;
							for (size_t c = 0; c < 4; c++)
							{
								oMin[c] = min(oMin[c], c1[c]);
								oMax[c] = max(oMax[c], c1[c]);
								cr[c] = c1[c] - c2[c];
								nMin[c] = min(nMin[c], cr[c]);
								nMax[c] = max(nMax[c], cr[c]);
							}
							res->Set(x, y, cr);
						}
					}
					float od = 0;
					float nd = 0;
					for (size_t c = 0; c < 4; c++)
					{
						od += oMax[c] - oMin[c];
						nd += nMax[c] - nMin[c];
					}
					return nd < od;
				}
				bool CLOAK_CALL EncodeTexture(In CloakEngine::Files::IWriter* write, In const BitInfo& bits, In const MipMapLevel& texture, In const ResponseCaller& response, In size_t imgNum, In size_t texNum, In API::Image::v1008::ColorChannel channels, In uint16_t blockSize, In bool hr, In_opt const MipMapLevel* prevTex, In_opt const MipMapLevel* prevMip)
				{
					MipMapLevel tex = texture;
					if (CanUseDiff(texture, prevTex, prevMip))
					{
						MipMapLevel dif;
						bool usePrev = CalculateTextureDifference(texture, *prevTex, &dif);
						write->WriteBits(1, usePrev ? 1 : 0);
						if (usePrev) { tex = dif; }
					}
					else { write->WriteBits(1, 0); }
					size_t mode = 0;
					CloakEngine::Files::IWriter* bstWrite = nullptr;
					CloakEngine::Files::IVirtualWriteBuffer* bstBuff = nullptr;
					for (size_t a = 0; a < ARRAYSIZE(FILTER); a++)
					{
#ifdef FORCED_FILTER
						if (a != FORCED_FILTER) { continue; }
#endif
						CloakEngine::Files::IVirtualWriteBuffer* vwb = CloakEngine::Files::CreateVirtualWriteBuffer();
						CloakEngine::Files::IWriter* vw = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&vw));
						vw->SetTarget(vwb);
						bool use = FILTER[a](vw, tex, bits, channels, blockSize, response, imgNum, texNum, hr, false, prevMip);
						if (use)
						{
							if (bstWrite == nullptr || bstWrite->GetPosition() > vw->GetPosition() || (bstWrite->GetPosition() == vw->GetPosition() && bstWrite->GetBitPosition() > vw->GetBitPosition()))
							{
								SWAP(bstWrite, vw);
								SWAP(bstBuff, vwb);
								mode = a;
							}
						}
						SAVE_RELEASE(vw);
						SAVE_RELEASE(vwb);
					}
					CloakDebugLog("Encoding texture with mode " + std::to_string(mode));
					write->WriteBits(8, mode);
					const unsigned long long byp = bstWrite->GetPosition();
					const unsigned int bip = bstWrite->GetBitPosition();
					bstWrite->Save();
					write->WriteBuffer(bstBuff, byp, bip);
					SAVE_RELEASE(bstWrite);
					SAVE_RELEASE(bstBuff);
					return false;
				}
				bool CLOAK_CALL EncodeTexture(In CloakEngine::Files::IWriter* write, In const BitInfoList& bitInfo, In const MipMapLevel& texture, In const TextureMeta& meta, In const ResponseCaller& response, In size_t imgNum, In size_t texNum, In API::Image::v1008::ColorChannel channels, In bool hr, In_opt const MipMapLevel* prevTex, In_opt const MipMapLevel* prevMip)
				{
					write->WriteDynamic(texture.Size.W);
					write->WriteDynamic(texture.Size.H);
					MipMapLevel tex = texture;
					if (CanColorConvert(meta.Encoding.Space, channels) == false)
					{
						SendResponse(response, API::Image::v1008::RespondeState::ERR_COLOR_CHANNELS, imgNum, "Cannot convert to given color space (wrong channels are used!)", texNum + 1);
						return true;
					}
					if (CanUseDiff(texture, prevTex, prevMip))
					{
						MipMapLevel dif;
						bool usePrev = CalculateTextureDifference(texture, *prevTex, &dif);
						write->WriteBits(1, usePrev ? 1 : 0);
						if (usePrev) { tex = dif; }
					}
					else { write->WriteBits(1, 0); }
					if (meta.Encoding.Space == API::Image::v1008::ColorSpace::YCCA) { ConvertToYCCA(&tex); }
					BitInfo bits;
					if (meta.Encoding.Space == API::Image::v1008::ColorSpace::RGBA) { bits = bitInfo.RGBA; }
					else if (meta.Encoding.Space == API::Image::v1008::ColorSpace::YCCA) { bits = bitInfo.YCCA; }
					else if (meta.Encoding.Space == API::Image::v1008::ColorSpace::Material) { bits = bitInfo.Material; }
					else
					{
						SendResponse(response, API::Image::v1008::RespondeState::ERR_ILLEGAL_PARAM, imgNum, "Unknown color space", texNum + 1);
						return true;
					}

					size_t mode = 0;
					CloakEngine::Files::IWriter* bstWrite = nullptr;
					CloakEngine::Files::IVirtualWriteBuffer* bstBuff = nullptr;
					for (size_t a = 0; a < ARRAYSIZE(FILTER); a++)
					{
#ifdef FORCED_FILTER
						if (a != FORCED_FILTER) { continue; }
#endif
						CloakEngine::Files::IVirtualWriteBuffer* vwb = CloakEngine::Files::CreateVirtualWriteBuffer();
						CloakEngine::Files::IWriter* vw = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&vw));
						vw->SetTarget(vwb);
						BitInfo b = bits;
						switch (FILREQ[a])
						{
							case REQ_ANY: break;
							case REQ_RGBA: b = bitInfo.RGBA; break;
							case REQ_RGBE: b = bitInfo.RGBE; break;
							case REQ_YCCA: b = bitInfo.YCCA; break;
							case REQ_MATERIAL: b = bitInfo.Material; break;
							default: break;
						}
						bool use = FILTER[a](vw, tex, bits, channels, meta.Encoding.BlockSize, response, imgNum, texNum, hr, false, prevMip);
						if (use)
						{
							CloakDebugLog("Resulting Length of mode " + std::to_string(a) + ": " + std::to_string(vw->GetPosition()) + " Bytes and " + std::to_string(vw->GetBitPosition()) + " Bits");
							if (bstWrite == nullptr || bstWrite->GetPosition() > vw->GetPosition() || (bstWrite->GetPosition() == vw->GetPosition() && bstWrite->GetBitPosition() > vw->GetBitPosition()))
							{
								SWAP(bstWrite, vw);
								SWAP(bstBuff, vwb);
								mode = a;
							}
						}
						SAVE_RELEASE(vw);
						SAVE_RELEASE(vwb);
					}
					CloakDebugLog("Encoding texture with mode " + std::to_string(mode));
					write->WriteBits(8, mode);
					const unsigned long long byp = bstWrite->GetPosition();
					const unsigned int bip = bstWrite->GetBitPosition();
					bstWrite->Save();
					write->WriteBuffer(bstBuff, byp, bip);
					SAVE_RELEASE(bstWrite);
					SAVE_RELEASE(bstBuff);
					return false;
				}
				void CLOAK_CALL EncodeAlphaMap(In CloakEngine::Files::IWriter* write, In const MipMapLevel& texture)
				{
					for (UINT y = 0; y < texture.Size.H; y++)
					{
						for (UINT x = 0; x < texture.Size.W; x++)
						{
							const UINT p = (texture.Size.W*y) + x;
							write->WriteBits(1, texture[p].A < ALPHAMAP_THRESHOLD);
						}
					}
				}
				bool CLOAK_CALL CheckHR(In const MipTexture& tex, In API::Image::ColorChannel channels)
				{
					if (tex.levelCount > 0)
					{
						for (UINT y = 0; y < tex.levels[0].Size.H; y++)
						{
							for (UINT x = 0; x < tex.levels[0].Size.W; x++)
							{
								const CloakEngine::Helper::Color::RGBA& c = tex.levels[0].Get(x, y);
								for (size_t a = 0; a < 4; a++)
								{
									if ((static_cast<uint8_t>(channels) & (1 << a)) != 0 && (c[a] < 0 || c[a]>1)) { return true; }
								}
							}
						}
					}
					return false;
				}

				namespace Lib {
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

					bool CLOAK_CALL EncodeTexture(In const CE::RefPointer<CE::Files::IWriter> file, In const MipTexture& texture, In API::Image::v1008::ColorChannel channel, In const BitInfo& bits, In bool writeMipMaps, In size_t entryID, In size_t textureID, In size_t tabCount)
					{
						const std::string ns = "E" + std::to_string(entryID) + "T" + std::to_string(textureID);
						size_t mlc = texture.levelCount;
						if (writeMipMaps == false) { mlc = min(mlc, 1); }
						if (mlc > 0)
						{
							uint32_t bitsPerChannel = 0;
							uint32_t channelCount = 0;
							for (size_t c = 0; c < 4; c++)
							{
								if (IsFlagSet(channel, static_cast<API::Image::v1008::ColorChannel>(c))) 
								{ 
									channelCount++; 
									bitsPerChannel = max(bitsPerChannel, bits.Channel[c].Bits);
								}
							}
							if (channelCount == 3) { channelCount = 4; }
							bitsPerChannel = CE::Global::Math::CeilPower2(min(32, max(bitsPerChannel, 8)));

							bool hdr = false;
							for (size_t a = 0; a < mlc; a++)
							{
								for (UINT y = 0; y < texture.levels[a].Size.H; y++)
								{
									for (UINT x = 0; x < texture.levels[a].Size.W; x++)
									{
										const CE::Helper::Color::RGBA& col = texture.levels[a].Get(x, y);
										for (size_t c = 0; c < 4; c++)
										{
											if (IsFlagSet(channel, static_cast<API::Image::v1008::ColorChannel>(c)) && (col[c] < 0 || col[c] > 1))
											{
												hdr = true;
												goto find_format;
											}
										}
									}
								}
							}

						find_format:
							size_t formatID = ARRAYSIZE(FORMAT_TYPE_NAMES);
							{
								bool formatRoundUp = false;
								bool formatRoundDown = false;
								for (size_t a = 0; a < ARRAYSIZE(FORMAT_TYPE_NAMES); a++)
								{
									if (bitsPerChannel >= FORMAT_TYPE_NAMES[a].MinMemorySize && bitsPerChannel <= FORMAT_TYPE_NAMES[a].MaxMemorySize && hdr == FORMAT_TYPE_NAMES[a].HDR)
									{
										formatID = a;
										goto encode;
									}
									else if (hdr == false || FORMAT_TYPE_NAMES[a].HDR == true)
									{
										if (bitsPerChannel >= FORMAT_TYPE_NAMES[a].MinMemorySize) { formatRoundDown = true; }
										if (bitsPerChannel <= FORMAT_TYPE_NAMES[a].MaxMemorySize) { formatRoundUp = true; }
									}
								}
								if (hdr == false)
								{
									for (size_t a = 0; a < ARRAYSIZE(FORMAT_TYPE_NAMES); a++)
									{
										if (bitsPerChannel >= FORMAT_TYPE_NAMES[a].MinMemorySize && bitsPerChannel <= FORMAT_TYPE_NAMES[a].MaxMemorySize && FORMAT_TYPE_NAMES[a].HDR == true)
										{
											formatID = a;
											goto encode;
										}
									}
								}
								if (formatRoundUp == true)
								{
									for (size_t a = 0; a < ARRAYSIZE(FORMAT_TYPE_NAMES); a++)
									{
										if (bitsPerChannel <= FORMAT_TYPE_NAMES[a].MaxMemorySize && hdr == FORMAT_TYPE_NAMES[a].HDR)
										{
											formatID = a;
											bitsPerChannel = FORMAT_TYPE_NAMES[a].MinMemorySize;
											goto encode;
										}
									}
									if (hdr == false)
									{
										for (size_t a = 0; a < ARRAYSIZE(FORMAT_TYPE_NAMES); a++)
										{
											if (bitsPerChannel <= FORMAT_TYPE_NAMES[a].MaxMemorySize && FORMAT_TYPE_NAMES[a].HDR == true)
											{
												formatID = a;
												bitsPerChannel = FORMAT_TYPE_NAMES[a].MinMemorySize;
												goto encode;
											}
										}
									}
								}
								if (formatRoundDown == true)
								{
									for (size_t a = 0; a < ARRAYSIZE(FORMAT_TYPE_NAMES); a++)
									{
										if (bitsPerChannel >= FORMAT_TYPE_NAMES[a].MinMemorySize && hdr == FORMAT_TYPE_NAMES[a].HDR)
										{
											formatID = a;
											bitsPerChannel = FORMAT_TYPE_NAMES[a].MaxMemorySize;
											goto encode;
										}
									}
									if (hdr == false)
									{
										for (size_t a = 0; a < ARRAYSIZE(FORMAT_TYPE_NAMES); a++)
										{
											if (bitsPerChannel >= FORMAT_TYPE_NAMES[a].MinMemorySize && FORMAT_TYPE_NAMES[a].HDR == true)
											{
												formatID = a;
												bitsPerChannel = FORMAT_TYPE_NAMES[a].MaxMemorySize;
												goto encode;
											}
										}
									}
								}
							}
						encode:
							if (formatID >= ARRAYSIZE(FORMAT_TYPE_NAMES)) { return false; }
							const FormatTypeName& format = FORMAT_TYPE_NAMES[formatID];
							std::string alpha = "";
							if (format.HDR == true)
							{
								switch (bitsPerChannel)
								{
									case 8: CLOAK_ASSUME(false); break;
									case 16: alpha = std::to_string(ToHalf(1.0f)); break;
									case 32: alpha = "1.0f"; break;
									default: alpha = "1.0"; break;
								}
							}
							else
							{
								alpha = std::to_string((1Ui64 << bitsPerChannel) - 1);
							}

							std::stringstream list;
							list << "constexpr Texture " << ns << "L[] = {\n";
							for (size_t a = 0; a < mlc; a++)
							{
								const std::string mns = ns + "M" + std::to_string(a);
								const MipMapLevel& tex = texture.levels[a];
								std::stringstream data;
								data << "const ";
								if (format.HDR == true)
								{
									switch (bitsPerChannel)
									{
										case 8: CLOAK_ASSUME(false); break;
										case 16: data << "std::uint16_t"; break;
										case 32: data << "float"; break;
										default: CLOAK_ASSUME(false); break;
									}
								}
								else
								{
									data << "std::uint" << bitsPerChannel << "_t";
								}
								data << " " << mns << "[] = { ";
								std::string pre = "";
								for (UINT y = 0; y < tex.Size.H; y++)
								{
									for (UINT x = 0; x < tex.Size.W; x++)
									{
										const CE::Helper::Color::RGBA& col = tex.Get(x, y);
										for (size_t c = 0; c < 4; c++)
										{
											if (IsFlagSet(channel, static_cast<API::Image::v1008::ColorChannel>(c)))
											{
												data << pre;
												if (format.HDR == true)
												{
													switch (bitsPerChannel)
													{
														case 8: CLOAK_ASSUME(false); break;
														case 16: data << ToHalf(col[c]); break;
														case 32: data << col[c] << "f"; break;
														default: CLOAK_ASSUME(false); break;
													}
												}
												else
												{
													const uint64_t mv = (1Ui64 << bitsPerChannel) - 1;
													uint64_t v = static_cast<uint64_t>(col[c] * mv);
													v = min(v, mv);
													data << v;
												}
												pre = ", ";
											}
											else if (c == 3 && bitsPerChannel == 4)
											{
												data << pre << alpha;
											}
										}
									}
								}
								data << " };";
								Engine::Lib::WriteWithTabs(file, tabCount, data.str());
								const uint64_t row = static_cast<uint64_t>(channelCount) * tex.Size.W * bitsPerChannel / 8;
								const uint64_t slice = row * tex.Size.H;
								if (a > 0) { list << ",\n"; }
								list << std::string(tabCount + 1, '\t') << "{ " << mns << ", " << row << ", " << slice << " }";
							}
							list << "\n" << std::string(tabCount, '\t') << "};";
							Engine::Lib::WriteWithTabs(file, tabCount, list.str());
							std::stringstream r;
							r << "constexpr Image " << ns << "{{ " << bitsPerChannel << ", " << channelCount << ", " << format.Name;
							r << "}, " << texture.levels[0].Size.W << ", " << texture.levels[0].Size.H << ", " << mlc << ", " << ns << "L };";
							Engine::Lib::WriteWithTabs(file, tabCount, r.str());
						}
						else
						{
							std::stringstream r;
							r << "constexpr Image " << ns << "{{ 0, 0, " << FORMAT_TYPE_NAMES[0].Name << "}, 0, 0, 0, nullptr };";
							Engine::Lib::WriteWithTabs(file, tabCount, r.str());
						}
						return true;
					}
					bool CLOAK_CALL FitsOneFile(In size_t textureCount, In_reads(textureCount) const MipTexture* textures, In API::Image::v1008::ColorChannel channel, In const BitInfo& bits, In bool writeMipMaps)
					{
						if (textureCount <= 1) { return true; }
						uint32_t bitsPerChannel = 0;
						uint32_t channelCount = 0;
						for (size_t c = 0; c < 4; c++)
						{
							if (IsFlagSet(channel, static_cast<API::Image::v1008::ColorChannel>(c)))
							{
								channelCount++;
								bitsPerChannel = max(bitsPerChannel, bits.Channel[c].Bits);
							}
						}
						if (channelCount == 3) { channelCount = 4; }
						bitsPerChannel = CE::Global::Math::CeilPower2(min(32, max(bitsPerChannel, 8)));
						uint64_t pixelCount = 0;
						for (size_t a = 0; a < textureCount; a++)
						{
							if (textures[a].levelCount > 0) { pixelCount += static_cast<uint64_t>(textures[a].levels[0].Size.W) * textures[a].levels[0].Size.H; }
						}
						if (writeMipMaps == true) { pixelCount = static_cast<uint64_t>(1.33 * static_cast<long double>(pixelCount)); }
						uint64_t charPerChannel = static_cast<uint64_t>(0.3 * static_cast<long double>(bitsPerChannel)); //estimate digit count in decimal
						return (pixelCount * channelCount * (charPerChannel + 2)) < 4 MB;
					}
				}
			}
		}
	}
}