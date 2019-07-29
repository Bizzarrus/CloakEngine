#include "stdafx.h"
#include "Implementation/Files/Language.h"
#include "Implementation/Files/Font.h"
#include "CloakEngine/Helper/StringConvert.h"
#include "CloakEngine/Helper/Types.h"

#include <sstream>

#define FLOAT_EQUALS(a,b) (fabsf((a)-(b))<1e-5f)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace Language_v2 {
				constexpr API::Files::FileType g_fileType{ "TextTranslation","CETT",1003 };

				constexpr size_t ENTRY_SIZE = sizeof(largest_type<Entry, EntryColor1, EntryColor2, EntryColor3, EntryColorDefault, EntryLink, EntryString, EntryValue>::type);

				struct LineData {
					size_t charCount;
					float length;
				};

				template<EntryType T> struct GetEntryType { };
				template<> struct GetEntryType<EntryType::NONE> { typedef Entry type; };
				template<> struct GetEntryType<EntryType::STRING> { typedef EntryString type; };
				template<> struct GetEntryType<EntryType::COLOR_1> { typedef EntryColor1 type; };
				template<> struct GetEntryType<EntryType::COLOR_2> { typedef EntryColor2 type; };
				template<> struct GetEntryType<EntryType::COLOR_3> { typedef EntryColor3 type; };
				template<> struct GetEntryType<EntryType::LINEBREAK> { typedef Entry type; };
				template<> struct GetEntryType<EntryType::LINK> { typedef EntryLink type; };
				template<> struct GetEntryType<EntryType::LINK_PROXY> { typedef EntryLinkProxy type; };
				template<> struct GetEntryType<EntryType::COLOR_DEFAULT> { typedef EntryColorDefault type; };
				template<> struct GetEntryType<EntryType::VALUE> { typedef EntryValue type; };

				template<EntryType T, typename R = GetEntryType<T>::type> inline R* CLOAK_CALL GetEntry(In Entry* entries, In size_t entryCount, In size_t pos)
				{
					if (pos >= entryCount) { return nullptr; }
					Entry* e = reinterpret_cast<Entry*>(reinterpret_cast<uintptr_t>(entries) + (pos*ENTRY_SIZE));
					if (e->Type != T)
					{
						e->~Entry();
						new(e)R();
						e->Type = T;
					}
					return reinterpret_cast<R*>(e);
				}
				inline Entry* CLOAK_CALL GetUnknownEntry(In Entry* entries, In size_t entryCount, In size_t pos)
				{
					if (pos >= entryCount) { return nullptr; }
					return reinterpret_cast<Entry*>(reinterpret_cast<uintptr_t>(entries) + (pos*ENTRY_SIZE));
				}
				inline Entry* CLOAK_CALL AllocateEntries(In size_t num)
				{
					uintptr_t e = reinterpret_cast<uintptr_t>(API::Global::Memory::MemoryHeap::Allocate(num*ENTRY_SIZE));
					for (size_t a = 0; a < num; a++)
					{
						Entry* en = reinterpret_cast<Entry*>(e + (a*ENTRY_SIZE));
						new(en)Entry();
						en->Type = EntryType::NONE;
					}
					return reinterpret_cast<Entry*>(e);
				}
				inline void CLOAK_CALL FreeEntries(In Entry* entry, In size_t num)
				{
					uintptr_t e = reinterpret_cast<uintptr_t>(entry);
					for (size_t a = 0; a < num; a++)
					{
						Entry* en = reinterpret_cast<Entry*>(e + (a*ENTRY_SIZE));
						en->~Entry();
					}
					API::Global::Memory::MemoryHeap::Free(entry);
				}

				CLOAK_CALL Tree::Tree(In API::Files::IReader* read)
				{
					m_nodeCount = static_cast<size_t>(read->ReadDynamic());
					m_nodes = reinterpret_cast<Node*>(API::Global::Memory::MemoryPool::Allocate(sizeof(Node)*m_nodeCount));
					API::Queue<Node*> stack;
					bool left = true;
					for (size_t a = 0; a < m_nodeCount; a++)
					{
						Node* next = &m_nodes[a];
						if (stack.empty() == false)
						{
							Node* l = stack.front();
							if (left == false) { stack.pop(); }
							assert(l->Type == Node::NODE_TYPE_NODE);
							l->Nodes[left ? 0 : 1] = next;
							left = !left;
						}
						if (read->ReadBits(1) == 1)
						{
							next->Type = Node::NODE_TYPE_LEAF;
							next->Char = static_cast<char16_t>(read->ReadBits(16));
						}
						else
						{
							next->Type = Node::NODE_TYPE_NODE;
							stack.push(next);
						}
					}
					assert(stack.empty());
				}
				CLOAK_CALL Tree::~Tree()
				{
					API::Global::Memory::MemoryPool::Free(m_nodes);
				}
				char16_t CLOAK_CALL_THIS Tree::Read(In API::Files::IReader* read) const
				{
					Node* node = m_nodes;
					while (node != nullptr && node->Type == Node::NODE_TYPE_NODE)
					{
						if (read->ReadBits(1) == 0) { node = node->Nodes[0]; }
						else { node = node->Nodes[1]; }
					}
					assert((node == nullptr) == (m_nodes == nullptr));
					return node == nullptr ? u'\0' : node->Char;
				}
				std::u16string CLOAK_CALL_THIS Tree::ReadString(In API::Files::IReader* read, In size_t len) const
				{
					std::u16stringstream stream;
					for (size_t b = 0; b < len; b++) { stream << Read(read); }
					return stream.str();
				}

				CLOAK_CALL StringProxy::StringProxy(In_opt Language* parent, In_opt size_t stringID) : m_parent(parent), m_stringID(stringID)
				{
					DEBUG_NAME(StringProxy);
					if (m_parent != nullptr) { m_parent->AddRef(); }
					m_lastFont = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					m_update = 0;
					m_entryAlloc = 0;
					m_lastUpdate = 0;
					m_lastParentUpdate = 0;
					m_updateCount = 0;
					m_lastUpdateCount = 0;
					m_lastSize = 0;
					m_lastWidth = 0;
					m_lastHeight = 0;
					m_lastSupressLB = false;
					m_entries = nullptr;
					m_entryCount = 0;
					m_lastParentUpdate = 0;
					INIT_EVENT(onLoad, m_sync);
					INIT_EVENT(onUnload, m_sync);
				}
				CLOAK_CALL StringProxy::~StringProxy()
				{
					FreeEntries(m_entries, m_entryCount);
					SAVE_RELEASE(m_parent);
					SAVE_RELEASE(m_sync);
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In const std::u16string& text)
				{
					API::Helper::Lock lock(m_sync);
					iResize(entry + 1);
					auto e = GetEntry<EntryType::STRING>(m_entries, m_entryCount, entry);
					e->Text = API::Helper::StringConvert::ConvertToU8(text);
					m_update = m_lastUpdate + 1;
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In const std::string& text)
				{
					API::Helper::Lock lock(m_sync);
					iResize(entry + 1);
					auto e = GetEntry<EntryType::STRING>(m_entries, m_entryCount, entry);
					e->Text = text;
					m_update = m_lastUpdate + 1;
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In float data)
				{
					API::Helper::Lock lock(m_sync);
					iResize(entry + 1);
					auto e = GetEntry<EntryType::STRING>(m_entries, m_entryCount, entry);
					e->Text = std::to_string(data);
					m_update = m_lastUpdate + 1;
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In unsigned int data)
				{
					API::Helper::Lock lock(m_sync);
					iResize(entry + 1);
					auto e = GetEntry<EntryType::STRING>(m_entries, m_entryCount, entry);
					e->Text = std::to_string(data);
					m_update = m_lastUpdate + 1;
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In int data)
				{
					API::Helper::Lock lock(m_sync);
					iResize(entry + 1);
					auto e = GetEntry<EntryType::STRING>(m_entries, m_entryCount, entry);
					e->Text = std::to_string(data);
					m_update = m_lastUpdate + 1;
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In API::Files::Language_v2::IString* link)
				{
					Impl::Files::Language_v2::IString* target = nullptr;
					if (SUCCEEDED(link->QueryInterface(CE_QUERY_ARGS(&target))))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::LINK>(m_entries, m_entryCount, entry);
						SAVE_RELEASE(e->Link);
						e->Link = target;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In_range(0, 2) uint8_t channel1, In const API::Helper::Color::RGBA& color1)
				{
					if (CloakDebugCheckOK(channel1 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_1>(m_entries, m_entryCount, entry);
						e->Color = color1;
						e->Default[0] = 3;
						e->Default[1] = 3;
						e->ColorID[0] = channel1;
						e->ColorID[1] = 3;
						e->ColorID[2] = 3;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t default1)
				{
					if (CloakDebugCheckOK(channel1 < 3 && default1 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_DEFAULT>(m_entries, m_entryCount, entry);
						e->Default[0] = e->Default[1] = e->Default[2] = 3;
						e->Default[channel1] = default1;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In const API::Helper::Color::RGBA& color1, In const API::Helper::Color::RGBA& color2)
				{
					if (CloakDebugCheckOK(channel1 < 3 && channel2 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_2>(m_entries, m_entryCount, entry);
						e->Color[0] = color1;
						e->Color[1] = color2;
						e->Default = 3;
						e->ColorID[0] = channel1;
						e->ColorID[1] = channel2;
						e->ColorID[2] = 3;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In const API::Helper::Color::RGBA& color1, In_range(0, 2) uint8_t default2)
				{
					if (CloakDebugCheckOK(channel1 < 3 && channel2 < 3 && default2 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_1>(m_entries, m_entryCount, entry);
						e->Color = color1;
						e->Default[0] = default2;
						e->Default[1] = 3;
						e->ColorID[0] = channel1;
						e->ColorID[1] = channel2;
						e->ColorID[2] = 3;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In_range(0, 2) uint8_t default1, In const API::Helper::Color::RGBA& color2)
				{
					if (CloakDebugCheckOK(channel1 < 3 && channel2 < 3 && default1 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_1>(m_entries, m_entryCount, entry);
						e->Color = color2;
						e->Default[0] = default1;
						e->Default[1] = 3;
						e->ColorID[0] = channel2;
						e->ColorID[1] = channel1;
						e->ColorID[2] = 3;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In_range(0, 2) uint8_t default1, In_range(0, 2) uint8_t default2)
				{
					if (CloakDebugCheckOK(channel1 < 3 && channel2 < 3 && default1 < 3 && default2 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_DEFAULT>(m_entries, m_entryCount, entry);
						e->Default[0] = e->Default[1] = e->Default[2] = 3;
						e->Default[channel1] = default1;
						e->Default[channel2] = default2;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In const API::Helper::Color::RGBA& color2, In const API::Helper::Color::RGBA& color3)
				{
					API::Helper::Lock lock(m_sync);
					iResize(entry + 1);
					auto e = GetEntry<EntryType::COLOR_3>(m_entries, m_entryCount, entry);
					e->Color[0] = color1;
					e->Color[1] = color2;
					e->Color[2] = color3;
					m_update = m_lastUpdate + 1;
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In const API::Helper::Color::RGBA& color2, In_range(0, 2) uint8_t default3)
				{
					if (CloakDebugCheckOK(default3 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_2>(m_entries, m_entryCount, entry);
						e->Color[0] = color1;
						e->Color[1] = color2;
						e->Default = default3;
						e->ColorID[0] = 0;
						e->ColorID[1] = 1;
						e->ColorID[2] = 2;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In_range(0, 2) uint8_t default2, In const API::Helper::Color::RGBA& color3)
				{
					if (CloakDebugCheckOK(default2 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_2>(m_entries, m_entryCount, entry);
						e->Color[0] = color1;
						e->Color[1] = color3;
						e->Default = default2;
						e->ColorID[0] = 0;
						e->ColorID[1] = 2;
						e->ColorID[2] = 1;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In_range(0, 2) uint8_t default2, In_range(0, 2) uint8_t default3)
				{
					if (CloakDebugCheckOK(default2 < 3 && default3 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_1>(m_entries, m_entryCount, entry);
						e->Color = color1;
						e->Default[0] = default2;
						e->Default[1] = default3;
						e->ColorID[0] = 0;
						e->ColorID[1] = 1;
						e->ColorID[2] = 2;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In_range(0, 2) uint8_t default1, In const API::Helper::Color::RGBA& color2, In const API::Helper::Color::RGBA& color3)
				{
					if (CloakDebugCheckOK(default1 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_2>(m_entries, m_entryCount, entry);
						e->Color[0] = color2;
						e->Color[1] = color3;
						e->Default = default1;
						e->ColorID[0] = 1;
						e->ColorID[1] = 2;
						e->ColorID[2] = 0;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In_range(0, 2) uint8_t default1, In const API::Helper::Color::RGBA& color2, In_range(0, 2) uint8_t default3)
				{
					if (CloakDebugCheckOK(default1 < 3 && default3 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_1>(m_entries, m_entryCount, entry);
						e->Color = color2;
						e->Default[0] = default1;
						e->Default[1] = default3;
						e->ColorID[0] = 1;
						e->ColorID[1] = 0;
						e->ColorID[2] = 2;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In_range(0, 2) uint8_t default1, In_range(0, 2) uint8_t default2, In const API::Helper::Color::RGBA& color3)
				{
					if (CloakDebugCheckOK(default1 < 3 && default2 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_1>(m_entries, m_entryCount, entry);
						e->Color = color3;
						e->Default[0] = default1;
						e->Default[1] = default2;
						e->ColorID[0] = 2;
						e->ColorID[1] = 0;
						e->ColorID[2] = 1;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::Set(In size_t entry, In_range(0, 2) uint8_t default1, In_range(0, 2) uint8_t default2, In_range(0, 2) uint8_t default3)
				{
					if (CloakDebugCheckOK(default1 < 3 && default2 < 3 && default3 < 3, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						iResize(entry + 1);
						auto e = GetEntry<EntryType::COLOR_DEFAULT>(m_entries, m_entryCount, entry);
						e->Default[0] = default1;
						e->Default[1] = default2;
						e->Default[2] = default3;
						m_update = m_lastUpdate + 1;
					}
				}
				void CLOAK_CALL_THIS StringProxy::SetDefaultColor(In size_t entry)
				{
					Set(entry, 0, 1, 2);
				}
				void CLOAK_CALL_THIS StringProxy::SetLineBreak(In size_t entry)
				{
					API::Helper::Lock lock(m_sync);
					iResize(entry + 1);
					auto e = GetEntry<EntryType::LINEBREAK>(m_entries, m_entryCount, entry);
					m_update = m_lastUpdate + 1;
				}
				void CLOAK_CALL_THIS StringProxy::Draw(In API::Rendering::IContext2D* context, In API::Rendering::IDrawable2D* drawing, In API::Files::Font_v2::IFont* font, In const API::Files::Language_v2::TextDesc& desc, In_opt UINT Z, In_opt bool supressLineBreaks)
				{
					if (font != nullptr && font->isLoaded(API::Files::Priority::NORMAL))
					{
						const float maxW = desc.BottomRight.X - desc.TopLeft.X;
						const float maxH = desc.BottomRight.Y - desc.TopLeft.Y;
						API::Helper::Lock lock(m_sync);
						const uint64_t upc = GetUpdateCount();
						bool update = (upc != m_lastUpdateCount) || font != m_lastFont || m_lastSupressLB != supressLineBreaks;
						for (size_t a = 0; a < m_linked.size() && update == false; a++) { update = update || m_linked[a].first->GetUpdateCount() != m_linked[a].second; }
						bool updateSize = update || !FLOAT_EQUALS(m_lastSize, desc.FontSize) || !FLOAT_EQUALS(m_lastWidth, maxW) || !FLOAT_EQUALS(m_lastHeight, maxH);

						const float ls = GET_FONT_SIZE(desc.FontSize);
						if (update)
						{
							m_lastUpdateCount = upc;
							m_chars.clear();
							m_lineSegments.clear();
							m_lines.clear();
							m_linked.clear();
							m_lastSupressLB = supressLineBreaks;

							LineSegment curSeg;
							Line curLine;
							for (size_t a = 0; a < 3; a++) { curSeg.Color[a] = desc.Color[a]; }
							curSeg.CharCount = 0;
							curLine.SegmentCount = 0;

							std::stringstream text;
							FillDrawInfos(desc, &m_lineSegments, &m_lines, &m_linked, &curSeg, &curLine, &text, supressLineBreaks);

							if (curSeg.CharCount > 0) { m_lineSegments.push_back(curSeg); curLine.SegmentCount++; }
							if (curLine.SegmentCount > 0) { m_lines.push_back(curLine); }

							const std::u32string ftext = API::Helper::StringConvert::ConvertToU32(text.str());
							m_chars.reserve(ftext.length());
							for (size_t b = 0; b < ftext.length(); b++)
							{
								API::Files::Font_v2::LetterInfo i = font->GetLetterInfo(ftext[b]);
								m_chars.push_back(i);
							}
						}
						if (updateSize)
						{
							m_lastSize = desc.FontSize;
							m_lastWidth = maxW;
							m_lastHeight = maxH;
							m_drawLines.clear();
							DrawLine curLine;
							curLine.CharCount = 0;
							curLine.Length = 0;
							for (size_t a = 0, sPos = 0, cPos = 0; a < m_lines.size(); a++)
							{
								const Line& l = m_lines[a];
								size_t lastNoSpace = 0;
								size_t spaceCount = 0;
								float lastNoSpaceLength = 0;
								float spaceLength = 0;
								for (size_t b = 0; b < l.SegmentCount; b++, sPos++)
								{
									const LineSegment& s = m_lineSegments[sPos];
									for (size_t c = 0; c < s.CharCount; c++, cPos++)
									{
										const API::Files::Font_v2::LetterInfo& i = m_chars[cPos];
										float iw = i.Width;
										if (curLine.CharCount > 0) { iw += desc.LetterSpace; }
										if (i.Type == API::Files::Font_v2::LetterType::SPACE)
										{
											if (lastNoSpace + spaceCount < curLine.CharCount)
											{
												lastNoSpace = curLine.CharCount;
												lastNoSpaceLength = curLine.Length;
												spaceCount = 0;
												spaceLength = 0;
											}
											spaceCount++;
											if (curLine.Length > 0) { spaceLength += iw*ls; }
											else { iw = 0; }
										}
										if (curLine.Length + (iw*ls) > maxW && supressLineBreaks == false)
										{
											if (spaceCount > 0)
											{
												const size_t nc = curLine.CharCount - (lastNoSpace + spaceCount);
												const float nl = curLine.Length - (lastNoSpaceLength + spaceLength);
												curLine.CharCount = lastNoSpace;
												curLine.Length = lastNoSpaceLength;
												m_drawLines.push_back(curLine);
												curLine.CharCount = nc;
												curLine.Length = nl;
												lastNoSpace = 0;
												spaceCount = 0;
												lastNoSpaceLength = 0;
												spaceLength = 0;
											}
											else
											{
												m_drawLines.push_back(curLine);
												curLine.CharCount = 0;
												curLine.Length = 0;
											}
											iw = i.Width;
										}
										curLine.CharCount++;
										curLine.Length += iw*ls;
									}
								}
								m_drawLines.push_back(curLine);
							}
						}
						assert(supressLineBreaks == false || m_drawLines.size() == 1);
						float maxLineW = 0;
						for (size_t a = 0; a < m_drawLines.size(); a++) { maxLineW = max(maxLineW, m_drawLines[a].Length); }
						const float letterHeight = font->getLineHeight(0)*ls;
						const float lineHeight = font->getLineHeight(desc.LineSpace)*ls;
						const float letterSpace = desc.LetterSpace*ls;
						const float textHeight = (lineHeight*(m_drawLines.size() - 1)) + letterHeight;

						API::Rendering::VERTEX_2D vertex;

						float Y = desc.TopLeft.Y;
						switch (desc.Anchor)
						{
							case API::Files::Language_v2::AnchorPoint::CENTER:
								Y += (maxH - textHeight)*0.5f;
								break;
							case API::Files::Language_v2::AnchorPoint::BOTTOM:
								Y += maxH - textHeight;
								break;
						}
						API::Rendering::SCALED_SCISSOR_RECT srect;
						srect.TopLeftX = desc.TopLeft.X;
						srect.TopLeftY = desc.TopLeft.Y;
						srect.Width = desc.BottomRight.X - desc.TopLeft.X;
						srect.Height = desc.BottomRight.Y - desc.TopLeft.Y;
						context->SetScissor(srect);
						for (size_t a = 0, sPos = 0, cPos = 0, scPos = 0; a < m_drawLines.size() && Y < desc.BottomRight.Y; a++)
						{
							const DrawLine& l = m_drawLines[a];
							float X = desc.TopLeft.X;
							switch (desc.Justify)
							{
								case API::Files::Language_v2::Justify::CENTER:
									X += (maxW - l.Length)*0.5f;
									break;
								case API::Files::Language_v2::Justify::RIGHT:
									X += maxW - l.Length;
									break;
								default:
									break;
							}
							for (size_t b = 0; b < l.CharCount; b++, cPos++, scPos++)
							{
								const LineSegment* seg = &m_lineSegments[sPos];
								while (scPos >= seg->CharCount)
								{
									scPos -= seg->CharCount;
									sPos++;
									assert(sPos < m_lineSegments.size());
									seg = &m_lineSegments[sPos];
								}
								const API::Files::Font_v2::LetterInfo& i = m_chars[cPos];

								vertex.Flags = API::Rendering::Vertex2DFlags::NONE;
								switch (i.Type)
								{
									case API::Files::Font_v2::LetterType::SINGLECOLOR:
										vertex.Color[0] = seg->Color[0];
										break;
									case API::Files::Font_v2::LetterType::MULTICOLOR:
										for (size_t d = 0; d < 3; d++) { vertex.Color[d] = seg->Color[d]; }
										vertex.Flags |= API::Rendering::Vertex2DFlags::MULTICOLOR;
										break;
									case API::Files::Font_v2::LetterType::TRUECOLOR:
										vertex.Color[0] = API::Helper::Color::White;
										break;
									default:
										break;
								}
								if (desc.Grayscale == true) { vertex.Flags |= API::Rendering::Vertex2DFlags::GRAYSCALE; }
								vertex.Position.X = X;
								vertex.Position.Y = Y;
								vertex.Size.X = i.Width*ls;
								vertex.Size.Y = letterHeight;
								vertex.TexTopLeft = i.TopLeft;
								vertex.TexBottomRight = i.BottomRight;
								vertex.Rotation.Sinus = 0;
								vertex.Rotation.Cosinus = 1;
								if (Y + lineHeight > desc.TopLeft.Y && Y<desc.BottomRight.Y && X + vertex.Size.X > desc.TopLeft.X && X < desc.BottomRight.X)
								{ 
									context->Draw(drawing, font->GetTexture(), vertex, Z); 
								}
								X += i.Width*ls;
								if (b > 0) { X += letterSpace; }
							}
							Y += lineHeight;
						}
						context->ResetScissor();
					}
				}
				std::u16string CLOAK_CALL_THIS StringProxy::GetAsString() const { return ConvertToString(); }

				ULONG CLOAK_CALL_THIS StringProxy::load(In_opt API::Files::Priority prio)
				{
					if (m_parent != nullptr) { return m_parent->load(prio); }
					return 0;
				}
				ULONG CLOAK_CALL_THIS StringProxy::unload()
				{
					if (m_parent != nullptr) { return m_parent->unload(); }
					return 0;
				}
				bool CLOAK_CALL_THIS StringProxy::isLoaded(In API::Files::Priority prio)
				{
					if (m_parent != nullptr) { return m_parent->isLoaded(prio); }
					return true;
				}
				bool CLOAK_CALL_THIS StringProxy::isLoaded() const
				{
					if (m_parent != nullptr) { return m_parent->isLoaded(); }
					return true;
				}

				void CLOAK_CALL_THIS StringProxy::FillDrawInfos(In const API::Files::Language_v2::TextDesc& desc, Inout API::List<LineSegment>* lineSegments, Inout API::List<Line>* lines, Inout API::List<std::pair<Impl::Files::Language_v2::IString*, uint64_t>>* linked, Inout LineSegment* curLineSeg, Inout Line* curLine, Inout std::stringstream* text, In bool supressLineBreaks, In_opt Entry* proxyEntries, In_opt size_t proxyCount) const
				{
					//String Proxy should not be called as data source, so proxy variables should be zero
					assert(proxyEntries == nullptr && proxyCount == 0);
					API::Helper::Lock lock(m_sync);
					if (m_parent != nullptr) 
					{
						if (m_parent->isLoaded(API::Files::Priority::NORMAL))
						{
							Impl::Files::Language_v2::IString* s = m_parent->GetRealString(m_stringID);
							if (s != nullptr)
							{
								s->FillDrawInfos(desc, lineSegments, lines, linked, curLineSeg, curLine, text, supressLineBreaks, m_entries, m_entryCount);
							}
							SAVE_RELEASE(s);
						}
					}
					else
					{
						for (size_t a = 0; a < m_entryCount; a++)
						{
							Entry* entry = GetUnknownEntry(m_entries, m_entryCount, a);
							switch (entry->Type)
							{
								case EntryType::NONE:
									break;
								case EntryType::COLOR_1:
								{
									if (curLineSeg->CharCount > 0) { lineSegments->push_back(*curLineSeg); curLine->SegmentCount++; }
									curLineSeg->CharCount = 0;
									auto* e = reinterpret_cast<GetEntryType<EntryType::COLOR_1>::type*>(entry);
									curLineSeg->Color[e->ColorID[0]] = e->Color;
									if (e->ColorID[1] < 3) { curLineSeg->Color[e->ColorID[1]] = desc.Color[e->Default[0]]; }
									if (e->ColorID[2] < 3) { curLineSeg->Color[e->ColorID[2]] = desc.Color[e->Default[1]]; }
									break;
								}
								case EntryType::COLOR_2:
								{
									if (curLineSeg->CharCount > 0) { lineSegments->push_back(*curLineSeg); curLine->SegmentCount++; }
									curLineSeg->CharCount = 0;
									auto* e = reinterpret_cast<GetEntryType<EntryType::COLOR_2>::type*>(entry);
									curLineSeg->Color[e->ColorID[0]] = e->Color[0];
									curLineSeg->Color[e->ColorID[1]] = e->Color[1];
									if (e->ColorID[2] < 3) { curLineSeg->Color[e->ColorID[2]] = desc.Color[e->Default]; }
									break;
								}
								case EntryType::COLOR_3:
								{
									if (curLineSeg->CharCount > 0) { lineSegments->push_back(*curLineSeg); curLine->SegmentCount++; }
									curLineSeg->CharCount = 0;
									auto* e = reinterpret_cast<GetEntryType<EntryType::COLOR_3>::type*>(entry);
									for (size_t b = 0; b < 3; b++) { curLineSeg->Color[b] = e->Color[b]; }
									break;
								}
								case EntryType::COLOR_DEFAULT:
								{
									if (curLineSeg->CharCount > 0) { lineSegments->push_back(*curLineSeg); curLine->SegmentCount++; }
									curLineSeg->CharCount = 0;
									auto* e = reinterpret_cast<GetEntryType<EntryType::COLOR_DEFAULT>::type*>(entry);
									for (size_t b = 0; b < 3; b++)
									{
										if (e->Default[b] < 3) { curLineSeg->Color[b] = desc.Color[e->Default[b]]; }
									}
									break;
								}
								case EntryType::STRING:
								{
									auto* e = reinterpret_cast<GetEntryType<EntryType::STRING>::type*>(entry);
									const size_t len = API::Helper::StringConvert::UTF8Length(e->Text);
									curLineSeg->CharCount += len;
									(*text) << e->Text;
									break;
								}
								case EntryType::LINK:
								{
									auto* e = reinterpret_cast<GetEntryType<EntryType::LINK>::type*>(entry);
									String* s = nullptr;
									if (e->Link != nullptr && SUCCEEDED(e->Link->QueryInterface(CE_QUERY_ARGS(&s))))
									{
										linked->push_back(std::make_pair(s, s->GetUpdateCount()));
										s->FillDrawInfos(desc, lineSegments, lines, linked, curLineSeg, curLine, text, supressLineBreaks);
									}
									SAVE_RELEASE(s);
									break;
								}
								case EntryType::LINEBREAK:
								{
									if (supressLineBreaks == false)
									{
										if (curLineSeg->CharCount > 0)
										{
											lineSegments->push_back(*curLineSeg);
											curLine->SegmentCount++;
										}
										lines->push_back(*curLine);
										curLine->SegmentCount = 0;
										curLineSeg->CharCount = 0;
									}
									break;
								}
								default:
									break;
							}
						}
					}
				}
				uint64_t CLOAK_CALL_THIS StringProxy::GetUpdateCount()
				{
					API::Helper::Lock lock(m_sync);
					bool req = m_update != m_lastUpdate;
					if (req) { m_lastUpdate = m_update; }
					if (m_parent != nullptr && m_parent->isLoaded()) 
					{ 
						Impl::Files::Language_v2::IString* s = m_parent->GetRealString(m_stringID);
						if (s != nullptr)
						{
							const uint64_t pu = s->GetUpdateCount();
							if (m_lastParentUpdate != pu)
							{
								m_lastParentUpdate = pu;
								req = true;
							}
						}
						SAVE_RELEASE(s);
					}
					if (req) { m_updateCount++; }
					return m_updateCount;
				}
				std::u16string CLOAK_CALL_THIS StringProxy::ConvertToString(In_opt Entry* proxyEntries, In_opt size_t proxyCount) const
				{
					//String Proxy should not be called as data source, so proxy variables should be zero
					assert(proxyEntries == nullptr && proxyCount == 0);
					API::Helper::Lock lock(m_sync);
					if (m_parent != nullptr)
					{
						if (m_parent->isLoaded(API::Files::Priority::NORMAL))
						{
							Impl::Files::Language_v2::IString* s = m_parent->GetRealString(m_stringID);
							if (s != nullptr) 
							{ 
								std::u16string r = s->ConvertToString(m_entries, m_entryCount);
								s->Release();
								return r;
							}
						}
					}
					else
					{
						std::u16stringstream s;
						for (size_t a = 0; a < m_entryCount; a++)
						{
							auto* entry = GetUnknownEntry(m_entries, m_entryCount, a);
							switch (entry->Type)
							{
								case EntryType::LINEBREAK: 
								{
									s << std::endl; 
									break; 
								}
								case EntryType::LINK:
								{
									auto* e = reinterpret_cast<GetEntryType<EntryType::LINK>::type*>(entry);
									s << e->Link->ConvertToString();
									break;
								}
								case EntryType::STRING:
								{
									auto* e = reinterpret_cast<GetEntryType<EntryType::STRING>::type*>(entry);
									s << API::Helper::StringConvert::ConvertToU16(e->Text);
									break;
								}
								default:
									break;
							}
						}
						return s.str();
					}
					return u"";
				}
			
				Success(return == true) bool CLOAK_CALL_THIS StringProxy::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::FileHandler_v1::IFileHandler)) { *ptr = (API::Files::FileHandler_v1::IFileHandler*)this; return true; }
					else if (riid == __uuidof(API::Files::Language_v2::IString)) { *ptr = (API::Files::Language_v2::IString*)this; return true; }
					else if (riid == __uuidof(Impl::Files::Language_v2::IString)) { *ptr = (Impl::Files::Language_v2::IString*)this; return true; }
					else if (riid == __uuidof(Impl::Files::Language_v2::StringProxy)) { *ptr = (Impl::Files::Language_v2::StringProxy*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS StringProxy::iResize(In size_t minSize)
				{
					if (minSize > m_entryAlloc)
					{
						Entry* last = m_entries;
						const size_t newSize = m_entryAlloc == 0 ? 8 : (m_entryAlloc < 32 ? m_entryAlloc << 1 : m_entryAlloc + 32);
						m_entries = AllocateEntries(newSize);
						for (size_t a = 0; a < m_entryCount; a++)
						{
							Entry* old = GetUnknownEntry(last, m_entryCount, a);
							switch (old->Type)
							{
								case EntryType::COLOR_1:
								{
									auto* o = reinterpret_cast<GetEntryType<EntryType::COLOR_1>::type*>(old);
									auto* n = GetEntry<EntryType::COLOR_1>(m_entries, m_entryCount, a);
									n->Color = o->Color;
									n->Default[0] = o->Default[0];
									n->Default[1] = o->Default[1];
									n->ColorID[0] = o->ColorID[0];
									n->ColorID[1] = o->ColorID[1];
									n->ColorID[2] = o->ColorID[2];
									break;
								}
								case EntryType::COLOR_2:
								{
									auto* o = reinterpret_cast<GetEntryType<EntryType::COLOR_2>::type*>(old);
									auto* n = GetEntry<EntryType::COLOR_2>(m_entries, m_entryCount, a);
									n->Color[0] = o->Color[0];
									n->Color[1] = o->Color[1];
									n->Default = o->Default;
									n->ColorID[0] = o->ColorID[0];
									n->ColorID[1] = o->ColorID[1];
									n->ColorID[2] = o->ColorID[2];
									break;
								}
								case EntryType::COLOR_3:
								{
									auto* o = reinterpret_cast<GetEntryType<EntryType::COLOR_3>::type*>(old);
									auto* n = GetEntry<EntryType::COLOR_3>(m_entries, m_entryCount, a);
									for (size_t b = 0; b < 3; b++) { n->Color[b] = o->Color[b]; }
									break;
								}
								case EntryType::COLOR_DEFAULT:
								{
									auto* o = reinterpret_cast<GetEntryType<EntryType::COLOR_DEFAULT>::type*>(old);
									auto* n = GetEntry<EntryType::COLOR_DEFAULT>(m_entries, m_entryCount, a);
									for (size_t b = 0; b < 3; b++) { n->Default[b] = o->Default[b]; }
									break;
								}
								case EntryType::LINK:
								{
									auto* o = reinterpret_cast<GetEntryType<EntryType::LINK>::type*>(old);
									auto* n = GetEntry<EntryType::LINK>(m_entries, m_entryCount, a);
									n->Link = o->Link;
									n->Link->AddRef();
									break;
								}
								case EntryType::STRING:
								{
									auto* o = reinterpret_cast<GetEntryType<EntryType::STRING>::type*>(old);
									auto* n = GetEntry<EntryType::STRING>(m_entries, m_entryCount, a);
									n->Text = o->Text;
									break;
								}
								default:
									break;
							}
						}
						m_entryAlloc = newSize;
						FreeEntries(last, m_entryCount);
					}
					m_entryCount = max(m_entryCount, minSize);
				}


				CLOAK_CALL String::String(In uint64_t updateCount) : m_updateCount(updateCount)
				{
					DEBUG_NAME(String);
					m_entries = nullptr;
					m_entryCount = 0;
				}
				CLOAK_CALL String::~String()
				{
					FreeEntries(m_entries, m_entryCount);
				}
				void CLOAK_CALL_THIS String::FillDrawInfos(In const API::Files::Language_v2::TextDesc& desc, Inout API::List<LineSegment>* lineSegments, Inout API::List<Line>* lines, Inout API::List<std::pair<Impl::Files::Language_v2::IString*, uint64_t>>* linked, Inout LineSegment* curLineSeg, Inout Line* curLine, Inout std::stringstream* text, In bool supressLineBreaks, In_opt Entry* proxyEntries, In_opt size_t proxyCount) const
				{
					for (size_t a = 0; a < m_entryCount; a++)
					{
						Entry* entry = GetUnknownEntry(m_entries, m_entryCount, a);
						if (entry->Type == EntryType::VALUE)
						{
							auto* e = reinterpret_cast<GetEntryType<EntryType::VALUE>::type*>(entry);
							if (e->ID < proxyCount) { entry = GetUnknownEntry(proxyEntries, proxyCount, e->ID); }
						}
						switch (entry->Type)
						{
							case EntryType::NONE:
								break;
							case EntryType::COLOR_1:
							{
								if (curLineSeg->CharCount > 0) { lineSegments->push_back(*curLineSeg); curLine->SegmentCount++; }
								curLineSeg->CharCount = 0;
								auto* e = reinterpret_cast<GetEntryType<EntryType::COLOR_1>::type*>(entry);
								curLineSeg->Color[e->ColorID[0]] = e->Color;
								if (e->ColorID[1] < 3) { curLineSeg->Color[e->ColorID[1]] = desc.Color[e->Default[0]]; }
								if (e->ColorID[2] < 3) { curLineSeg->Color[e->ColorID[2]] = desc.Color[e->Default[1]]; }
								break;
							}
							case EntryType::COLOR_2:
							{
								if (curLineSeg->CharCount > 0) { lineSegments->push_back(*curLineSeg); curLine->SegmentCount++; }
								curLineSeg->CharCount = 0;
								auto* e = reinterpret_cast<GetEntryType<EntryType::COLOR_2>::type*>(entry);
								curLineSeg->Color[e->ColorID[0]] = e->Color[0];
								curLineSeg->Color[e->ColorID[1]] = e->Color[1];
								if (e->ColorID[2] < 3) { curLineSeg->Color[e->ColorID[2]] = desc.Color[e->Default]; }
								break;
							}
							case EntryType::COLOR_3:
							{
								if (curLineSeg->CharCount > 0) { lineSegments->push_back(*curLineSeg); curLine->SegmentCount++; }
								curLineSeg->CharCount = 0;
								auto* e = reinterpret_cast<GetEntryType<EntryType::COLOR_3>::type*>(entry);
								for (size_t b = 0; b < 3; b++) { curLineSeg->Color[b] = e->Color[b]; }
								break;
							}
							case EntryType::COLOR_DEFAULT:
							{
								if (curLineSeg->CharCount > 0) { lineSegments->push_back(*curLineSeg); curLine->SegmentCount++; }
								curLineSeg->CharCount = 0;
								auto* e = reinterpret_cast<GetEntryType<EntryType::COLOR_DEFAULT>::type*>(entry);
								for (size_t b = 0; b < 3; b++)
								{
									if (e->Default[b] < 3) { curLineSeg->Color[b] = desc.Color[e->Default[b]]; }
								}
								break;
							}
							case EntryType::STRING:
							{
								auto* e = reinterpret_cast<GetEntryType<EntryType::STRING>::type*>(entry);
								const size_t len = API::Helper::StringConvert::UTF8Length(e->Text);
								curLineSeg->CharCount += len;
								(*text) << e->Text;
								break;
							}
							case EntryType::LINK:
							{
								auto* e = reinterpret_cast<GetEntryType<EntryType::LINK>::type*>(entry);
								String* s = nullptr;
								if (e->Link != nullptr && SUCCEEDED(e->Link->QueryInterface(CE_QUERY_ARGS(&s))))
								{
									linked->push_back(std::make_pair(s, s->GetUpdateCount()));
									s->FillDrawInfos(desc, lineSegments, lines, linked, curLineSeg, curLine, text, supressLineBreaks);
								}
								SAVE_RELEASE(s);
								break;
							}
							case EntryType::LINEBREAK:
							{
								if (supressLineBreaks == false)
								{
									if (curLineSeg->CharCount > 0)
									{
										lineSegments->push_back(*curLineSeg);
										curLine->SegmentCount++;
									}
									lines->push_back(*curLine);
									curLine->SegmentCount = 0;
									curLineSeg->CharCount = 0;
								}
								break;
							}
							default:
								break;
						}
					}
				}
				uint64_t CLOAK_CALL_THIS String::GetUpdateCount()
				{
					return m_updateCount;
				}
				std::u16string CLOAK_CALL_THIS String::ConvertToString(In_opt Entry* proxyEntries, In_opt size_t proxyCount) const
				{
					std::u16stringstream s;
					for (size_t a = 0; a < m_entryCount; a++)
					{
						auto* entry = GetUnknownEntry(m_entries, m_entryCount, a);
						if (entry->Type == EntryType::VALUE)
						{
							auto* e = reinterpret_cast<GetEntryType<EntryType::VALUE>::type*>(entry);
							if (e->ID < proxyCount) { entry = GetUnknownEntry(proxyEntries, proxyCount, e->ID); }
						}
						switch (entry->Type)
						{
							case EntryType::LINEBREAK:
							{
								s << std::endl;
								break;
							}
							case EntryType::LINK:
							{
								auto* e = reinterpret_cast<GetEntryType<EntryType::LINK>::type*>(entry);
								s << e->Link->ConvertToString();
								break;
							}
							case EntryType::STRING:
							{
								auto* e = reinterpret_cast<GetEntryType<EntryType::STRING>::type*>(entry);
								s << API::Helper::StringConvert::ConvertToU16(e->Text);
								break;
							}
							default:
								break;
						}
					}
					return s.str();
				}

				void CLOAK_CALL_THIS String::ReadData(In API::Files::IReader* read, In API::Files::FileVersion version, In const Tree& tree)
				{
					const size_t entryCount = static_cast<size_t>(read->ReadDynamic());
					if (m_entryCount != entryCount)
					{
						FreeEntries(m_entries, m_entryCount);
						m_entryCount = entryCount;
						m_entries = AllocateEntries(m_entryCount);
					}
					for (size_t a = 0; a < m_entryCount; a++)
					{
						const uint8_t type = static_cast<uint8_t>(read->ReadBits(3));
						switch (type)
						{
							case 1://COLOR
							{
								if (version <= 1002)
								{
									size_t count = 0;
									bool set[3];
									API::Helper::Color::RGBA colors[3];
									for (size_t b = 0; b < 3; b++)
									{
										set[b] = read->ReadBits(1) == 1;
										if (set[b] == true)
										{
											count++;
											for (size_t c = 0; c < 4; c++)
											{
												colors[b][c] = static_cast<float>(read->ReadDouble(32));
											}
										}
									}
									if (count == 1)
									{
										auto* e = GetEntry<EntryType::COLOR_1>(m_entries, m_entryCount, a);
										for (size_t b = 0; b < 3; b++)
										{
											if (set[b] == true)
											{
												e->Color = colors[b];
												e->ColorID[0] = b;
												e->ColorID[1] = 3;
												e->ColorID[2] = 3;
												e->Default[0] = 3;
												e->Default[1] = 3;
											}
										}
									}
									else if (count == 2)
									{
										auto* e = GetEntry<EntryType::COLOR_2>(m_entries, m_entryCount, a);
										for (size_t b = 0, c = 0; b < 3; b++)
										{
											if (set[b] == true)
											{
												e->Color[c] = colors[b];
												e->ColorID[c] = b;
												c++;
											}
										}
										e->ColorID[2] = 3;
										e->Default = 3;
									}
									else
									{
										auto* e = GetEntry<EntryType::COLOR_3>(m_entries, m_entryCount, a);
										for (size_t b = 0; b < 3; b++)
										{
											e->Color[b] = colors[b];
										}
									}
								}
								else
								{
									API::Helper::Color::RGBA Colors[3];
									uint8_t Defaults[3];
									size_t IDColors[3];
									size_t IDDefaults[3];
									size_t numColors = 0;
									size_t numDefault = 0;
									for (size_t b = 0; b < 3; b++)
									{
										if (read->ReadBits(1) == 1)
										{
											if (read->ReadBits(1) == 1)
											{
												for (size_t c = 0; c < 4; c++) { Colors[numColors][c] = static_cast<float>(read->ReadDouble(32)); }
												IDColors[numColors] = b;
												numColors++;
											}
											else
											{
												Defaults[numDefault] = static_cast<uint8_t>(read->ReadBits(2));
												IDDefaults[numDefault] = b;
												numDefault++;
											}
										}
									}

									CLOAK_ASSUME(numDefault + numColors <= 3 && numDefault + numColors > 0);
									if (numColors == 0)
									{
										auto* e = GetEntry<EntryType::COLOR_DEFAULT>(m_entries, m_entryCount, a);
										for (size_t b = 0; b < numDefault; b++)
										{
											e->Default[b] = Defaults[IDDefaults[b]];
										}
										for (size_t b = numDefault; b < 3; b++)
										{
											e->Default[b] = 3;
										}
									}
									else if (numColors == 1)
									{
										auto* e = GetEntry<EntryType::COLOR_1>(m_entries, m_entryCount, a);
										CLOAK_ASSUME(numDefault <= 2);
										e->Color = Colors[0];
										e->ColorID[0] = IDColors[0];
										for (size_t b = 0; b < numDefault; b++)
										{
											e->Default[b] = Defaults[b];
											e->ColorID[b + numColors] = IDDefaults[b];
										}
										for (size_t b = numDefault; b < 2; b++)
										{
											e->Default[b] = 3;
											e->ColorID[b + numColors] = 3;
										}
									}
									else if (numColors == 2)
									{
										auto* e = GetEntry<EntryType::COLOR_2>(m_entries, m_entryCount, a);
										CLOAK_ASSUME(numDefault <= 1);
										for (size_t b = 0; b < numColors; b++)
										{
											e->Color[b] = Colors[b];
											e->ColorID[b] = IDColors[b];
										}
										if (numDefault > 0)
										{
											e->Default = Defaults[0];
											e->ColorID[2] = IDDefaults[0];
										}
										else
										{
											e->Default = 3;
											e->ColorID[2] = 3;
										}
									}
									else if (numColors == 3)
									{
										auto* e = GetEntry<EntryType::COLOR_3>(m_entries, m_entryCount, a);
										for (size_t b = 0; b < numColors; b++)
										{
											e->Color[b] = Colors[IDColors[b]];
										}
									}
								}
								break;
							}
							case 2://COLOR_DEFAULT
							{
								if (version <= 1002)
								{
									auto* e = GetEntry<EntryType::COLOR_DEFAULT>(m_entries, m_entryCount, a);
									for (uint8_t b = 0; b < 3; b++)
									{
										e->Default[b] = (read->ReadBits(1) == 1 ? b : 3);
									}
								}
								break;
							}
							case 3://LINE_BREAK
							{
								auto* e = GetEntry<EntryType::LINEBREAK>(m_entries, m_entryCount, a);
								break;
							}
							case 4://LINK
							{
								auto* e = GetEntry<EntryType::LINK_PROXY>(m_entries, m_entryCount, a);
								e->LinkID = static_cast<size_t>(read->ReadDynamic());
								break;
							}
							case 5://STRING
							{
								auto* e = GetEntry<EntryType::STRING>(m_entries, m_entryCount, a);
								const size_t cc = static_cast<size_t>(read->ReadDynamic());
								e->Text = API::Helper::StringConvert::ConvertToU8(tree.ReadString(read, cc));
								break;
							}
							case 6://VALUE
							{
								auto* e = GetEntry<EntryType::VALUE>(m_entries, m_entryCount, a);
								e->ID = static_cast<size_t>(read->ReadDynamic());
								break;
							}
							case 0:
							default:
							{
								auto* e = GetEntry<EntryType::NONE>(m_entries, m_entryCount, a);
								break;
							}
						}
					}
				}
				void CLOAK_CALL_THIS String::SetDefaultName(In size_t name)
				{
					if (m_entryCount != 1)
					{
						FreeEntries(m_entries, m_entryCount);
						m_entryCount = 1;
						m_entries = AllocateEntries(m_entryCount);
					}
					auto* e = GetEntry<EntryType::STRING>(m_entries, m_entryCount, 0);
					e->Text = "Unnamed String " + std::to_string(name);
				}
				void CLOAK_CALL_THIS String::InitializeLinks(In Language* parent)
				{
					for (size_t a = 0; a < m_entryCount; a++)
					{
						Entry* entry = GetUnknownEntry(m_entries, m_entryCount, a);
						if (entry->Type == EntryType::LINK_PROXY)
						{
							auto* oe = reinterpret_cast<GetEntryType<EntryType::LINK_PROXY>::type*>(entry);
							IString* target = parent->GetRealString(oe->LinkID);
							if (target == this || target == nullptr)
							{
								auto* ne = GetEntry<EntryType::NONE>(m_entries, m_entryCount, a);
							}
							else
							{
								auto* ne = GetEntry<EntryType::LINK>(m_entries, m_entryCount, a);
								ne->Link = target;
								target->AddRef();
							}
							SAVE_RELEASE(target);
						}
					}
				}
					
				Success(return == true) bool CLOAK_CALL_THIS String::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(Impl::Files::Language_v2::IString)) { *ptr = (Impl::Files::Language_v2::IString*)this; return true; }
					else if (riid == __uuidof(Impl::Files::Language_v2::String)) { *ptr = (Impl::Files::Language_v2::String*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}

				CLOAK_CALL Language::Language()
				{
					DEBUG_NAME(Language);
					m_strings = nullptr;
					m_stringCount = 0;
					m_curLang = 0;
					m_updateCount = 0;
				}
				CLOAK_CALL Language::~Language()
				{
					for (size_t a = 0; a < m_stringCount; a++) { SAVE_RELEASE(m_strings[a]); }
					API::Global::Memory::MemoryHeap::Free(m_strings);
				}
				void CLOAK_CALL_THIS Language::SetLanguage(In API::Files::Language_v2::LanguageID language)
				{
					API::Helper::WriteLock lock(m_sync);
					if (m_curLang != language)
					{
						m_curLang = language;
						if (isLoaded()) { iRequestReload(API::Files::Priority::NORMAL); }
					}
				}
				API::Files::Language_v2::IString* CLOAK_CALL_THIS Language::GetString(In size_t pos) { return new StringProxy(this, pos); }

				String* CLOAK_CALL_THIS Language::GetRealString(In size_t pos) const
				{
					API::Helper::ReadLock lock(m_sync);
					if (isLoaded() && pos < m_stringCount) 
					{
						String* s = m_strings[pos];
						s->AddRef();
						return s;
					}
					return nullptr;
				}

				Success(return == true) bool CLOAK_CALL_THIS Language::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, FileHandler_v1::RessourceHandler, Files::Language_v2, Language);
				}
				LoadResult CLOAK_CALL_THIS Language::iLoad(In API::Files::IReader* read, In API::Files::FileVersion version)
				{
					if (version >= 1002)
					{
						API::Helper::WriteLock lock(m_sync);
						m_updateCount++;
						const size_t langCount = static_cast<size_t>(read->ReadDynamic());
						m_stringCount = static_cast<size_t>(read->ReadDynamic());
						m_strings = reinterpret_cast<String**>(API::Global::Memory::MemoryHeap::Allocate(sizeof(String*)*m_stringCount));
						Tree defTree(read);
						for (size_t a = 0; a < m_stringCount; a++)
						{
							m_strings[a] = new String(m_updateCount);
							if (read->ReadBits(1) == 1)
							{
								m_strings[a]->ReadData(read, version, defTree);
							}
							else
							{
								m_strings[a]->SetDefaultName(a);
							}
						}
						for (size_t a = 0; a < langCount; a++)
						{
							const API::Files::Language_v2::LanguageID lang = static_cast<API::Files::Language_v2::LanguageID>(read->ReadBits(10));
							const unsigned long long byp = static_cast<unsigned long long>(read->ReadDynamic());
							const uint8_t bip = static_cast<uint8_t>(read->ReadBits(3));
							if (lang == m_curLang)
							{
								Tree langTree(read);
								for (size_t b = 0; b < m_stringCount; b++)
								{
									if (read->ReadBits(1) == 1) { m_strings[b]->ReadData(read, version, langTree); }
								}
								break;
							}
							else 
							{
								read->Move(byp, bip); 
							}
						}
						for (size_t a = 0; a < m_stringCount; a++)
						{
							m_strings[a]->InitializeLinks(this);
						}
					}
					return LoadResult::Finished;
				}
				void CLOAK_CALL_THIS Language::iUnload()
				{
					API::Helper::WriteLock lock(m_sync);
					for (size_t a = 0; a < m_stringCount; a++) { SAVE_RELEASE(m_strings[a]); }
					API::Global::Memory::MemoryHeap::Free(m_strings);
					m_strings = nullptr;
					m_stringCount = 0;
				}
				IMPLEMENT_FILE_HANDLER(Language);
			}
		}
	}
}