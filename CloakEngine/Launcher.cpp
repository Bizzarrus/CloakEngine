#include "stdafx.h"
#include "Engine/Launcher.h"

#include "Implementation/Launcher/Launcher.h"

#include <atomic>

namespace CloakEngine {
	namespace Engine {
		namespace Launcher {
			constexpr float SLEEP_TIME = 1000000.0f / 30.0f;

			std::atomic<bool> g_init = false;
			std::atomic<bool> g_result = false;
			Impl::Launcher::Launcher* g_manager = nullptr;

			LRESULT CALLBACK LauncherCallback(In HWND hWnd, In UINT msg, In WPARAM wParam, In LPARAM lParam)
			{
				switch (msg)
				{
					case WM_DESTROY:
						if (g_manager != nullptr) { g_manager->SendResultAndClose(false, false); }
						break;
					default:
						break;
				}
				return DefWindowProc(hWnd, msg, wParam, lParam);
			}
			void CLOAK_CALL SetLauncherResult(In bool res) { g_result = res; }
			bool CLOAK_CALL StartLauncher(In API::Global::ILauncherEvent* events, Inout Impl::Global::Game::ThreadSleepState* tsi)
			{
				if (events == nullptr) 
				{ 
					g_init = true;
					g_result = true;
				}
				if (g_init.exchange(true) == false)
				{
					g_manager = new Impl::Launcher::Launcher();
					events->OnOpenLauncher(g_manager);
					g_manager->Init();
					
					while (g_manager->IsOpen() && Impl::Global::Game::canThreadRun())
					{
						const unsigned long long ctm = Impl::Global::Game::getCurrentTimeMicroSeconds();
						const unsigned long long ct = ctm / 1000;
						MSG msg;
						while (PeekMessage(&msg, g_manager->GetWindow(), 0, 0, PM_REMOVE) == TRUE)
						{
							TranslateMessage(&msg);
							DispatchMessage(&msg);
						}
						Sleep(static_cast<DWORD>(tsi->GetSleepTime(ctm, SLEEP_TIME) / 1000));
					}
					SAVE_RELEASE(g_manager);
				}
				return g_result;
			}
		}
	}
}