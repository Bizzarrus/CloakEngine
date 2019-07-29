#include "stdafx.h"
#include "Engine/Codecs/HDRI.h"

#include <sstream>

#define CheckString(read, strings) CheckStringImpl<ARRAYSIZE(strings)>(read, strings)
#define CheckStringTrim(read, strings) CheckStringImpl<ARRAYSIZE(strings)>(read, strings, true)
#define CheckStringChar(read, strings, c) CheckStringImpl<ARRAYSIZE(strings)>(read, strings, false, c)
#define CheckStringCharTrim(read, strings, c) CheckStringImpl<ARRAYSIZE(strings)>(read, strings, true, c)

namespace CloakCompiler {
	namespace Engine {
		namespace Codecs {
			constexpr char* SIGNATURE[] = { "#?RADIANCE", "#?RGBE" };
			constexpr char* META[] = { "FORMAT=","EXPOSURE=","GAMMA=" };
			constexpr char* CODEC[] = { "32-bit_rle_rgbe","32-bit_rle_xyze" };

			constexpr size_t NOT_FOUND = static_cast<size_t>(-1);
			constexpr float DEFAULT_GAMMA = 1 / 2.2f;

			template<size_t N> inline size_t CLOAK_CALL CheckStringImpl(In CloakEngine::Files::IReader* read, In char* const strings[N], In_opt bool trimString = false, In_opt char firstChar = '\0')
			{
				size_t len[N];
				bool s[N];
				size_t ml = 0;
				for (size_t a = 0; a < N; a++) 
				{ 
					len[a] = strlen(strings[a]); 
					ml = max(ml, len[a]); 
					s[a] = true; 
				}
				size_t l = 0;
				while (l < ml && read->IsAtEnd() == false)
				{
					//const char c = static_cast<char>(read->ReadBits(8));
					char c = firstChar;
					if (c == '\0') { c = static_cast<char>(read->ReadBits(8)); }
					firstChar = '\0';
					if (trimString == false || (c != ' ' && c != '\t') || l > 0)
					{
						bool os = false;
						for (size_t a = 0; a < N; a++)
						{
							if (l < len[a])
							{
								if (c != strings[a][l]) { s[a] = false; }
								else if (s[a] == true) { os = true; }
							}
							if (l + 1 == len[a] && s[a] == true) { return a; }
						}
						l++;
						if (os == false) { return NOT_FOUND; }
					}
				}
				return NOT_FOUND;
			}
			inline void CLOAK_CALL MoveToEOL(In CloakEngine::Files::IReader* read)
			{
				while (read->IsAtEnd() == false) 
				{
					char c = static_cast<char>(read->ReadBits(8));
					if (c == '\n') { break; }
				}
			}
			inline int CLOAK_CALL ReadInt(In CloakEngine::Files::IReader* read, In_opt bool toEOL = false)
			{
				char c;
				do {
					c = static_cast<char>(read->ReadBits(8));
				} while (read->IsAtEnd() == false && (c == ' ' || c == '\t'));
				std::stringstream exp;
				do {
					exp << c;
					c = static_cast<char>(read->ReadBits(8));
				} while (read->IsAtEnd() == false && c >= '0' && c <= '9');
				while (toEOL && c != '\n') { c = static_cast<char>(read->ReadBits(8)); }
				try {
					std::string e = exp.str();
					int nexp = static_cast<int>(std::atoi(e.c_str()));
					return nexp;
				}
				catch (...) { }
				return 0;
			}
			inline float CLOAK_CALL ReadFloat(In CloakEngine::Files::IReader* read, In_opt bool toEOL = false)
			{
				char c;
				do {
					c = static_cast<char>(read->ReadBits(8));
				} while (read->IsAtEnd() == false && (c == ' ' || c == '\t'));
				std::stringstream exp;
				do {
					exp << c;
					c = static_cast<char>(read->ReadBits(8));
				} while (read->IsAtEnd() == false && ((c>='0' && c<='9') || c == '.'));
				while (toEOL && c != '\n') { c = static_cast<char>(read->ReadBits(8)); }
				try {
					std::string e = exp.str();
					float nexp = static_cast<float>(std::atof(e.c_str()));
					return nexp;
				}
				catch (...) {}
				return 0;
			}
			inline CloakEngine::Helper::Color::RGBA CLOAK_CALL ToColor(In uint8_t data[4], In float exposure, In float gamma, In size_t format)
			{
				CloakEngine::Helper::Color::RGBA linColor;
				linColor.R = std::pow((1.0f / exposure)*std::ldexp(static_cast<float>(data[0]) + 0.5f, data[3] - 136), gamma);
				linColor.G = std::pow((1.0f / exposure)*std::ldexp(static_cast<float>(data[1]) + 0.5f, data[3] - 136), gamma);
				linColor.B = std::pow((1.0f / exposure)*std::ldexp(static_cast<float>(data[2]) + 0.5f, data[3] - 136), gamma);
				linColor.A = 1.0f;

				CloakEngine::Helper::Color::RGBA res;
				if (format == 0) //rgbe encoding
				{
					res.R = std::pow(linColor.R, DEFAULT_GAMMA);
					res.G = std::pow(linColor.G, DEFAULT_GAMMA);
					res.B = std::pow(linColor.B, DEFAULT_GAMMA);
					res.A = 1;
				}
				else if (format == 1) //xyze encoding
				{
					res.R = std::pow((linColor.R *  2.3706743f) + (linColor.G * -0.9000405f) + (linColor.B * -0.4706338f), DEFAULT_GAMMA);
					res.G = std::pow((linColor.R * -0.5138850f) + (linColor.G *  1.4253036f) + (linColor.B *  0.0885814f), DEFAULT_GAMMA);
					res.B = std::pow((linColor.R *  0.0052982f) + (linColor.G * -0.0146949f) + (linColor.B * -1.0093968f), DEFAULT_GAMMA);
					res.A = 1;
				}
				return res;
			}
			inline size_t CLOAK_CALL GetDataPos(In UINT x, In UINT y, In UINT W, In UINT H, In bool nd[3])
			{
				if (nd[0] == true) { x = W - (x + 1); }
				if (nd[1] == false) { y = H - (y + 1); }
				if (nd[2] == true) { UINT t = x; x = y; y = t; }
				return (static_cast<size_t>(y)*W) + x;
			}

			CLOAK_CALL HDRI::HDRI()
			{

			}
			CLOAK_CALL HDRI::~HDRI()
			{

			}
			HRESULT CLOAK_CALL_THIS HDRI::Decode(In const CloakEngine::Files::UFI& path, Out TextureLoader::Image* img)
			{
				m_exposure = 1;
				m_format = NOT_FOUND;
				m_gamma = 1;
				CloakEngine::Files::IReader* read = nullptr;
				CREATE_INTERFACE(CE_QUERY_ARGS(&read));
				CloakEngine::Files::IReadBuffer* buffer = path.OpenReader();
				HRESULT hRet = E_FAIL;
				if (read->SetTarget(buffer) != 0)
				{
					hRet = ReadHeader(read);
					if (FAILED(hRet)) { SAVE_RELEASE(read); return hRet; }
					if (img->dataSize > 0)
					{
						delete[] img->data;
						img->data = nullptr;
						img->dataSize = 0;
					}
					img->width = m_W;
					img->height = m_H;
					img->dataSize = img->width*img->height;
					img->data = new CloakEngine::Helper::Color::RGBA[img->dataSize];
					hRet = ReadData(read, img);
				}
				SAVE_RELEASE(buffer);
				SAVE_RELEASE(read);
				return hRet;
			}

			HRESULT CLOAK_CALL_THIS HDRI::ReadHeader(In CloakEngine::Files::IReader* read)
			{
				if (CheckString(read, SIGNATURE) == NOT_FOUND) { return E_FAIL; }
				MoveToEOL(read);
				while (read->IsAtEnd() == false)
				{
					char c = static_cast<char>(read->ReadBits(8));
					if (c == '\n') { break; }
					const size_t metF = CheckStringChar(read, META, c);
					if (metF == 0) //FORMAT=
					{
						m_format = CheckStringTrim(read, CODEC);
						if (m_format == NOT_FOUND) { return E_FAIL; }
						MoveToEOL(read);
					}
					else if (metF == 1) //EXPOSURE=
					{
						float nexp = ReadFloat(read, true);
						if (nexp > 1e-12f && nexp < 1e12f) { m_exposure *= nexp; }
					}
					else if (metF == 2) //GAMMA=
					{
						float nexp = ReadFloat(read, true);
						if (nexp > 1e-12f && nexp < 1e12f) { m_gamma = nexp; }
					}
					else
					{
						MoveToEOL(read);
					}
				}
				if (m_format == NOT_FOUND) { return E_FAIL; }

				bool neg = false;
				bool hX = false;

				char c = static_cast<char>(read->ReadBits(8));
				if (c == '-') { neg = true; }
				else if (c == '+') { neg = false; }
				else { return HRESULT_FROM_WIN32(ERROR_INVALID_DATA); }

				c = static_cast<char>(read->ReadBits(8));
				if (c == 'X') { hX = true; }
				else if (c == 'Y') { hX = false; }
				else { return HRESULT_FROM_WIN32(ERROR_INVALID_DATA); }

				UINT v = static_cast<UINT>(ReadInt(read));
				if (hX == true) { m_W = v; m_nd[0] = neg; }
				else { m_H = v; m_nd[1] = neg; }

				do {
					c = static_cast<char>(read->ReadBits(8));
				} while (read->IsAtEnd() == false && (c == ' ' || c == '\t'));

				if (c == '-') { neg = true; }
				else if (c == '+') { neg = false; }
				else { return HRESULT_FROM_WIN32(ERROR_INVALID_DATA); }

				c = static_cast<char>(read->ReadBits(8));
				if ((hX == true && c == 'X') || (hX == false && c == 'Y') || (c != 'X' && c != 'Y')) { return HRESULT_FROM_WIN32(ERROR_INVALID_DATA); }

				v = static_cast<UINT>(ReadInt(read, true));
				if (hX == true) { m_H = v; m_nd[1] = neg; }
				else { m_W = v; m_nd[0] = neg; }

				m_nd[2] = hX;

				return read->IsAtEnd() ? E_FAIL : S_OK;
			}
			HRESULT CLOAK_CALL_THIS HDRI::ReadData(In CloakEngine::Files::IReader* read, Out TextureLoader::Image* img)
			{
				const UINT W = m_nd[2] ? m_H : m_W;
				const UINT H = m_nd[2] ? m_W : m_H;

				uint8_t* line = new uint8_t[W << 2];
				for (UINT y = 0; y < H; y++)
				{
					uint8_t colData[4];
					for (size_t a = 0; a < 4; a++) { colData[a] = static_cast<uint8_t>(read->ReadBits(8)); }
					if (colData[0] == 2 && colData[1] == 2 && colData[2] < 128)
					{
						if (read->IsAtEnd()) { return E_FAIL; }
						//Adaptive run length encoding
						if ((static_cast<size_t>(colData[2]) << 8) + colData[3] != W) { return E_FAIL; }
						for (size_t c = 0; c < 4; c++)
						{
							for (UINT x = 0; x < W;)
							{
								uint8_t runLen = static_cast<uint8_t>(read->ReadBits(8));
								if (runLen > 128)
								{
									runLen &= 0x7F;
									if (x + runLen > W) { return E_FAIL; }
									uint8_t val = static_cast<uint8_t>(read->ReadBits(8));
									for (size_t a = 0; a < runLen; a++, x++)
									{
										line[(x << 2) + c] = val;
									}
								}
								else if (x + runLen > W)
								{
									return E_FAIL;
								}
								else
								{
									for (size_t a = 0; a < runLen; a++, x++)
									{
										line[(x << 2) + c] = static_cast<uint8_t>(read->ReadBits(8));
									}
								}
							}
						}
						for (UINT x = 0; x < W; x++)
						{
							img->data[GetDataPos(x, y, m_W, m_H, m_nd)] = ToColor(&line[x << 2], m_exposure, m_gamma, m_format);
						}
					}
					else
					{
						uint8_t prevData[4];
						for (size_t a = 0; a < 4; a++) { prevData[a] = colData[a]; }
						uint8_t bitShift = 0;
						for (UINT x = 0; x < W;)
						{
							if (colData[0] == 1 && colData[1] == 1 && colData[2] == 1)
							{
								//Standard run length encoding
								if (bitShift > 24) { return E_FAIL; }
								const size_t len = colData[3] << bitShift;
								if (x + len > W) { return E_FAIL; }
								for (size_t a = 0; a < len; a++, x++)
								{
									img->data[GetDataPos(x, y, m_W, m_H, m_nd)] = ToColor(prevData, m_exposure, m_gamma, m_format);
								}
								bitShift += 8;
							}
							else
							{
								//Uncompressed
								img->data[GetDataPos(x, y, m_W, m_H, m_nd)] = ToColor(colData, m_exposure, m_gamma, m_format);
								for (size_t a = 0; a < 4; a++) { prevData[a] = colData[a]; }
								bitShift = 0;
								x++;
							}
						}
						for (size_t a = 0; a < 4; a++) { colData[a] = static_cast<uint8_t>(read->ReadBits(8)); }
					}
				}
				delete[] line;
				return S_OK;
			}
		}
	}
}