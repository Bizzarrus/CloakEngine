#include "stdafx.h"
#include "Engine/Input/Keyboard.h"
#include "Engine/FileLoader.h"
#include "Engine/WindowHandler.h"

#include "Implementation/Global/Mouse.h"
#include "Implementation/Global/Input.h"

#include "CloakEngine/Global/Input.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Memory.h"

namespace CloakEngine {
	namespace Engine {
		namespace Input {
			namespace Keyboard {
				constexpr uint8_t VK_TO_CODE[] = {
					//		0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F 
					/*0x00*/0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x01,0xFF,0xFF,0x02,0x03,0xFF,0xFF,
					/*0x10*/0xFF,0xFF,0xFF,0x04,0x05,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x06,0xFF,0xFF,0xFF,0xFF,
					/*0x20*/0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
					/*0x30*/0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
					/*0x40*/0xFF,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
					/*0x50*/0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0xFF,
					/*0x60*/0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x3F,0x40,0x41,0x42,0x43,0x44,
					/*0x70*/0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,
					/*0x80*/0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
					/*0x90*/0x5D,0x5E,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
					/*0xA0*/0x5F,0x60,0x61,0x62,0x63,0x64,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
					/*0xB0*/0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x65,0x6E,0x6F,0x70,0x71,0x66,
					/*0xC0*/0x67,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
					/*0xD0*/0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x68,0x69,0x6A,0x6B,0x6C,
					/*0xE0*/0xFF,0xFF,0x6D,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
					/*0xF0*/0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x72,0xFF,
				};

				struct RawInputPage {
					RAWINPUT* Data = nullptr;
					size_t PageSize = 0;
				};

				API::Helper::ISyncSection* g_syncPage = nullptr;
				API::Queue<RawInputPage>* g_pagePool = nullptr;

				inline RawInputPage CLOAK_CALL GetPage(In size_t size)
				{
					API::Helper::Lock lock(g_syncPage);
					RawInputPage r;
					if (g_pagePool->empty() == false) { r = g_pagePool->front(); g_pagePool->pop(); }
					lock.unlock();
					if (r.PageSize < size) 
					{
						API::Global::Memory::MemoryHeap::Free(r.Data);
						r.Data = reinterpret_cast<RAWINPUT*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(RAWINPUT)*size));
						r.PageSize = size;
					}
					return r;
				}
				inline void CLOAK_CALL FreePage(In const RawInputPage& page)
				{
					API::Helper::Lock lock(g_syncPage);
					g_pagePool->push(page);
				}

				void CLOAK_CALL RawInputEvent(In const RAWINPUT& raw)
				{
					if (raw.header.dwType == RIM_TYPEKEYBOARD && Impl::Global::Mouse::IsInClip())
					{
						const RAWKEYBOARD& keyboard = raw.data.keyboard;
						UINT vk = keyboard.VKey;
						UINT sc = keyboard.MakeCode;
						UINT flags = keyboard.Flags;
						//read flags
						const bool e0 = (flags & RI_KEY_E0) != 0;
						const bool e1 = (flags & RI_KEY_E1) != 0;
						const bool down = (flags & RI_KEY_BREAK) == 0;
						//correct vk & sc:
						if (vk == 255) { return; }
						else if (vk == VK_SHIFT) { vk = MapVirtualKey(sc, MAPVK_VSC_TO_VK_EX); }
						//else if (vk == VK_NUMLOCK) { sc = (MapVirtualKey(vk, MAPVK_VK_TO_VSC)) | 0x100; }
						//if (e1)
						//{
						//	if (vk == VK_PAUSE) { sc = 0x45; }//MapVirtualKey doesn't like VK_PAUSE (known bug)
						//	else { sc = MapVirtualKey(vk, MAPVK_VK_TO_VSC); }
						//}
						//Translate key:
						switch (vk)
						{
							case VK_CONTROL:
								if (e0) { vk = VK_RCONTROL; }
								else { vk = VK_LCONTROL; }
								break;
							case VK_MENU:
								if (e0) { vk = VK_RMENU; }
								else { vk = VK_LMENU; }
								break;
							case VK_SHIFT:
								if (e0) { vk = VK_RSHIFT; }
								else { vk = VK_LSHIFT; }
								break;
							case VK_INSERT:
								if (!e0) { vk = VK_NUMPAD0; }
								break;
							case VK_DELETE:
								if (!e0) { vk = VK_DECIMAL; }
								break;
							case VK_HOME:
								if (!e0) { vk = VK_NUMPAD7; }
								break;
							case VK_END:
								if (!e0) { vk = VK_NUMPAD1; }
								break;
							case VK_PRIOR:
								if (!e0) { vk = VK_NUMPAD9; }
								break;
							case VK_NEXT:
								if (!e0) { vk = VK_NUMPAD3; }
								break;
							case VK_LEFT:
								if (!e0) { vk = VK_NUMPAD4; }
								break;
							case VK_RIGHT:
								if (!e0) { vk = VK_NUMPAD6; }
								break;
							case VK_UP:
								if (!e0) { vk = VK_NUMPAD8; }
								break;
							case VK_DOWN:
								if (!e0) { vk = VK_NUMPAD2; }
								break;
							case VK_CLEAR:
								if (!e0) { vk = VK_NUMPAD5; }
								break;
						}
						CLOAK_ASSUME(vk < ARRAYSIZE(VK_TO_CODE));
						const uint8_t keyCode = VK_TO_CODE[vk];
						CLOAK_ASSUME(keyCode < 0x80);
						const API::Global::Input::KeyCode key = API::Global::Input::KeyCodeGeneration::GenerateKeyCode(API::Global::Input::KeyCodeGeneration::KeyCodeType::KEYBOARD, keyCode);
						Impl::Global::Input::OnKeyEvent(key, down, 0);
					}
					else if (raw.header.dwType == RIM_TYPEMOUSE)
					{
						const RAWMOUSE& mouse = raw.data.mouse;
						if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
						{
							Impl::Global::Mouse::SetPos(mouse.lLastX, mouse.lLastY);
						}
						else if (mouse.usFlags == MOUSE_MOVE_RELATIVE)
						{
							Impl::Global::Mouse::Move(mouse.lLastX, mouse.lLastY);
						}

						if (mouse.usButtonFlags != 0)
						{
							API::Global::Input::KeyCode down = API::Global::Input::Keys::NONE;
							API::Global::Input::KeyCode up = API::Global::Input::Keys::NONE;
							if ((mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) != 0) { down |= API::Global::Input::Keys::Mouse::LEFT; }
							if ((mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) != 0) { down |= API::Global::Input::Keys::Mouse::RIGHT; }
							if ((mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) != 0) { down |= API::Global::Input::Keys::Mouse::MIDDLE; }
							if ((mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) != 0) { down |= API::Global::Input::Keys::Mouse::X1; }
							if ((mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) != 0) { down |= API::Global::Input::Keys::Mouse::X2; }
							if ((mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP) != 0) { up |= API::Global::Input::Keys::Mouse::LEFT; }
							if ((mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP) != 0) { up |= API::Global::Input::Keys::Mouse::RIGHT; }
							if ((mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP) != 0) { up |= API::Global::Input::Keys::Mouse::MIDDLE; }
							if ((mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) != 0) { up |= API::Global::Input::Keys::Mouse::X1; }
							if ((mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) != 0) { up |= API::Global::Input::Keys::Mouse::X2; }

							if (down != API::Global::Input::Keys::NONE || up != API::Global::Input::Keys::NONE)
							{
								Impl::Global::Input::OnKeyEvent(down, up, 0);
							}

							if ((mouse.usButtonFlags & RI_MOUSE_WHEEL) != 0)
							{
								const float delta = static_cast<float>(static_cast<SHORT>(mouse.usButtonData)) / WHEEL_DELTA;
								Impl::Global::Input::OnMouseScrollEvent(delta, API::Global::Input::SCROLL_VERTICAL, 0);
							}
#ifdef RI_MOUSE_HWHEEL
							else if ((mouse.usButtonFlags & RI_MOUSE_HWHEEL) != 0)
							{
								const float delta = static_cast<float>(static_cast<SHORT>(mouse.usButtonData)) / WHEEL_DELTA;
								Impl::Global::Input::OnMouseScrollEvent(delta, API::Global::Input::SCROLL_HORIZONTAL, 0);
							}
#endif
						}
					}
				}

				void CLOAK_CALL Initialize()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncPage));
					g_pagePool = new API::Queue<RawInputPage>();
					const HWND wind = Engine::WindowHandler::getWindow();

					RAWINPUTDEVICE rid[2];
					//Keyboard
					rid[0].dwFlags = RIDEV_NOHOTKEYS;//| RIDEV_NOLEGACY
					rid[0].hwndTarget = wind;
					rid[0].usUsage = 0x06;
					rid[0].usUsagePage = 0x01;
					//mouse
					rid[1].dwFlags = 0;
					rid[1].hwndTarget = wind;
					rid[1].usUsage = 0x02;
					rid[1].usUsagePage = 0x01;

					BOOL hRet = RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE));
					CloakCheckOK(hRet == TRUE, API::Global::Debug::Error::INPUT_REGISTER_FAIL, true);
				}
				void CLOAK_CALL Release()
				{
					while (g_pagePool->empty() == false)
					{
						const RawInputPage& p = g_pagePool->front();
						API::Global::Memory::MemoryHeap::Free(p.Data);
						g_pagePool->pop();
					}
					delete g_pagePool;
					SAVE_RELEASE(g_syncPage);
				}
				void CLOAK_CALL InputEvent(In WPARAM wParam, In LPARAM lParam)
				{
					if (wParam == RIM_INPUT)
					{
						UINT size = 0;
						const HRAWINPUT hRaw = reinterpret_cast<HRAWINPUT>(lParam);
						GetRawInputData(hRaw, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
						if (size > 0)
						{
							UINT rSize = (size + sizeof(RAWINPUT) - 1) / sizeof(RAWINPUT);
							RawInputPage page = GetPage(rSize);
							CLOAK_ASSUME(page.PageSize >= rSize);
							const UINT resSize = GetRawInputData(hRaw, RID_INPUT, page.Data, &size, sizeof(RAWINPUTHEADER));
							CLOAK_ASSUME(resSize <= (rSize * sizeof(RAWINPUT)));
							LRESULT r = DefRawInputProc(&page.Data, rSize, sizeof(RAWINPUTHEADER));
							for (UINT a = 0; a < rSize; a++) { RawInputEvent(page.Data[a]); }
							FreePage(page);
						}
					}
				}
				void CLOAK_CALL DoubleClickEvent(In API::Global::Input::KeyCode key)
				{
					Impl::Global::Input::OnDoubleClick(key, 0);
				}
			}
		}
	}
}