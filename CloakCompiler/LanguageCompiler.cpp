#include "stdafx.h"
#include "CloakCompiler/Language.h"
#include "CloakEngine/Files/ExtendedBuffers.h"

#include "Engine/TempHandler.h"

#include <sstream>

namespace CloakCompiler {
	namespace API {
		namespace Language {
			namespace v1000 {
				constexpr CloakEngine::Files::FileType g_TempFileType{ "LangTemp","CELT",1000 };
				constexpr CloakEngine::Files::FileType g_headerFileType{ FileType.Key,"H",FileType.Version };

				struct Leaf {
					uint32_t Len = 0;//Range 0 to 256
					uint64_t Data[4] = { 0,0,0,0 };
					char16_t Char = 0;

					CLOAK_CALL Leaf() {}
					CLOAK_CALL Leaf(In const Leaf& l) { Len = l.Len; Char = l.Char; for (size_t a = 0; a < 4; a++) { Data[a] = l.Data[a]; } }
					Leaf& CLOAK_CALL_THIS operator=(In const Leaf& l) { Len = l.Len; Char = l.Char; for (size_t a = 0; a < 4; a++) { Data[a] = l.Data[a]; } return *this; }
					Leaf CLOAK_CALL_THIS operator<<(In uint32_t c) const
					{
						Leaf l = *this;
						return l <<= c;
					}
					Leaf CLOAK_CALL_THIS operator|(In bool c) const
					{
						Leaf l = *this;
						return l |= c;
					}
					Leaf& CLOAK_CALL_THIS operator<<=(In uint32_t c)
					{
						for (size_t a = 0; a < 4; a++)
						{
							Data[a] <<= c;
							if (a < 3) { Data[a] |= Data[a + 1] >> (64 - c); }
						}
						Len = min(Len + c, 256);
						return *this;
					}
					Leaf& CLOAK_CALL_THIS operator|=(In bool c)
					{
						if (c == true) { Data[3] |= 1; }
						return *this;
					}
					bool CLOAK_CALL_THIS operator==(const Leaf& l) const
					{
						return Len == l.Len && Data[0] == l.Data[0] && Data[1] == l.Data[1] && Data[2] == l.Data[2] && Data[3] == l.Data[3];
					}
					void CLOAK_CALL_THIS write(In CloakEngine::Files::IWriter* write) const
					{
						if (Len > 192) { write->WriteBits(min(64, Len - 192), Data[0]); }
						if (Len > 128) { write->WriteBits(min(64, Len - 128), Data[1]); }
						if (Len > 64) { write->WriteBits(min(64, Len - 64), Data[2]); }
						if (Len > 0) { write->WriteBits(min(64, Len), Data[3]); }
					}
				};
				struct Node {
					uint64_t chance;
					Leaf code;
					bool isVal;
					bool inUse;
					union {
						char16_t val;
						struct {
							size_t Left;
							size_t Right;
						} Knot;
					};
				};
				struct VirtualLangInfo {
					CloakEngine::Files::IWriter* write = nullptr;
					CloakEngine::Files::IVirtualWriteBuffer* buffer = nullptr;
					Language LangCode = 0;
				};

				void CLOAK_CALL EncodeTranslation(In CloakEngine::Files::IWriter* write, In CloakEngine::Files::IVirtualWriteBuffer* buffer, In_reads(IDCount) TextID* IDs, In size_t IDCount, In const LanguageDesc& desc, In bool useTemp, In bool saveTemp, In const std::u16string& tempPath, const RespondeInfo& resInfo, std::function<void(const RespondeInfo&)> response)
				{
					RespondeInfo res = resInfo;
					if (useTemp)
					{
						useTemp = false;
						CloakEngine::Files::IReader* tempRead = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&tempRead));
						CloakEngine::Files::FileVersion version = tempRead->SetTarget(tempPath, g_TempFileType, false);
						if (version == g_TempFileType.Version)
						{
							bool ferr = false;
							for (size_t a = 0; a < IDCount && ferr == false; a++)
							{
								TextID id = static_cast<TextID>(tempRead->ReadBits(32));
								uint64_t tl = 0;
								while (tempRead->ReadBits(1) == 1) { tl += 256; }
								tl += tempRead->ReadBits(8);
								std::basic_ostringstream<char16_t, std::char_traits<char16_t>, std::allocator<char16_t>> tempText;
								for (size_t b = 0; b < tl; b++) { tempText << static_cast<char16_t>(tempRead->ReadBits(16)); }
								auto f = desc.Text.find(id);
								const std::u16string temp = tempText.str();
								if ((f == desc.Text.end() && temp.length() > 0) || (f != desc.Text.end() && temp.length() == 0) || (f != desc.Text.end() && temp.length() > 0 && temp.compare(f->second) != 0)) { ferr = true; }
							}
							if (ferr == false) { useTemp = true; }
						}
						if (useTemp == true)
						{
							const uint64_t byCount = tempRead->ReadBits(64);
							const unsigned int biCount = static_cast<unsigned int>(tempRead->ReadBits(3));
							for (uint64_t a = 0; a < byCount; a++) { write->WriteBits(8, tempRead->ReadBits(8)); }
							write->WriteBits(biCount, tempRead->ReadBits(biCount));
						}
						tempRead->Release();
					}
					if (useTemp == false)
					{
						//Count letters:
						std::unordered_map<char16_t, uint64_t> charCount;
						for (size_t a = 0; a < IDCount; a++)
						{
							auto f = desc.Text.find(IDs[a]);
							if (f != desc.Text.end())
							{
								for (size_t b = 0; b < f->second.length(); b++)
								{
									char16_t c = f->second[b];
									auto fc = charCount.find(c);
									if (fc == charCount.end()) { charCount[c] = 1; }
									else { charCount[c]++; }
								}
							}
						}
						if (charCount.size() > 0)
						{
							//Create graph tree
							Node* Tree = new Node[(2 * charCount.size()) - 1];
							size_t tp = 0;
							for (const auto& it : charCount)
							{
								Tree[tp].isVal = true;
								Tree[tp].chance = it.second;
								Tree[tp].val = it.first;
								Tree[tp].inUse = false;
								tp++;
							}
							for (size_t a = tp; a < (2 * charCount.size()) - 1; a++)
							{
								uint64_t c[2] = { 0,0 };
								bool s[2] = { false,false };
								size_t p[2] = { 0,0 };
								for (size_t b = 0; b < a; b++)
								{
									if (Tree[b].inUse == false)
									{
										if (s[0] == false || Tree[b].chance < c[0])
										{
											s[1] = s[0];
											p[1] = p[0];
											c[1] = c[0];
											s[0] = true;
											p[0] = b;
											c[0] = Tree[b].chance;
										}
										else if (s[1] == false || Tree[b].chance < c[1])
										{
											s[1] = true;
											p[1] = b;
											c[1] = Tree[b].chance;
										}
									}
								}
								Tree[a].chance = c[0] + c[1];
								Tree[a].inUse = false;
								Tree[a].isVal = false;
								Tree[a].Knot.Left = p[0];
								Tree[a].Knot.Right = p[1];
								Tree[p[0]].inUse = true;
								Tree[p[1]].inUse = true;
							}
							//Calculate code:
							Leaf* Code = new Leaf[charCount.size()];
							size_t CodePos = 0;
							size_t* pos = new size_t[charCount.size()];
							pos[0] = (2 * charCount.size()) - 2;
							size_t posSize = 1;
							do {
								const size_t p = pos[posSize - 1];
								posSize--;
								const Node* n = &Tree[p];
								if (n->isVal)
								{
									Code[CodePos] = n->code;
									Code[CodePos].Char = n->val;
									CodePos++;
								}
								else
								{
									const Leaf l = n->code << 1;
									Tree[n->Knot.Left].code = l | false;
									Tree[n->Knot.Right].code = l | true;
									pos[posSize] = n->Knot.Left;
									pos[posSize + 1] = n->Knot.Right;
									posSize += 2;
								}
							} while (posSize > 0);
							//Write tree:
							size_t cc = charCount.size();
							while (cc >= 256) { cc -= 256; write->WriteBits(1, 1); }
							write->WriteBits(1, 0);
							write->WriteBits(8, cc);
							for (size_t a = 0; a < charCount.size(); a++)
							{
								write->WriteBits(16, Code[a].Char);
								write->WriteBits(6, Code[a].Len - 1);
								Code[a].write(write);
							}
							//Write text lines:
							for (size_t a = 0; a < IDCount; a++)
							{
								if (response)
								{
									res.Type = RespondeType::TEXT;
									res.Text = IDs[a];
									response(res);
								}

								auto f = desc.Text.find(IDs[a]);
								if (f != desc.Text.end())
								{
									write->WriteBits(1, 1);
									const std::u16string& s = f->second;
									size_t l = s.length();
									while (l >= 256) { write->WriteBits(1, 1); l -= 256; }
									write->WriteBits(1, 0);
									write->WriteBits(8, l);
									for (size_t b = 0; b < s.length(); b++)
									{
										const char16_t c = s[b];
										bool f = false;
										for (size_t d = 0; d < charCount.size() && f == false; d++)
										{
											const Leaf* l = &Code[d];
											if (l->Char == c)
											{
												l->write(write);
												f = true;
											}
										}
									}
								}
								else
								{
									write->WriteBits(1, 0);
								}
							}

							delete[] pos;
							delete[] Code;
							delete[] Tree;

							if (saveTemp)
							{
								CloakEngine::Files::IWriter* tempWrite = nullptr;
								CREATE_INTERFACE(CE_QUERY_ARGS(&tempWrite));
								tempWrite->SetTarget(tempPath, g_TempFileType, CloakEngine::Files::CompressType::NONE);
								for (size_t a = 0; a < IDCount; a++)
								{
									const TextID id = IDs[a];
									tempWrite->WriteBits(32, id);
									auto f = desc.Text.find(IDs[a]);
									if (f != desc.Text.end())
									{
										size_t tl = f->second.length();
										while (tl >= 256) { tl -= 256; tempWrite->WriteBits(1, 1); }
										tempWrite->WriteBits(1, 0);
										tempWrite->WriteBits(8, tl);
										for (size_t b = 0; b < f->second.length(); b++)
										{
											const char16_t c = f->second[b];
											tempWrite->WriteBits(16, c);
										}
									}
									else
									{
										tempWrite->WriteBits(1, 0);
										tempWrite->WriteBits(8, 0);
									}
								}
								tempWrite->WriteBits(64, write->GetPosition());
								tempWrite->WriteBits(3, write->GetBitPosition());
								tempWrite->WriteBuffer(buffer);
								tempWrite->Save();
								tempWrite->Release();
							}
						}
						else
						{
							write->WriteBits(1, 0);
							write->WriteBits(8, 0);
							for (size_t a = 0; a < IDCount; a++)
							{
								write->WriteBits(1, 0);
							}
						}
					}
				}

				CLOAKCOMPILER_API void CLOAK_CALL EncodeToFile(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In std::function<void(const RespondeInfo&)> response)
				{
					if (output != nullptr)
					{
						bool doEncode = true;
						if ((encode.flags & EncodeFlags::NO_TEMP_READ) == EncodeFlags::NONE)
						{
							CloakEngine::Files::IReader* tempRead = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&tempRead));
							if (tempRead->SetTarget(encode.tempPath + u"_Base", g_TempFileType, false) == g_TempFileType.Version)
							{
								const size_t lanNum = static_cast<size_t>(tempRead->ReadBits(8));
								if (lanNum == desc.Translation.size())
								{
									bool f = false;
									for (size_t a = 0; a < lanNum && f == false; a++)
									{
										bool ft = false;
										const Language lanID = static_cast<Language>(tempRead->ReadBits(8));
										for (const auto& it : desc.Translation)
										{
											if (it.first == lanID) { ft = true; }
										}
										if (ft == false) { f = true; }
									}
									if (f == false)
									{
										uint64_t textNum = 0;
										while (tempRead->ReadBits(1) == 1) { textNum += 65536; }
										textNum += tempRead->ReadBits(16);
										if (textNum == desc.TextNames.size())
										{
											for (size_t a = 0; a < textNum && f == false; a++)
											{
												TextID tid = static_cast<TextID>(tempRead->ReadBits(32));
												size_t sl = 0;
												while (tempRead->ReadBits(1) == 1) { sl += 256; }
												sl += static_cast<size_t>(tempRead->ReadBits(8));
												std::stringstream tt;
												for (size_t b = 0; b < sl; b++) { tt << static_cast<char16_t>(tempRead->ReadBits(8)); }
												auto ftid = desc.TextNames.find(tid);
												if (ftid == desc.TextNames.end() || ftid->second.length() != sl || ftid->second.compare(tt.str()) != 0) { f = true; }
											}
											if (f == false) { f = !tempRead->ReadGameID(encode.targetGameID); }
											if (f == false)
											{
												doEncode = false;
												const uint32_t byc = static_cast<uint32_t>(tempRead->ReadBits(32));
												const uint32_t bic = static_cast<uint32_t>(tempRead->ReadBits(3));
												for (size_t a = 0; a < byc; a++) { output->WriteBits(8, tempRead->ReadBits(8)); }
												if (bic > 0) { output->WriteBits(bic, tempRead->ReadBits(bic)); }
											}
										}
									}
								}
							}
							tempRead->Release();
						}
						if (doEncode)
						{
							TextID* IDs = new TextID[desc.TextNames.size()];
							size_t idp = 0;
							for (const auto& id : desc.TextNames)
							{
								IDs[idp] = id.first;
								idp++;
							}
							std::vector<VirtualLangInfo> vw;
							uint64_t maxBytePos = 0;
							for (const auto& lan : desc.Translation)
							{
								CloakEngine::Files::IWriter* v = nullptr;
								CREATE_INTERFACE(CE_QUERY_ARGS(&v));
								CloakEngine::Files::IVirtualWriteBuffer* buffer = CloakEngine::Files::CreateVirtualWriteBuffer();
								v->SetTarget(buffer, CloakEngine::Files::CompressType::NONE);
								VirtualLangInfo vli;
								vli.write = v;
								vli.buffer = buffer;
								vli.LangCode = lan.first;
								vw.push_back(vli);
								RespondeInfo res;
								if (response)
								{
									res.Lan = lan.first;
									res.Text = 0;
									res.Type = RespondeType::LANGUAGE;
									response(res);
								}
								EncodeTranslation(v, buffer, IDs, idp, lan.second, (encode.flags & EncodeFlags::NO_TEMP_READ) == EncodeFlags::NONE, (encode.flags & EncodeFlags::NO_TEMP_WRITE) == EncodeFlags::NONE, encode.tempPath + u"_Lan" + CloakEngine::Helper::StringConvert::ConvertToU16(std::to_string(lan.first)), res, response);
								maxBytePos = max(maxBytePos, v->GetPosition());
							}

							CloakEngine::Files::IWriter* write = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&write));
							CloakEngine::Files::IVirtualWriteBuffer* buffer = CloakEngine::Files::CreateVirtualWriteBuffer();
							write->SetTarget(buffer, CloakEngine::Files::CompressType::NONE);
							RespondeInfo res;
							if (response)
							{
								res.Lan = 0;
								res.Text = 0;
								res.Type = RespondeType::SAVE;
								response(res);
							}
							//Write file info
							write->WriteBits(8, vw.size());
							const unsigned int bybs = static_cast<unsigned int>(floorf(log2f(static_cast<float>(maxBytePos)))) + 1;
							write->WriteBits(6, bybs - 1);
							size_t idc = idp;
							while (idc >= 65536) { idc -= 65536; write->WriteBits(1, 1); }
							write->WriteBits(1, 0);
							write->WriteBits(16, idc);
							//Write jump marks
							for (size_t a = 1; a < vw.size(); a++)
							{
								const VirtualLangInfo& v = vw[a - 1];
								write->WriteBits(8, v.LangCode);
								write->WriteBits(bybs, v.write->GetPosition());
								write->WriteBits(3, v.write->GetBitPosition());
							}
							if (vw.size() > 0) { write->WriteBits(8, vw[vw.size() - 1].LangCode); }
							//Write translations
							for (size_t a = 0; a < vw.size(); a++)
							{
								const VirtualLangInfo& v = vw[a];
								write->WriteBuffer(v.buffer);
								v.write->Release();
								v.buffer->Release();
							}

							if ((encode.flags & EncodeFlags::NO_TEMP_WRITE) == EncodeFlags::NONE)
							{
								CloakEngine::Files::IWriter* tempWrite = nullptr;
								CREATE_INTERFACE(CE_QUERY_ARGS(&tempWrite));
								tempWrite->SetTarget(encode.tempPath + u"_Base", g_TempFileType, CloakEngine::Files::CompressType::NONE);
								tempWrite->WriteBits(8, desc.Translation.size());
								for (const auto& it : desc.Translation) { tempWrite->WriteBits(8, it.first); }
								size_t tn = desc.TextNames.size();
								while (tn >= 65536) { tn -= 65536; tempWrite->WriteBits(1, 1); }
								tempWrite->WriteBits(1, 0);
								tempWrite->WriteBits(16, tn);
								for (const auto& it : desc.TextNames)
								{
									tempWrite->WriteBits(32, it.first);
									size_t sl = it.second.length();
									while (sl >= 256) { sl -= 256; tempWrite->WriteBits(1, 1); }
									tempWrite->WriteBits(1, 0);
									tempWrite->WriteBits(8, sl);
									for (size_t a = 0; a < it.second.length(); a++) { tempWrite->WriteBits(8, it.second[a]); }
								}
								tempWrite->WriteGameID(encode.targetGameID);
								tempWrite->WriteBits(32, write->GetPosition());
								tempWrite->WriteBits(3, write->GetBitPosition());
								tempWrite->WriteBuffer(buffer);
								tempWrite->Save();
								tempWrite->Release();
							}
							output->WriteBuffer(buffer);
							write->Release();
							buffer->Release();

							if (desc.CreateHeader)
							{
								if (response)
								{
									res.Type = RespondeType::HEADER;
									res.Lan = 0;
									res.Text = 0;
									response(res);
								}
								CloakEngine::Files::IWriter* write = nullptr;
								CREATE_INTERFACE(CE_QUERY_ARGS(&write));
								write->SetTarget(desc.HeaderPath, g_headerFileType, CloakEngine::Files::CompressType::NONE, true);
								write->WriteString("/*\nThis file was automaticly created with CloakEditor©. Do not change this file!\n*/\n");
								write->WriteString("#pragma once\n");
								write->WriteString("#ifndef CE_LANG_" + desc.HeaderName + "_H\n#define CE_LANG_" + desc.HeaderName + "_H\n");
								write->WriteString("#include \"CloakEngine/CloakEngine.h\"\n\nnamespace CloakEngine {\n\tnamespace Language {\n\t\tnamespace " + desc.HeaderName + " {\n");
								for (size_t a = 0; a < idp; a++)
								{
									const TextID id = IDs[a];
									auto f = desc.TextNames.find(IDs[a]);
									if (f != desc.TextNames.end())
									{
										write->WriteString("\t\t\tconstexpr CloakEngine::API::Global::Localization::TextID " + f->second + " = " + std::to_string(a) + ";\n");
									}
								}
								write->WriteString("\t\t}\n\t}\n}\n#endif");
								write->Save();
								write->Release();
							}

							delete[] IDs;
						}
					}
				}
			}
			namespace v1002 {
				constexpr CloakEngine::Files::FileType g_TempFileType{ "LangTemp","CELT",1001 };
				constexpr CloakEngine::Files::FileType g_headerFileType{ FileType.Key,"H",FileType.Version };

				inline void CLOAK_CALL SendResponse(In ResponseFunc response, In ResponseCode code, In_opt Language lang = 0, In_opt TextID text = 0)
				{
					RespondeInfo i;
					i.Code = code;
					i.Language = lang;
					i.Text = text;
					if (response) { response(i); }
				}

				struct Leaf {
					uint32_t Len = 0;//Range 0 to 256
					uint64_t Data[4] = { 0,0,0,0 };
					char16_t Char = 0;

					CLOAK_CALL Leaf() {}
					CLOAK_CALL Leaf(In const Leaf& l) { Len = l.Len; Char = l.Char; for (size_t a = 0; a < 4; a++) { Data[a] = l.Data[a]; } }
					Leaf& CLOAK_CALL_THIS operator=(In const Leaf& l) { Len = l.Len; Char = l.Char; for (size_t a = 0; a < 4; a++) { Data[a] = l.Data[a]; } return *this; }
					Leaf CLOAK_CALL_THIS operator<<(In uint32_t c) const
					{
						Leaf l = *this;
						return l <<= c;
					}
					Leaf CLOAK_CALL_THIS operator|(In bool c) const
					{
						Leaf l = *this;
						return l |= c;
					}
					Leaf& CLOAK_CALL_THIS operator<<=(In uint32_t c)
					{
						for (size_t a = 0; a < 4; a++)
						{
							Data[a] <<= c;
							if (a < 3) { Data[a] |= Data[a + 1] >> (64 - c); }
						}
						Len = min(Len + c, 256);
						return *this;
					}
					Leaf& CLOAK_CALL_THIS operator|=(In bool c)
					{
						if (c == true) { Data[3] |= 1; }
						return *this;
					}
					bool CLOAK_CALL_THIS operator==(const Leaf& l) const
					{
						return Len == l.Len && Data[0] == l.Data[0] && Data[1] == l.Data[1] && Data[2] == l.Data[2] && Data[3] == l.Data[3];
					}
					void CLOAK_CALL_THIS write(In CloakEngine::Files::IWriter* write) const
					{
						CloakDebugLog("\tWrite Leaf: " + std::to_string(Len) + " - " + std::to_string(Data[0]) + " | " + std::to_string(Data[1]) + " | " + std::to_string(Data[2]) + " | " + std::to_string(Data[3]));
						if (Len > 192) { write->WriteBits(min(64, Len - 192), Data[0]); }
						if (Len > 128) { write->WriteBits(min(64, Len - 128), Data[1]); }
						if (Len > 64) { write->WriteBits(min(64, Len - 64), Data[2]); }
						if (Len > 0) { write->WriteBits(min(64, Len), Data[3]); }
					}
				};
				struct Node {
					uint64_t chance;
					Leaf code;
					bool isVal;
					bool inUse;
					union {
						char16_t val;
						struct {
							size_t Left;
							size_t Right;
						} Knot;
					};
				};
				struct OptColor {
					union {
						CloakEngine::Helper::Color::RGBA Color;
						uint8_t Default;
					};
					bool UseDefault;
					CLOAK_CALL OptColor() : Color(), UseDefault(false) {}
					OptColor& CLOAK_CALL_THIS operator=(In const OptColor& o)
					{
						UseDefault = o.UseDefault;
						if (UseDefault) { Default = o.Default; }
						else { Color = o.Color; }
						return *this;
					}
				};
				class Tree {
					private:
						CloakEngine::HashMap<char16_t, uint64_t> m_letterCount;
						CloakEngine::HashMap<char16_t, size_t> m_leafPos;
						Leaf* m_leafs;

						enum class SegmentType {NONE, COLOR, STRING, LINEBREAK, LINK, VALUE};
						struct Segment {
							SegmentType Type;
							union {
								struct {
									OptColor Color[3];
									bool Set[3];
								} Color;
								struct {
									size_t Begin;
									size_t End;
								} String;
								struct {

								} LineBreak;
								struct {
									size_t Target;
								} Link;
								struct {
									size_t ID;
								} Value;
							};
							CLOAK_CALL Segment() { Type = SegmentType::NONE; }
							Segment& CLOAK_CALL_THIS operator=(In const Segment& s)
							{
								if (this == &s) { return *this; }
								Type = s.Type;
								switch (Type)
								{
									case SegmentType::NONE:
										break;
									case SegmentType::COLOR:
										for (size_t a = 0; a < 3; a++) { Color.Color[a] = s.Color.Color[a]; Color.Set[a] = s.Color.Set[a]; }
										break;
									case SegmentType::STRING:
										String.Begin = s.String.Begin;
										String.End = s.String.End;
										break;
									case SegmentType::LINEBREAK:
										break;
									case SegmentType::LINK:
										Link.Target = s.Link.Target;
										break;
									case SegmentType::VALUE:
										Value.ID = s.Value.ID;
										break;
									default:
										break;
								}
								return *this;
							}
						};

						enum State {
							TEXT,
							TEXT_ESCAPE,
							CMD_START,
							CMD_TYPE,
							CMD_NAME_START,
							CMD_NAME,
							CMD_VALUE_START,
							CMD_VALUE,
							INVALID,
						};
						enum class InputType {
							BACKSLASH,
							CMD_BEGIN,
							CMD_END,
							CMD_SEPERATOR,
							CMD_SET,
							OTHER,
						};
						enum class CommandType {
							COLOR,
							LINK,
							VALUE,
							SYSTEM,
						};
						struct Command {
							CommandType Type;
							CloakEngine::FlatMap<std::u16string, std::u16string> Values;
							std::pair<std::u16string, std::u16string> CurValue;
						};
						static constexpr State MACHINE[][static_cast<size_t>(InputType::OTHER) + 1] = {
							//						BACKSLASH		CMD_BEGIN		CMD_END		CMD_SEPERATOR		CMD_SET				OTHER
							/*TEXT*/			{	TEXT_ESCAPE,	CMD_START,		INVALID,	TEXT,				TEXT,				TEXT,		},
							/*TEXT_ESCAPE*/		{	TEXT,			TEXT,			TEXT,		TEXT,				TEXT,				TEXT,		},
							/*CMD_START*/		{	INVALID,		INVALID,		INVALID,	INVALID,			INVALID,			CMD_TYPE,	},
							/*CMD_TYPE*/		{	INVALID,		INVALID,		TEXT,		CMD_NAME_START,		INVALID,			CMD_TYPE,	},
							/*CMD_NAME_START*/	{	INVALID,		INVALID,		INVALID,	INVALID,			INVALID,			CMD_NAME,	},
							/*CMD_NAME*/		{	INVALID,		INVALID,		TEXT,		CMD_NAME_START,		CMD_VALUE_START,	CMD_NAME,	},
							/*CMD_VALUE_START*/	{	INVALID,		INVALID,		INVALID,	INVALID,			INVALID,			CMD_VALUE,	},
							/*CMD_VALUE*/		{	INVALID,		INVALID,		TEXT,		CMD_NAME_START,		INVALID,			CMD_VALUE,	},
							/*INVALID*/			{	INVALID,		INVALID,		INVALID,	INVALID,			INVALID,			INVALID,	},
						};

						inline void CLOAK_CALL_THIS PushSegment(Inout CloakEngine::List<Segment>& segments, Inout std::u16stringstream& s, Inout std::u16stringstream& rTxt)
						{
							const std::u16string l = s.str();
							if (l.length() > 0)
							{
								Segment nseg;
								nseg.Type = SegmentType::STRING;
								nseg.String.Begin = rTxt.str().length();
								nseg.String.End = nseg.String.Begin + l.length();
								segments.push_back(nseg);
								rTxt << l;
								s.str(u"");
							}
						}
						inline State CLOAK_CALL_THIS SetCommandType(In State curState, Inout Command* curCmd, In const std::u16string& l)
						{
							curCmd->Values.clear();
							curCmd->CurValue.first = u"";
							curCmd->CurValue.second = u"";
							if (l.compare(u"C") == 0) { curCmd->Type = CommandType::COLOR; }
							else if (l.compare(u"V") == 0) { curCmd->Type = CommandType::VALUE; }
							else if (l.compare(u"S") == 0) { curCmd->Type = CommandType::SYSTEM; }
							else if (l.compare(u"L") == 0) { curCmd->Type = CommandType::LINK; }
							else { return INVALID; }
							return curState;
						}
						inline State CLOAK_CALL_THIS SetSegmentColor(In State curState, Inout Segment* nseg, In size_t id, In const std::u16string& l, Inout CloakEngine::Stack<OptColor>& stack)
						{
							CLOAK_ASSUME(nseg->Type == SegmentType::COLOR);
							if (id >= ARRAYSIZE(nseg->Color.Set) || nseg->Color.Set[id] == true) { return INVALID; }
							nseg->Color.Set[id] = true;
							if (l.compare(u"LAST") == 0)
							{
								nseg->Color.Color[id] = stack.top();
								stack.pop();
							}
							else if (l.compare(u"RESET") == 0)
							{
								nseg->Color.Color[id].UseDefault = true;
								nseg->Color.Color[id].Default = static_cast<uint8_t>(id);
								stack.clear();
							}
							else
							{
								nseg->Color.Color[id].UseDefault = false;
								if (l.compare(u"RED") == 0) {				nseg->Color.Color[id].Color = CloakEngine::Helper::Color::Red; }
								else if (l.compare(u"BLUE") == 0) {			nseg->Color.Color[id].Color = CloakEngine::Helper::Color::Blue; }
								else if (l.compare(u"GREEN") == 0) {		nseg->Color.Color[id].Color = CloakEngine::Helper::Color::Green; }
								else if (l.compare(u"BLACK") == 0) {		nseg->Color.Color[id].Color = CloakEngine::Helper::Color::Black; }
								else if (l.compare(u"WHITE") == 0) {		nseg->Color.Color[id].Color = CloakEngine::Helper::Color::White; }
								else if (l.compare(u"YELLOW") == 0) {		nseg->Color.Color[id].Color = CloakEngine::Helper::Color::Yellow; }
								else if (l.compare(u"TURQUOISE") == 0) {	nseg->Color.Color[id].Color = CloakEngine::Helper::Color::Turquoise; }
								else if (l.compare(u"VIOLETT") == 0) {		nseg->Color.Color[id].Color = CloakEngine::Helper::Color::Violett; }
								else if (l.compare(u"GRAY") == 0) {			nseg->Color.Color[id].Color = CloakEngine::Helper::Color::Gray; }
								else if (l.compare(u"DARK_GRAY") == 0) {	nseg->Color.Color[id].Color = CloakEngine::Helper::Color::DarkGray; }
								else if (l.compare(u"DEFAULT_0") == 0) {	nseg->Color.Color[id].UseDefault = true; nseg->Color.Color[id].Default = 0; }
								else if (l.compare(u"DEFAULT_1") == 0) {	nseg->Color.Color[id].UseDefault = true; nseg->Color.Color[id].Default = 1; }
								else if (l.compare(u"DEFAULT_2") == 0) {	nseg->Color.Color[id].UseDefault = true; nseg->Color.Color[id].Default = 2; }
								else if (l.length() == 6 || l.length() == 8)
								{
									std::string s = CloakEngine::Helper::StringConvert::ConvertToU8(l);
									for (size_t a = 0, b = 0; a < s.length(); a += 2, b++)
									{
										try {
											size_t pos = 0;
											nseg->Color.Color[id].Color[b] = std::stoi(s.substr(a, a + 2), &pos, 16) / 255.0f;
											if (pos != 2 || nseg->Color.Color[id].Color[b] < 0) { return INVALID; }
										}
										catch (...) { return INVALID; }
									}
								}
								else { return INVALID; }
								stack.push(nseg->Color.Color[id]);
							}
							return curState;
						}
					public:
						CLOAK_CALL Tree()
						{
							m_leafs = nullptr;
						}
						CLOAK_CALL ~Tree()
						{
							DeleteArray(m_leafs);
						}
						void CLOAK_CALL_THIS PushString(In const std::u16string& text)
						{
							for (size_t a = 0; a < text.length(); a++)
							{
								auto f = m_letterCount.find(text[a]);
								if (f == m_letterCount.end()) { m_letterCount[text[a]] = 1; }
								else { f->second++; }
							}
						}
						void CLOAK_CALL_THIS BuildTree(In CloakEngine::Files::IWriter* write)
						{
							const size_t letterCount = m_letterCount.size();
							if (m_letterCount.size() > 0)
							{
								const size_t nodeCount = (letterCount << 1) - 1;
								Node* nodes = NewArray(Node, nodeCount);
								//Create graph tree
								size_t tp = 0;
								for (const auto& it : m_letterCount)
								{
									nodes[tp].isVal = true;
									nodes[tp].chance = it.second;
									nodes[tp].val = it.first;
									nodes[tp].inUse = false;
									tp++;
								}
								for (size_t a = tp; a < nodeCount; a++)
								{
									uint64_t c[2] = { 0,0 };
									bool s[2] = { false,false };
									size_t p[2] = { 0,0 };
									for (size_t b = 0; b < a; b++)
									{
										if (nodes[b].inUse == false)
										{
											if (s[0] == false || nodes[b].chance < c[0])
											{
												s[1] = s[0];
												p[1] = p[0];
												c[1] = c[0];
												s[0] = true;
												p[0] = b;
												c[0] = nodes[b].chance;
											}
											else if (s[1] == false || nodes[b].chance < c[1])
											{
												s[1] = true;
												p[1] = b;
												c[1] = nodes[b].chance;
											}
										}
									}
									nodes[a].chance = c[0] + c[1];
									nodes[a].inUse = false;
									nodes[a].isVal = false;
									nodes[a].Knot.Left = p[0];
									nodes[a].Knot.Right = p[1];
									nodes[p[0]].inUse = true;
									nodes[p[1]].inUse = true;
								}
								//Calculate code:
								m_leafs = NewArray(Leaf, letterCount);
								size_t* pos = NewArray(size_t, letterCount);
								size_t CodePos = 0;
								pos[0] = nodeCount-1;
								size_t posSize = 1;
								do {
									const size_t p = pos[posSize - 1];
									posSize--;
									const Node* n = &nodes[p];
									if (n->isVal)
									{
										m_leafs[CodePos] = n->code;
										m_leafs[CodePos].Char = n->val;
										m_leafPos[n->val] = CodePos;
										CodePos++;
									}
									else
									{
										const Leaf l = n->code << 1;
										nodes[n->Knot.Left].code = l | false;
										nodes[n->Knot.Right].code = l | true;
										pos[posSize] = n->Knot.Left;
										pos[posSize + 1] = n->Knot.Right;
										posSize += 2;
									}
								} while (posSize > 0);
								//Write Tree:
								write->WriteDynamic(nodeCount);
								CloakEngine::Queue<Node*> writeStack;
								writeStack.push(&nodes[nodeCount - 1]);
								do {
									Node* n = writeStack.front();
									writeStack.pop();
									if (n->isVal == true)
									{
										write->WriteBits(1, 1);
										write->WriteBits(16, n->val);
									}
									else
									{
										write->WriteBits(1, 0);
										writeStack.push(&nodes[n->Knot.Left]);
										writeStack.push(&nodes[n->Knot.Right]);
									}
								} while (writeStack.empty() == false);

								DeleteArray(nodes);
								DeleteArray(pos);
							}
							else
							{
								write->WriteDynamic(0);
							}
						}
						void CLOAK_CALL_THIS WriteString(In CloakEngine::Files::IWriter* write, In const std::u16string& text, In const Desc& desc, In const CloakEngine::HashMap<TextID, size_t>& IDToPos)
						{
							if (m_leafs != nullptr)
							{
								CloakEngine::Stack<OptColor> cols[3];
								CloakEngine::List<Segment> segments;

								State curState = TEXT;
								State lastState = curState;
								InputType it = InputType::OTHER;
								std::u16stringstream s;
								std::u16stringstream rTxt;
								std::u16stringstream cmd;
								Command curCmd;
								for (size_t p = 0; p < text.length(); p++)
								{
									const char16_t c = text[p];
									switch (c)
									{
										case '\\':	it = InputType::BACKSLASH; break;
										case '<':	it = InputType::CMD_BEGIN; break;
										case '>':	it = InputType::CMD_END; break;
										case ';':	it = InputType::CMD_SEPERATOR; break;
										case ':':	it = InputType::CMD_SET; break;
										default:	it = InputType::OTHER; break;
									}
									lastState = curState;
									curState = MACHINE[curState][static_cast<size_t>(it)];
									switch (curState)
									{
										case TEXT: 
										{
											switch (lastState)
											{
												case TEXT: s << c; break;
												case TEXT_ESCAPE:
												{
													switch (c)
													{
														case 'n':
														{
															PushSegment(segments, s, rTxt);
															Segment nseg;
															nseg.Type = SegmentType::LINEBREAK;
															segments.push_back(nseg);
															break;
														}
														default: s << c; break;
													}
													break;
												}
												case CMD_VALUE:
												{
													const std::u16string l = cmd.str();
													cmd.str(u"");
													CLOAK_ASSUME(l.length() > 0);
													curCmd.CurValue.second = l;
													//Fall through
												}
												case CMD_NAME:
												{
													const std::u16string l = cmd.str();
													cmd.str(u"");
													if (l.length() > 0)
													{
														if (curCmd.CurValue.first.length() > 0) { curCmd.Values.insert(curCmd.CurValue); }
														curCmd.CurValue.first = l;
														curCmd.CurValue.second = u"";
													}
													//Fall through
												}
												case CMD_TYPE:
												{
													const std::u16string l = cmd.str();
													cmd.str(u"");
													if (l.length() > 0) { curState = SetCommandType(curState, &curCmd, l); }
													else if (curCmd.CurValue.first.length() > 0) { curCmd.Values.insert(curCmd.CurValue); }
													switch (curCmd.Type)
													{
														case CommandType::COLOR:
														{
															PushSegment(segments, s, rTxt);
															Segment nseg;
															nseg.Type = SegmentType::COLOR;
															nseg.Color.Set[0] = nseg.Color.Set[1] = nseg.Color.Set[2] = false;
															if (segments.size() > 0 && segments.back().Type == SegmentType::COLOR)
															{
																nseg = segments.back();
																segments.pop_back();
															}
															if (curCmd.Values.size() == 0) { curState = INVALID; }
															size_t i = 0;
															for (const auto& a : curCmd.Values)
															{
																if (a.second.length() > 0)
																{
																	if (a.first.compare(u"0") == 0) { curState = SetSegmentColor(curState, &nseg, 0, a.second, cols[0]); i = 1; }
																	else if (a.first.compare(u"1") == 0) { curState = SetSegmentColor(curState, &nseg, 1, a.second, cols[1]); i = 2; }
																	else if (a.first.compare(u"2") == 0) { curState = SetSegmentColor(curState, &nseg, 2, a.second, cols[2]); i = 3; }
																	else { curState = INVALID; }
																}
																else
																{
																	if (curCmd.Values.size() == 1)
																	{
																		curState = SetSegmentColor(curState, &nseg, 0, a.first, cols[0]);
																		curState = SetSegmentColor(curState, &nseg, 1, a.first, cols[1]);
																		curState = SetSegmentColor(curState, &nseg, 2, a.first, cols[2]);
																	}
																	else
																	{
																		curState = SetSegmentColor(curState, &nseg, i, a.first, cols[i]);
																		i++;
																	}
																}
															}
															segments.push_back(nseg);
															break;
														}
														case CommandType::VALUE:
														{
															PushSegment(segments, s, rTxt);
															if (curCmd.Values.size() != 1 || curCmd.Values.begin()->second.length() != 0) { curState = INVALID; }
															else
															{
																Segment nseg;
																nseg.Type = SegmentType::VALUE;
																try {
																	const std::string v = CloakEngine::Helper::StringConvert::ConvertToU8(curCmd.Values.begin()->first);
																	size_t pos;
																	nseg.Value.ID = static_cast<size_t>(std::stoi(v, &pos));
																	if (pos != v.length()) { curState = INVALID; }
																}
																catch (...) { curState = INVALID; }
																segments.push_back(nseg);
															}
															break;
														}
														case CommandType::SYSTEM:
														{
															if (l.compare(u"NL") == 0)
															{
																PushSegment(segments, s, rTxt);
																Segment nseg;
																nseg.Type = SegmentType::LINEBREAK;
																segments.push_back(nseg);
															}
															else
															{
																if (l.compare(u"GP_DP_U") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::DPad_Up; }
																else if (l.compare(u"GP_DP_D") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::DPad_Down; }
																else if (l.compare(u"GP_DP_L") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::DPad_Left; }
																else if (l.compare(u"GP_DP_R") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::DPad_Right; }
																else if (l.compare(u"GP_BACK") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::Back; }
																else if (l.compare(u"GP_START") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::Start; }
																else if (l.compare(u"GP_LTHUMB") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::LeftThumb; }
																else if (l.compare(u"GP_RTHUMB") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::RightThumb; }
																else if (l.compare(u"GP_LS") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::LeftShoulder; }
																else if (l.compare(u"GP_RS") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::RightShoulder; }
																else if (l.compare(u"GP_A") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::A; }
																else if (l.compare(u"GP_B") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::B; }
																else if (l.compare(u"GP_X") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::X; }
																else if (l.compare(u"GP_Y") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::Y; }
																else if (l.compare(u"GP_LT") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::LeftTrigger; }
																else if (l.compare(u"GP_RT") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::RightTrigger; }
																else if (l.compare(u"GP_LTHUMB_P") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::LeftThumb_Pressed; }
																else if (l.compare(u"GP_RTHUMB_P") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::RightThumb_Pressed; }
																else if (l.compare(u"GP_LTHUMB_D") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::LeftThumb_Down; }
																else if (l.compare(u"GP_RTHUMB_D") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::RightThumb_Down; }
																else if (l.compare(u"GP_LTHUMB_U") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::LeftThumb_Up; }
																else if (l.compare(u"GP_RTHUMB_U") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::RightThumb_Up; }
																else if (l.compare(u"GP_LTHUMB_L") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::LeftThumb_Left; }
																else if (l.compare(u"GP_RTHUMB_L") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::RightThumb_Left; }
																else if (l.compare(u"GP_LTHUMB_R") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::LeftThumb_Right; }
																else if (l.compare(u"GP_RTHUMB_R") == 0) { s << CloakEngine::Global::Localization::Chars::Gamepad::RightThumb_Right; }
																else { curState = INVALID; }
															}
															break;
														}
														case CommandType::LINK:
														{
															PushSegment(segments, s, rTxt);
															bool f = false;
															for (const auto& a : desc.TextNames)
															{
																if (a.second.compare(CloakEngine::Helper::StringConvert::ConvertToU8(l)) == 0)
																{
																	Segment nseg;
																	nseg.Type = SegmentType::LINK;
																	nseg.Link.Target = IDToPos.at(a.first);
																	segments.push_back(nseg);
																	f = true;
																	break;
																}
															}
															if (f == false) { curState = INVALID; }
															break;
														}
														default:
															break;
													}
													break;
												}
												default:
													break;
											}
										}
										case CMD_TYPE: cmd << c; break;
										case CMD_NAME_START: 
										{
											const std::u16string l = cmd.str();
											cmd.str(u"");
											CLOAK_ASSUME(l.length() > 0);
											if (lastState == CMD_TYPE) { curState = SetCommandType(curState, &curCmd, l); }
											else if (lastState == CMD_NAME)
											{
												if (curCmd.CurValue.first.length() > 0) { curCmd.Values.insert(curCmd.CurValue); }
												curCmd.CurValue.first = l;
												curCmd.CurValue.second = u"";
											}
											else if (lastState == CMD_VALUE)
											{
												curCmd.CurValue.second = l;
											}
											break;
										}
										case CMD_NAME: cmd << c; break;
										case CMD_VALUE_START: 
										{
											const std::u16string l = cmd.str();
											cmd.str(u"");
											CLOAK_ASSUME(l.length() > 0);
											if (curCmd.CurValue.first.length() > 0) { curCmd.Values.insert(curCmd.CurValue); }
											curCmd.CurValue.first = l;
											curCmd.CurValue.second = u"";
											break;
										}
										case CMD_VALUE: cmd << c; break;
										default:
											break;
									}
								}
								if (curState == TEXT) { PushSegment(segments, s, rTxt); }
								else { curState = INVALID; }

								if (curState == INVALID)
								{
									//TODO
									CloakDebugLog("Language failed to parse '" + CloakEngine::Helper::StringConvert::ConvertToU8(text) + "'");
								}
								else
								{
									write->WriteDynamic(segments.size());
									const std::u16string ftext = rTxt.str();
									for (size_t a = 0; a < segments.size(); a++)
									{
										const Segment& seg = segments[a];
										switch (seg.Type)
										{
											case SegmentType::COLOR:
											{
												write->WriteBits(3, 1);
												for (size_t b = 0; b < 3; b++)
												{
													if (seg.Color.Set[b] == true) 
													{ 
														write->WriteBits(1, 1);
														if (seg.Color.Color[b].UseDefault == true)
														{
															write->WriteBits(1, 0);
															write->WriteBits(2, seg.Color.Color[b].Default);
														}
														else
														{
															write->WriteBits(1, 1);
															for (size_t c = 0; c < 4; c++) { write->WriteDouble(32, seg.Color.Color[b].Color[c]); }
														}
													}
													else { write->WriteBits(1, 0); }
												}
												break;
											}
											case SegmentType::LINEBREAK:
												write->WriteBits(3, 3);
												break;
											case SegmentType::LINK:
												write->WriteBits(3, 4);
												write->WriteDynamic(seg.Link.Target);
												break;
											case SegmentType::STRING:
												write->WriteBits(3, 5);
												write->WriteDynamic(seg.String.End - seg.String.Begin);
												for (size_t b = seg.String.Begin; b < seg.String.End; b++) { m_leafs[m_leafPos[ftext[b]]].write(write); }
												break;
											case SegmentType::VALUE:
												write->WriteBits(3, 6);
												write->WriteDynamic(seg.Value.ID);
												break;
											case SegmentType::NONE:
											default:
												write->WriteBits(3, 0);
												break;
										}
									}
								}
							}
						}

				};

				bool CLOAK_CALL CheckTemp(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In ResponseFunc response)
				{
					if ((encode.flags & EncodeFlags::NO_TEMP_READ) == EncodeFlags::NONE)
					{
						CloakEngine::Files::IReader* read = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&read));
						if (read != nullptr)
						{
							const std::u16string tmpPath = encode.tempPath;
							bool suc = (read->SetTarget(tmpPath, g_TempFileType, false, true) == g_TempFileType.Version);
							if (suc) { SendResponse(response, ResponseCode::READ_TEMP); }
							if (suc) { suc = !Engine::TempHandler::CheckGameID(read, encode.targetGameID); }
							if (suc) { suc = desc.TextNames.size() == read->ReadDynamic(); }
							if (suc) { suc = desc.Translation.size() == read->ReadDynamic(); }
							if (suc)
							{
								for (const auto& a : desc.TextNames)
								{
									if (suc) { suc = a.first == read->ReadDynamic(); }
									if (suc) { suc = a.second.compare(read->ReadStringWithLength()) == 0; }
								}
							}
							if (suc)
							{
								for (const auto& a : desc.Translation)
								{
									if (suc) { suc = a.first == read->ReadDynamic(); }
									if (suc) { suc = a.second.Text.size() == read->ReadDynamic(); }
									if (suc)
									{
										for (const auto& b : a.second.Text)
										{
											if (suc) { suc = b.first == read->ReadDynamic(); }
											if (suc) { suc = b.second.compare(CE::Helper::StringConvert::ConvertToU16(read->ReadStringWithLength())) == 0; }
										}
									}
								}
							}
							if (suc)
							{
								const unsigned long long byp = read->ReadDynamic();
								const unsigned int bip = static_cast<unsigned int>(read->ReadBits(3));
								for (unsigned long long a = 0; a < byp; a++) { output->WriteBits(8, read->ReadBits(8)); }
								output->WriteBits(bip, read->ReadBits(bip));
							}

							SAVE_RELEASE(read);
							return suc;
						}
					}
					return false;
				}
				void CLOAK_CALL WriteTemp(In const EncodeDesc& encode, In const Desc& desc, In CloakEngine::Files::IVirtualWriteBuffer* buffer, In unsigned long long byp, In unsigned int bip, In ResponseFunc response)
				{
					if ((encode.flags & EncodeFlags::NO_TEMP_WRITE) == EncodeFlags::NONE)
					{
						CloakEngine::Files::IWriter* write = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&write));
						if (write != nullptr)
						{
							SendResponse(response, ResponseCode::WRITE_TEMP);
							const std::u16string tmpPath = encode.tempPath;
							write->SetTarget(tmpPath, g_TempFileType, CloakEngine::Files::CompressType::NONE);
							Engine::TempHandler::WriteGameID(write, encode.targetGameID);
							write->WriteDynamic(desc.TextNames.size());
							write->WriteDynamic(desc.Translation.size());
							for (const auto& a : desc.TextNames)
							{
								write->WriteDynamic(a.first);
								write->WriteStringWithLength(a.second);
							}
							for (const auto& a : desc.Translation)
							{
								write->WriteDynamic(a.first);
								write->WriteDynamic(a.second.Text.size());
								for (const auto& b : a.second.Text)
								{
									write->WriteDynamic(b.first);
									write->WriteStringWithLength(b.second);
								}
							}
							write->WriteDynamic(byp);
							write->WriteBits(3, bip);
							write->WriteBuffer(buffer, byp, bip);
							write->Save();
						}
						SAVE_RELEASE(write);
					}
				}
				void CLOAK_CALL WriteHeader(In const HeaderDesc& header, In const Desc& desc, In const CloakEngine::HashMap<TextID, size_t>& IDToPos, In ResponseFunc response)
				{
					if (header.CreateHeader)
					{
						SendResponse(response, ResponseCode::WRITE_HEADER);
						CloakEngine::Files::IWriter* write = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&write));
						write->SetTarget(header.Path, g_headerFileType, CloakEngine::Files::CompressType::NONE, true);
						write->WriteString("/*\nThis file was automaticly created with CloakEditor©. Do not change this file!\n*/\n");
						write->WriteString("#pragma once\n");
						write->WriteString("#ifndef CE_LANG_" + header.HeaderName + "_H\n#define CE_LANG_" + header.HeaderName + "_H\n");
						write->WriteString("#include \"CloakEngine\\CloakEngine.h\"\n\nnamespace CloakEngine {\n\tnamespace Language {\n\t\tnamespace " + header.HeaderName + " {\n");
						for (const auto& a : desc.TextNames)
						{
							write->WriteString("\t\t\tconstexpr CloakEngine::API::Files::TextID " + a.second + " = " + std::to_string(IDToPos.at(a.first)) + ";\n");
						}
						write->WriteString("\t\t}\n\t}\n}\n#endif");
						write->Save();
						write->Release();
					}
				}
				void CLOAK_CALL WriteLanguage(In CloakEngine::Files::IWriter* write, In const LanguageDesc& lang, In TextID* PosToID, In size_t IDCount, In const Desc& desc, In const CloakEngine::HashMap<TextID, size_t>& IDToPos, In ResponseFunc response, In Language langID)
				{
					Tree tree;
					for (const auto& a : lang.Text) { tree.PushString(a.second); }
					tree.BuildTree(write);
					for (size_t a = 0; a < IDCount; a++)
					{
						auto f = lang.Text.find(PosToID[a]);
						if (f == lang.Text.end()) { write->WriteBits(1, 0); }
						else
						{
							write->WriteBits(1, 1);
							tree.WriteString(write, f->second, desc, IDToPos);
						}
						SendResponse(response, ResponseCode::FINISH_TEXT, langID, PosToID[a]);
					}
					SendResponse(response, ResponseCode::FINISH_LANGUAGE, langID);
				}


				CLOAKCOMPILER_API void CLOAK_CALL EncodeToFile(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In_opt const HeaderDesc& header, In ResponseFunc response)
				{
					CloakEngine::HashList<TextID> usedIDs;
					for (const auto& a : desc.TextNames) { usedIDs.insert(a.first); }
					for (const auto& a : desc.Translation) { for (const auto& b : a.second.Text) { usedIDs.insert(b.first); } }
					CloakEngine::HashMap<TextID, size_t> IDToPos;
					size_t idPos = 0;
					for (const auto& a : usedIDs) { IDToPos[a] = idPos++; }
					const size_t IDCount = IDToPos.size();
					TextID* PosToID = NewArray(TextID, IDCount);
					for (const auto& a : IDToPos) { PosToID[a.second] = a.first; }

					if (CheckTemp(output, encode, desc, response) == false)
					{
						CloakEngine::Files::IVirtualWriteBuffer* wbuffer = CloakEngine::Files::CreateVirtualWriteBuffer();
						CloakEngine::Files::IWriter* write = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&write));
						write->SetTarget(wbuffer);
						write->WriteDynamic(desc.Translation.size());
						write->WriteDynamic(IDCount);
						Tree NameTree;
						for (const auto& a : desc.TextNames) { NameTree.PushString(CloakEngine::Helper::StringConvert::ConvertToU16(a.second)); }
						NameTree.BuildTree(write);
						for (size_t a = 0; a < IDCount; a++)
						{
							auto b = desc.TextNames.find(PosToID[a]);
							if (b == desc.TextNames.end()) { write->WriteBits(1, 0); }
							else {
								write->WriteBits(1, 1);
								std::u16string t = CloakEngine::Helper::StringConvert::ConvertToU16(b->second);
								NameTree.WriteString(write, t, desc, IDToPos);
							}
						}
						for (const auto& a : desc.Translation)
						{
							CloakEngine::Files::IVirtualWriteBuffer* lbuffer = CloakEngine::Files::CreateVirtualWriteBuffer();
							CloakEngine::Files::IWriter* lw = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&lw));
							lw->SetTarget(lbuffer);
							WriteLanguage(lw, a.second, PosToID, IDCount, desc, IDToPos, response, a.first);
							const unsigned long long byp = lw->GetPosition();
							const unsigned int bip = lw->GetBitPosition();
							lw->Save();
							write->WriteBits(10, a.first);
							write->WriteDynamic(byp);
							write->WriteBits(3, bip);
							write->WriteBuffer(lbuffer, byp, bip);
							SAVE_RELEASE(lw);
							SAVE_RELEASE(lbuffer);
						}

						const unsigned long long byp = write->GetPosition();
						const unsigned int bip = write->GetBitPosition();
						write->Save();
						output->WriteBuffer(wbuffer, byp, bip);

						WriteTemp(encode, desc, wbuffer, byp, bip, response);
						SAVE_RELEASE(write);
						SAVE_RELEASE(wbuffer);
					}
					WriteHeader(header, desc, IDToPos, response);
					DeleteArray(PosToID);
				}
			}
		}
	}
}