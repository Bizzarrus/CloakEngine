#include "stdafx.h"
#include "CloakEngine/Global/Graphic.h"
#include "Implementation/Global/Graphic.h"
#include "Implementation/Global/Mouse.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Rendering/RenderPass.h"

#include "Engine/Graphic/Core.h"
#include "Engine/Graphic/GraphicLock.h"
#include "Engine/WindowHandler.h"

#include <sstream>

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Graphic {
				API::Helper::ISyncSection* g_syncSettings;
				API::Helper::ISyncSection* g_syncRes;
				API::List<API::Global::Graphic::Resolution>* g_res = nullptr;
				API::Global::Graphic::Settings g_set;
				void CLOAK_CALL Initialize() 
				{
					g_res = new API::List<API::Global::Graphic::Resolution>();
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncSettings));
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncRes));
					DISPLAY_DEVICE dev;
					dev.cb = sizeof(dev);
					DEVMODE screen;
					screen.dmSize = sizeof(screen);
					API::Global::Graphic::Resolution res;
					API::Helper::Lock lock(g_syncRes);
					std::stringstream logMsg;
					logMsg << "Supported Display Resolutions:";
					for (DWORD a = 0; EnumDisplayDevices(NULL, a, &dev, 0) != 0; a++)
					{
						for (DWORD b = 0; EnumDisplaySettings(dev.DeviceName, b, &screen) != 0; b++)
						{
							for (size_t c = 0; c < g_res->size(); c++)
							{
								const API::Global::Graphic::Resolution& r = g_res->at(c);
								if (r.Height == screen.dmPelsHeight && r.Width == screen.dmPelsWidth) 
								{ 
									goto load_resolution_found;
								}
							}
							res.Width = screen.dmPelsWidth;
							res.Height = screen.dmPelsHeight;
							g_res->push_back(res);
							logMsg << std::endl << "\t- " << screen.dmPelsWidth << " x " << screen.dmPelsHeight;

						load_resolution_found:;

						}
					}
					API::Global::Log::WriteToLog(logMsg.str());
				}
				void CLOAK_CALL Release() 
				{
					if (g_syncSettings != nullptr) { g_syncSettings->Release(); }
					if (g_syncRes != nullptr) { g_syncRes->Release(); }
					delete g_res;
					g_res = nullptr;
				}
				void CLOAK_CALL GetModifedSettings(Out API::Global::Graphic::Settings* gset)
				{
					if (gset != nullptr)
					{
						API::Helper::Lock lock(Impl::Global::Graphic::g_syncSettings);
						*gset = g_set;
					}
				}
			}
		}
	}
	namespace API {
		namespace Global {
			namespace Graphic {
				Settings g_set;

				CLOAKENGINE_API void CLOAK_CALL SetSettings(In const Settings& setting)
				{
					API::Helper::Lock lock(Impl::Global::Graphic::g_syncSettings);
					Settings lset = Impl::Global::Graphic::g_set;
					g_set = setting;
					Impl::Global::Graphic::g_set = setting;
					Engine::WindowHandler::checkSettings(&Impl::Global::Graphic::g_set);
					if (Game::HasWindow())
					{
						Engine::FileLoader::updateSetting(Impl::Global::Graphic::g_set);
						Engine::Graphic::Lock::UpdateSettings(lset);
						Impl::Global::Mouse::SetGraphicSettings(Impl::Global::Graphic::g_set);
					}
				}
				CLOAKENGINE_API void CLOAK_CALL GetSettings(Out Settings* setting)
				{
					if (setting != nullptr)
					{
						API::Helper::Lock lock(Impl::Global::Graphic::g_syncSettings);
						*setting = g_set;
					}
				}
				Success(return == true)
				CLOAKENGINE_API bool CLOAK_CALL EnumerateResolution(In size_t num, Out Resolution* res)
				{
					API::Helper::Lock lock(Impl::Global::Graphic::g_syncRes);
					if (Impl::Global::Graphic::g_res == nullptr) { return false; }
					if (num >= Impl::Global::Graphic::g_res->size()) { return false; }
					if (res != nullptr) { *res = Impl::Global::Graphic::g_res->at(num); }
					return true;
				}
				CLOAKENGINE_API void CLOAK_CALL ShowWindow() { Engine::WindowHandler::showWindow(); }
				CLOAKENGINE_API void CLOAK_CALL HideWindow() { Engine::WindowHandler::hideWindow(); }
				CLOAKENGINE_API Global::Debug::Error CLOAK_CALL GetManager(In REFIID riid, Outptr void** ppvObject)
				{
					Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
					HRESULT hRet = man->QueryInterface(riid, ppvObject);
					if (SUCCEEDED(hRet)) { return Global::Debug::Error::OK; }
					return Global::Debug::Error::NO_INTERFACE;
				}
				CLOAKENGINE_API long double CLOAK_CALL GetAspectRatio() 
				{
					API::Helper::Lock lock(Impl::Global::Graphic::g_syncSettings);
					const long double W = static_cast<long double>(g_set.Resolution.Width);
					const long double H = static_cast<long double>(g_set.Resolution.Height);
					return W / H;
				}
				CLOAKENGINE_API void CLOAK_CALL SetRenderPass(In Rendering::IRenderPass* pass)
				{
					if (CloakCheckOK(Impl::Global::Game::GetCurrentComLevel() > 1, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, true))
					{
						Engine::Graphic::Core::SetRenderingPass(pass);
					}
				}
			}
		}
	}
}