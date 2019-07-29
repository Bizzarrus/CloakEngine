#include "stdafx.h"
#include "Implementation/Launcher/Launcher.h"
#include "Engine/Launcher.h"

#include "CloakEngine/Helper/StringConvert.h"

#define WINDOW_CLASS_NAME TEXT("CloakEngineLauncher")

/*
Notice: New launcher draws gui items using GDI (HDC handle)
*/

namespace CloakEngine {
	namespace Impl {
		namespace Launcher {
			inline namespace Launcher_v1 {
				CLOAK_CALL Launcher::Launcher()
				{
					m_isOpen = true;
					m_isInit = false;
					m_width = 0;
					m_height = 0;
					m_wind = (HWND)INVALID_HANDLE_VALUE;
					m_name = L"";
				}
				CLOAK_CALL Launcher::~Launcher()
				{
					SendResultAndClose(false);
				}

				void CLOAK_CALL_THIS Launcher::Init()
				{
					if (m_isOpen && m_isInit.exchange(true) == false)
					{
						HINSTANCE hInst = GetModuleHandle(nullptr);
						WNDCLASSEX wc;
						ZeroMemory(&wc, sizeof(WNDCLASSEX));
						wc.cbSize = sizeof(WNDCLASSEX);
						wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
						wc.lpfnWndProc = Engine::Launcher::LauncherCallback;
						wc.cbClsExtra = 0;
						wc.cbWndExtra = 0;
						wc.hInstance = hInst;
						wc.hCursor = nullptr;
						wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
						wc.lpszClassName = WINDOW_CLASS_NAME;
						wc.lpszMenuName = nullptr;
						wc.hIcon = nullptr;
						wc.hIconSm = nullptr;
						RegisterClassEx(&wc);

						const int screenW = GetSystemMetrics(SM_CXSCREEN);
						const int screenH = GetSystemMetrics(SM_CYSCREEN);
						const unsigned int W = static_cast<unsigned int>(m_width*screenW);
						const unsigned int H = static_cast<unsigned int>(m_height*screenH);
						const int X = (screenW - W) / 2;
						const int Y = (screenH - H) / 2;

						m_wind = CreateWindowEx(WS_EX_LAYERED | WS_EX_APPWINDOW, WINDOW_CLASS_NAME, m_name.c_str(), WS_POPUP | WS_MINIMIZEBOX, X, Y, W, H, nullptr, nullptr, nullptr, nullptr);
						ShowWindow(m_wind, SW_SHOWNORMAL);
					}
				}
				void CLOAK_CALL_THIS Launcher::SendResultAndClose(In bool success, In_opt bool destroy)
				{
					if (m_isOpen.exchange(false) == true)
					{
						Engine::Launcher::SetLauncherResult(success);
						if (destroy) { DestroyWindow(m_wind); }
					}
				}
				bool CLOAK_CALL_THIS Launcher::IsOpen() const { return m_isOpen; }
				HWND CLOAK_CALL_THIS Launcher::GetWindow() const { return m_wind; }

				void CLOAK_CALL_THIS Launcher::SetWidth(In float w) { m_width = max(0, w); }
				void CLOAK_CALL_THIS Launcher::SetHeight(In float h) { m_height = max(0, h); }
				void CLOAK_CALL_THIS Launcher::SetName(In const std::u16string& name) { m_name = API::Helper::StringConvert::ConvertToW(name); }
			}
		}
	}
}
