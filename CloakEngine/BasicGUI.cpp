#include "stdafx.h"
#include "Implementation/Interface/BasicGUI.h"
#include "Implementation/Global/Mouse.h"
#include "Implementation/Global/Input.h"

#include "Engine/FlipVector.h"

#include <atomic>

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			std::atomic<API::Interface::BasicGUI_v1::IBasicGUI*> g_Focus;
			API::FlatSet<API::Interface::BasicGUI_v1::IBasicGUI*>* g_Shown;
			API::Helper::ISyncSection* g_SyncShown;
			API::Helper::ISyncSection* g_SyncUpdate;
			Engine::FlipVector<API::Interface::BasicGUI_v1::IBasicGUI*>* g_toUpdate;
			API::Interface::BasicGUI_v1::IBasicGUI** g_curUpd = nullptr;
			size_t g_curUpdSize = 0;

			void CLOAK_CALL Initialize()
			{
				g_Shown = new API::FlatSet<API::Interface::BasicGUI_v1::IBasicGUI*>();
				g_toUpdate = new Engine::FlipVector<API::Interface::BasicGUI_v1::IBasicGUI*>();
				CREATE_INTERFACE(CE_QUERY_ARGS(&g_SyncShown));
				CREATE_INTERFACE(CE_QUERY_ARGS(&g_SyncUpdate));
			}
			void CLOAK_CALL Release()
			{
				auto focus = g_Focus.load();
				SAVE_RELEASE(g_SyncShown);
				SAVE_RELEASE(g_SyncUpdate);
				SAVE_RELEASE(focus);
				for (auto a = g_Shown->begin(); a != g_Shown->end(); ++a) { SAVE_RELEASE(*a); }
				if (g_curUpd != nullptr)
				{
					for (size_t a = 0; a < g_curUpdSize; a++) { SAVE_RELEASE(g_curUpd[a]); }
					DeleteArray(g_curUpd);
				}
				for (size_t i = 0; i < 2; i++)
				{
					const API::List<API::Interface::BasicGUI_v1::IBasicGUI*>& upd = g_toUpdate->flip();
					for (size_t a = 0; a < upd.size(); a++) { SAVE_RELEASE(upd[a]); }
				}
				delete g_toUpdate;
				delete g_Shown;
			}
			API::Interface::IBasicGUI** CLOAK_CALL GetVisibleGUIs(In API::Global::Memory::PageStackAllocator* alloc, Out size_t* count)
			{
				API::Helper::Lock gl(Impl::Interface::g_SyncShown);
				const size_t s = g_Shown->size();
				API::Interface::IBasicGUI** res = reinterpret_cast<API::Interface::IBasicGUI**>(alloc->Allocate(sizeof(API::Interface::IBasicGUI*) * s));
				size_t a = 0;
				for (auto b = g_Shown->begin(); b != g_Shown->end(); ++b, ++a)
				{
					res[a] = *b;
					res[a]->AddRef();
				}
				*count = s;
				return res;
			}
			size_t CLOAK_CALL BeginToUpdate()
			{
				API::Helper::Lock lock(g_SyncUpdate);
				API::List<API::Interface::BasicGUI_v1::IBasicGUI*>& upd = g_toUpdate->flip();
				lock.unlock();
				g_curUpdSize = upd.size();
				if (g_curUpdSize > 0)
				{
					g_curUpd = NewArray(API::Interface::BasicGUI_v1::IBasicGUI*, g_curUpdSize);
					for (size_t a = 0; a < g_curUpdSize; a++) { g_curUpd[a] = upd[a]; }
				}
				else { g_curUpd = nullptr; }
				upd.clear();
				return g_curUpdSize;
			}
			API::Interface::IBasicGUI* CLOAK_CALL GetUpdateGUI(In size_t s)
			{
				assert(s < g_curUpdSize);
				return g_curUpd[s];
			}
			void CLOAK_CALL EndToUpdate()
			{
				for (size_t a = 0; a < g_curUpdSize; a++) { SAVE_RELEASE(g_curUpd[a]); }
				DeleteArray(g_curUpd);
				g_curUpd = nullptr;
			}

			namespace BasicGUI_v1 {
				unsigned int CLOAK_CALL operator&(In const API::Interface::BasicGUI_v1::AnchorPoint& a, In const API::Interface::BasicGUI_v1::AnchorPoint& b) { return ((unsigned int)a) & ((unsigned int)b); }
				bool CLOAK_CALL isParAllowed(In_opt const API::Interface::BasicGUI_v1::IBasicGUI* g, In_opt const API::Interface::BasicGUI_v1::IBasicGUI* par)
				{
					if (par == nullptr) { return true; }
					else if (g == par) { return false; }
					return isParAllowed(g, par->GetParent());
				}
				bool CLOAK_CALL isAnchorAllowed(In_opt const API::Interface::BasicGUI_v1::IBasicGUI* g, In const API::Interface::BasicGUI_v1::Anchor& anch)
				{
					if (anch.relativeTarget == nullptr) { return true; }
					else if (g == anch.relativeTarget) { return false; }
					return isAnchorAllowed(g, anch.relativeTarget->GetAnchor());
				}
				inline float CLOAK_CALL getAPXO(In API::Interface::BasicGUI_v1::AnchorPoint ap)
				{
					if ((ap & API::Interface::BasicGUI_v1::AnchorPoint::LEFT) != 0) { return 0.0f; }
					if ((ap & API::Interface::BasicGUI_v1::AnchorPoint::RIGHT) != 0) { return 1.0f; }
					return 0.5f;
				}
				inline float CLOAK_CALL getAPYO(In API::Interface::BasicGUI_v1::AnchorPoint ap)
				{
					if ((ap & API::Interface::BasicGUI_v1::AnchorPoint::TOP) != 0) { return 0.0f; }
					if ((ap & API::Interface::BasicGUI_v1::AnchorPoint::BOTTOM) != 0) { return 1.0f; }
					return 0.5f;
				}

				CLOAK_CALL_THIS BasicGUI::BasicGUI()
				{
					m_ZSize = 0;
					m_Z = 0;
					m_visible = false;
					m_parent = nullptr;
					m_W = 0;
					m_H = 0;
					m_Width = 0;
					m_Height = 0;
					m_sizeType = API::Interface::SizeType::NORMAL;
					m_rotation.alpha = 0;
					m_rotation.cos_a = 1;
					m_rotation.sin_a = 0;
					m_d_Z = 0;
					m_d_ZSize = 0;
					m_d_childs = nullptr;
					m_d_childSize = 0;
					m_d_visible = false;
					m_d_ios = false;
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncDraw));
					INIT_EVENT(onGainFocus, m_sync);
					INIT_EVENT(onResize, m_sync);
					INIT_EVENT(onLooseFocus, m_sync);
					INIT_EVENT(onParentChange, m_sync);
					INIT_EVENT(onMouseEnter, m_sync);
					INIT_EVENT(onMouseLeave, m_sync);
					DEBUG_NAME(BasicGUI);
				}
				CLOAK_CALL_THIS BasicGUI::~BasicGUI()
				{
					m_childs.for_each([=](API::Interface::BasicGUI_v1::IBasicGUI*& g) {API::Interface::IBasicGUI* const h = g; SAVE_RELEASE(h); });
					m_anchChilds.for_each([=](API::Interface::BasicGUI_v1::IBasicGUI*& g) {API::Interface::IBasicGUI* const h = g; SAVE_RELEASE(h); });
					SAVE_RELEASE(m_sync);
					SAVE_RELEASE(m_syncDraw);
					for (size_t a = 0; a < m_d_childSize; a++) { SAVE_RELEASE(m_d_childs[a]); }
					DeleteArray(m_d_childs);
				}
				void CLOAK_CALL_THIS BasicGUI::Show()
				{
					API::Helper::Lock lock(m_sync);
					if (m_visible == false) 
					{ 
						m_visible = true; 
						if (m_parent == nullptr) { API::Interface::setShown(this, true); }
						UpdatePosition();
					}
				}
				void CLOAK_CALL_THIS BasicGUI::Hide()
				{
					API::Helper::Lock lock(m_sync);
					if (m_visible == true) 
					{ 
						m_visible = false; 
						if (m_parent == nullptr) { API::Interface::setShown(this, false); }
						API::Interface::registerForUpdate(this);
						iHide();
					}
				}
				void CLOAK_CALL_THIS BasicGUI::SetParent(In_opt IBasicGUI* parent)
				{
					API::Helper::Lock lock(m_sync);
					if (m_parent != parent && CloakCheckOK(isParAllowed(this, parent), API::Global::Debug::Error::INTERFACE_ILLEGAL_PARENT, false))
					{
						if (parent != nullptr)
						{
							if (m_parent == nullptr && m_visible == true) { API::Interface::setShown(this, false); }
						}
						if (m_parent != nullptr)
						{

							BasicGUI* p = nullptr;
							m_parent->QueryInterface(CE_QUERY_ARGS(&p));
							if (p != nullptr) { p->UnregisterChild(this); p->Release(); }
						}
						API::Global::Events::onParentChange opc;
						opc.anchor = false;
						opc.lastParent = m_parent;
						opc.nextParent = parent;
						m_parent = parent;
						if (m_parent != nullptr) { m_parent->AddChild(this); }
						else if (m_visible == true) { API::Interface::setShown(this, true); }
						UpdatePosition();
						triggerEvent(opc);
					}
				}
				void CLOAK_CALL_THIS BasicGUI::SetSize(In float width, In float height, In_opt API::Interface::SizeType type)
				{
					API::Helper::Lock lock(m_sync);
					m_H = max(0, height);
					m_W = max(0, width);
					m_sizeType = type;
					UpdatePosition();
					API::Global::Events::onResize res;
					res.newHeight = m_Height;
					res.newWidth = m_Width;
					lock.unlock();
					triggerEvent(res);
				}
				void CLOAK_CALL_THIS BasicGUI::SetAnchor(In const API::Interface::BasicGUI_v1::Anchor& a)
				{
					API::Helper::Lock lock(m_sync);
					if (m_anch.relativeTarget != a.relativeTarget)
					{
						if (m_anch.relativeTarget != nullptr)
						{
							BasicGUI* p = nullptr;
							m_anch.relativeTarget->QueryInterface(CE_QUERY_ARGS(&p));
							if (p != nullptr) { p->UnregisterAnchChild(this); p->Release(); }
						}
						if (a.relativeTarget != nullptr)
						{
							BasicGUI* p = nullptr;
							a.relativeTarget->QueryInterface(CE_QUERY_ARGS(&p));
							if (p != nullptr) { p->RegisterAnchChild(this); p->Release(); }
						}
					}
					API::Global::Events::onParentChange opc;
					opc.anchor = true;
					opc.lastParent = m_anch.relativeTarget;
					opc.nextParent = a.relativeTarget;
					m_anch = a;
					UpdatePosition();
					if (opc.lastParent!=opc.nextParent) { triggerEvent(opc); }
				}
				void CLOAK_CALL_THIS BasicGUI::SetFocus()
				{
					BasicGUI* t = const_cast<BasicGUI*>(this);
					auto last = g_Focus.exchange(t);
					if (last != static_cast<API::Interface::IBasicGUI*>(t))
					{
						API::Global::Events::onGainFocus gain;
						gain.lastFocus = last;
						triggerEvent(gain);
						if (last != nullptr)
						{
							API::Global::Events::onLooseFocus loose;
							loose.nextFocus = t;
							last->triggerEvent(loose);
							last->Release();
						}
						t->AddRef();
						Impl::Global::Input::OnUpdateFocus(last, t);
					}
				}
				void CLOAK_CALL_THIS BasicGUI::LooseFocus()
				{
					API::Interface::BasicGUI_v1::IBasicGUI* p = this;
					if (g_Focus.compare_exchange_strong(p, (API::Interface::BasicGUI_v1::IBasicGUI*)nullptr))
					{
						API::Global::Events::onLooseFocus loose;
						loose.nextFocus = nullptr;
						triggerEvent(loose);
						Release();
						Impl::Global::Input::OnUpdateFocus(this, nullptr);
					}
				}
				void CLOAK_CALL_THIS BasicGUI::AddChild(In IBasicGUI* child) { RegisterChild(child); }
				void CLOAK_CALL_THIS BasicGUI::UpdatePosition()
				{
					API::Helper::Lock lock(m_sync);
					API::Global::Math::Space2D::Vector pH = { 1,0 };
					API::Global::Math::Space2D::Vector pV = { 0,1 };
					API::Global::Math::Space2D::Point pP = { 0,0 };
					if (m_anch.relativeTarget != nullptr)
					{
						pH.X = m_anch.relativeTarget->GetWidth();
						pV.Y = m_anch.relativeTarget->GetHeight();
						pP = m_anch.relativeTarget->GetPositionOnScreen();
						float r = m_anch.relativeTarget->GetRotation();
						if (r != 0)
						{
							const float sr = sin(r);
							const float cr = cos(r);
							const API::Global::Math::Point lH = { pH.X,pH.Y,0 };
							const API::Global::Math::Point lV = { pV.X,pV.Y,0 };
							pH.X = (cr*lH.X) - (sr*lH.Y);
							pH.Y = (sr*lH.X) + (cr*lH.Y);
							pV.X = (cr*lV.X) - (sr*lV.Y);
							pV.Y = (sr*lV.X) + (cr*lV.Y);
						}
					}
					m_Width = m_W;
					m_Height = m_H;
					switch (m_sizeType)
					{
						case CloakEngine::API::Interface::BasicGUI_v1::SizeType::RELATIVE_WIDTH:
							m_Width = m_W/static_cast<float>(API::Global::Graphic::GetAspectRatio());
							break;
						case CloakEngine::API::Interface::BasicGUI_v1::SizeType::RELATIVE_HEIGHT:
							m_Height = m_H*static_cast<float>(API::Global::Graphic::GetAspectRatio());
							break;
						case CloakEngine::API::Interface::BasicGUI_v1::SizeType::NORMAL:
						default:
							break;
					}
					API::Global::Math::Space2D::Vector mW = { m_Width,0 };
					API::Global::Math::Space2D::Vector mH = { 0,m_Height };
					if (m_rotation.alpha != 0)
					{
						const API::Global::Math::Point lH = { mW.X,mW.Y,0 };
						const API::Global::Math::Point lV = { mH.X,mH.Y,0 };
						mW.X = (m_rotation.cos_a*lH.X) - (m_rotation.sin_a*lH.Y);
						mW.Y = (m_rotation.sin_a*lH.X) + (m_rotation.cos_a*lH.Y);
						mH.X = (m_rotation.cos_a*lV.X) - (m_rotation.sin_a*lV.Y);
						mH.Y = (m_rotation.sin_a*lV.X) + (m_rotation.cos_a*lV.Y);
					}
					m_pos = pP + m_anch.offset + (pH*getAPXO(m_anch.relativePoint)) + (pV*getAPYO(m_anch.relativePoint)) - ((mW*getAPXO(m_anch.point)) + (mH*getAPYO(m_anch.point)));
					for (const auto& e : m_anchChilds) { e->UpdatePosition(); }
					API::Interface::registerForUpdate(this);
				}
				void CLOAK_CALL_THIS BasicGUI::SetRotation(In float alpha)
				{
					API::Helper::Lock lock(m_sync);
					m_rotation.alpha = alpha;
					m_rotation.cos_a = cos(alpha);
					m_rotation.sin_a = sin(alpha);
					UpdatePosition();
				}
				void CLOAK_CALL_THIS BasicGUI::SetZ(In UINT Z)
				{
					API::Helper::Lock lock(m_sync);
					m_Z = Z;
				}
				bool CLOAK_CALL_THIS BasicGUI::IsVisible() const { API::Helper::Lock lock(m_sync); return m_visible; }
				bool CLOAK_CALL_THIS BasicGUI::IsVisibleOnScreen() const { API::Helper::Lock lock(m_sync); return m_visible && (m_parent == nullptr || m_parent->IsVisibleOnScreen()) && IsOnScreen(); }
				bool CLOAK_CALL_THIS BasicGUI::IsOnScreen() const { API::Helper::Lock lock(m_sync); return m_pos.X < 1.0f && m_pos.X + m_Width >= 0.0f && m_pos.Y < 1.0f && m_pos.Y + m_Height >= 0.0f; }
				bool CLOAK_CALL_THIS BasicGUI::IsClickable() const { return false; }
				bool CLOAK_CALL_THIS BasicGUI::IsFocused() const { API::Helper::Lock lock(m_sync); return g_Focus.load() == this; }
				bool CLOAK_CALL_THIS BasicGUI::IsEnabled() const { return true; }
				float CLOAK_CALL_THIS BasicGUI::GetRotation() const { API::Helper::Lock lock(m_sync); return m_rotation.alpha; }
				float CLOAK_CALL_THIS BasicGUI::GetWidth() const { API::Helper::Lock lock(m_sync); return m_Width; }
				float CLOAK_CALL_THIS BasicGUI::GetHeight() const { API::Helper::Lock lock(m_sync); return m_Height; }
				Ret_maybenull API::Interface::BasicGUI_v1::IBasicGUI* CLOAK_CALL_THIS BasicGUI::GetParent() const { API::Helper::Lock lock(m_sync); return m_parent; }
				const API::Interface::BasicGUI_v1::Anchor& CLOAK_CALL_THIS BasicGUI::GetAnchor() const { API::Helper::Lock lock(m_sync); return m_anch; }
				API::Global::Math::Space2D::Point CLOAK_CALL_THIS BasicGUI::GetPositionOnScreen() const { API::Helper::Lock lock(m_sync); return m_pos; }

				void CLOAK_CALL_THIS BasicGUI::RegisterChild(In API::Interface::BasicGUI_v1::IBasicGUI* c)
				{
					API::Helper::Lock lock(m_sync);
					if (m_childs.contains(c) == false) 
					{
						m_childs.push(c);
						c->SetParent(this);
						c->AddRef();
					}
				}
				void CLOAK_CALL_THIS BasicGUI::RegisterAnchChild(In API::Interface::BasicGUI_v1::IBasicGUI* c)
				{
					API::Helper::Lock lock(m_sync);
					if (m_anchChilds.contains(c) == false)
					{
						m_anchChilds.push(c);
						c->AddRef();
					}
				}
				void CLOAK_CALL_THIS BasicGUI::UnregisterChild(In API::Interface::BasicGUI_v1::IBasicGUI* c)
				{
					API::Helper::Lock lock(m_sync);
					Engine::LinkedList<API::Interface::BasicGUI_v1::IBasicGUI*>::iterator pos = m_childs.find(c);
					if (pos != nullptr)
					{ 
						m_childs.remove(pos); 
						c->Release();
					}
				}
				void CLOAK_CALL_THIS BasicGUI::UnregisterAnchChild(In API::Interface::BasicGUI_v1::IBasicGUI* c)
				{
					API::Helper::Lock lock(m_sync);
					Engine::LinkedList<API::Interface::BasicGUI_v1::IBasicGUI*>::iterator pos = m_anchChilds.find(c);
					if (pos != nullptr)
					{
						m_anchChilds.remove(pos);
						c->Release();
					}
				}
				API::Global::Math::Space2D::Point CLOAK_CALL_THIS BasicGUI::GetRelativePos(In const API::Global::Math::Space2D::Point& pos) const
				{
					API::Helper::Lock lock(m_sync);
					const API::Global::Math::Space2D::Point cen = { m_pos.X + (m_Width / 2.0f),m_pos.Y + (m_Height / 2.0f) };
					API::Global::Math::Space2D::Point rp = { pos.X - cen.X,pos.Y - cen.Y };
					if (m_rotation.alpha != 0)
					{
						API::Global::Math::Space2D::Point tp = { rp.X,rp.Y };
						rp.X = (m_rotation.cos_a*tp.X) + (m_rotation.sin_a*tp.Y);
						rp.Y = (m_rotation.cos_a*tp.Y) - (m_rotation.sin_a*tp.X);
					}
					rp.X += cen.X;
					rp.Y += cen.Y;
					API::Global::Math::Space2D::Point sp;
					sp.X = (rp.X - m_pos.X) / m_Width;
					sp.Y = (rp.Y - m_pos.Y) / m_Height;
					return sp;
				}
				void CLOAK_CALL_THIS BasicGUI::Draw(In API::Rendering::Context_v1::IContext2D* context)
				{
					API::Helper::ReadLock lock(m_syncDraw);
					if (m_d_visible && context != nullptr)
					{
						context->StartNewPatch(m_d_Z);
						if (m_d_ios) { iDraw(context); }
						if (m_d_childSize > 0)
						{
							API::Interface::IBasicGUI** childs = NewArray(API::Interface::IBasicGUI*, m_d_childSize);
							for (size_t a = 0; a < m_d_childSize; a++) { childs[a] = m_d_childs[a]; }
							const size_t childSize = m_d_childSize;
							context->StartNewPatch(m_d_ZSize + 1);
							lock.unlock();
							for (size_t a = 0; a < childSize; a++)
							{
								childs[a]->Draw(context);
							}
							context->EndPatch();
							DeleteArray(childs);
						}
						context->EndPatch();
					}
				}
				void CLOAK_CALL_THIS BasicGUI::UpdateDrawInformations(In API::Global::Time time, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager)
				{ 
					if (m_updateFlag.exchange(false) == true)
					{
						API::Helper::WriteLock lockDraw(m_syncDraw);
						API::Helper::Lock lock(m_sync);
						m_d_visible = m_visible;
						if (m_visible) 
						{
							m_d_Z = m_Z;
							m_d_ZSize = m_ZSize;
							m_d_ios = IsOnScreen();
							const size_t childSize = m_childs.size();
							if (childSize > m_d_childSize)
							{
								for (size_t a = 0; a < m_d_childSize; a++) { SAVE_RELEASE(m_d_childs[a]); }
								DeleteArray(m_d_childs);
								m_d_childs = NewArray(API::Interface::IBasicGUI*, childSize);
								m_d_childSize = childSize;
								size_t p = 0;
								for (const auto& g : m_childs)
								{
									g->AddRef();
									m_d_childs[p] = g;
									p++;
								}
								for (size_t a = p; a < childSize; a++) { m_d_childs[a] = nullptr; }
							}
							else
							{
								size_t p = 0;
								for (const auto& g : m_childs)
								{
									if (m_d_childs[p] != g)
									{
										g->AddRef();
										m_d_childs[p]->Release();
										m_d_childs[p] = g;
									}
									p++;
								}
								for (size_t a = p; a < m_d_childSize; a++) { SAVE_RELEASE(m_d_childs[a]); }
							}
							if (iUpdateDrawInfo(time)) { API::Interface::registerForUpdate(this); }
						}
					}
				}
				bool CLOAK_CALL_THIS BasicGUI::SetUpdateFlag() { return m_updateFlag.exchange(true); }

				Success(return == true) bool CLOAK_CALL_THIS BasicGUI::iQueryInterface(In REFIID id, Outptr void** ptr)
				{
					CLOAK_QUERY(id, ptr, SavePtr, Interface::BasicGUI_v1, BasicGUI);
				}
				bool CLOAK_CALL_THIS BasicGUI::iUpdateDrawInfo(In unsigned long long time) { return false; }
				void CLOAK_CALL_THIS BasicGUI::iDraw(In API::Rendering::Context_v1::IContext2D* context) {}
				void CLOAK_CALL_THIS BasicGUI::iHide()
				{
					API::Helper::Lock lock(m_sync);
					LooseFocus();
					m_childs.for_each([=](API::Interface::BasicGUI_v1::IBasicGUI*& g) 
					{
						Impl::Interface::BasicGUI* h = nullptr;
						if (SUCCEEDED(g->QueryInterface(CE_QUERY_ARGS(&h))))
						{
							h->iHide();
							h->Release();
						}
					});
				}
			}
		}
	}
	namespace API {
		namespace Interface {
			Ret_maybenull CLOAKENGINE_API IBasicGUI* CLOAK_CALL getFocused()
			{
				IBasicGUI* g = Impl::Interface::g_Focus.load();
				if (g != nullptr) { g->AddRef(); }
				return g;
			}
			CLOAKENGINE_API void CLOAK_CALL setShown(In IBasicGUI* gui, In bool visible)
			{
				if (CloakCheckOK(gui != nullptr, Global::Debug::Error::ILLEGAL_ARGUMENT, false))
				{
					API::Helper::Lock gl(Impl::Interface::g_SyncShown);
					size_t p = 0;
					const bool f = Impl::Interface::g_Shown->find(gui) != Impl::Interface::g_Shown->end();
					if (visible == true && gui->GetParent() == nullptr)
					{
						if (f == false)
						{
							gui->AddRef();
							Impl::Interface::g_Shown->insert(gui);
						}
					}
					else if (f == true)
					{
						Impl::Interface::g_Shown->erase(gui);
						gui->Release();
					}
					if (visible) { API::Interface::registerForUpdate(gui); }
				}
			}
			CLOAKENGINE_API void CLOAK_CALL registerForUpdate(In IBasicGUI* gui)
			{
				if (gui != nullptr)
				{
					API::Helper::Lock lock(Impl::Interface::g_SyncUpdate);
					if (gui->SetUpdateFlag() == false)
					{
						gui->AddRef();
						Impl::Interface::g_toUpdate->get()->push_back(gui);
					}
				}
			}
		}
	}
}