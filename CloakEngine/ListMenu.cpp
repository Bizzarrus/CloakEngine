#include "stdafx.h"
#include "Implementation/Interface/ListMenu.h"

#include "CloakEngine/Global/Game.h"

#define SAVE_SWAP(src,dst) if(dst!=src){SAVE_RELEASE(dst); dst=src; if(src!=nullptr){src->AddRef();}}

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace ListMenu_v1 {
				CLOAK_CALL ListMenu::ListMenu()
				{
					INIT_EVENT(onMenuButtonClick, m_sync);
					m_usedPoolSize = 0;
					m_stepSize = 0;
					m_btnW = 0;
					m_btnH = 0;
					m_btnSymbW = 0;
					m_btnLetterSpace = 0;
					m_btnLineSpace = 0;
					m_imgBtnBackground = nullptr;
					m_imgBtnChecked = nullptr;
					m_imgBtnHighlight = nullptr;
					m_imgBtnSubMenuOpen = nullptr;
					m_imgBtnSubMenuClosed = nullptr;
					m_btnFont = nullptr;
					m_btnFontSize = 0;
					m_btnJustifyH = API::Files::Justify::LEFT;
					m_btnJustifyV = API::Files::AnchorPoint::CENTER;
					m_btnHighlightType = API::Interface::HighlightType::OVERRIDE;
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_slider));
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_frame));
					m_frame->SetParent(this);
					m_slider->SetParent(this);
					API::Interface::Anchor anch;
					anch.point = API::Interface::AnchorPoint::TOPLEFT;
					anch.relativePoint = API::Interface::AnchorPoint::TOPLEFT;
					anch.offset.X = 0;
					anch.offset.Y = 0;
					anch.relativeTarget = this;
					m_frame->SetAnchor(anch);
					anch.point = API::Interface::AnchorPoint::TOPLEFT;
					anch.relativePoint = API::Interface::AnchorPoint::TOPRIGHT;
					m_slider->SetAnchor(anch);
					m_slider->SetRotation(static_cast<float>(API::Global::Math::PI / 2));
					m_slider->SetMin(0);
					m_slider->Hide();
					m_frame->Show();
				}
				CLOAK_CALL ListMenu::~ListMenu()
				{
					for (size_t a = 0; a < m_pool.size(); a++) { SAVE_RELEASE(m_pool[a]); }
					SAVE_RELEASE(m_slider);
					SAVE_RELEASE(m_frame);
					SAVE_RELEASE(m_imgBtnBackground);
					SAVE_RELEASE(m_imgBtnChecked);
					SAVE_RELEASE(m_imgBtnHighlight);
					SAVE_RELEASE(m_imgBtnSubMenuOpen);
					SAVE_RELEASE(m_imgBtnSubMenuClosed);
					SAVE_RELEASE(m_btnFont);
				}
				void CLOAK_CALL_THIS ListMenu::SetMenu(In const API::Interface::Menu_v1::MENU_DESC& desc)
				{
					if (CloakCheckOK(desc.IsValid(), API::Global::Debug::Error::INTERFACE_MENU_NOT_VALID, false))
					{
						API::Helper::Lock lock(m_sync);
						for (size_t a = 0; a < m_usedPoolSize; a++) { m_pool[a]->Reset(); }
						m_usedPoolSize = 0;

						size_t pos = 0;
						for (size_t a = 0; a < desc.MenuSize; a++)
						{
							MenuButton* b = GetBtnFromPool();
							pos = 1 + b->SetMenu(desc.Menu[a], pos, 0, m_openState);
						}
						size_t curLvl = 0;
						for (size_t a = 0; a < m_usedPoolSize; a++)
						{
							curLvl = m_pool[a]->TryShow(curLvl);
						}
						iUpdateItemPositions();
					}
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonTexture(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					SAVE_SWAP(img, m_imgBtnBackground);
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetBackgroundTexture(img); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonHighlightTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight)
				{
					API::Helper::Lock lock(m_sync);
					m_btnHighlightType = highlight;
					SAVE_SWAP(img, m_imgBtnHighlight);
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetHighlightTexture(img, highlight); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonCheckTexture(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					SAVE_SWAP(img, m_imgBtnChecked);
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetCheckedTexture(img); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonSubMenuTexture(In API::Files::ITexture* open, In API::Files::ITexture* closed)
				{
					API::Helper::Lock lock(m_sync);
					SAVE_SWAP(open, m_imgBtnSubMenuOpen);
					SAVE_SWAP(closed, m_imgBtnSubMenuClosed);
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetSubMenuTexture(open, closed); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonWidth(In float W)
				{
					API::Helper::Lock lock(m_sync);
					m_btnW = W;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetSize(m_btnW,m_btnH); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonHeight(In float H)
				{
					API::Helper::Lock lock(m_sync);
					m_btnH = H;
					for (size_t a = 0; a < m_pool.size(); a++) 
					{ 
						MenuButton* const b = m_pool[a];
						b->SetSize(m_btnW, m_btnH);
						b->UpdateListAnchor(); 
					}
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonSize(In float W, In float H)
				{
					API::Helper::Lock lock(m_sync);
					m_btnW = W;
					m_btnH = H;
					for (size_t a = 0; a < m_pool.size(); a++)
					{
						MenuButton* const b = m_pool[a];
						b->SetSize(m_btnW, m_btnH);
						b->UpdateListAnchor();
					}
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonFont(In API::Files::IFont* font)
				{
					API::Helper::Lock lock(m_sync);
					if (m_btnFont != nullptr) { m_btnFont->unload(); }
					SAVE_RELEASE(m_btnFont);
					m_btnFont = font;
					if (m_btnFont != nullptr)
					{
						m_btnFont->AddRef();
						m_btnFont->load();
					}
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetFont(m_btnFont); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonFontSize(In API::Files::FontSize size)
				{
					API::Helper::Lock lock(m_sync);
					m_btnFontSize = size;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetFontSize(m_btnFontSize); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonFontColor(In const API::Interface::FontColor& color)
				{
					API::Helper::Lock lock(m_sync);
					for (unsigned int a = 0; a < 3; a++) { m_btnFontColor.Color[a] = color.Color[a]; }
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetFontColor(m_btnFontColor); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonJustify(In API::Files::Justify hJust)
				{
					API::Helper::Lock lock(m_sync);
					m_btnJustifyH = hJust;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetJustify(m_btnJustifyH); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonJustify(In API::Files::Justify hJust, In API::Files::AnchorPoint vJust)
				{
					API::Helper::Lock lock(m_sync);
					m_btnJustifyH = hJust;
					m_btnJustifyV = vJust;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetJustify(m_btnJustifyH, m_btnJustifyV); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonJustify(In API::Files::AnchorPoint vJust)
				{
					API::Helper::Lock lock(m_sync);
					m_btnJustifyV = vJust;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetJustify(m_btnJustifyV); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonLetterSpace(In float space)
				{
					API::Helper::Lock lock(m_sync);
					m_btnLetterSpace = space;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetLetterSpace(m_btnLetterSpace); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonLineSpace(In float space)
				{
					API::Helper::Lock lock(m_sync);
					m_btnLineSpace = space;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetLineSpace(m_btnLineSpace); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonSymbolWidth(In float W)
				{
					API::Helper::Lock lock(m_sync);
					m_btnSymbW = W;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetSymbolWidth(W); }
				}
				void CLOAK_CALL_THIS ListMenu::SetButtonTextOffset(In const API::Global::Math::Space2D::Vector& offset)
				{
					API::Helper::Lock lock(m_sync);
					m_btnTextOffset = offset;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetTextOffset(offset); }
				}
				void CLOAK_CALL_THIS ListMenu::SetLevelStepSize(In float s)
				{
					API::Helper::Lock lock(m_sync);
					m_stepSize = s;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->UpdateListAnchor(); }
				}
				void CLOAK_CALL_THIS ListMenu::SetSliderWidth(In float w)
				{
					API::Helper::Lock lock(m_sync);
					m_sliderW = w;
					if (m_slider->IsVisible())
					{
						m_slider->SetSize(GetHeight(), m_sliderW, m_sizeType);
						m_frame->SetSize(GetWidth() - m_sliderW, GetHeight(), m_sizeType);
					}
				}
				void CLOAK_CALL_THIS ListMenu::SetSliderTexture(In API::Files::ITexture* img)
				{
					m_slider->SetBackgroundTexture(img);
				}
				void CLOAK_CALL_THIS ListMenu::SetSliderThumbTexture(In API::Files::ITexture* img)
				{
					m_slider->SetThumbTexture(img);
				}
				void CLOAK_CALL_THIS ListMenu::SetSliderLeftArrowTexture(In API::Files::ITexture* img)
				{
					m_slider->SetLeftArrowTexture(img);
				}
				void CLOAK_CALL_THIS ListMenu::SetSliderRightArrowTexture(In API::Files::ITexture* img)
				{
					m_slider->SetRightArrowTexture(img);
				}
				void CLOAK_CALL_THIS ListMenu::SetSliderArrowHeight(In float w)
				{
					m_slider->SetArrowWidth(w);
				}
				void CLOAK_CALL_THIS ListMenu::SetSliderThumbHeight(In float w)
				{
					m_slider->SetThumbWidth(w);
				}
				void CLOAK_CALL_THIS ListMenu::SetSliderStepSize(In float ss)
				{
					m_slider->SetStepSize(ss);
				}
				bool CLOAK_CALL_THIS ListMenu::IsClickable() const { return true; }

				Success(return == true) bool CLOAK_CALL_THIS ListMenu::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Interface::Menu_v1::IMenu)) { *ptr = (API::Interface::Menu_v1::IMenu*)this; return true; }
					else if (riid == __uuidof(API::Interface::ListMenu_v1::IListMenu)) { *ptr = (API::Interface::ListMenu_v1::IListMenu*)this; return true; }
					else if (riid == __uuidof(ListMenu)) { *ptr = (ListMenu*)this; return true; }
					return Frame::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS ListMenu::iSetOpenState(In API::Interface::ButtonID id, In bool state)
				{
					API::Helper::Lock lock(m_sync);
					if (state) { m_openState.insert(id); }
					else { m_openState.erase(id); }
					iUpdateItemPositions();
				}
				void CLOAK_CALL_THIS ListMenu::iUpdateItemPositions()
				{
					API::Helper::Lock lock(m_sync);
					size_t p = 0;
					for (size_t a = 0; a < m_usedPoolSize; a++)
					{
						MenuButton* b = m_pool[a];
						b->SetOffset(p);
						if (b->IsVisible()) { p++; }
					}
					iCheckSlider();
				}
				void CLOAK_CALL_THIS ListMenu::iCheckSlider()
				{
					const float maxVis = GetHeight() / m_btnH;
					if (m_usedPoolSize > static_cast<size_t>(maxVis))
					{
						m_slider->Show();
						m_slider->SetMax((m_usedPoolSize - maxVis)*m_btnH);
						m_slider->SetSize(GetHeight(), m_sliderW, m_sizeType);
						m_frame->SetSize(GetWidth() - m_sliderW, GetHeight(), m_sizeType);
					}
					else
					{
						m_slider->Hide();
						m_frame->SetSize(GetWidth(), GetHeight(), m_sizeType);
					}
				}
				float CLOAK_CALL_THIS ListMenu::GetStepSize() const
				{
					API::Helper::Lock lock(m_sync);
					return m_stepSize;
				}
				ListMenu::MenuButton* CLOAK_CALL_THIS ListMenu::GetBtnFromPool()
				{
					API::Helper::Lock lock(m_sync);
					if (m_usedPoolSize >= m_pool.size())
					{
						MenuButton* b = new MenuButton(this, m_usedPoolSize);
						b->Initialize();
						m_pool.push_back(b);
						b->SetParent(m_frame);
						b->SetSize(m_btnW, m_btnH);
						b->SetBackgroundTexture(m_imgBtnBackground);
						b->SetCheckedTexture(m_imgBtnChecked);
						b->SetHighlightTexture(m_imgBtnHighlight, m_btnHighlightType);
						b->SetSubMenuTexture(m_imgBtnSubMenuOpen, m_imgBtnSubMenuClosed);
						b->SetSymbolWidth(m_btnSymbW);
						b->SetTextOffset(m_btnTextOffset);
						b->SetFont(m_btnFont);
						b->SetFontSize(m_btnFontSize);
						b->SetFontColor(m_btnFontColor);
						b->SetJustify(m_btnJustifyH, m_btnJustifyV);
						b->SetLetterSpace(m_btnLetterSpace);
						b->SetLineSpace(m_btnLineSpace);
					}
					return m_pool[m_usedPoolSize++];
				}
				ListMenu::MenuButton* CLOAK_CALL_THIS ListMenu::GetBtnFromPool(In size_t index)
				{
					API::Helper::Lock lock(m_sync);
					if (index < m_usedPoolSize) { return m_pool[index]; }
					return nullptr;
				}
				API::Interface::IMoveableFrame* CLOAK_CALL_THIS ListMenu::GetMovableFrame() const
				{
					return m_frame;
				}


				CLOAK_CALL ListMenu::MenuButton::MenuButton(In ListMenu* par, In size_t poolPos) : m_menuPar(par), m_poolPos(poolPos)
				{
					m_listPos = 0;
					m_lvl = 0;
					m_open = false;
					m_subMenu = false;
					m_checked = false;
					m_imgChecked = nullptr;
					m_imgSubMenuOpen = nullptr;
					m_imgSubMenuClosed = nullptr;
					m_symWidth = 0;
					m_canChecked = false;
				}
				CLOAK_CALL ListMenu::MenuButton::~MenuButton()
				{
					SAVE_RELEASE(m_imgChecked);
					SAVE_RELEASE(m_imgSubMenuOpen);
					SAVE_RELEASE(m_imgSubMenuClosed);
				}
				void CLOAK_CALL_THIS ListMenu::MenuButton::Reset()
				{
					API::Helper::Lock lock(m_sync);
					Hide();
					m_listPos = 0;
					m_lvl = 0;
					m_ID = 0;
					m_open = false;
					m_listOffset = 0;
					m_subMenu = false;
					m_checked = false;
					m_canChecked = false;
					//TODO
				}
				void CLOAK_CALL_THIS ListMenu::MenuButton::Open()
				{
					API::Helper::Lock lock(m_sync);
					if (m_open == false && m_subMenu == true)
					{
						m_open = true;
						size_t lvl = m_lvl + 1;
						size_t i = m_poolPos + 1;
						do {
							MenuButton* b = m_menuPar->GetBtnFromPool(i);
							if (b == nullptr) { lvl = 0; }
							else { lvl = b->TryShow(lvl); }
							i++;
						} while (lvl > m_lvl);
						m_menuPar->iSetOpenState(m_ID, true);
					}
				}
				void CLOAK_CALL_THIS ListMenu::MenuButton::Close()
				{
					API::Helper::Lock lock(m_sync);
					if (m_open == true)
					{
						m_open = false;
						bool f = true;
						size_t i = m_poolPos + 1;
						do {
							MenuButton* b = m_menuPar->GetBtnFromPool(i);
							if (b == nullptr) { f = false; }
							else { f = b->TryHide(m_lvl); }
							i++;
						} while (f == true);
						m_menuPar->iSetOpenState(m_ID, false);
					}
				}
				void CLOAK_CALL_THIS ListMenu::MenuButton::SetOffset(In size_t pos)
				{
					API::Helper::Lock lock(m_sync);
					m_listOffset = pos;
					UpdateListAnchor();
				}
				void CLOAK_CALL_THIS ListMenu::MenuButton::UpdateListAnchor()
				{
					API::Helper::Lock lock(m_sync);
					API::Interface::Anchor anch;
					anch.point = API::Interface::AnchorPoint::TOPLEFT;
					anch.relativePoint = API::Interface::AnchorPoint::TOPLEFT;
					anch.relativeTarget = m_menuPar->GetMovableFrame();
					anch.offset.X = m_lvl*m_menuPar->GetStepSize();
					anch.offset.Y = m_listOffset*GetHeight();
					SetAnchor(anch);
				}
				void CLOAK_CALL_THIS ListMenu::MenuButton::SetCheckedTexture(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(img, m_imgChecked);
				}
				void CLOAK_CALL_THIS ListMenu::MenuButton::SetSubMenuTexture(In API::Files::ITexture* open, In API::Files::ITexture* closed)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(open, m_imgSubMenuOpen);
					GUI_SET_TEXTURE(closed, m_imgSubMenuClosed);
				}
				void CLOAK_CALL_THIS ListMenu::MenuButton::SetSymbolWidth(In float w)
				{
					API::Helper::Lock lock(m_sync);
					m_symWidth = w;
					iUpdateTextPos();
				}
				void CLOAK_CALL_THIS ListMenu::MenuButton::SetTextOffset(In const API::Global::Math::Space2D::Vector& offset)
				{
					API::Helper::Lock lock(m_sync);
					m_baseTextOff = offset;
					iUpdateTextPos();
				}
				bool CLOAK_CALL_THIS ListMenu::MenuButton::TryHide(In size_t lvl)
				{
					API::Helper::Lock lock(m_sync);
					if (m_lvl > lvl)
					{
						Hide();
						return true;
					}
					return false;
				}
				size_t CLOAK_CALL_THIS ListMenu::MenuButton::TryShow(In size_t lvl)
				{
					API::Helper::Lock lock(m_sync);
					if (m_lvl <= lvl) 
					{
						Show();
						if (m_open) { return m_lvl + 1; }
						return m_lvl;
					}
					return lvl;
				}
				size_t CLOAK_CALL_THIS ListMenu::MenuButton::SetMenu(In const API::Interface::MENU_ITEM& menu, In size_t pos, In size_t lvl, In const std::unordered_set<API::Interface::ButtonID>& state)
				{
					API::Helper::Lock lock(m_sync);
					m_listPos = pos;
					m_lvl = lvl;
					m_ID = menu.ID;
					m_open = state.find(m_ID) != state.end();
					SetText(menu.Text);
					m_checked = menu.Checked && menu.CanChecked;
					m_canChecked = menu.CanChecked;
					//TODO
					if (menu.SubMenuSize > 0)
					{
						m_subMenu = true;
						for (size_t a = 0; a < menu.SubMenuSize; a++)
						{
							MenuButton* b = m_menuPar->GetBtnFromPool();
							pos = b->SetMenu(menu, pos + 1, lvl + 1, state);
						}
					}
					else
					{
						m_subMenu = false;
					}
					return pos;
				}

				bool CLOAK_CALL_THIS ListMenu::MenuButton::iUpdateDrawInfo(In unsigned long long time)
				{
					m_subMenuVertex.Position = m_pos;
					m_subMenuVertex.Color[0] = API::Helper::Color::White;
					m_subMenuVertex.Size.X = m_symWidth;
					m_subMenuVertex.Size.Y = m_Height;
					m_subMenuVertex.TexBottomRight.X = 1;
					m_subMenuVertex.TexBottomRight.Y = 1;
					m_subMenuVertex.TexTopLeft.X = 0;
					m_subMenuVertex.TexTopLeft.Y = 0;
					m_subMenuVertex.Rotation.Sinus = m_rotation.sin_a;
					m_subMenuVertex.Rotation.Cosinus = m_rotation.cos_a;
					m_subMenuVertex.Flags = API::Rendering::Vertex2DFlags::NONE;

					if (m_subMenu)
					{
						m_checkedVertex.Position.X = m_pos.X + (m_rotation.cos_a*m_symWidth);
						m_checkedVertex.Position.Y = m_pos.Y + (m_rotation.sin_a*m_symWidth);
					}
					else
					{
						m_checkedVertex.Position = m_pos;
					}
					m_checkedVertex.Color[0] = API::Helper::Color::White;
					m_checkedVertex.Size.X = m_symWidth;
					m_checkedVertex.Size.Y = m_Height;
					m_checkedVertex.TexBottomRight.X = 1;
					m_checkedVertex.TexBottomRight.Y = 1;
					m_checkedVertex.TexTopLeft.X = 0;
					m_checkedVertex.TexTopLeft.Y = 0;
					m_checkedVertex.Rotation.Sinus = m_rotation.sin_a;
					m_checkedVertex.Rotation.Cosinus = m_rotation.cos_a;
					m_checkedVertex.Flags = API::Rendering::Vertex2DFlags::NONE;

					m_d_checkedInfo.Update(m_imgChecked, m_checked);
					m_d_subMenuOpenInfo.Update(m_imgSubMenuOpen, m_subMenu && m_open == true);
					m_d_subMenuClosedInfo.Update(m_imgSubMenuClosed, m_subMenu && m_open == false);

					return Button::iUpdateDrawInfo(time);
				}
				void CLOAK_CALL_THIS ListMenu::MenuButton::iDraw(In API::Rendering::Context_v1::IContext2D* context)
				{
					Button::iDraw(context);
					if (m_d_checkedInfo.CanDraw())
					{
						context->Draw(this, m_d_checkedInfo.GetImage(), m_checkedVertex, 5);
					}
					if (m_d_subMenuOpenInfo.CanDraw())
					{
						context->Draw(this, m_d_subMenuOpenInfo.GetImage(), m_subMenuVertex, 5);
					}
					else if (m_d_subMenuClosedInfo.CanDraw())
					{
						context->Draw(this, m_d_subMenuClosedInfo.GetImage(), m_subMenuVertex, 5);
					}
				}
				void CLOAK_CALL_THIS ListMenu::MenuButton::iUpdateTextPos()
				{
					m_textOffset.X = m_baseTextOff.X + (m_canChecked ? m_symWidth : 0) + (m_subMenu ? m_symWidth : 0);
					m_textOffset.Y = m_baseTextOff.Y;
				}
			}
		}
	}
}