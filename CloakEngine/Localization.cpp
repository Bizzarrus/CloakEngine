#include "stdafx.h"
#include "CloakEngine/Global/Localization.h"
#include "Implementation/Global/Localization.h"
#include "Implementation/Global/Game.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Files/Language.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Helper/StringConvert.h"

#include "Engine/WindowHandler.h"

#include <unordered_map>
#include <sstream>
#include <atomic>

namespace CloakEngine {
	namespace API {
		namespace Global {
			namespace Localization {
				/*
				Notice: We use the Private Usage Zone of the Multilingual Plane (U+E000 to U+F8FF) for any special Character (like
				gamepad buttons, etc)
				*/
				API::List<Files::ILanguage*>* g_langs;
				Language g_curLan = 0;
				bool g_setLang = false;
				Helper::ISyncSection* g_sync = nullptr;
				Helper::ISyncSection* g_syncWinName = nullptr;
				LanguageFileID g_nextLFID = 0;
				std::atomic<unsigned int> g_langLoadCount = 0;
				API::Files::IString* g_windName = nullptr;

				inline std::u16string CLOAK_CALL getDefaultName(In Language lan, In LanguageFileID fID, In TextID id)
				{
					return API::Helper::StringConvert::ConvertToU16("<" + std::to_string(lan) + "|" + std::to_string(fID) + "|" + std::to_string(id) + ">");
				}
				void onLangLoad(In const API::Global::Events::onLoad& ev)
				{
					const unsigned int loadCount = --g_langLoadCount;
					if (loadCount == 0)
					{
						API::Helper::Lock lock(g_syncWinName);
						if (Impl::Global::Game::canThreadRun())
						{
							Engine::WindowHandler::setWindowName(g_windName->GetAsString());
							lock.unlock();
							//Impl::Global::Game::onGameLangUpdate();
						}
					}
				}

				CLOAKENGINE_API LanguageFileID CLOAK_CALL registerLanguageFile(In Files::ILanguage* file)
				{
					if (CloakDebugCheckOK(file != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						Helper::WriteLock lock(g_sync);
						for (size_t a = 0; a < g_langs->size(); a++)
						{
							if (g_langs->at(a) == file) { return static_cast<LanguageFileID>(a); }
						}
						file->AddRef();
						file->registerListener(onLangLoad);
						if (g_setLang)
						{
							file->SetLanguage(g_curLan);
							if (file->isLoaded() == false)
							{
								g_langLoadCount++;
								file->load(Files::Priority::HIGH);
							}
						}
						g_langs->push_back(file);
						return static_cast<LanguageFileID>(g_langs->size() - 1);
					}
					return 0;
				}
				CLOAKENGINE_API void CLOAK_CALL setLanguage(In Language lan)
				{
					API::Helper::WriteLock lock(g_sync);
					for (size_t a = 0; a < g_langs->size(); a++)
					{ 
						g_langs->at(a)->SetLanguage(lan);
						if (g_setLang == false) 
						{
							if (g_langs->at(a)->isLoaded() == false)
							{
								g_langLoadCount++;
								g_langs->at(a)->load(Files::Priority::HIGH);
							}
						}
					}
					g_curLan = lan;
					g_setLang = true;
					lock.unlock();
					lock.lock(g_syncWinName);
					Engine::WindowHandler::setWindowName(g_windName->GetAsString());
				}
				CLOAKENGINE_API void CLOAK_CALL setWindowText(In API::Files::IString* str)
				{
					API::Helper::Lock lock(g_syncWinName);
					SWAP(g_windName, str);
					if (g_windName != nullptr) { Engine::WindowHandler::setWindowName(g_windName->GetAsString()); }
					else { Engine::WindowHandler::setWindowName(u""); }
				}
				CLOAKENGINE_API API::Files::IString* CLOAK_CALL getText(In LanguageFileID fID, In TextID id)
				{
					Helper::ReadLock lock(g_sync);
					API::Files::IString* dn = nullptr;
					if (CloakDebugCheckOK(fID < g_langs->size(), API::Global::Debug::Error::ILLEGAL_ARGUMENT, false)) { return g_langs->at(fID)->GetString(id); }
					return dn;
				}
				CLOAKENGINE_API Language CLOAK_CALL getCurrentLanguage()
				{
					Helper::ReadLock lock(g_sync);
					return g_curLan;
				}
			}
		}
	}
	namespace Impl {
		namespace Global {
			namespace Localization {
				void CLOAK_CALL Initialize()
				{
					API::Global::Localization::g_langs = new API::List<API::Files::ILanguage*>();
					CREATE_INTERFACE(CE_QUERY_ARGS(&API::Global::Localization::g_sync));
					CREATE_INTERFACE(CE_QUERY_ARGS(&API::Global::Localization::g_syncWinName));
				}
				void CLOAK_CALL Release()
				{
					for (size_t a = 0; a < API::Global::Localization::g_langs->size(); a++)
					{
						SAVE_RELEASE(API::Global::Localization::g_langs->at(a));
					}
					SAVE_RELEASE(API::Global::Localization::g_sync);
					SAVE_RELEASE(API::Global::Localization::g_syncWinName);
					delete API::Global::Localization::g_langs;
					SAVE_RELEASE(API::Global::Localization::g_windName);
				}
				bool CLOAK_CALL GetState(In API::Global::Localization::LanguageFileID fID)
				{
					API::Helper::ReadLock lock(API::Global::Localization::g_sync);
					if (fID < API::Global::Localization::g_langs->size()) { return API::Global::Localization::g_langs->at(fID)->isLoaded(); }
					return false;
				}
			}
		}
	}
}