#pragma once
#ifndef CE_IMPL_FILES_LANGUAGE_H
#define CE_IMPL_FILES_LANGUAGE_H

#include "CloakEngine/Files/Language.h"
#include "Implementation/Files/FileHandler.h"

#include "CloakEngine/Files/Font.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace Language_v2 {
				struct Entry;
				class Language;

				struct LineSegment {
					API::Helper::Color::RGBA Color[3];
					size_t CharCount;
				};
				struct Line {
					size_t SegmentCount;
				};
				struct DrawLine {
					size_t CharCount;
					float Length;
				};

				CLOAK_INTERFACE_BASIC CLOAK_UUID("{B0C1A98E-E234-405E-83D1-C86AC53A4193}") IString : public virtual API::Helper::SavePtr_v1::ISavePtr {
					public:
						virtual void CLOAK_CALL_THIS FillDrawInfos(In const API::Files::Language_v2::TextDesc& desc, Inout API::List<LineSegment>* lineSegments, Inout API::List<Line>* lines, Inout API::List<std::pair<IString*, uint64_t>>* linked, Inout LineSegment* curLineSeg, Inout Line* curLine, Inout std::stringstream* text, In bool supressLineBreaks, In_opt Entry* proxyEntries = nullptr, In_opt size_t proxyCount = 0) const = 0;
						virtual uint64_t CLOAK_CALL_THIS GetUpdateCount() = 0;
						virtual std::u16string CLOAK_CALL_THIS ConvertToString(In_opt Entry* proxyEntries = nullptr, In_opt size_t proxyCount = 0) const = 0;
				};
				enum class EntryType { NONE, STRING, COLOR_1, COLOR_2, COLOR_3, COLOR_DEFAULT, LINK, LINK_PROXY, LINEBREAK, VALUE };

				struct Entry {
					EntryType Type;
					virtual CLOAK_CALL ~Entry() {}
				};
				struct EntryString : public Entry {
					std::string Text;
				};
				struct EntryColor1 : public Entry {
					API::Helper::Color::RGBA Color;
					uint8_t Default[2];
					size_t ColorID[3];
				};
				struct EntryColor2 : public Entry {
					API::Helper::Color::RGBA Color[2];
					uint8_t Default;
					size_t ColorID[3];
				};
				struct EntryColor3 : public Entry {
					API::Helper::Color::RGBA Color[3];
				};
				struct EntryColorDefault : public Entry {
					uint8_t Default[3];
				};
				struct EntryLink : public Entry {
					IString* Link;
					CLOAK_CALL ~EntryLink() { SAVE_RELEASE(Link); }
				};
				struct EntryLinkProxy : public Entry {
					size_t LinkID;
				};
				struct EntryValue : public Entry {
					size_t ID;
				};

				struct Node {
					enum { NODE_TYPE_LEAF, NODE_TYPE_NODE } Type;
					union {
						Node* Nodes[2];
						char16_t Char;
					};
				};
				class Tree {
					public:
						CLOAK_CALL Tree(In API::Files::IReader* read);
						CLOAK_CALL ~Tree();
						char16_t CLOAK_CALL_THIS Read(In API::Files::IReader* read) const;
						std::u16string CLOAK_CALL_THIS ReadString(In API::Files::IReader* read, In size_t len) const;
					private:
						Node* m_nodes;
						size_t m_nodeCount;
				};
				class CLOAK_UUID("{4BFE7937-0973-49DC-B808-4D305FB8F946}") StringProxy : public virtual API::Files::Language_v2::IString, public virtual IString, public virtual Impl::Helper::SavePtr_v1::SavePtr, IMPL_EVENT(onLoad), IMPL_EVENT(onUnload) {
					public:
						CLOAK_CALL StringProxy(In_opt Language* parent = nullptr, In_opt size_t stringID = 0);
						virtual CLOAK_CALL ~StringProxy();
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const std::u16string& text) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const std::string& text) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In float data) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In unsigned int data) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In int data) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In API::Files::Language_v2::IString* link) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In const API::Helper::Color::RGBA& color1) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t default1) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In const API::Helper::Color::RGBA& color1, In const API::Helper::Color::RGBA& color2) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In const API::Helper::Color::RGBA& color1, In_range(0, 2) uint8_t default2) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In_range(0, 2) uint8_t default1, In const API::Helper::Color::RGBA& color2) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In_range(0, 2) uint8_t default1, In_range(0, 2) uint8_t default2) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In const API::Helper::Color::RGBA& color2, In const API::Helper::Color::RGBA& color3) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In const API::Helper::Color::RGBA& color2, In_range(0, 2) uint8_t default3) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In_range(0, 2) uint8_t default2, In const API::Helper::Color::RGBA& color3) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In_range(0, 2) uint8_t default2, In_range(0, 2) uint8_t default3) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t default1, In const API::Helper::Color::RGBA& color2, In const API::Helper::Color::RGBA& color3) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t default1, In const API::Helper::Color::RGBA& color2, In_range(0, 2) uint8_t default3) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t default1, In_range(0, 2) uint8_t default2, In const API::Helper::Color::RGBA& color3) override;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t default1, In_range(0, 2) uint8_t default2, In_range(0, 2) uint8_t default3) override;
						virtual void CLOAK_CALL_THIS SetDefaultColor(In size_t entry) override;
						virtual void CLOAK_CALL_THIS SetLineBreak(In size_t entry) override;
						virtual void CLOAK_CALL_THIS Draw(In API::Rendering::IContext2D* context, In API::Rendering::IDrawable2D* drawing, In API::Files::Font_v2::IFont* font, In const API::Files::Language_v2::TextDesc& desc, In_opt UINT Z = 0, In_opt bool supressLineBreaks = false) override;
						virtual std::u16string CLOAK_CALL_THIS GetAsString() const override;

						virtual ULONG CLOAK_CALL_THIS load(In_opt API::Files::Priority prio = API::Files::Priority::LOW) override;
						virtual ULONG CLOAK_CALL_THIS unload() override;
						virtual bool CLOAK_CALL_THIS isLoaded(In API::Files::Priority prio) override;
						virtual bool CLOAK_CALL_THIS isLoaded() const override;

						virtual void CLOAK_CALL_THIS FillDrawInfos(In const API::Files::Language_v2::TextDesc& desc, Inout API::List<LineSegment>* lineSegments, Inout API::List<Line>* lines, Inout API::List<std::pair<Impl::Files::Language_v2::IString*, uint64_t>>* linked, Inout LineSegment* curLineSeg, Inout Line* curLine, Inout std::stringstream* text, In bool supressLineBreaks, In_opt Entry* proxyEntries = nullptr, In_opt size_t proxyCount = 0) const override;
						virtual uint64_t CLOAK_CALL_THIS GetUpdateCount() override;
						virtual std::u16string CLOAK_CALL_THIS ConvertToString(In_opt Entry* proxyEntries = nullptr, In_opt size_t proxyCount = 0) const override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS iResize(In size_t minSize);

						Language* const m_parent;
						const size_t m_stringID;
						API::Files::Font_v2::IFont* m_lastFont;
						API::List<API::Files::Font_v2::LetterInfo> m_chars;
						API::List<LineSegment> m_lineSegments;
						API::List<Line> m_lines;
						API::List<DrawLine> m_drawLines;
						API::List<std::pair<Impl::Files::Language_v2::IString*, uint64_t>> m_linked;
						API::Helper::ISyncSection* m_sync;
						uint64_t m_update;
						uint64_t m_lastUpdate;
						uint64_t m_lastParentUpdate;
						uint64_t m_updateCount;
						uint64_t m_lastUpdateCount;
						API::Files::Language_v2::FontSize m_lastSize;
						bool m_lastSupressLB;
						float m_lastWidth;
						float m_lastHeight;
						Entry* m_entries;
						size_t m_entryCount;
						size_t m_entryAlloc;
				};
				class CLOAK_UUID("{903DB66C-3091-4432-8249-B0D6BCDA788E}") String : public IString, public virtual Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL String(In uint64_t updateCount);
						virtual CLOAK_CALL ~String();
						virtual void CLOAK_CALL_THIS FillDrawInfos(In const API::Files::Language_v2::TextDesc& desc, Inout API::List<LineSegment>* lineSegments, Inout API::List<Line>* lines, Inout API::List<std::pair<Impl::Files::Language_v2::IString*, uint64_t>>* linked, Inout LineSegment* curLineSeg, Inout Line* curLine, Inout std::stringstream* text, In bool supressLineBreaks, In_opt Entry* proxyEntries = nullptr, In_opt size_t proxyCount = 0) const override;
						virtual uint64_t CLOAK_CALL_THIS GetUpdateCount() override;
						virtual std::u16string CLOAK_CALL_THIS ConvertToString(In_opt Entry* proxyEntries = nullptr, In_opt size_t proxyCount = 0) const override;

						virtual void CLOAK_CALL_THIS ReadData(In API::Files::IReader* read, In API::Files::FileVersion version, In const Tree& tree);
						virtual void CLOAK_CALL_THIS SetDefaultName(In size_t name);
						virtual void CLOAK_CALL_THIS InitializeLinks(In Language* parent);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						Entry* m_entries;
						size_t m_entryCount;
						const uint64_t m_updateCount;
				};
				class CLOAK_UUID("{9892FCCB-B9B9-4FED-AC2E-28FF71565F88}") Language : public API::Files::Language_v2::ILanguage, public virtual Impl::Files::FileHandler_v1::RessourceHandler {
					public:
						CLOAK_CALL Language();
						virtual CLOAK_CALL ~Language();
						virtual void CLOAK_CALL_THIS SetLanguage(In API::Files::Language_v2::LanguageID language) override;
						virtual API::Files::Language_v2::IString* CLOAK_CALL_THIS GetString(In size_t pos) override;

						virtual String* CLOAK_CALL_THIS GetRealString(In size_t pos) const;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual LoadResult CLOAK_CALL_THIS iLoad(In API::Files::IReader* read, In API::Files::FileVersion version) override;
						virtual void CLOAK_CALL_THIS iUnload() override;

						uint64_t m_updateCount;
						API::Files::Language_v2::LanguageID m_curLang;
						String** m_strings;
						size_t m_stringCount;
				};
				DECLARE_FILE_HANDLER(Language, "{58FA6A3E-44E2-4438-8CDA-E7A4E8CD7CD3}");
			}
		}
	}
}

#pragma warning(pop)

#endif