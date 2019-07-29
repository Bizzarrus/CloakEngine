#include "stdafx.h"
#include "Implementation/Global/Mouse.h"
#include "Implementation/Global/Input.h"

#include "CloakEngine/Interface/Frame.h"

#include "Engine/WindowHandler.h"

#include <atomic>

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Mouse {
				constexpr uint32_t MAX_MOUSE_WAIT = 10;

				struct LongData {
					double X = 0;
					double Y = 0;
				};
				struct MousePosition : public LongData {
					LONG dX = 0;
					LONG dY = 0;
				};

				const HCURSOR DEFAULT_CURSOR = LoadCursor(NULL, IDC_ARROW);

				std::atomic<MousePosition> g_pos;
				std::atomic<LongData> g_lastPos;
				std::atomic<LongData> g_screen;
				std::atomic<API::Global::Math::Space2D::Point> g_softwareOffset = API::Global::Math::Space2D::Point(0, 0);
				std::atomic<RECT> g_lastClip;
				std::atomic<bool> g_inClip = false;
				std::atomic<uint32_t> g_mouseWait = MAX_MOUSE_WAIT;
				std::atomic<bool> g_keepInWindow = false;
				std::atomic<bool> g_visible = true;
				std::atomic<bool> g_userVisible = true;
				std::atomic<bool> g_updateMouse = false;
				std::atomic<bool> g_lockedInClip = false;
				std::atomic<HCURSOR> g_nextCursor = NULL;
				std::atomic<uint64_t> g_forceVisible = 0;
				std::atomic<bool> g_useSoftware = false;
				std::atomic<float> g_mouseSpeed = 1;
				API::Interface::IFrame* g_software = nullptr;

				inline void CLOAK_CALL UpdateMouseGraphic(In bool visible, In bool software, In bool inClip)
				{
					if (inClip == false) 
					{ 
						g_nextCursor = DEFAULT_CURSOR;
						g_software->Hide();
					}
					else if (visible == false)
					{
						g_nextCursor = NULL;
						g_software->Hide();
					}
					else if (software == false)
					{
						g_nextCursor = DEFAULT_CURSOR;//TODO: Allow custom cursor
						g_software->Hide();
					}
					else
					{
						g_nextCursor = NULL;
						g_software->Show();
					}
					g_updateMouse = true;
				}
				inline void CLOAK_CALL MouseLeave(In const MousePosition& pos, In bool visible, In bool software)
				{
					if (g_inClip.exchange(false) == true)
					{
						POINT p = { static_cast<int>(pos.X), static_cast<int>(pos.Y) };
						ClientToScreen(Engine::WindowHandler::getWindow(), &p);
						ClipCursor(nullptr);
						UpdateMouseGraphic(visible, software, false);
						if (visible == false || software) { SetCursorPos(p.x, p.y); }
						Impl::Global::Input::OnResetKeys(0);
					}
				}
				inline void CLOAK_CALL UpdateSoftwareMousePos(In bool software, In const LongData& pos, In const LongData& screen)
				{
					if (software)
					{
						API::Interface::Anchor anch;
						anch.point = API::Interface::AnchorPoint::TOPLEFT;
						anch.relativePoint = API::Interface::AnchorPoint::TOPLEFT;
						const API::Global::Math::Space2D::Point off = g_softwareOffset;
						anch.offset.X = static_cast<float>(static_cast<double>(pos.X) / screen.X) - off.X;
						anch.offset.Y = static_cast<float>(static_cast<double>(pos.Y) / screen.Y) - off.Y;
						anch.relativeTarget = nullptr;
						g_software->SetAnchor(anch);
					}
				}
				inline void CLOAK_CALL UpdateMouseClipping(In bool software, In bool visible, In bool kis, In bool inClip)
				{
					if (inClip && (software == true || visible == false || kis == true)) 
					{
						RECT rc = g_lastClip;
						ClipCursor(&rc); 
						g_lockedInClip = true;
					}
					else
					{
						ClipCursor(nullptr);
						g_lockedInClip = false;
					}
				}
				inline void CLOAK_CALL UpdateMouseVisible()
				{
					const bool sof = Impl::Global::Mouse::g_useSoftware;
					const bool inc = Impl::Global::Mouse::g_inClip;
					const bool v = g_userVisible || g_forceVisible > 0;
					Impl::Global::Mouse::g_visible = v;
					Impl::Global::Mouse::UpdateMouseClipping(sof, v, Impl::Global::Mouse::g_keepInWindow, inc);
					Impl::Global::Mouse::UpdateMouseGraphic(v, sof, inc);
				}

				void CLOAK_CALL Initiate()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_software));
					g_software->Hide();
				}
				void CLOAK_CALL Update()
				{
					if (g_inClip == true)
					{
						uint32_t lmw = g_mouseWait;
						if (lmw > 0)
						{
							uint32_t nmw = lmw - 1;
							while (!g_mouseWait.compare_exchange_strong(lmw, nmw)) { nmw = lmw > 0 ? lmw - 1 : 0; }
						}
						else if (lmw == 0)
						{
							const bool vis = g_visible;
							const bool sof = g_useSoftware;
							const bool kiw = g_keepInWindow;
							const LongData screen = g_screen;
							MousePosition xnmp;
							MousePosition nmp = g_pos;
							do {
								xnmp = nmp;
								xnmp.dX = 0;
								xnmp.dY = 0;
							} while (!g_pos.compare_exchange_strong(nmp, xnmp));
							if (sof == false && vis == true)
							{
								POINT cp;
								GetCursorPos(&cp);
								HWND wind = Engine::WindowHandler::getWindow();
								ScreenToClient(wind, &cp);
								nmp.X = cp.x;
								nmp.Y = cp.y;
								if (kiw == false && (nmp.X<0 || nmp.Y<0 || nmp.X>screen.X || nmp.Y>screen.Y))
								{
									MouseLeave(nmp, vis, sof);
									return;
								}
							}
							const LongData lmp = g_lastPos.exchange(nmp);
							const double dx = nmp.X - lmp.X;
							const double dy = nmp.Y - lmp.Y;
							if (std::abs(dx) > 1 / screen.X || std::abs(dy) > 1 / screen.Y || nmp.dX != 0 || nmp.dY != 0)
							{
								const float speed = g_mouseSpeed;
								API::Global::Math::Space2D::Point curPos(static_cast<float>(static_cast<double>(nmp.X) / screen.X), static_cast<float>(static_cast<double>(nmp.Y) / screen.Y));
								API::Global::Math::Space2D::Vector move(static_cast<float>(static_cast<double>(nmp.dX*speed) / screen.X), static_cast<float>(static_cast<double>(nmp.dY*speed) / screen.Y));
								Impl::Global::Input::OnMouseMoveEvent(curPos, move, g_visible, 0);
								UpdateSoftwareMousePos(sof, nmp, screen);
							}
							else
							{
								API::Global::Math::Space2D::Point curPos(static_cast<float>(static_cast<double>(nmp.X) / screen.X), static_cast<float>(static_cast<double>(nmp.Y) / screen.Y));
								API::Global::Math::Space2D::Vector move(0, 0);
								Impl::Global::Input::OnMouseMoveEvent(curPos, move, g_visible, 0);
							}
							if (g_lockedInClip == true)
							{
								RECT rc;
								HWND wind = Engine::WindowHandler::getWindow();
								if (GetClientRect(wind, &rc) == TRUE)
								{
									POINT wtl = { static_cast<int>(rc.left), static_cast<int>(rc.top) };
									POINT wbr = { static_cast<int>(rc.right), static_cast<int>(rc.bottom) };
									ClientToScreen(wind, &wtl);
									ClientToScreen(wind, &wbr);
									rc.left = wtl.x;
									rc.right = wbr.x;
									rc.top = wtl.y;
									rc.bottom = wbr.y;
									g_lastClip = rc;
									UpdateMouseClipping(sof, vis, kiw, true);
								}
							}
						}
					}
				}
				void CLOAK_CALL Release()
				{
					SAVE_RELEASE(g_software);
				}
				void CLOAK_CALL CheckClipping()
				{
					if (g_inClip == false)
					{
						if (Engine::WindowHandler::isFocused() && Engine::WindowHandler::isVisible())
						{
							HWND wind = Engine::WindowHandler::getWindow();
							RECT rc;
							if (IsIconic(wind) == FALSE && GetClientRect(wind, &rc) == TRUE)
							{
								POINT wtl = { static_cast<int>(rc.left), static_cast<int>(rc.top) };
								POINT wbr = { static_cast<int>(rc.right), static_cast<int>(rc.bottom) };
								ClientToScreen(wind, &wtl);
								ClientToScreen(wind, &wbr);
								POINT cp;
								GetCursorPos(&cp);
								if (cp.x > wtl.x && cp.x < wbr.x && cp.y > wtl.y && cp.y < wbr.y)
								{
									const bool sof = g_useSoftware;
									const bool vis = g_visible;
									const bool kiw = g_keepInWindow;

									ScreenToClient(wind, &cp);
									MousePosition nmp;
									nmp.X = cp.x;
									nmp.Y = cp.y;
									nmp.dX = 0;
									nmp.dY = 0;
									g_pos = nmp;
									g_lastPos = nmp;
									rc.left = wtl.x;
									rc.right = wbr.x;
									rc.top = wtl.y;
									rc.bottom = wbr.y;
									g_lastClip = rc;
									UpdateMouseClipping(sof, vis, kiw, true);
									g_mouseWait = MAX_MOUSE_WAIT;
									g_inClip = true;
									const LongData screen = g_screen;
									API::Global::Math::Space2D::Point curPos(static_cast<float>(static_cast<double>(nmp.X) / screen.X), static_cast<float>(static_cast<double>(nmp.Y) / screen.Y));
									Impl::Global::Input::OnMouseSetEvent(curPos, vis, 0);
									UpdateMouseGraphic(vis, sof, true);
									UpdateSoftwareMousePos(sof, nmp, screen);
								}
							}
						}
					}
					else
					{
						HWND wind = Engine::WindowHandler::getWindow();
						if (IsIconic(wind) == TRUE) 
						{ 
							MouseLeave(g_pos, g_visible, g_useSoftware); 
						}
						else 
						{
							RECT rc;
							if (GetClientRect(wind, &rc) == TRUE)
							{
								const bool sof = g_useSoftware;
								const bool vis = g_visible;
								const bool kiw = g_keepInWindow;
								g_lastClip = rc;
								UpdateMouseClipping(sof, vis, kiw, true);
							}
						}
					}
				}
				void CLOAK_CALL LooseFocus()
				{
					const MousePosition lmp = g_pos;
					MouseLeave(lmp, g_visible, g_useSoftware);
				}
				void CLOAK_CALL Move(In LONG X, In LONG Y)
				{
					if (g_inClip == false) { CheckClipping(); }
					else if (g_inClip == true && g_mouseWait == 0)
					{
						const bool visible = g_visible;
						const bool kiw = g_keepInWindow;
						const bool sof = g_useSoftware;
						const LongData screen = g_screen;
						const float speed = g_mouseSpeed;
						const float fX = X * speed;
						const float fY = Y * speed;
						MousePosition lmp = g_pos;
						MousePosition nmp;
						do {
							nmp = lmp;
							if (visible && sof)
							{
								nmp.X += fX;
								nmp.Y += fY;
								if (kiw)
								{
									nmp.X = max(0, min(nmp.X, screen.X));
									nmp.Y = max(0, min(nmp.Y, screen.Y));
								}
							}
							nmp.dX += X;
							nmp.dY += Y;
						} while (!g_pos.compare_exchange_strong(lmp, nmp));
						if (visible == true && sof == true && kiw == false && (nmp.X<0 || nmp.Y<0 || nmp.X>screen.X || nmp.Y>screen.Y))
						{ 
							MouseLeave(nmp, visible, sof); 
						}
					}
				}
				void CLOAK_CALL SetPos(In LONG X, In LONG Y)
				{
					if (g_inClip == false) { CheckClipping(); }
					if (g_inClip == true)
					{
						if (Engine::WindowHandler::isFocused())
						{
							HWND wind = Engine::WindowHandler::getWindow();
							RECT rc;
							if (IsIconic(wind) == FALSE && GetClientRect(wind, &rc) == TRUE)
							{
								POINT wtl = { static_cast<int>(rc.left), static_cast<int>(rc.top) };
								POINT wbr = { static_cast<int>(rc.bottom), static_cast<int>(rc.right) };
								ClientToScreen(wind, &wtl);
								ClientToScreen(wind, &wbr);
								POINT cp = { static_cast<int>(X), static_cast<int>(Y) };
								if (cp.x >= wtl.x && cp.x <= wbr.x && cp.y >= wtl.y && cp.y <= wbr.y)
								{
									ScreenToClient(wind, &cp);
									MousePosition nmp;
									nmp.X = cp.x;
									nmp.Y = cp.y;
									nmp.dX = 0;
									nmp.dY = 0;
									g_pos = nmp;
								}
								else 
								{
									ScreenToClient(wind, &cp);
									MousePosition nmp;
									nmp.X = cp.x;
									nmp.Y = cp.y;
									nmp.dX = 0;
									nmp.dY = 0;
									MouseLeave(nmp, g_visible, g_useSoftware);
								}
							}
						}
					}
				}
				void CLOAK_CALL SetGraphicSettings(In const API::Global::Graphic::Settings& set)
				{
					LongData screen = { static_cast<double>(set.Resolution.Width), static_cast<double>(set.Resolution.Height) };
					g_screen = screen;
					CheckClipping();
				}
				void CLOAK_CALL UpdateCursor()
				{
					if (g_updateMouse.exchange(false) == true) { SetCursor(g_nextCursor); }
				}
				bool CLOAK_CALL IsInClip() { return g_inClip; }
				uint64_t CLOAK_CALL Show()
				{
					const uint64_t r = g_forceVisible.fetch_add(1) + 1;
					UpdateMouseVisible();
					return r;
				}
				uint64_t CLOAK_CALL Hide()
				{
					const uint64_t r = g_forceVisible.fetch_sub(1);
					UpdateMouseVisible();
					return r > 0 ? r - 1 : r;
				}
			}
		}
	}
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Mouse {
				CLOAKENGINE_API void CLOAK_CALL SetImage(In Files::ITexture* img)
				{
					Impl::Global::Mouse::g_software->SetBackgroundTexture(img);
				}
				CLOAKENGINE_API void CLOAK_CALL SetImage(In Files::ITexture* img, In float offsetX, In float offsetY)
				{
					Impl::Global::Mouse::g_software->SetBackgroundTexture(img);
					Impl::Global::Mouse::g_softwareOffset = API::Global::Math::Space2D::Point(offsetX, offsetY);
					Impl::Global::Mouse::UpdateSoftwareMousePos(Impl::Global::Mouse::g_useSoftware, Impl::Global::Mouse::g_pos, Impl::Global::Mouse::g_screen);
				}
				CLOAKENGINE_API void CLOAK_CALL SetImage(In Files::ITexture* img, In const Math::Space2D::Point& offset)
				{
					Impl::Global::Mouse::g_software->SetBackgroundTexture(img);
					Impl::Global::Mouse::g_softwareOffset = offset;
					Impl::Global::Mouse::UpdateSoftwareMousePos(Impl::Global::Mouse::g_useSoftware, Impl::Global::Mouse::g_pos, Impl::Global::Mouse::g_screen);
				}
				CLOAKENGINE_API void CLOAK_CALL SetPos(In const Math::Space2D::Point& p)
				{
					SetPos(p.X, p.Y);
				}
				CLOAKENGINE_API void CLOAK_CALL SetPos(In float X, In float Y)
				{
					X = min(1, max(0, X));
					Y = min(1, max(0, Y));
					const Impl::Global::Mouse::LongData s = Impl::Global::Mouse::g_screen;
					Impl::Global::Mouse::MousePosition nmp;
					nmp.X = X*s.X;
					nmp.Y = Y*s.Y;
					Impl::Global::Mouse::g_pos = nmp;
					Impl::Global::Mouse::CheckClipping();
				}
				CLOAKENGINE_API void CLOAK_CALL GetPos(Out Math::Space2D::Point* p)
				{
					if (p != nullptr)
					{
						const Impl::Global::Mouse::MousePosition mp = Impl::Global::Mouse::g_pos;
						const Impl::Global::Mouse::LongData s = Impl::Global::Mouse::g_screen;
						p->X = static_cast<float>(static_cast<double>(mp.X) / s.X);
						p->Y = static_cast<float>(static_cast<double>(mp.Y) / s.Y);
					}
				}
				CLOAKENGINE_API void CLOAK_CALL Hide()
				{
					Impl::Global::Mouse::g_userVisible = false;
					Impl::Global::Mouse::UpdateMouseVisible();
					//TODO: should reset pressed keys if focus != nullptr
				}
				CLOAKENGINE_API void CLOAK_CALL Show()
				{
					Impl::Global::Mouse::g_userVisible = true;
					Impl::Global::Mouse::UpdateMouseVisible();
					//TODO: should reset pressed keys if focus != nullptr
				}
				CLOAKENGINE_API void CLOAK_CALL SetUseSoftwareMouse(In bool use)
				{
					Impl::Global::Mouse::g_useSoftware = use;
					const bool vis = Impl::Global::Mouse::g_visible;
					const bool inClip = Impl::Global::Mouse::g_inClip;
					Impl::Global::Mouse::UpdateMouseGraphic(vis, use, inClip);
					Impl::Global::Mouse::UpdateMouseClipping(use, vis, Impl::Global::Mouse::g_keepInWindow, inClip);
					if (use == false && inClip)
					{
						const Impl::Global::Mouse::MousePosition mp = Impl::Global::Mouse::g_pos;
						SetCursorPos(static_cast<int>(mp.X), static_cast<int>(mp.Y));
					}
				}
				CLOAKENGINE_API void CLOAK_CALL SetKeepInScreen(In bool keep)
				{
					const bool vis = Impl::Global::Mouse::g_visible;
					const bool sof = Impl::Global::Mouse::g_useSoftware;
					const bool inc = Impl::Global::Mouse::g_inClip;
					Impl::Global::Mouse::g_keepInWindow = keep;
					Impl::Global::Mouse::UpdateMouseClipping(sof, vis, keep, inc);
				}
				CLOAKENGINE_API void CLOAK_CALL SetMouseSpeed(In float speed)
				{
					Impl::Global::Mouse::g_mouseSpeed = speed;
				}
				CLOAKENGINE_API bool CLOAK_CALL IsVisible()
				{
					return Impl::Global::Mouse::g_visible;
				}
				CLOAKENGINE_API bool CLOAK_CALL IsSoftwareMouse()
				{
					return Impl::Global::Mouse::g_useSoftware;
				}
				CLOAKENGINE_API Interface::IBasicGUI* CLOAK_CALL GetSoftwareMouse()
				{
					return Impl::Global::Mouse::g_software;
				}
			}
		}
	}
}