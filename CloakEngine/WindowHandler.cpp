#include "stdafx.h"
#include "Engine/WindowHandler.h"
#include "Engine/Graphic/Core.h"
#include "Engine/Graphic/GraphicLock.h"
#include "Engine/Input/Keyboard.h"

#include "Implementation/Global/Game.h"
#include "Implementation/Global/Input.h"
#include "Implementation/Global/Mouse.h"
#include "Implementation/Global/Audio.h"
#include "Implementation/Global/Video.h"
#include "Implementation/Global/Debug.h"
#include "Implementation/Global/Connection.h"
#include "Implementation/Global/Graphic.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Helper/StringConvert.h"

#include <atomic>

#define WINDOW_CLASS_NAME TEXT("CloakEngineWindow")

namespace CloakEngine {
	namespace Engine {
		namespace WindowHandler {
			constexpr DWORD HWND_STYLE_DEFAULT = (WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX)) | WS_CLIPCHILDREN | WS_VISIBLE;
			constexpr DWORD HWND_STYLE_BORDERLESS = WS_POPUP | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_VISIBLE;

			bool g_externWindow = false;
			std::atomic<bool> g_updateName = false;
			std::atomic<bool> g_initWindow = false;
			std::atomic<bool> g_everVisible = false;
			std::atomic<bool> g_isVisible = false;
			std::atomic<bool> g_lastVisible = false;
			std::atomic<bool> g_isFocused = false;
			std::atomic<bool> g_isMinimized = false;
			HWND g_wind = static_cast<HWND>(INVALID_HANDLE_VALUE);
			RECT g_screen;
			API::Global::Graphic::WindowMode g_curMode;
			API::Helper::ISyncSection* g_sync = nullptr;
			API::Helper::ISyncSection* g_syncVisible = nullptr;
			API::Helper::ISyncSection* g_syncName = nullptr;
			WNDPROC g_extWndProc;
			std::tstring* g_windowName = nullptr;

			
			inline void CLOAK_CALL setWindowLong(In int index, In LONG_PTR val)
			{
				SetWindowLongPtr(g_wind, index, val);
				SetWindowPos(g_wind, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}
			inline bool CLOAK_CALL checkFirstVisible()
			{
				API::Helper::Lock lock(g_syncVisible);
				if (g_isMinimized == true || g_externWindow == true) { return false; }
				if (g_everVisible.exchange(true) == false)
				{
					g_isVisible = true;
					ShowWindow(g_wind, SW_SHOWNORMAL);
					API::Helper::Lock lock(g_syncName);
					if (g_updateName.exchange(false) == true) { SetWindowText(g_wind, g_windowName->c_str()); }
					CloakDebugLog("Show Window first time");
					return false;
				}
				return true;
			}
			inline void CLOAK_CALL minimizeWindow()
			{
				if (g_externWindow == false && g_isMinimized.exchange(true) == false)
				{
					if (g_everVisible) 
					{ 
						CloakDebugLog("Minimize Window!");
						ShowWindow(g_wind, SW_MINIMIZE); 
					}
				}
			}
			inline void CLOAK_CALL restoreWindow()
			{
				if (g_externWindow == false && g_isMinimized.exchange(false) == true)
				{
					if (checkFirstVisible() == true) 
					{
						CloakDebugLog("Restore Window!");
						ShowWindow(g_wind, SW_RESTORE); 
					}
				}
			}
			inline void CLOAK_CALL resize(In int W, In int H, In_opt bool center = false, In_opt int xOffset = 0, In_opt int yOffset = 0)
			{
				if (g_externWindow == false)
				{
					UINT flags = SWP_NOOWNERZORDER | SWP_NOZORDER;
					int X = 0;
					int Y = 0;
					const DWORD dst = GetWindowLong(g_wind, GWL_STYLE);
					const DWORD est = GetWindowLong(g_wind, GWL_EXSTYLE);
					RECT r;
					r.left = X;
					r.right = X + W;
					r.top = Y;
					r.bottom = Y + H;
					AdjustWindowRectEx(&r, dst, FALSE, est);
					W = r.right - r.left;
					H = r.bottom - r.top;
					if (center == true)
					{
						X = xOffset + ((g_screen.left + g_screen.right - W) >> 1);
						Y = yOffset + ((g_screen.top + g_screen.bottom - H) >> 1);
					}
					else { flags |= SWP_NOMOVE; }
					SetWindowPos(g_wind, HWND_TOP, X, Y, W, H, flags);
				}
			}
			inline void CLOAK_CALL gainFocus()
			{
				if (g_isFocused.exchange(true) == false)
				{
					CloakDebugLog("Gain Focus!");
					API::Global::Graphic::Settings gset;
					Impl::Global::Graphic::GetModifedSettings(&gset);
					restoreWindow();
					if (gset.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN)
					{
						Graphic::Lock::UpdateSettings(gset); //enables fullscreen
					}
					Impl::Global::Audio::getFocus();
					Impl::Global::Game::gainFocus();
					//TODO
				}
			}
			inline void CLOAK_CALL looseFocus()
			{
				if (g_isFocused.exchange(false) == true)
				{
					CloakDebugLog("Loose Focus!");
					API::Global::Graphic::Settings gset;
					Impl::Global::Graphic::GetModifedSettings(&gset);
					//TODO
					Impl::Global::Mouse::LooseFocus();
					Impl::Global::Input::LooseFocus();
					Impl::Global::Audio::looseFocus();
					Impl::Global::Game::looseFocus();
					switch (gset.WindowMode)
					{
						case API::Global::Graphic::WindowMode::FULLSCREEN:
						case API::Global::Graphic::WindowMode::FULLSCREEN_WINDOW:
							minimizeWindow();
							break;
						default:
							break;
					}
				}
			}
			

			LRESULT CALLBACK WindowMessage(In HWND hWnd, In UINT msg, In WPARAM wParam, In LPARAM lParam)
			{
				switch (msg)
				{
					case WM_CLOSE:
						if (API::Global::Game::IsRunning())
						{
							Impl::Global::Input::OnWindowClose();
							API::Global::Game::Stop();
							return 0;
						}
						break;
					case WM_DESTROY:
						API::Global::Game::Stop();
						return 0;

					case WM_SIZE:
					{
						if (wParam != SIZE_MINIMIZED && g_isFocused == true)
						{
							const UINT W = static_cast<UINT>(LOWORD(lParam));
							const UINT H = static_cast<UINT>(HIWORD(lParam));
							CloakDebugLog("Got Resize Message: W = " + std::to_string(W) + " H = " + std::to_string(H));
#ifdef _DEBUG
							if (g_curMode == API::Global::Graphic::WindowMode::WINDOW)
							{
								API::Global::Graphic::Settings gset;
								Impl::Global::Graphic::GetModifedSettings(&gset);
								if (W != gset.Resolution.Width || H != gset.Resolution.Height) { CloakDebugLog("Got wrong window size!"); }
							}
#endif
							Graphic::Lock::RegisterResizeBuffers(W, H);
						}
						else { looseFocus(); }
						Impl::Global::Mouse::CheckClipping();
						break;
					}
					//Focus control:
					case WM_KILLFOCUS:
						looseFocus();
						break;
					case WM_SETFOCUS:
						gainFocus();
						break;
					case WM_ACTIVATEAPP:
						if (wParam == TRUE) { gainFocus(); }
						else { looseFocus(); }
						break;
					case WM_MOVE:
						Impl::Global::Mouse::CheckClipping();
						break;

						//Logging for double clicks:
					case WM_LBUTTONDBLCLK:
						Engine::Input::Keyboard::DoubleClickEvent(API::Global::Input::Keys::Mouse::LEFT);
						return TRUE;
					case WM_RBUTTONDBLCLK:
						Engine::Input::Keyboard::DoubleClickEvent(API::Global::Input::Keys::Mouse::RIGHT);
						return TRUE;
					case WM_MBUTTONDBLCLK:
						Engine::Input::Keyboard::DoubleClickEvent(API::Global::Input::Keys::Mouse::MIDDLE);
						return TRUE;
					case WM_XBUTTONDBLCLK:
					{
						const WORD btn = GET_XBUTTON_WPARAM(wParam);
						if (btn == XBUTTON1) { Engine::Input::Keyboard::DoubleClickEvent(API::Global::Input::Keys::Mouse::X1); }
						else if (btn == XBUTTON2) { Engine::Input::Keyboard::DoubleClickEvent(API::Global::Input::Keys::Mouse::X2); }
						return TRUE;
					}

					//Disabling special system keys:
					case WM_SYSKEYDOWN:
					case WM_SYSKEYUP:
						if (wParam == VK_MENU || wParam == VK_F10) { return 0; }
						break;

					//Check key input:
					case WM_INPUT:
						Engine::Input::Keyboard::InputEvent(wParam, lParam);
						Impl::Global::Mouse::UpdateCursor();

						/*
						TODO:
						- Do nothing on WM_INPUT but calling return 0;
						- Use GetRawInputBuffer in Input::Keyboard::Update to query all recent RawInput events
						- Call DefRawInputProc with the buffer from GetRawInputBuffer after processing it, to dequeue processed RawInput events

						This way, we don't need a secondary input buffer in Input::Keyboard, and input query frequency can be independent of window message loop frequency,
						which allows to process input in the pipelined buildup while letting the window message loop run in a parallel, unimportant thread.
						NOTICE: It is not clear in the documentation, wether the RawInputBuffer is bound to the window thread. This requires testing!!
						*/

						break;
					case WM_CHAR:
					{
						API::Interface::IBasicGUI* g = API::Interface::getFocused();
						if (g != nullptr && g->IsClickable())
						{
							API::Global::Events::onEnterChar c;
							c.c = static_cast<char16_t>(wParam);
							g->triggerEvent(c, false);
						}
						SAVE_RELEASE(g);
						return 0;
					}

					//Special Event handling:
					case WM_SETCURSOR:
						Impl::Global::Mouse::UpdateCursor();
						break;
					case WM_VIDEO_EVENT:
						Impl::Global::Video::OnVideoEvent();
						break;
					case WM_DISPLAYCHANGE:
						//TODO
						break;
					case WM_SOCKET:
						Impl::Global::Lobby::OnMessage(wParam, lParam);
						break;
					default:
						break;
				}
				if (g_externWindow == true) { return g_extWndProc(hWnd, msg, wParam, lParam); }
				return DefWindowProc(hWnd, msg, wParam, lParam);
			}

			void CLOAK_CALL setExternWindow(In HWND wnd)
			{
				if (wnd != INVALID_HANDLE_VALUE)
				{
					API::Helper::Lock lock(g_sync);
					if (g_initWindow == false && g_externWindow == false)
					{
						g_externWindow = true;
						g_wind = wnd;
					}
				}
			}
			void CLOAK_CALL onInit()
			{
				CREATE_INTERFACE(CE_QUERY_ARGS(&g_sync));
				CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncVisible));
				CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncName));
				g_windowName = new std::tstring();
			}
			void CLOAK_CALL onLoad()
			{
				API::Helper::Lock lock(g_sync);
				API::Global::Graphic::Settings gset;
				Impl::Global::Graphic::GetModifedSettings(&gset);
				HMONITOR monitor = Graphic::Core::GetCommandListManager()->GetMonitor();
				MONITORINFO monInfo = {};
				if (monitor != static_cast<HMONITOR>(INVALID_HANDLE_VALUE) && GetMonitorInfo(monitor, &monInfo) == TRUE)
				{
					g_screen = monInfo.rcMonitor;
				}
				else
				{
					g_screen.left = 0;
					g_screen.top = 0;
					g_screen.right = GetSystemMetrics(SM_CXSCREEN);
					g_screen.bottom = GetSystemMetrics(SM_CYSCREEN);
				}
				if (g_externWindow)
				{
					CloakDebugLog("Load extern window");
					g_extWndProc = (WNDPROC)SetWindowLongPtr(g_wind, GWLP_WNDPROC, (LONG_PTR)WindowMessage);
				}
				else
				{
					CloakDebugLog("Load intern window");
					//Create window class:
					WNDCLASSEX wc;
					ZeroMemory(&wc, sizeof(WNDCLASSEX));
					wc.cbSize = sizeof(WNDCLASSEX);
					wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
					wc.lpfnWndProc = WindowMessage;
					wc.cbClsExtra = 0;
					wc.cbClsExtra = 0;
					wc.cbWndExtra = 0;
					wc.hInstance = GetModuleHandle(nullptr);
					wc.hCursor = nullptr;
					wc.hbrBackground = NULL;
					wc.lpszClassName = WINDOW_CLASS_NAME;
					wc.lpszMenuName = nullptr;
					wc.hIcon = nullptr;
					wc.hIconSm = nullptr;
					RegisterClassEx(&wc);

					g_curMode = gset.WindowMode;
					int W = gset.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN || gset.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN_WINDOW ? g_screen.right - g_screen.left : gset.Resolution.Width;
					int H = gset.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN || gset.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN_WINDOW ? g_screen.bottom - g_screen.top : gset.Resolution.Height;
					int X = gset.Resolution.XOffset + ((g_screen.left + g_screen.right - W) >> 1);
					int Y = gset.Resolution.YOffset + ((g_screen.top + g_screen.bottom - H) >> 1);
					DWORD style = HWND_STYLE_DEFAULT;
					if (gset.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN || gset.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN_WINDOW) { style = HWND_STYLE_BORDERLESS; }
					RECT p;
					p.left = X;
					p.right = X + W;
					p.top = Y;
					p.bottom = Y + H;
					AdjustWindowRectEx(&p, style, FALSE, WS_EX_APPWINDOW);
					W = p.right - p.left;
					H = p.bottom - p.top;
					g_wind = CreateWindowEx(WS_EX_APPWINDOW, WINDOW_CLASS_NAME, L"", style & ~WS_VISIBLE, X, Y, W, H, nullptr, nullptr, nullptr, nullptr);
					if (CloakCheckError(g_wind != INVALID_HANDLE_VALUE, API::Global::Debug::Error::WINDOW_CREATE, false)) { return; }
				}
				SetFocus(g_wind);
				g_initWindow = true;
			}
			bool CLOAK_CALL UpdateOnce()
			{
				if (g_externWindow == false)
				{
					MSG msg;
					if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == TRUE)
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
						return true;
					}
				}
				if (g_isVisible == true && g_lastVisible == false)
				{
					g_lastVisible = IsWindowVisible(g_wind);
				}
				return false;
			}
			bool CLOAK_CALL WaitForMessage(In API::Global::Time maxWait)
			{
				return MsgWaitForMultipleObjectsEx(0, nullptr, static_cast<DWORD>(maxWait), QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_TIMEOUT;
			}
			void CLOAK_CALL onStop()
			{
				looseFocus();
				if (g_externWindow == false && g_wind != static_cast<HWND>(INVALID_HANDLE_VALUE)) { DestroyWindow(g_wind); }
				SAVE_RELEASE(g_sync);
				SAVE_RELEASE(g_syncVisible);
				SAVE_RELEASE(g_syncName);
				delete g_windowName;
			}
			HWND CLOAK_CALL getWindow()
			{
				return g_wind;
			}
			void CLOAK_CALL showWindow()
			{
				if (g_initWindow && API::Global::Game::IsRunning() && g_isVisible.exchange(true) == false)
				{
					API::Helper::Lock lock(g_syncVisible);
					if (checkFirstVisible() == true) 
					{ 
						ShowWindow(g_wind, SW_SHOW);
						CloakDebugLog("Show Window");
					}
				}
			}
			void CLOAK_CALL hideWindow()
			{
				if (g_initWindow && API::Global::Game::IsRunning() && g_isVisible.exchange(false) == true)
				{
					API::Helper::Lock lock(g_syncVisible);
					g_lastVisible = false;
					ShowWindow(g_wind, SW_HIDE);
					CloakDebugLog("Hide Window");
				}
			}
			void CLOAK_CALL setWindowName(In const std::u16string& name)
			{
				API::Helper::Lock lock(g_syncName);
				if (g_windowName == nullptr) { return; }
				*g_windowName = API::Helper::StringConvert::ConvertToT(name);
				g_updateName = true;
				lock.lock(g_syncVisible);
				if (g_everVisible == true)
				{
					API::Helper::Lock nlock(g_syncName);
					SetWindowText(g_wind, g_windowName->c_str());
				}
			}
			bool CLOAK_CALL isFocused()
			{
				if (g_externWindow)
				{
					const HWND foc = GetForegroundWindow();
					const bool isp = IsChild(foc, g_wind) == TRUE;
					const bool lfc = g_isFocused;
					if (lfc == true && isp == false) { looseFocus(); }
					else if (lfc == false && isp == true) { gainFocus(); }
				}
				return g_isFocused && g_isVisible;
			}
			bool CLOAK_CALL isVisible()
			{
				return g_isVisible && g_lastVisible;
			}
			void CLOAK_CALL updateSettings(In const API::Global::Graphic::Settings& newSet, In const API::Global::Graphic::Settings& oldSet)
			{
				if (g_externWindow == false)
				{
					API::Helper::Lock lock(g_sync);
					gainFocus();
					const bool nfs = newSet.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN_WINDOW || newSet.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN;
					const bool lfs = g_curMode == API::Global::Graphic::WindowMode::FULLSCREEN_WINDOW || g_curMode == API::Global::Graphic::WindowMode::FULLSCREEN;
					if (nfs && !lfs) { setWindowLong(GWL_STYLE, HWND_STYLE_BORDERLESS); }
					else if (lfs && !nfs) { setWindowLong(GWL_STYLE, HWND_STYLE_DEFAULT); }
					if (nfs != lfs || newSet.Resolution.Width != oldSet.Resolution.Width || newSet.Resolution.Height != oldSet.Resolution.Height)
					{
						resize(newSet.Resolution.Width, newSet.Resolution.Height, true, newSet.Resolution.XOffset, newSet.Resolution.YOffset);
						CloakDebugLog("Resize Window to " + std::to_string(newSet.Resolution.Width) + " x " + std::to_string(newSet.Resolution.Height));
					}
					g_curMode = newSet.WindowMode;
					
					lock.unlock();
					checkFirstVisible();
				}
			}
			void CLOAK_CALL checkSettings(Inout API::Global::Graphic::Settings* gset)
			{
				if (g_initWindow == true)
				{
					if (gset->WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN_WINDOW)
					{
						gset->Resolution.Width = g_screen.right - g_screen.left;
						gset->Resolution.Height = g_screen.bottom - g_screen.top;
					}
				}
				else
				{
					if (gset->WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN_WINDOW)
					{
						gset->Resolution.Width = GetSystemMetrics(SM_CXSCREEN);
						gset->Resolution.Height = GetSystemMetrics(SM_CYSCREEN);
					}
				}
			}
		}
	}
}