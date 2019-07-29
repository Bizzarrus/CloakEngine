#include "stdafx.h"
#include "CloakEngine/Global/Input.h"
#include "Implementation/Global/Input.h"
#include "Implementation/Global/Mouse.h"
#include "Implementation/Global/Game.h"
#include "Implementation/Interface/RayTestContext.h"

#include "Engine/Input/Gamepad.h"
#include "Engine/Input/Keyboard.h"

#include <atomic>

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Input {
				constexpr API::Global::Time MAX_MOUSE_CLICK = 250;
				constexpr size_t MOUSE_STATE_COUNT = static_cast<size_t>(API::Global::Input::KeyCodeGeneration::KeyCodeLength::MOUSE);
				constexpr float DRAG_THRESHOLD = 2.5e-5f;//0.005 squared (about 10 px at 2000 px width)
				constexpr API::Global::Input::KeyCode DOUBLE_KEYS_LEFT = API::Global::Input::Keys::Keyboard::SHIFT_LEFT | API::Global::Input::Keys::Keyboard::CONTROL_LEFT | API::Global::Input::Keys::Keyboard::WIN_LEFT | API::Global::Input::Keys::Keyboard::ALT_LEFT;
				constexpr API::Global::Input::KeyCode DOUBLE_KEYS_RIGHT = API::Global::Input::Keys::Keyboard::SHIFT_RIGHT | API::Global::Input::Keys::Keyboard::CONTROL_RIGHT | API::Global::Input::Keys::Keyboard::WIN_RIGHT | API::Global::Input::Keys::Keyboard::ALT_RIGHT;
				constexpr API::Global::Input::KeyCode MOUSE_BEGIN = API::Global::Input::KeyCodeGeneration::GenerateKeyCode(API::Global::Input::KeyCodeGeneration::KeyCodeType::MOUSE, 0x00);
				constexpr uint32_t SCROLL_POINT_SIZE = 1024;
				static_assert((DOUBLE_KEYS_LEFT << 1) == DOUBLE_KEYS_RIGHT, "Double key masks are set the wrong way");

				struct UserData {
					API::Global::Input::KeyCode HoldKeys = API::Global::Input::Keys::NONE;
					API::Global::Input::KeyCode LastHoldKeys = API::Global::Input::Keys::NONE;
					API::Global::Math::Space2D::Point Mouse = API::Global::Math::Space2D::Point(0, 0);
					API::Global::Math::Space2D::Point LeftThumb = API::Global::Math::Space2D::Point(0, 0);
					API::Global::Math::Space2D::Point RightThumb = API::Global::Math::Space2D::Point(0, 0);
					API::Helper::ISyncSection* Sync = nullptr;
					API::Global::Time LastMouseUpdate = 0;
					API::Global::Time LastThumbUpdate = 0;
					API::Global::Input::MOUSE_STATE MouseState[MOUSE_STATE_COUNT];
					API::Interface::IBasicGUI* LastMouseClicked[MOUSE_STATE_COUNT];
					API::Interface::IBasicGUI* LastMouseOver = nullptr;
					API::Global::Time MousePressTime[MOUSE_STATE_COUNT];
					KeyBindingPage* CurrentPage = nullptr;
					std::atomic<uint32_t> ScrollDelta[2] = { 0, 0 };
				} g_Users[MAX_USER_COUNT];
				API::Global::IInputEvent* g_handler = nullptr;
				std::atomic<API::Interface::IBasicGUI*> g_mouseOver = nullptr;
				std::atomic<API::Global::Input::KEY_BROADCAST> g_broadcastMode = API::Global::Input::KEY_BROADCAST::CURRENT | API::Global::Input::KEY_BROADCAST::GLOBAL;
				API::Helper::ISyncSection* g_syncPages = nullptr;
				API::List<CE::RefPointer<KeyBindingPage>>* g_pages = nullptr;
				KeyBindingPage* g_defaultPage = nullptr;
				KeyBindingPage* g_globalPage = nullptr;

				CLOAK_CALL KeyBinding::KeyBinding(In size_t slotCount, In bool exact, In API::Global::Input::KeyBindingFunc&& onClick) : m_slotCount(slotCount), m_exact(exact), m_func(onClick), m_entered(false)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					m_keys = reinterpret_cast<API::Global::Input::KeyCode*>(API::Global::Memory::MemoryHeap::Allocate(m_slotCount * sizeof(API::Global::Input::KeyCode)));
					for (size_t a = 0; a < m_slotCount; a++) { m_keys[a] = API::Global::Input::Keys::NONE; }
				}
				CLOAK_CALL KeyBinding::KeyBinding(In const KeyBinding& k) : m_slotCount(k.m_slotCount), m_exact(k.m_exact), m_func(k.m_func), m_entered(false)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					m_keys = reinterpret_cast<API::Global::Input::KeyCode*>(API::Global::Memory::MemoryHeap::Allocate(m_slotCount * sizeof(API::Global::Input::KeyCode)));
					for (size_t a = 0; a < m_slotCount; a++) { m_keys[a] = k.m_keys[a]; }
				}
				CLOAK_CALL KeyBinding::KeyBinding(In KeyBinding&& k) : m_slotCount(k.m_slotCount), m_exact(k.m_exact), m_keys(k.m_keys), m_func(k.m_func), m_sync(k.m_sync), m_entered(false) {}
				CLOAK_CALL KeyBinding::~KeyBinding()
				{
					SAVE_RELEASE(m_sync);
					API::Global::Memory::MemoryHeap::Free(m_keys);
				}
				void CLOAK_CALL_THIS KeyBinding::SetKeys(In size_t slot, In const API::Global::Input::KeyCode& keys)
				{
					API::Helper::Lock lock(m_sync);
					CLOAK_ASSUME(slot < m_slotCount);
					m_keys[slot] = keys;
				}
				const API::Global::Input::KeyCode& CLOAK_CALL_THIS KeyBinding::GetKeys(In size_t slot) const
				{
					API::Helper::Lock lock(m_sync);
					CLOAK_ASSUME(slot < m_slotCount);
					return m_keys[slot];
				}

				void CLOAK_CALL_THIS KeyBinding::CheckKeys(In const API::Global::Input::KeyCode& nexthold, In DWORD user)
				{
					API::Helper::Lock lock(m_sync);
					bool next = false;
					if (nexthold != API::Global::Input::Keys::NONE)
					{
						for (size_t a = 0; a < m_slotCount && next == false; a++)
						{
							API::Global::Input::KeyCode k = nexthold;
							const API::Global::Input::KeyCode dk = k & m_keys[a];
							const API::Global::Input::KeyCode pk = ((dk & DOUBLE_KEYS_LEFT) << 1) | ((dk & DOUBLE_KEYS_RIGHT) >> 1);
							if (m_exact) { k |= pk & m_keys[a]; }
							else { k = (k | pk) & m_keys[a]; }
							next = k == m_keys[a];
						}
					}
					if (m_entered != next) { m_func(next, static_cast<API::Global::Input::User>(user)); }
					m_entered = next;
				}
				KeyBinding& CLOAK_CALL_THIS KeyBinding::operator=(In const KeyBinding& k)
				{
					if (&k != this)
					{
						API::Helper::Lock lock(m_sync);
						if (m_slotCount != k.m_slotCount)
						{
							m_slotCount = k.m_slotCount;
							API::Global::Memory::MemoryHeap::Free(m_keys);
							m_keys = reinterpret_cast<API::Global::Input::KeyCode*>(API::Global::Memory::MemoryHeap::Allocate(m_slotCount * sizeof(API::Global::Input::KeyCode)));
						}
						m_exact = k.m_exact;
						m_func = k.m_func;
						m_entered = k.m_entered;
						for (size_t a = 0; a < m_slotCount; a++) { m_keys[a] = k.m_keys[a]; }
					}
					return *this;
				}
				KeyBinding& CLOAK_CALL_THIS KeyBinding::operator=(In KeyBinding&& k)
				{
					if (&k != this)
					{
						delete m_keys;
						m_slotCount = k.m_slotCount;
						m_exact = k.m_exact;
						m_keys = k.m_keys;
						m_func = k.m_func;
						m_entered = k.m_entered;
						SWAP(m_sync, k.m_sync);
					}
					return *this;
				}
				void* CLOAK_CALL KeyBinding::operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
				void CLOAK_CALL KeyBinding::operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }

				CLOAK_CALL KeyBindingPage::KeyBindingPage(In_opt KeyBindingPage* parent, In_opt API::Global::Time timeout, In_opt API::Global::Input::KeyPageFunc func) : m_parent(parent), m_timeout(timeout), m_func(func)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
				}
				CLOAK_CALL KeyBindingPage::~KeyBindingPage()
				{
					SAVE_RELEASE(m_sync);
					for (const auto& a : m_keyBindings) { delete a; }
				}
				CE::RefPointer<API::Global::Input::IKeyBindingPage> CLOAK_CALL_THIS KeyBindingPage::CreateNewPage(In_opt API::Global::Time timeout, In_opt API::Global::Input::KeyPageFunc func)
				{
					KeyBindingPage* r = new KeyBindingPage(m_timeout > 0 ? m_parent : this, timeout, func);
					API::Helper::Lock lock(g_syncPages);
					g_pages->push_back(r);
					CE::RefPointer<API::Global::Input::IKeyBindingPage> res = r;
					r->Release();
					return res;
				}
				void CLOAK_CALL_THIS KeyBindingPage::Enter(In API::Global::Input::User user)
				{
					CLOAK_ASSUME(user < MAX_USER_COUNT);
					API::Helper::Lock lock(m_sync);
					m_enterTime[user] = m_timeout;
					lock.unlock();
					lock.lock(g_Users[user].Sync);
					if (g_Users[user].CurrentPage != this)
					{
						g_Users[user].CurrentPage->OnLeave(user);
						g_Users[user].CurrentPage = this;
						if (m_func) { m_func(true, user); }
					}
				}
				API::Global::Input::IKeyBinding* CLOAK_CALL_THIS KeyBindingPage::CreateKeyBinding(In size_t slotCount, In bool exact, In API::Global::Input::KeyBindingFunc onKeyCode)
				{
					API::Helper::Lock lock(m_sync);
					CLOAK_ASSUME(slotCount > 0);
					KeyBinding* r = new KeyBinding(slotCount, exact, std::move(onKeyCode));
					m_keyBindings.push_back(r);
					return r;
				}

				void CLOAK_CALL_THIS KeyBindingPage::Update(In API::Global::Time etime, In DWORD user)
				{
					if (m_timeout > 0)
					{
						API::Helper::Lock lock(m_sync);
						if (etime > m_enterTime[user]) { m_enterTime[user] -= etime; }
						else { m_parent->Enter(static_cast<API::Global::Input::User>(user)); }
					}
				}
				void CLOAK_CALL_THIS KeyBindingPage::OnLeave(In DWORD user)
				{
					API::Helper::Lock lock(m_sync);
					m_enterTime[user] = 0;
					if ((g_broadcastMode & API::Global::Input::KEY_BROADCAST::INACTIVE) == API::Global::Input::KEY_BROADCAST::NONE)
					{
						for (const auto& b : m_keyBindings) { b->CheckKeys(API::Global::Input::Keys::NONE, user); }
					}
					if (m_func) { m_func(false, static_cast<API::Global::Input::User>(user)); }
				}
				void CLOAK_CALL_THIS KeyBindingPage::CheckKeys(In const API::Global::Input::KeyCode& nexthold, In DWORD user)
				{
					API::Helper::Lock lock(m_sync);
					for (const auto& a : m_keyBindings)
					{
						a->CheckKeys(nexthold, user);
					}
				}
				
				Success(return == true) bool CLOAK_CALL_THIS KeyBindingPage::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, SavePtr, Global::Input, KeyBindingPage);
				}

				inline void CLOAK_CALL OnCheckMouseOver(In DWORD user)
				{
					API::Interface::IBasicGUI* mouseOver = Impl::Interface::RayTestContext::FindGUIAt(g_Users[user].Mouse);
					if (mouseOver != nullptr && mouseOver->IsClickable() == false) { SAVE_RELEASE(mouseOver); }
					API::Interface::IBasicGUI* last = g_mouseOver.exchange(mouseOver);
					if (mouseOver != last)
					{
						if (last != nullptr)
						{
							API::Global::Events::onMouseLeave oml;
							oml.next = mouseOver;
							last->triggerEvent(oml, false);
						}
						if (mouseOver != nullptr)
						{
							API::Global::Events::onMouseEnter ome;
							ome.last = last;
							ome.pos = g_Users[user].Mouse;
							ome.relativePos = mouseOver->GetRelativePos(g_Users[user].Mouse);
							mouseOver->triggerEvent(ome, false);
						}
					}
					SAVE_RELEASE(last);
				}
				inline void CLOAK_CALL OnCheckKeyBinding(In const API::Global::Input::KeyCode& down, In const API::Global::Input::KeyCode& up, In const API::Global::Input::KeyCode& nexthold, In DWORD user)
				{
					const API::Global::Input::KEY_BROADCAST mode = g_broadcastMode;
					if ((mode & API::Global::Input::KEY_BROADCAST::GLOBAL) != API::Global::Input::KEY_BROADCAST::NONE) { g_globalPage->CheckKeys(nexthold, user); }
					if ((mode & API::Global::Input::KEY_BROADCAST::INACTIVE) != API::Global::Input::KEY_BROADCAST::NONE)
					{
						API::Helper::Lock lock(g_syncPages);
						for (size_t a = 0; a < g_pages->size(); a++)
						{
							if (g_Users[user].CurrentPage != g_pages->at(a)) { g_pages->at(a)->CheckKeys(nexthold, user); }
						}
					}
					if ((mode & API::Global::Input::KEY_BROADCAST::CURRENT) != API::Global::Input::KEY_BROADCAST::NONE) { g_Users[user].CurrentPage->CheckKeys(nexthold, user); }
					if (g_handler != nullptr) { g_handler->OnKeyEnter(static_cast<API::Global::Input::User>(user), down, up); }
				}
				inline void CLOAK_CALL OnSendKeyEvents(In API::Interface::IBasicGUI* focus, In const API::Global::Input::KeyCode& d, In const API::Global::Input::KeyCode& u, In const API::Global::Input::KeyCode& p, In DWORD user)
				{
					API::Global::Events::onEnterKey oek;
					oek.userID = static_cast<API::Global::Input::User>(user);
					for (API::Global::Input::KeyCode k = API::Global::Input::KeyCode(1, 0, 0); k != API::Global::Input::Keys::NONE; k <<= 1)
					{
						if ((d & k) != API::Global::Input::Keys::NONE)
						{
							oek.key = k;
							oek.pressTpye = API::Global::Input::KEY_DOWN;
							focus->triggerEvent(oek, false);
						}
						else if ((u & k) != API::Global::Input::Keys::NONE)
						{
							oek.key = k;
							oek.pressTpye = API::Global::Input::KEY_UP;
							focus->triggerEvent(oek, false);
						}
						else if ((p & k) != API::Global::Input::Keys::NONE)
						{
							oek.key = k;
							oek.pressTpye = API::Global::Input::KEY_PRESS;
							focus->triggerEvent(oek, false);
						}
					}
				}
				inline void CLOAK_CALL OnKeyEvent(In const API::Global::Input::KeyCode& down, In const API::Global::Input::KeyCode& up, In const API::Global::Input::KeyCode& lasthold, In const API::Global::Input::KeyCode& nexthold, In DWORD user)
				{
					CLOAK_ASSUME((down & up) == API::Global::Input::Keys::NONE);
					API::Interface::IBasicGUI* focus = API::Interface::getFocused();
					if (lasthold != nexthold)
					{
						CLOAK_ASSUME((down | up) != API::Global::Input::Keys::NONE);
						if (focus == nullptr) { OnCheckKeyBinding(down, up, nexthold, user); }
						API::Global::Input::KeyCode p = lasthold & nexthold;
						API::Global::Input::KeyCode d = down;
						API::Global::Input::KeyCode u = up;
						CLOAK_ASSUME((p & (d | u)) == API::Global::Input::Keys::NONE);
						if (API::Global::Mouse::IsVisible())
						{
							if (((d | u | p) & API::Global::Input::Keys::Masks::MOUSE) != API::Global::Input::Keys::NONE)
							{
								API::Global::Input::KeyCode k = MOUSE_BEGIN;
								API::Global::Events::onClick oce;
								oce.activeKeys = (d | p) & API::Global::Input::Keys::Masks::MOUSE;
								oce.userID = static_cast<API::Global::Input::User>(user);
								oce.moveDir = API::Global::Math::Space2D::Vector(0, 0);
								oce.clickPos = g_Users[user].Mouse;
								CloakDebugLog("Check Mouse Keys");
								for (size_t a = 0; a < MOUSE_STATE_COUNT; a++, k <<= 1)
								{
									if ((d & k) != API::Global::Input::Keys::NONE)
									{
										CLOAK_ASSUME(g_Users[user].LastMouseClicked[a] == nullptr);
										CloakDebugLog("Mouse click registered!");
										if (g_Users[user].LastMouseOver != nullptr)
										{
											if (g_Users[user].LastMouseOver->IsClickable())
											{
												oce.pressType = API::Global::Input::MOUSE_DOWN;
												oce.previousType = g_Users[user].MouseState[a];
												oce.key = k;
												oce.relativePos = g_Users[user].LastMouseOver->GetRelativePos(g_Users[user].Mouse);
												g_Users[user].LastMouseOver->triggerEvent(oce, false);
												g_Users[user].LastMouseOver->AddRef();
												g_Users[user].LastMouseClicked[a] = g_Users[user].LastMouseOver;
											}
										}
										else
										{
											//TODO: Clicked in space
										}
										g_Users[user].MouseState[a] = API::Global::Input::MOUSE_DOWN;
										g_Users[user].MousePressTime[a] = Impl::Global::Game::getCurrentTimeMilliSeconds();
									}
									else if ((u & k) != API::Global::Input::Keys::NONE)
									{
										if (g_Users[user].LastMouseClicked[a] != nullptr)
										{
											oce.pressType = API::Global::Input::MOUSE_UP;
											oce.previousType = g_Users[user].MouseState[a];
											oce.key = k;
											oce.relativePos = g_Users[user].LastMouseClicked[a]->GetRelativePos(g_Users[user].Mouse);
											g_Users[user].LastMouseClicked[a]->triggerEvent(oce, false);
										}
										else
										{
											//TODO: Released in space
										}
										g_Users[user].MouseState[a] = API::Global::Input::MOUSE_UP;
										SAVE_RELEASE(g_Users[user].LastMouseClicked[a]);
									}
									else if ((p & k) != API::Global::Input::Keys::NONE)
									{
										if (g_Users[user].MouseState[a] == API::Global::Input::MOUSE_PRESSED || (g_Users[user].MouseState[a] == API::Global::Input::MOUSE_DOWN && Impl::Global::Game::getCurrentTimeMilliSeconds() - g_Users[user].MousePressTime[a] > MAX_MOUSE_CLICK))
										{
											if (g_Users[user].LastMouseClicked[a] != nullptr)
											{
												oce.pressType = API::Global::Input::MOUSE_PRESSED;
												oce.previousType = g_Users[user].MouseState[a];
												oce.key = k;
												oce.relativePos = g_Users[user].LastMouseClicked[a]->GetRelativePos(g_Users[user].Mouse);
												g_Users[user].LastMouseClicked[a]->triggerEvent(oce, false);
											}
											else
											{
												//TODO: Pressed in space
											}
											g_Users[user].MouseState[a] = API::Global::Input::MOUSE_PRESSED;
										}
									}
								}
								p &= ~API::Global::Input::Keys::Masks::MOUSE;
								d &= ~API::Global::Input::Keys::Masks::MOUSE;
								u &= ~API::Global::Input::Keys::Masks::MOUSE;
							}
							if ((up & API::Global::Input::Keys::Masks::MOUSE) != API::Global::Input::Keys::NONE && (nexthold & API::Global::Input::Keys::Masks::MOUSE) == API::Global::Input::Keys::NONE) { OnCheckMouseOver(user); }
						}
						if (focus != nullptr && focus->IsClickable())
						{
							OnSendKeyEvents(focus, d, u, p, user);
						}
					}
					else if (focus != nullptr && focus->IsClickable() && nexthold != API::Global::Input::Keys::NONE)
					{
						CLOAK_ASSUME(down == API::Global::Input::Keys::NONE && up == API::Global::Input::Keys::NONE);
						API::Global::Input::KeyCode p = nexthold;
						if (API::Global::Mouse::IsVisible()) { p &= ~API::Global::Input::Keys::Masks::MOUSE; }
						API::Global::Events::onEnterKey oek;
						oek.userID = static_cast<API::Global::Input::User>(user);
						for (API::Global::Input::KeyCode k = API::Global::Input::KeyCode(1, 0, 0); k != API::Global::Input::Keys::NONE; k <<= 1)
						{
							if ((p & k) != API::Global::Input::Keys::NONE)
							{
								oek.key = k;
								oek.pressTpye = API::Global::Input::KEY_PRESS;
								focus->triggerEvent(oek, false);
							}
						}
					}
					SAVE_RELEASE(focus);
				}

				void CLOAK_CALL SetInputHandler(In API::Global::IInputEvent* handle)
				{
					g_handler = handle;
				}
				void CLOAK_CALL LooseFocus()
				{
					for (size_t a = 0; a < MAX_USER_COUNT; a++)
					{
						API::Helper::Lock lock(g_Users[a].Sync);
						OnKeyEvent(API::Global::Input::Keys::NONE, g_Users[a].HoldKeys, static_cast<DWORD>(a));
					}
				}
				void CLOAK_CALL OnInitialize()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncPages));
					g_defaultPage = new KeyBindingPage();
					g_globalPage = new KeyBindingPage();
					g_pages = new API::List<CE::RefPointer<KeyBindingPage>>();
					g_pages->push_back(g_defaultPage);
					for (size_t a = 0; a < MAX_USER_COUNT; a++)
					{
						CREATE_INTERFACE(CE_QUERY_ARGS(&g_Users[a].Sync));
						g_Users[a].LastMouseUpdate = API::Global::Game::GetTimeStamp();
						g_Users[a].LastThumbUpdate = API::Global::Game::GetTimeStamp();
						g_Users[a].CurrentPage = g_defaultPage;
						for (size_t b = 0; b < MOUSE_STATE_COUNT; b++)
						{
							g_Users[a].MouseState[b] = API::Global::Input::MOUSE_UP;
							g_Users[a].LastMouseClicked[b] = nullptr;
						}
					}
				}
				void CLOAK_CALL onLoad(In size_t threadID)
				{
					Impl::Interface::RayTestContext::Initialize();
					Engine::Input::Keyboard::Initialize();
					Engine::Input::Gamepad::Initialize();
				}
				void CLOAK_CALL onUpdate(In size_t threadID, In API::Global::Time etime)
				{
					Engine::Input::Gamepad::Update(etime);
					Impl::Global::Mouse::Update();
					for (size_t a = 0; a < MAX_USER_COUNT; a++)
					{
						API::Helper::Lock lock(g_Users[a].Sync);
						API::Interface::IBasicGUI* lmo = Impl::Interface::RayTestContext::FindGUIAt(g_Users[a].Mouse);
						if (lmo != g_Users[a].LastMouseOver) { SWAP(g_Users[a].LastMouseOver, lmo); }
						SAVE_RELEASE(lmo);

						if (g_Users[a].LastHoldKeys != g_Users[a].HoldKeys)
						{
							const API::Global::Input::KeyCode diff = g_Users[a].HoldKeys ^ g_Users[a].LastHoldKeys;
							const API::Global::Input::KeyCode down = g_Users[a].HoldKeys & diff;
							const API::Global::Input::KeyCode up = g_Users[a].LastHoldKeys & diff;
							OnKeyEvent(down, up, g_Users[a].LastHoldKeys, g_Users[a].HoldKeys, static_cast<DWORD>(a));
							g_Users[a].LastHoldKeys = g_Users[a].HoldKeys;
						}
						g_Users[a].CurrentPage->Update(etime, static_cast<DWORD>(a));

						for (size_t b = 0; b < ARRAYSIZE(g_Users[a].ScrollDelta); b++)
						{
							const uint32_t d = g_Users[a].ScrollDelta[b].exchange(0);
							if (d != 0)
							{
								const float delta = static_cast<float>(d) / SCROLL_POINT_SIZE;
								const API::Global::Input::SCROLL_STATE button = b == 0 ? API::Global::Input::SCROLL_HORIZONTAL : API::Global::Input::SCROLL_VERTICAL;
								if (g_Users[a].LastMouseOver != nullptr && g_Users[a].LastMouseOver->IsClickable())
								{
									API::Global::Events::onScroll ose;
									ose.button = button;
									ose.clickPos = g_Users[a].Mouse;
									ose.relativePos = g_Users[a].LastMouseOver->GetRelativePos(g_Users[a].Mouse);
									ose.delta = delta;
									ose.userID = static_cast<API::Global::Input::User>(a);
									g_Users[a].LastMouseOver->triggerEvent(ose);
								}
								else
								{
									//TODO: Scroll in space
								}
								if (g_handler != nullptr) { g_handler->OnScroll(static_cast<API::Global::Input::User>(a), button, delta); }
							}
						}
					}
				}
				void CLOAK_CALL onStop(In size_t threadID)
				{
					API::Interface::IBasicGUI* mouseOver = g_mouseOver.exchange(nullptr);
					SAVE_RELEASE(mouseOver);
					Engine::Input::Gamepad::Release();
					Engine::Input::Keyboard::Release();
					Impl::Interface::RayTestContext::Release();
				}
				void CLOAK_CALL OnRelease()
				{
					delete g_pages;
					for (size_t a = 0; a < MAX_USER_COUNT; a++)
					{
						SAVE_RELEASE(g_Users[a].Sync);
						for (size_t b = 0; b < MOUSE_STATE_COUNT; b++)
						{
							SAVE_RELEASE(g_Users[a].LastMouseClicked[b]);
						}
					}
					SAVE_RELEASE(g_defaultPage);
					SAVE_RELEASE(g_globalPage);
					SAVE_RELEASE(g_syncPages);
				}
				void CLOAK_CALL OnWindowClose()
				{
					if (g_handler != nullptr) { g_handler->OnWindowClose(); }
				}
				void CLOAK_CALL OnKeyEvent(In const API::Global::Input::KeyCode& key, In bool down, In DWORD user)
				{
					CLOAK_ASSUME(user < MAX_USER_COUNT);
					API::Helper::Lock lock(g_Users[user].Sync);
					if (down)
					{
						g_Users[user].HoldKeys |= key;
					}
					else
					{
						g_Users[user].HoldKeys &= ~key;
					}
				}
				void CLOAK_CALL OnKeyEvent(In const API::Global::Input::KeyCode& down, In const API::Global::Input::KeyCode& up, In DWORD user)
				{
					CLOAK_ASSUME(user < MAX_USER_COUNT);
					API::Helper::Lock lock(g_Users[user].Sync);
					g_Users[user].HoldKeys |= down;
					g_Users[user].HoldKeys &= ~up;
				}
				void CLOAK_CALL OnConnect(In DWORD user, In bool connected)
				{
					CLOAK_ASSUME(user < MAX_USER_COUNT);
					API::Helper::Lock lock(g_Users[user].Sync);
					g_Users[user].LastThumbUpdate = API::Global::Game::GetTimeStamp();
					if (g_handler != nullptr) { g_handler->OnGamepadConnect(static_cast<API::Global::Input::User>(user), connected ? API::Global::Input::CONNECTED : API::Global::Input::DISCONNECTED); }
				}
				void CLOAK_CALL OnMouseMoveEvent(In const API::Global::Math::Space2D::Point& pos, In const API::Global::Math::Space2D::Vector& move, In bool visible, In DWORD user)
				{
					CLOAK_ASSUME(user < MAX_USER_COUNT);
					API::Helper::Lock lock(g_Users[user].Sync);
					API::Global::Math::Space2D::Vector rmove = pos - g_Users[user].Mouse;
					g_Users[user].Mouse = pos;
					API::Global::Time ct = API::Global::Game::GetTimeStamp();
					if (visible)
					{
						API::Global::Events::onClick oce;
						oce.activeKeys = g_Users[user].HoldKeys & API::Global::Input::Keys::Masks::MOUSE;
						oce.userID = static_cast<API::Global::Input::User>(user);
						oce.moveDir = rmove;
						oce.clickPos = pos;
						if (oce.activeKeys != API::Global::Input::Keys::NONE)
						{
							API::Global::Input::KeyCode k = MOUSE_BEGIN;
							CloakDebugLog("Check Mouse Keys");
							for (size_t a = 0; a < MOUSE_STATE_COUNT; a++, k <<= 1)
							{
								if (g_Users[user].MouseState[a] != API::Global::Input::MOUSE_UP && (g_Users[user].MouseState[a] != API::Global::Input::MOUSE_DOWN || Impl::Global::Game::getCurrentTimeMilliSeconds() - g_Users[user].MousePressTime[a] > MAX_MOUSE_CLICK || ((move.X*move.X) + (move.Y*move.Y)) > DRAG_THRESHOLD))
								{
									API::Interface::IBasicGUI* f = g_Users[user].LastMouseClicked[a];
									if (f != nullptr)
									{
										oce.pressType = API::Global::Input::MOUSE_DRAGED;
										oce.previousType = g_Users[user].MouseState[a];
										oce.key = k;
										oce.relativePos = f->GetRelativePos(pos);
										f->triggerEvent(oce, false);
									}
									g_Users[user].MouseState[a] = API::Global::Input::MOUSE_DRAGED;
								}
							}
						}
						else { OnCheckMouseOver(user); }
					}
					else if (g_handler != nullptr) { g_handler->OnMouseMove(static_cast<API::Global::Input::User>(user), move, pos, ct - g_Users[user].LastMouseUpdate); }
					g_Users[user].LastMouseUpdate = ct;
				}
				void CLOAK_CALL OnMouseSetEvent(In const API::Global::Math::Space2D::Point& pos, In bool visible, In DWORD user)
				{
					CLOAK_ASSUME(user < MAX_USER_COUNT);
					API::Helper::Lock lock(g_Users[user].Sync);
					API::Global::Math::Space2D::Vector move(0, 0);
					g_Users[user].Mouse = pos;
					API::Global::Time ct = API::Global::Game::GetTimeStamp();
					if (visible)
					{
						API::Global::Events::onClick oce;
						oce.activeKeys = g_Users[user].HoldKeys & API::Global::Input::Keys::Masks::MOUSE;
						oce.userID = static_cast<API::Global::Input::User>(user);
						oce.moveDir = move;
						oce.clickPos = pos;
						if (oce.activeKeys != API::Global::Input::Keys::NONE)
						{
							API::Global::Input::KeyCode k = MOUSE_BEGIN;
							CloakDebugLog("Check Mouse Keys");
							for (size_t a = 0; a < MOUSE_STATE_COUNT; a++, k <<= 1)
							{
								if (g_Users[user].MouseState[a] != API::Global::Input::MOUSE_UP && (g_Users[user].MouseState[a] != API::Global::Input::MOUSE_DOWN || Impl::Global::Game::getCurrentTimeMilliSeconds() - g_Users[user].MousePressTime[a] > MAX_MOUSE_CLICK || ((move.X*move.X) + (move.Y*move.Y)) > DRAG_THRESHOLD))
								{
									API::Interface::IBasicGUI* f = g_Users[user].LastMouseClicked[a];
									if (f != nullptr)
									{
										oce.pressType = API::Global::Input::MOUSE_DRAGED;
										oce.previousType = g_Users[user].MouseState[a];
										oce.key = k;
										oce.relativePos = f->GetRelativePos(pos);
										f->triggerEvent(oce, false);
									}
									g_Users[user].MouseState[a] = API::Global::Input::MOUSE_DRAGED;
								}
							}
						}
						else { OnCheckMouseOver(user); }
					}
					else if (g_handler != nullptr) { g_handler->OnMouseMove(static_cast<API::Global::Input::User>(user), move, pos, ct - g_Users[user].LastMouseUpdate); }
					g_Users[user].LastMouseUpdate = ct;
				}
				void CLOAK_CALL OnResetKeys(In DWORD user)
				{
					CLOAK_ASSUME(user < MAX_USER_COUNT);
					API::Helper::Lock lock(g_Users[user].Sync);
					OnKeyEvent(API::Global::Input::Keys::NONE, g_Users[user].HoldKeys, user);
					CloakDebugLog("Reset keys!");
				}
				void CLOAK_CALL OnThumbMoveEvent(In const API::Global::Math::Space2D::Point& left, In const API::Global::Math::Space2D::Point& right, In DWORD user)
				{
					CLOAK_ASSUME(user < MAX_USER_COUNT);
					API::Helper::Lock lock(g_Users[user].Sync);
					API::Global::Input::ThumbMoveData tmd;
					tmd.LeftMovement = left - g_Users[user].LeftThumb;
					tmd.RightMovement = right - g_Users[user].RightThumb;
					tmd.LeftResPos = left;
					tmd.RightResPos = right;
					API::Global::Time ct = API::Global::Game::GetTimeStamp();
					if (g_handler != nullptr) { g_handler->OnGamepadThumbMove(static_cast<API::Global::Input::User>(user), tmd, ct - g_Users[user].LastThumbUpdate); }
					g_Users[user].LastThumbUpdate = ct;
					g_Users[user].LeftThumb = left;
					g_Users[user].RightThumb = right;
				}
				void CLOAK_CALL OnMouseScrollEvent(In float delta, In API::Global::Input::SCROLL_STATE button, In DWORD user)
				{
					CLOAK_ASSUME(user < MAX_USER_COUNT);
					g_Users[user].ScrollDelta[button == API::Global::Input::SCROLL_HORIZONTAL ? 0 : 1] += static_cast<uint32_t>(delta * SCROLL_POINT_SIZE);
				}
				void CLOAK_CALL OnDoubleClick(In const API::Global::Input::KeyCode& key, In DWORD user)
				{
					CLOAK_ASSUME(user < MAX_USER_COUNT);
					API::Helper::Lock lock(g_Users[user].Sync);
					API::Interface::IBasicGUI* clicked = Impl::Interface::RayTestContext::FindGUIAt(g_Users[user].Mouse);
					if (clicked != nullptr)
					{
						API::Global::Events::onClick oce;
						oce.activeKeys = g_Users[user].HoldKeys | key;
						oce.userID = static_cast<API::Global::Input::User>(user);
						oce.moveDir = API::Global::Math::Space2D::Vector(0, 0);
						oce.clickPos = g_Users[user].Mouse;
						oce.pressType = API::Global::Input::MOUSE_DOUBLE;
						oce.relativePos = clicked->GetRelativePos(g_Users[user].Mouse);
						API::Global::Input::KeyCode k = MOUSE_BEGIN;
						CloakDebugLog("Check Mouse Keys");
						for (size_t a = 0; a < MOUSE_STATE_COUNT; a++, k <<= 1)
						{
							if ((k & key) != API::Global::Input::Keys::NONE)
							{
								oce.previousType = g_Users[user].MouseState[a];
								oce.key = k;
								clicked->triggerEvent(oce, false);
							}
						}
					}
					SAVE_RELEASE(clicked);
				}
				void CLOAK_CALL OnUpdateFocus(In API::Interface::IBasicGUI* lastFocus, In API::Interface::IBasicGUI* nextFocus)
				{
					CLOAK_ASSUME(lastFocus != nextFocus);
					for (DWORD a = 0; a < MAX_USER_COUNT; a++)
					{
						API::Helper::Lock lock(g_Users[a].Sync);
						if (g_Users[a].HoldKeys != API::Global::Input::Keys::NONE)
						{
							if (lastFocus == nullptr) { OnCheckKeyBinding(API::Global::Input::Keys::NONE, g_Users[a].HoldKeys, API::Global::Input::Keys::NONE, a); }
							else { OnSendKeyEvents(lastFocus, API::Global::Input::Keys::NONE, g_Users[a].HoldKeys, API::Global::Input::Keys::NONE, a); }
							if (nextFocus == nullptr) { OnCheckKeyBinding(g_Users[a].HoldKeys, API::Global::Input::Keys::NONE, g_Users[a].HoldKeys, a); }
							else { OnSendKeyEvents(nextFocus, g_Users[a].HoldKeys, API::Global::Input::Keys::NONE, API::Global::Input::Keys::NONE, a); }
						}
					}
				}
			}
		}
	}
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Input {
				CLOAKENGINE_API void CLOAK_CALL SetLeftVibration(In User user, In float power)
				{
					Engine::Input::Gamepad::SetVibrationLeft(user, max(0, power));
				}
				CLOAKENGINE_API void CLOAK_CALL SetRightVibration(In User user, In float power)
				{
					Engine::Input::Gamepad::SetVibrationRight(user, max(0, power));
				}
				CLOAKENGINE_API void CLOAK_CALL SetVibration(In User user, In float left, In float right)
				{
					Engine::Input::Gamepad::SetVibration(user, max(0, left), max(0, right));
				}
				CLOAKENGINE_API void CLOAK_CALL SetBroadcastMode(In KEY_BROADCAST mode)
				{
					KEY_BROADCAST lmode = Impl::Global::Input::g_broadcastMode.exchange(mode);
					const KEY_BROADCAST lin = lmode & KEY_BROADCAST::INACTIVE;
					const KEY_BROADCAST nin = mode & KEY_BROADCAST::INACTIVE;
					const KEY_BROADCAST lcur = lmode & KEY_BROADCAST::CURRENT;
					const KEY_BROADCAST ncur = mode & KEY_BROADCAST::CURRENT;
					const KEY_BROADCAST lgl = lmode & KEY_BROADCAST::GLOBAL;
					const KEY_BROADCAST ngl = mode & KEY_BROADCAST::GLOBAL;
					if (lgl != ngl)
					{
						if (lgl == KEY_BROADCAST::NONE)
						{
							for (DWORD a = 0; a < Impl::Global::Input::MAX_USER_COUNT; a++)
							{
								API::Helper::Lock ulock(Impl::Global::Input::g_Users[a].Sync);
								Impl::Global::Input::g_globalPage->CheckKeys(Impl::Global::Input::g_Users[a].HoldKeys, a);
							}
						}
						else
						{
							for (DWORD a = 0; a < Impl::Global::Input::MAX_USER_COUNT; a++)
							{
								Impl::Global::Input::g_globalPage->CheckKeys(API::Global::Input::Keys::NONE, a);
							}
						}
					}
					if (lcur != ncur)
					{
						if (lcur == KEY_BROADCAST::NONE)
						{
							for (DWORD a = 0; a < Impl::Global::Input::MAX_USER_COUNT; a++)
							{
								API::Helper::Lock ulock(Impl::Global::Input::g_Users[a].Sync);
								Impl::Global::Input::g_Users[a].CurrentPage->CheckKeys(Impl::Global::Input::g_Users[a].HoldKeys, a);
							}
						}
						else
						{
							for (DWORD a = 0; a < Impl::Global::Input::MAX_USER_COUNT; a++)
							{
								API::Helper::Lock ulock(Impl::Global::Input::g_Users[a].Sync);
								Impl::Global::Input::g_Users[a].CurrentPage->CheckKeys(Impl::Global::Input::g_Users[a].HoldKeys, a);
							}
						}
					}
					if (lin != nin)
					{
						if (lin == KEY_BROADCAST::NONE) 
						{
							for (DWORD a = 0; a < Impl::Global::Input::MAX_USER_COUNT; a++)
							{
								API::Helper::Lock ulock(Impl::Global::Input::g_Users[a].Sync);
								API::Helper::Lock lock(Impl::Global::Input::g_syncPages);
								for (size_t b = 0; b < Impl::Global::Input::g_pages->size(); b++)
								{
									if (Impl::Global::Input::g_pages->at(b) != Impl::Global::Input::g_Users[a].CurrentPage) 
									{ 
										Impl::Global::Input::g_pages->at(b)->CheckKeys(Impl::Global::Input::g_Users[a].HoldKeys, a); 
									}
								}
							}
						}
						else 
						{
							for (DWORD a = 0; a < Impl::Global::Input::MAX_USER_COUNT; a++)
							{
								API::Helper::Lock ulock(Impl::Global::Input::g_Users[a].Sync);
								API::Helper::Lock lock(Impl::Global::Input::g_syncPages);
								for (size_t b = 0; b < Impl::Global::Input::g_pages->size(); b++)
								{
									if (Impl::Global::Input::g_pages->at(b) != Impl::Global::Input::g_Users[a].CurrentPage)
									{
										Impl::Global::Input::g_pages->at(b)->CheckKeys(API::Global::Input::Keys::NONE, a);
									}
								}
							}
						}
					}
				}
				CLOAKENGINE_API IKeyBinding* CLOAK_CALL CreateGlobalKeyBinding(In size_t slotCount, In bool exact, In KeyBindingFunc onKeyCode)
				{
					return Impl::Global::Input::g_globalPage->CreateKeyBinding(slotCount, exact, std::move(onKeyCode));
				}
				CLOAKENGINE_API Debug::Error CLOAK_CALL GetBasePage(In REFIID riid, Outptr void** ptr)
				{
					HRESULT hRet = Impl::Global::Input::g_defaultPage->QueryInterface(riid, ptr);
					return SUCCEEDED(hRet) ? Debug::Error::OK : Debug::Error::NO_INTERFACE;
				}
			}
		}
	}
}