#include "stdafx.h"
#include "CloakCompiler/Font.h"

#include "Engine/TextureLoader.h"
#include "Engine/TempHandler.h"

#define BORDER_LEFT 0
#define BORDER_RIGHT 2
#define BORDER_TOP 1
#define BORDER_BOTTOM 3
#define IMG_SPACE_BEGIN 1
#define IMG_SPACE_END 2

#define PAL(L,T,TL,R) pal(L.R,T.R,TL.R,L.A,T.A,TL.A)

namespace CloakCompiler {
	namespace API {
		namespace Font {
			constexpr CloakEngine::Files::FileType g_TempFileType{ "FontTemp","CEFT",1002 };
			constexpr size_t g_maxLineWidth = 1000000;
			struct LetterInfo {
				char32_t Letter;
				Engine::TextureLoader::Image Image;
				unsigned int Border[4];
				LetterColorType Colored;
				unsigned int Width;
			};

			bool CLOAK_CALL CalculateBorder(In LetterInfo* letter)
			{
				letter->Border[BORDER_LEFT] = letter->Image.width;
				letter->Border[BORDER_RIGHT] = 0;
				letter->Border[BORDER_TOP] = letter->Image.height;
				letter->Border[BORDER_BOTTOM] = 0;
				for (UINT y = 0; y < letter->Image.height; y++)
				{
					for (UINT x = 0; x < letter->Image.width; x++)
					{
						const size_t p = static_cast<size_t>(x) + (static_cast<size_t>(y)*letter->Image.width);
						if (letter->Image.data[p].A != 0)
						{
							letter->Border[BORDER_LEFT] = min(letter->Border[BORDER_LEFT], x);
							letter->Border[BORDER_RIGHT] = max(letter->Border[BORDER_RIGHT], x);
							letter->Border[BORDER_TOP] = min(letter->Border[BORDER_TOP], y);
							letter->Border[BORDER_BOTTOM] = max(letter->Border[BORDER_BOTTOM], y);
						}
					}
				}
				return letter->Border[BORDER_LEFT] <= letter->Border[BORDER_RIGHT] && letter->Border[BORDER_TOP] <= letter->Border[BORDER_BOTTOM];
			}

			inline uint32_t CLOAK_CALL alignUp(In uint32_t v, In uint32_t align)
			{
				return (v + align - 1) & ~(align - 1);
			}
			UINT CLOAK_CALL CalculateMipMaps(In UINT W, In UINT H)
			{
				UINT v = min(W, H);
				UINT r = 1;
				while (alignUp(W, 1U << r) <= g_maxLineWidth && (v >> r) > 0) { r++; }
				return r;
			}
			void CLOAK_CALL SortLetters(In LetterInfo* info, In size_t infoCount, Out size_t* index)
			{
				UINT* const sortHeap = new UINT[2 * infoCount];
				UINT* sorted = sortHeap;
				UINT* countSort = &sortHeap[infoCount];
				size_t* const sortHeapPos = new size_t[2 * infoCount];
				size_t* sortedP = sortHeapPos;
				size_t* countSortP = &sortHeapPos[infoCount];
				UINT counting[16];
				size_t size = 0;
				for (size_t a = 0; a < infoCount; a++)
				{
					sortedP[size] = a;
					sorted[size] = info[a].Width;
					size++;
				}
				const UINT d = static_cast<UINT>(ceil(log2(max(1, size)) / 4));
				for (UINT a = 0xf << (4 * (d - 1)), b = d - 1; a > 0; a >>= 4, b--)
				{
					for (size_t c = 0; c < 16; c++) { counting[c] = 0; }
					for (size_t c = 0; c < size; c++) { counting[(sorted[c] & a) >> (4 * b)]++; }
					for (size_t c = 1; c < 16; c++) { counting[c] += counting[c - 1]; }
					for (size_t c = size; c > 0; c--)
					{
						const size_t p = (sorted[c - 1] & a) >> (4 * b);
						countSort[counting[p] - 1] = sorted[c - 1];
						countSortP[counting[p] - 1] = sortedP[c - 1];
						counting[p]--;
					}
					UINT* tmp = sorted;
					sorted = countSort;
					countSort = tmp;
					size_t* tmpP = sortedP;
					sortedP = countSortP;
					countSortP = tmpP;
				}
				for (size_t a = 0; a < infoCount; a++) { index[a] = sortedP[a]; }
				delete[] sortHeap;
				delete[] sortHeapPos;
			}
			size_t CLOAK_CALL CalculateLines(In UINT mipMaps, In LetterInfo* info, In size_t infoCount, Out size_t* lines, Out uint64_t* maxLineWidth)
			{
				const size_t mask = (1 << mipMaps) - 1;
				const size_t maxLineW = g_maxLineWidth - (g_maxLineWidth & mask);
				size_t* ind = new size_t[infoCount];
				uint64_t WSum = IMG_SPACE_BEGIN;
				for (size_t a = 0; a < infoCount; a++) 
				{ 
					ind[a] = a; 
					WSum += IMG_SPACE_BEGIN + info[a].Width + IMG_SPACE_END;
				}
				SortLetters(info, infoCount, ind);
				size_t lineC = static_cast<size_t>((WSum + maxLineW - 1) / maxLineW);
				uint64_t maxLineLen = 0;
				do {
					uint64_t* lineW = new uint64_t[lineC];
					for (size_t a = 0; a < lineC; a++) { lineW[a] = IMG_SPACE_BEGIN; }
					for (size_t a = infoCount; a > 0; a--)
					{
						const size_t p = ind[a - 1];
						uint64_t mw = WSum;
						size_t lp = 0;
						for (size_t b = 0; b < lineC; b++)
						{
							if (lineW[b] < mw)
							{
								lp = b;
								mw = lineW[b];
							}
						}
						lines[p] = lp;
						lineW[lp] += IMG_SPACE_BEGIN + info[p].Width + IMG_SPACE_END;
						maxLineLen = max(maxLineLen, lineW[lp]);
					}
					delete[] lineW;
					lineC++;
				} while (maxLineLen > maxLineW);
				*maxLineWidth = maxLineLen - (maxLineLen & mask) + ((maxLineLen & mask) > 0 ? (mask + 1) : 0);
				delete[] ind;
				return lineC - 1;
			}

			void CLOAK_CALL EncodeColorPlane(In CloakEngine::Files::IWriter* write, In uint32_t* colD, In const LetterInfo& info, In UINT maxH, In UINT topBorder, In size_t bitW, In size_t bitLen)
			{
				const unsigned H = 1 + info.Border[BORDER_BOTTOM] - topBorder;
				uint32_t* line = new uint32_t[info.Width];
				const size_t bitLL = static_cast<size_t>(ceil(log2(bitLen + 1)));
				for (unsigned int y = topBorder; y <= info.Border[BORDER_BOTTOM]; y++)
				{
					for (unsigned int x = 0; x < info.Width; x++) 
					{
						const size_t p = info.Border[BORDER_LEFT] + x + (info.Image.width*y);
						line[x] = static_cast<uint32_t>(ceil(log2(colD[p])));
					}
					size_t dataC = 0;
					uint32_t maxBits = 0;
					for (unsigned int x = 0; x < info.Width; x++)
					{
						if (line[x] > maxBits)
						{
							if (line[x] * (dataC + 1) < (maxBits*dataC) + (bitW + bitLL + line[x]))
							{
								maxBits = line[x];
							}
							else
							{
								write->WriteBits(bitW, dataC);
								write->WriteBits(bitLL, maxBits);
								if (maxBits > 0)
								{
									for (size_t xi = x - dataC; xi < x; xi++)
									{
										const size_t p = info.Border[BORDER_LEFT] + xi + (info.Image.width*y);
										write->WriteBits(maxBits, colD[p]);
									}
								}
								dataC = 0;
								maxBits = line[x];
							}
						}
						else if (line[x] < maxBits)
						{
							for (size_t dc = 1; dc < dataC && line[x] < maxBits; dc++)
							{
								const size_t dcn = 1 + dataC - dc;
								uint32_t nmbits = line[x];
								for (size_t xi = x - dcn; xi < x; xi++)
								{
									nmbits = max(nmbits, line[xi]);
								}
								if ((dc*maxBits) + ((1 + dataC - dc)*nmbits) + bitW + bitLL < (dataC + 1)*maxBits)
								{
									write->WriteBits(bitW, dc);
									write->WriteBits(bitLL, maxBits);
									for (size_t xi = x - dataC; xi < x + dc - dataC; xi++)
									{
										const size_t p = info.Border[BORDER_LEFT] + xi + (info.Image.width*y);
										write->WriteBits(maxBits, colD[p]);
									}
									dataC -= dc;
									maxBits = nmbits;
									dc = 1;
								}
							}
						}
						dataC++;
					}
					if (dataC > 0)
					{
						write->WriteBits(bitW, dataC);
						write->WriteBits(bitLL, maxBits);
						if (maxBits > 0)
						{
							for (size_t xi = info.Width - dataC; xi < info.Width; xi++)
							{
								const size_t p = info.Border[BORDER_LEFT] + xi + (info.Image.width*y);
								write->WriteBits(maxBits, colD[p]);
							}
						}
					}
				}
				delete[] line;
				for (unsigned int y = 0; y < maxH - H; y++)
				{
					write->WriteBits(bitW, info.Width);
					write->WriteBits(bitLL, 0);
				}
			}
			void CLOAK_CALL EncodeLetter(In CloakEngine::Files::IWriter* write, In const LetterInfo& info, In size_t bitW, In UINT maxH, In UINT topBorder, In size_t bitLen)
			{
				write->WriteBits(32, info.Letter);
				write->WriteBits(bitW, info.Width);
				LetterColorType colored = info.Colored;
				size_t maxC = 1;
				uint32_t maxD = (1 << bitLen) - 1;
				switch (info.Colored)
				{
					case LetterColorType::SINGLECOLOR:
						write->WriteBits(2, 0);
						maxC = 1;
						break;
					case LetterColorType::MULTICOLOR:
					{
						size_t dc = 0;
						for (size_t c = 0; c < 4 && dc < 2; c++)
						{
							for (size_t b = 1; b < info.Image.dataSize; b++)
							{
								if (static_cast<uint32_t>(maxD*info.Image.data[b - 1][c]) != static_cast<uint32_t>(maxD*info.Image.data[b][c])) 
								{ 
									dc++;
									break;
								}
							}
						}
						if (dc < 2)
						{
							write->WriteBits(2, 0);
							maxC = 1;
							colored = LetterColorType::SINGLECOLOR;
						}
						else
						{
							write->WriteBits(2, 1);
							maxC = 4;
						}
						break;
					}
					case LetterColorType::TRUECOLOR:
						write->WriteBits(2, 2);
						maxC = 4;
						break;
					default:
						colored = LetterColorType::SINGLECOLOR;
						write->WriteBits(2, 0);
						maxC = 1;
						break;
				}
				uint32_t* col = new uint32_t[info.Image.dataSize];
				for (size_t c = 0; c < maxC; c++)
				{
					for (size_t b = 0; b < info.Image.dataSize; b++) 
					{
						if (colored == LetterColorType::SINGLECOLOR) { col[b] = static_cast<uint32_t>(maxD*info.Image.data[b].A); }
						else if (colored == LetterColorType::MULTICOLOR) { col[b] = static_cast<uint32_t>(maxD*info.Image.data[b][c]); }
						else if (colored == LetterColorType::TRUECOLOR) { col[b] = static_cast<uint32_t>(maxD*info.Image.data[b][c]); }
					}
					if (colored == LetterColorType::MULTICOLOR && c < 3)
					{
						for (size_t b = 1; b < info.Image.dataSize; b++)
						{
							if (col[b - 1] != col[b]) 
							{ 
								goto encode_plane_yes;
							}
						}
						goto encode_plane_no;
					}

				encode_plane_yes:
					if (colored == LetterColorType::MULTICOLOR && c < 3) { write->WriteBits(1, 1); }
					EncodeColorPlane(write, col, info, maxH, topBorder, bitW, bitLen);

					goto encode_plane_finish;
				encode_plane_no:
					write->WriteBits(1, 0);

					goto encode_plane_finish;
				encode_plane_finish:;

				}
				delete[] col;
			}

			CLOAKCOMPILER_API void CLOAK_CALL EncodeToFile(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In std::function<void(const RespondeInfo&)> response)
			{
				const bool writeTmp = (encode.flags & EncodeFlags::NO_TEMP_WRITE) == EncodeFlags::NONE;
				const bool readTmp = (encode.flags & EncodeFlags::NO_TEMP_READ) == EncodeFlags::NONE;

				LetterInfo* letters = new LetterInfo[desc.Alphabet.size()];
				std::vector<std::u16string> toCheckPaths;
				size_t cl = 0;
				UINT minBordTOP = -1;
				UINT minBordBOTTOM = 0;
				UINT minW = -1;
				UINT maxW = 0;
				for (const auto& letter : desc.Alphabet)
				{
					LetterInfo* l = &letters[cl];
					l->Letter = letter.first;
					l->Colored = letter.second.Colored;
					l->Border[0] = l->Border[1] = l->Border[2] = l->Border[3] = 0;
					HRESULT hRet = Engine::TextureLoader::LoadTextureFromFile(letter.second.FilePath, &l->Image);
					if (SUCCEEDED(hRet) && CalculateBorder(l))
					{
						cl++;
						l->Width = 1 + l->Border[BORDER_RIGHT] - l->Border[BORDER_LEFT];
						minW = min(minW, l->Width);
						maxW = max(maxW, l->Width);
						minBordBOTTOM = max(minBordBOTTOM, l->Border[BORDER_BOTTOM]);
						minBordTOP = min(minBordTOP, l->Border[BORDER_TOP]);
						toCheckPaths.push_back(letter.second.FilePath);
					}
				}
				if (cl > 0)
				{
					CloakEngine::Files::IVirtualWriteBuffer* buffer = CloakEngine::Files::CreateVirtualWriteBuffer();
					CloakEngine::Files::IWriter* write = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&write));
					write->SetTarget(buffer, CloakEngine::Files::CompressType::NONE);
					bool useTmp = false;

					std::vector<Engine::TempHandler::FileDateInfo> changeDates;
					Engine::TempHandler::ReadDates(toCheckPaths, &changeDates);
					if (readTmp)
					{
						CloakEngine::Files::IReader* readTemp = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&readTemp));
						if (readTemp != nullptr)
						{
							if (readTemp->SetTarget(encode.tempPath, g_TempFileType, false, true) == g_TempFileType.Version)
							{
								if (Engine::TempHandler::CheckDateChanges(changeDates, readTemp, encode.targetGameID) == false)
								{
									if (readTemp->ReadBits(8) == desc.BitLen && readTemp->ReadBits(desc.BitLen) == desc.SpaceWidth)
									{
										useTmp = true;
										const uint32_t byl = static_cast<uint32_t>(readTemp->ReadBits(32));
										const unsigned int bil = static_cast<unsigned int>(readTemp->ReadBits(3));
										for (uint32_t a = 0; a < byl; a++) { write->WriteBits(8, readTemp->ReadBits(8)); }
										if (bil > 0) { write->WriteBits(bil, readTemp->ReadBits(bil)); }
									}
								}
							}
						}
						SAVE_RELEASE(readTemp);
					}

					if (useTmp == false)
					{
						const UINT maxH = 1 + minBordBOTTOM - minBordTOP;
						UINT mipMaps = CalculateMipMaps(minW, maxH);
						mipMaps = min(15, mipMaps);
						size_t* lines = new size_t[cl];
						uint64_t maxLineW = 0;
						size_t lineC = CalculateLines(mipMaps, letters, cl, lines, &maxLineW);
						std::vector<size_t>* toWriteLines = new std::vector<size_t>[lineC];
						for (size_t a = 0; a < cl; a++) { toWriteLines[lines[a]].push_back(a); }

						write->WriteBits(8, desc.BitLen - 1);
						write->WriteBits(desc.BitLen, desc.SpaceWidth);
						write->WriteBits(4, mipMaps);
						write->WriteBits(20, maxLineW);
						const size_t bitH = static_cast<size_t>(ceil(log2(maxH + 1)));
						const size_t bitW = static_cast<size_t>(ceil(log2(maxW + 1)));
						write->WriteBits(4, bitW);
						write->WriteBits(4, bitH);
						write->WriteBits(bitH, maxH);
						size_t lnc = lineC;
						while (lnc >= 16) { write->WriteBits(1, 1); lnc -= 16; }
						write->WriteBits(1, 0);
						write->WriteBits(4, lnc);
						const size_t bitL = static_cast<size_t>(ceil(log2(lineC)));
						RespondeInfo ri;
						for (size_t a = 0; a < lineC; a++)
						{
							const std::vector<size_t>& l = toWriteLines[a];
							for (size_t b = 0; b < l.size(); b++)
							{
								const LetterInfo& i = letters[l[b]];
								ri.Letter = i.Letter;
								ri.Type = RespondeType::STARTED;
								if (response) { response(ri); }
								write->WriteBits(1, 1);
								write->WriteBits(bitL, a);
								EncodeLetter(write, i, bitW, maxH, minBordTOP, desc.BitLen);
								ri.Type = RespondeType::COMPLETED;
								if (response) { response(ri); }
							}
						}
						write->WriteBits(1, 0);

						if (writeTmp)
						{
							CloakEngine::Files::IWriter* writeTemp = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&writeTemp));
							if (writeTemp != nullptr)
							{
								writeTemp->SetTarget(encode.tempPath, g_TempFileType, CloakEngine::Files::CompressType::NONE);
								Engine::TempHandler::WriteDates(changeDates, writeTemp, encode.targetGameID);
								writeTemp->WriteBits(8, desc.BitLen);
								writeTemp->WriteBits(desc.BitLen, desc.SpaceWidth);
								writeTemp->WriteBits(32, write->GetPosition());
								writeTemp->WriteBits(3, write->GetBitPosition());
								writeTemp->WriteBuffer(buffer);
								writeTemp->Save();
							}
							SAVE_RELEASE(writeTemp);
						}

						delete[] toWriteLines;
						delete[] lines;
					}
					output->WriteBuffer(buffer);

					SAVE_RELEASE(write);
					SAVE_RELEASE(buffer);
				}
				else if(response) 
				{
					RespondeInfo ri;
					ri.Type = RespondeType::FAIL_NO_IMG;
					ri.Letter = 0;
					response(ri);
				}
				delete[] letters;
			}

#ifdef OLD_FONT
			constexpr CloakEngine::Files::FileType g_TempFileType{ "FontTemp","CEFT",1001 };
			struct LetterInfo {
				char32_t Letter;
				Engine::TextureLoader::Image Image;
				unsigned int Border[4];
				LetterColorType Colored;
			};

			template<typename T> inline T CLOAK_CALL RoundFloat(float f)
			{
				if (f - floorf(f) < 0.5f) { return static_cast<T>(floorf(f)); }
				return static_cast<T>(ceilf(f));
			}
			auto const Round = RoundFloat<uint32_t>;
			inline float CLOAK_CALL pal(float l, float t, float tl, float lA, float tA, float tlA)
			{
				const float L = lA > 0 ? l : 0;
				const float T = tA > 0 ? t : 0;
				const float TL = tlA > 0 ? tl : 0;
				return min(1.0f, max(0, L + T - TL));
			}
			void CLOAK_CALL EncodeLetter(CloakEngine::Files::IWriter* write, const LetterInfo& letter, uint32_t maxVal, unsigned int bitLen, unsigned int lTOP, unsigned int hBOT, std::function<void(const RespondeInfo&)> response, size_t num, size_t count)
			{
				const uint32_t W = letter.Border[BORDER_RIGHT] - letter.Border[BORDER_LEFT];
				const unsigned int bwW = static_cast<unsigned int>(ceil(log2(W + 1)));
				if (letter.Colored == LetterColorType::MULTICOLOR) { write->WriteBits(2, 1); }
				else if (letter.Colored == LetterColorType::TRUECOLOR) { write->WriteBits(2, 2); }
				else { write->WriteBits(2, 0); }
				if (response)
				{
					RespondeInfo ri;
					ri.Letter = letter.Letter;
					ri.Type = RespondeType::STARTED;
					response(ri);
				}
				write->WriteBits(32, static_cast<uint32_t>(letter.Letter));
				write->WriteBits(32, W);
				for (unsigned int y = lTOP; y < hBOT; y++)
				{
					if (y >= letter.Border[BORDER_TOP] && y < letter.Border[BORDER_BOTTOM])
					{
						uint32_t last = 0;
						for (unsigned int x = letter.Border[BORDER_LEFT]; x < letter.Border[BORDER_RIGHT]; x++)
						{
							const size_t p = x + (y*letter.Image.width);
							const uint32_t v = Round(maxVal*letter.Image.data[p].A);
							if (v != last)
							{
								write->WriteBits(bwW, x - letter.Border[BORDER_LEFT]);
								write->WriteBits(bitLen, v);
								last = v;
							}
						}
					}
					write->WriteBits(bwW, W);
				}
				if (letter.Colored == LetterColorType::MULTICOLOR)
				{
					unsigned int stage[3] = { 0,0,0 };
					uint32_t last[3] = { 0,0,0 };
					for (unsigned int y = letter.Border[BORDER_TOP]; y < letter.Border[BORDER_BOTTOM] && (stage[0] < 2 || stage[1] < 2 || stage[2] < 2); y++)
					{
						for (unsigned int x = letter.Border[BORDER_LEFT]; x < letter.Border[BORDER_RIGHT] && (stage[0] < 2 || stage[1] < 2 || stage[2] < 2); x++)
						{
							const size_t p = x + (y*letter.Image.width);
							if (Round(maxVal*letter.Image.data[p].A) > 0)
							{
								const uint32_t vr = Round(maxVal*letter.Image.data[p].R);
								const uint32_t vg = Round(maxVal*letter.Image.data[p].G);
								const uint32_t vb = Round(maxVal*letter.Image.data[p].B);
								if (stage[0] == 0 || (stage[0] < 2 && last[0] != vr)) { last[0] = vr; stage[0]++; }
								if (stage[1] == 0 || (stage[1] < 2 && last[1] != vg)) { last[1] = vg; stage[1]++; }
								if (stage[2] == 0 || (stage[2] < 2 && last[2] != vb)) { last[2] = vb; stage[2]++; }
							}
						}
					}
					const unsigned int channel = (stage[0] == 2 ? 1 : 0) + (stage[1] == 2 ? 1 : 0) + (stage[2] == 2 ? 1 : 0);
					write->WriteBits(2, channel);
					if (stage[0] < 2) { write->WriteBits(bitLen, last[0]); }
					if (stage[1] < 2) { write->WriteBits(bitLen, last[1]); }
					if (stage[2] < 2) { write->WriteBits(bitLen, last[2]); }
					for (unsigned int a = 0; a < 3; a++)
					{
						if (stage[a] == 2)
						{
							for (unsigned int y = lTOP; y < hBOT; y++)
							{
								if (y >= letter.Border[BORDER_TOP] && y < letter.Border[BORDER_BOTTOM])
								{
									uint32_t last = 0;
									for (unsigned int x = letter.Border[BORDER_LEFT]; x < letter.Border[BORDER_RIGHT]; x++)
									{
										const size_t p = x + (y*letter.Image.width);
										if (Round(maxVal*letter.Image.data[p].A) > 0)
										{
											const CloakEngine::Helper::Color::RGBA& c = letter.Image.data[p];
											const uint32_t v = Round(maxVal*(a == 0 ? c.R : (a == 1 ? c.G : c.B)));
											if (v != last)
											{
												write->WriteBits(bwW, x - letter.Border[BORDER_LEFT]);
												write->WriteBits(bitLen, v);
												last = v;
											}
										}
									}
								}
								write->WriteBits(bwW, W);
							}
						}
					}
				}
				else if (letter.Colored == LetterColorType::TRUECOLOR)
				{
					const unsigned int bll = static_cast<unsigned int>(ceil(log2(bitLen + 1)));
					for (unsigned int y = lTOP; y < hBOT; y++)
					{
						if (y >= letter.Border[BORDER_TOP] && y < letter.Border[BORDER_BOTTOM])
						{
							write->WriteBits(1, 1);
							for (unsigned int x = letter.Border[BORDER_LEFT]; x < letter.Border[BORDER_RIGHT]; x++)
							{
								const size_t p = x + (y*letter.Image.width);
								if (Round(maxVal*letter.Image.data[p].A) > 0)
								{
									CloakEngine::Helper::Color::RGBA Left, Top, TopLeft;
									if (x > 0) { Left = letter.Image.data[p - 1]; }
									if (y > 0) { Top = letter.Image.data[p - letter.Image.width]; }
									if (x > 0 && y > 0) { TopLeft = letter.Image.data[p - (1 + letter.Image.width)]; }
									const int32_t R = RoundFloat<int32_t>(maxVal*(letter.Image.data[p].R - PAL(Left, Top, TopLeft, R)));
									const int32_t G = RoundFloat<int32_t>(maxVal*(letter.Image.data[p].G - PAL(Left, Top, TopLeft, G)));
									const int32_t B = RoundFloat<int32_t>(maxVal*(letter.Image.data[p].B - PAL(Left, Top, TopLeft, B)));
									const unsigned int Rl = 1 + static_cast<unsigned int>(ceil(log2(abs(R) + 1)));
									const unsigned int Gl = 1 + static_cast<unsigned int>(ceil(log2(abs(G) + 1)));
									const unsigned int Bl = 1 + static_cast<unsigned int>(ceil(log2(abs(B) + 1)));
									const unsigned int mcl = max(max(Rl, Gl), Bl);
									write->WriteBits(bll, min(mcl, bitLen));
									if (mcl < bitLen)
									{
										if (R < 0) { write->WriteBits(1, 1); }
										else { write->WriteBits(1, 0); }
										write->WriteBits(mcl - 1, abs(R));
										if (G < 0) { write->WriteBits(1, 1); }
										else { write->WriteBits(1, 0); }
										write->WriteBits(mcl - 1, abs(G));
										if (B < 0) { write->WriteBits(1, 1); }
										else { write->WriteBits(1, 0); }
										write->WriteBits(mcl - 1, abs(B));
									}
									else
									{
										write->WriteBits(bitLen, Round(maxVal*letter.Image.data[p].R));
										write->WriteBits(bitLen, Round(maxVal*letter.Image.data[p].G));
										write->WriteBits(bitLen, Round(maxVal*letter.Image.data[p].B));
									}
								}
							}
						}
						else
						{
							write->WriteBits(1, 0);
						}
					}
				}
				if (response)
				{
					RespondeInfo ri;
					ri.Letter = letter.Letter;
					ri.Type = RespondeType::COMPLETED;
					response(ri);
				}
			}
			bool CLOAK_CALL CalculateBorer(LetterInfo* li)
			{
				if (li != nullptr)
				{
					li->Border[BORDER_LEFT] = li->Image.width;
					li->Border[BORDER_TOP] = li->Image.height;
					li->Border[BORDER_RIGHT] = 0;
					li->Border[BORDER_BOTTOM] = 0;
					bool inCY = false;
					for (UINT y = 0; y < li->Image.height; y++)
					{
						bool inCX = false;
						bool evInCX = false;
						for (UINT x = 0; x < li->Image.width; x++)
						{
							const size_t p = static_cast<size_t>(x) + (static_cast<size_t>(y)*li->Image.width);
							if (li->Image.data[p].A == 0)
							{
								if (inCX == true && li->Border[BORDER_RIGHT] < x) { li->Border[BORDER_RIGHT] = x; }
								inCX = false;
							}
							else
							{
								evInCX = true;
								if (inCX == false && li->Border[BORDER_LEFT] > x) { li->Border[BORDER_LEFT] = x; }
								inCX = true;
							}
						}
						if (inCX == true) { li->Border[BORDER_RIGHT] = li->Image.width; }
						if (evInCX == false)
						{
							if (inCY == true && li->Border[BORDER_BOTTOM] < y) { li->Border[BORDER_BOTTOM] = y; }
							inCY = false;
						}
						else
						{
							if (inCY == false && li->Border[BORDER_TOP] > y) { li->Border[BORDER_TOP] = y; }
							inCY = true;
						}
					}
					if (inCY == true) { li->Border[BORDER_BOTTOM] = li->Image.height; }
					return li->Border[BORDER_RIGHT] > li->Border[BORDER_LEFT] && li->Border[BORDER_BOTTOM] > li->Border[BORDER_TOP];
				}
				return false;
			}

			CLOAKCOMPILER_API void CLOAK_CALL EncodeToFile(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In std::function<void(const RespondeInfo&)> response)
			{
				if (output != nullptr && desc.Alphabet.size() > 0 && desc.BitLen > 0 && desc.BitLen < 32)
				{
					LetterInfo* letters = new LetterInfo[desc.Alphabet.size()];
					size_t letterSize = 0;
					std::vector<std::u16string> toCheckFiles;
					for (const auto& it : desc.Alphabet)
					{
						LetterInfo* i = &letters[letterSize];
						i->Letter = it.first;
						i->Colored = it.second.Colored;
						for (size_t a = 0; a < 4; a++) { i->Border[a] = 0; }
						HRESULT hRet = Engine::TextureLoader::LoadTextureFromFile(it.second.FilePath, &i->Image);
						if (SUCCEEDED(hRet) && CalculateBorer(i))
						{
							letterSize++;
							toCheckFiles.push_back(it.second.FilePath);
						}
					}
					std::vector<Engine::TempHandler::FileDateInfo> changeDates;
					Engine::TempHandler::ReadDates(toCheckFiles, &changeDates);
					bool doEncode = true;
					if ((encode.flags & EncodeFlags::NO_TEMP_READ) == EncodeFlags::NONE)
					{
						CloakEngine::Files::IReader* readTemp = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&readTemp));
						if (readTemp != nullptr)
						{
							if (readTemp->SetTarget(encode.tempPath, g_TempFileType, false) == g_TempFileType.Version)
							{
								if (Engine::TempHandler::CheckDateChanges(changeDates, readTemp, encode.targetCGID) == false)
								{
									if (readTemp->ReadBits(8) == desc.BitLen && readTemp->ReadBits(desc.BitLen) == desc.SpaceWidth)
									{
										doEncode = false;
										const uint32_t byl = static_cast<uint32_t>(readTemp->ReadBits(32));
										const unsigned int bil = static_cast<unsigned int>(readTemp->ReadBits(3));
										for (uint32_t a = 0; a < byl; a++) { output->WriteBits(8, readTemp->ReadBits(8)); }
										if (bil > 0) { output->WriteBits(bil, readTemp->ReadBits(bil)); }
									}
								}
							}
						}
						SAVE_RELEASE(readTemp);
					}
					if (doEncode)
					{
						CloakEngine::Files::IWriter* write = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&write));

						CloakEngine::Files::IVirtualWriteBuffer* buffer = CloakEngine::Files::CreateVirtualWriteBuffer();
						write->SetTarget(buffer, CloakEngine::Files::CompressType::NONE);

						write->WriteBits(32, letterSize - 1);
						write->WriteBits(5, desc.BitLen - 1);
						write->WriteBits(desc.BitLen, desc.SpaceWidth);
						unsigned int lowTOP = 0;
						unsigned int highBOTTOM = 0;
						for (size_t a = 0; a < letterSize; a++)
						{
							if (a == 0 || letters[a].Border[BORDER_TOP] < lowTOP) { lowTOP = letters[a].Border[BORDER_TOP]; }
							if (a == 0 || letters[a].Border[BORDER_BOTTOM] > highBOTTOM) { highBOTTOM = letters[a].Border[BORDER_BOTTOM]; }
						}
						const uint32_t maxVal = (1 << desc.BitLen) - 1;
						write->WriteBits(32, highBOTTOM - lowTOP);
						for (size_t a = 0; a < letterSize; a++)
						{
							EncodeLetter(write, letters[a], maxVal, desc.BitLen, lowTOP, highBOTTOM, response, a + 1, letterSize);
						}

						output->WriteBuffer(buffer);

						if ((encode.flags & EncodeFlags::NO_TEMP_WRITE) == EncodeFlags::NONE)
						{
							CloakEngine::Files::IWriter* writeTemp = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&writeTemp));
							if (writeTemp != nullptr)
							{
								writeTemp->SetTarget(encode.tempPath, g_TempFileType, CloakEngine::Files::CompressType::NONE);
								Engine::TempHandler::WriteDates(changeDates, writeTemp, encode.targetCGID); 
								writeTemp->WriteBits(8, desc.BitLen);
								writeTemp->WriteBits(desc.BitLen, desc.SpaceWidth);
								writeTemp->WriteBits(32, write->GetPosition());
								writeTemp->WriteBits(3, write->GetBitPosition());
								writeTemp->WriteBuffer(buffer);
								writeTemp->Save();
							}
							SAVE_RELEASE(writeTemp);
						}

						write->Release();
						buffer->Release();
					}
					delete[] letters;
				}
			}
#endif
		}
	}
}