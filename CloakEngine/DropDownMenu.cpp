#include "stdafx.h"
#include "Implementation/Interface/DropDownMenu.h"

#include "CloakEngine/Global/Debug.h"
#include <atomic>

#define SAVE_SWAP(src,dst) if(dst!=src){SAVE_RELEASE(dst); dst=src; if(src!=nullptr){src->AddRef();}}

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace DropDownMenu_v1 {
				CLOAK_CALL DropDownMenu::DropDownMenu()
				{
					m_usedPoolSize = 0;
					m_open = false;
					m_btnBackground = nullptr;
					m_btnChecked = nullptr;
					m_btnHighlight = nullptr;
					m_btnHighlightType = API::Interface::HighlightType::OVERRIDE;
					m_btnSubMenuOpen = nullptr;
					m_btnSubMenuClosed = nullptr;
					m_btnTextOffset = { 0,0 };
					m_btnWidth = 0;
					m_btnHeight = 0;
					m_btnSymbolWidth = 0;
					m_btnMaxLen = 0;
					m_stateSize = 0;
					m_btnLetterSpace = 0;
					m_btnLineSpace = 0;
					m_btnFont = nullptr;
					m_btnFontSize = 0;
					m_btnJustifyH = API::Files::Justify::LEFT;
					m_btnJustifyV = API::Files::AnchorPoint::CENTER;
					INIT_EVENT(onMenuButtonClick, m_sync);
				}
				CLOAK_CALL DropDownMenu::~DropDownMenu()
				{
					for (size_t a = 0; a < m_pool.size(); a++)
					{
						SAVE_RELEASE(m_pool[a]);
					}
					SAVE_RELEASE(m_btnBackground);
					SAVE_RELEASE(m_btnChecked);
					SAVE_RELEASE(m_btnHighlight);
					SAVE_RELEASE(m_btnSubMenuOpen);
					SAVE_RELEASE(m_btnSubMenuClosed);
					SAVE_RELEASE(m_btnFont);
				}
				void CLOAK_CALL_THIS DropDownMenu::SetMenu(In const API::Interface::Menu_v1::MENU_DESC& desc)
				{
					if (CloakCheckOK(desc.IsValid(), API::Global::Debug::Error::INTERFACE_MENU_NOT_VALID, false))
					{
						API::Helper::Lock lock(m_sync);
						for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->ResetMenu(); }
						m_usedPoolSize = 0;
						m_menu.clear();
						MenuAnchor anchor;
						anchor.CanCheck = false;
						for (size_t a = 0; a < desc.MenuSize; a++)
						{
							if (desc.Menu[a].CanChecked) 
							{ 
								anchor.CanCheck = true; 
								break;
							}
						}
						for (size_t a = 0; a < desc.MenuSize; a++)
						{
							MenuButton* b = GetBtnFromPool();
							m_menu.push_back(b);
							anchor.MenuNum = a;
							anchor.Point = API::Interface::AnchorPoint::TOPLEFT;
							anchor.RelativePoint = API::Interface::AnchorPoint::BOTTOMLEFT;
							anchor.RelativeAnchor = this;
							b->SetMenu(desc.Menu[a], anchor, 0);
						}
						if (m_open && m_stateSize > 0)
						{
							for (size_t a = 0; a < m_menu.size(); a++)
							{
								m_menu[a]->SetOpenState(m_openState, 0, m_stateSize);
							}
						}
					}
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonTexture(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					SAVE_SWAP(img, m_btnBackground);
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetBackgroundTexture(img); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonHighlightTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight)
				{
					API::Helper::Lock lock(m_sync);
					SAVE_SWAP(img, m_btnHighlight);
					m_btnHighlightType = highlight;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetHighlightTexture(img, highlight); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonCheckTexture(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					SAVE_SWAP(img, m_btnChecked);
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetCheckTexture(img); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonSubMenuTexture(In API::Files::ITexture* open, In API::Files::ITexture* closed)
				{
					API::Helper::Lock lock(m_sync);
					SAVE_SWAP(open, m_btnSubMenuOpen);
					SAVE_SWAP(closed, m_btnSubMenuClosed);
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetSubMenuTexture(open, closed); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonWidth(In float W)
				{
					API::Helper::Lock lock(m_sync);
					SetButtonSize(W, m_btnHeight);
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonHeight(In float H)
				{
					API::Helper::Lock lock(m_sync);
					SetButtonSize(m_btnWidth, H);
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonSize(In float W, In float H)
				{
					API::Helper::Lock lock(m_sync);
					m_btnWidth = W;
					m_btnHeight = H;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetSize(W, H); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonSymbolWidth(In float W)
				{
					API::Helper::Lock lock(m_sync);
					m_btnSymbolWidth = W;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetSymbolWidth(W); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonFont(In API::Files::IFont* font)
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
				void CLOAK_CALL_THIS DropDownMenu::SetButtonFontSize(In API::Files::FontSize size)
				{
					API::Helper::Lock lock(m_sync);
					m_btnFontSize = size;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetFontSize(m_btnFontSize); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonFontColor(In const API::Interface::FontColor& color)
				{
					API::Helper::Lock lock(m_sync);
					for (unsigned int a = 0; a < 3; a++) { m_btnFontColor.Color[a] = color.Color[a]; }
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetFontColor(m_btnFontColor); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonJustify(In API::Files::Justify hJust)
				{
					API::Helper::Lock lock(m_sync);
					m_btnJustifyH = hJust;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetJustify(m_btnJustifyH); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonJustify(In API::Files::Justify hJust, In API::Files::AnchorPoint vJust)
				{
					API::Helper::Lock lock(m_sync);
					m_btnJustifyH = hJust;
					m_btnJustifyV = vJust;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetJustify(m_btnJustifyH, m_btnJustifyV); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonJustify(In API::Files::AnchorPoint vJust)
				{
					API::Helper::Lock lock(m_sync);
					m_btnJustifyV = vJust;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetJustify(m_btnJustifyV); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonLetterSpace(In float space)
				{
					API::Helper::Lock lock(m_sync);
					m_btnLetterSpace = space;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetLetterSpace(m_btnLetterSpace); }
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonLineSpace(In float space)
				{
					API::Helper::Lock lock(m_sync);
					m_btnLineSpace = space;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetLineSpace(m_btnLineSpace); }
				}
				void CLOAK_CALL_THIS DropDownMenu::Open()
				{
					API::Helper::Lock lock(m_sync);
					if (m_open == false)
					{
						m_open = true;
						for (size_t a = 0; a < m_menu.size(); a++) { m_menu[a]->Show(); }
					}
				}
				void CLOAK_CALL_THIS DropDownMenu::Close()
				{
					API::Helper::Lock lock(m_sync);
					if (m_open == true)
					{
						m_open = false;
						for (size_t a = 0; a < m_menu.size(); a++) { m_menu[a]->Close(); }
						m_stateSize = 0;
					}
				}
				void CLOAK_CALL_THIS DropDownMenu::SetButtonTextOffset(In const API::Global::Math::Space2D::Vector& offset)
				{
					API::Helper::Lock lock(m_sync);
					m_btnTextOffset = offset;
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->SetTextOffset(offset); }
				}
				void CLOAK_CALL_THIS DropDownMenu::CloseLevel(In size_t lvl)
				{
					API::Helper::Lock lock(m_sync);
					for (size_t a = 0; a < m_pool.size(); a++) { m_pool[a]->CloseLevel(lvl); }
				}
				bool CLOAK_CALL_THIS DropDownMenu::IsOpen() const
				{
					API::Helper::Lock lock(m_sync);
					return m_open;
				}

				Success(return == true) bool CLOAK_CALL_THIS DropDownMenu::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Interface::Menu_v1::IMenu)) { *ptr = (API::Interface::Menu_v1::IMenu*)this; return true; }
					else if (riid == __uuidof(API::Interface::DropDownMenu_v1::IDropDownMenu)) { *ptr = (API::Interface::DropDownMenu_v1::IDropDownMenu*)this; return true; }
					else if (riid == __uuidof(DropDownMenu)) { *ptr = (DropDownMenu*)this; return true; }
					return Button::iQueryInterface(riid, ptr);
				}
				DropDownMenu::MenuButton* CLOAK_CALL_THIS DropDownMenu::GetBtnFromPool()
				{
					API::Helper::Lock lock(m_sync);
					if (m_usedPoolSize >= m_pool.size())
					{
						MenuButton* r = new DropDownMenu::MenuButton(this);
						r->Initialize();
						m_pool.push_back(r);
						r->SetParent(this);
						r->SetBackgroundTexture(m_btnBackground);
						r->SetHighlightTexture(m_btnHighlight, m_btnHighlightType);
						r->SetCheckTexture(m_btnChecked);
						r->SetSubMenuTexture(m_btnSubMenuOpen, m_btnSubMenuClosed);
						r->SetSize(m_btnWidth, m_btnHeight);
						r->SetSymbolWidth(m_btnSymbolWidth);
						r->SetTextMaxWidth(m_btnMaxLen);
						r->SetTextOffset(m_btnTextOffset);
						r->SetFont(m_btnFont);
						r->SetFontSize(m_btnFontSize);
						r->SetFontColor(m_btnFontColor);
						r->SetJustify(m_btnJustifyH, m_btnJustifyV);
						r->SetLetterSpace(m_btnLetterSpace);
						r->SetLineSpace(m_btnLineSpace);
					}
					return m_pool[m_usedPoolSize++];
				}
				void CLOAK_CALL_THIS DropDownMenu::RegisterClose()
				{
					API::Helper::Lock lock(m_sync);
					if (m_stateSize > 0) { m_stateSize--; }
				}
				void CLOAK_CALL_THIS DropDownMenu::RegisterOpen(In API::Interface::Menu_v1::ButtonID id, In size_t lvl)
				{
					API::Helper::Lock lock(m_sync);
					if (m_stateSize == lvl)
					{
						if (m_openState.size() == m_stateSize) { m_openState.push_back(id); }
						else { m_openState[m_stateSize] = id; }
						m_stateSize++;
					}
				}


				CLOAK_CALL DropDownMenu::MenuButton::MenuButton(In DropDownMenu* par) : m_menuPar(par)
				{
					m_open = false;
					m_checked = false;
					m_canBeChecked = false;
					m_checkSpace = false;
					m_symWidth = 0;
					m_imgChecked = nullptr;
					m_imgSubMenuOpen = nullptr;
					m_imgSubMenuClosed = nullptr;
					m_ID = 0;
				}
				CLOAK_CALL DropDownMenu::MenuButton::~MenuButton()
				{
					SAVE_RELEASE(m_imgChecked);
					SAVE_RELEASE(m_imgSubMenuOpen);
					SAVE_RELEASE(m_imgSubMenuClosed);
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::ResetMenu()
				{
					Hide();
					m_subMenu.clear();
					m_open = false;
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::SetMenu(In const API::Interface::Menu_v1::MENU_ITEM& desc, In const MenuAnchor& anchor, In size_t level)
				{
					API::Helper::Lock lock(m_sync);
					SetText(desc.Text);
					m_checked = desc.Checked && desc.CanChecked;
					m_canBeChecked = desc.CanChecked;
					m_checkSpace = anchor.CanCheck;
					m_menAnch = anchor;
					m_ID = desc.ID;
					m_level = level;
					UpdateMenuAnchor();
					UpdateTextOffset();

					if (CloakDebugCheckOK(desc.SubMenuSize == 0 || desc.SubMenu != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						MenuAnchor menAnch;
						menAnch.CanCheck = false;
						for (size_t a = 0; a < desc.SubMenuSize; a++)
						{
							if (desc.SubMenu[a].CanChecked == true) 
							{ 
								menAnch.CanCheck = true; 
								break;
							}
						}
						for (size_t a = 0; a < desc.SubMenuSize; a++)
						{
							MenuButton* b = m_menuPar->GetBtnFromPool();
							m_subMenu.push_back(b);
							menAnch.MenuNum = a;
							menAnch.Point = API::Interface::AnchorPoint::TOPLEFT;
							menAnch.RelativePoint = API::Interface::AnchorPoint::TOPRIGHT;
							menAnch.RelativeAnchor = this;
							b->SetMenu(desc.SubMenu[a], menAnch, level + 1);
						}
					}
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::Open()
				{
					API::Helper::Lock lock(m_sync);
					if (m_open == false)
					{
						m_menuPar->CloseLevel(m_level);
						m_menuPar->RegisterOpen(m_ID, m_level);
						m_open = true;
						for (size_t a = 0; a < m_subMenu.size(); a++) { m_subMenu[a]->Show(); }
					}
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::Close()
				{
					API::Helper::Lock lock(m_sync);
					Hide();
					if (m_open == true)
					{
						m_open = false;
						for (size_t a = 0; a < m_subMenu.size(); a++) { m_subMenu[a]->Close(); }
						m_menuPar->RegisterClose();
					}
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::SetCheckTexture(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(img, m_imgChecked);
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::SetSubMenuTexture(In API::Files::ITexture* open, In API::Files::ITexture* closed)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(open, m_imgSubMenuOpen);
					GUI_SET_TEXTURE(closed, m_imgSubMenuClosed);
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::SetSymbolWidth(In float W)
				{
					API::Helper::Lock lock(m_sync);
					m_symWidth = W;
					UpdateTextOffset();
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::SetTextOffset(In const API::Global::Math::Space2D::Vector& offset)
				{
					API::Helper::Lock lock(m_sync);
					m_baseTextOff = offset;
					UpdateTextOffset();
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::SetOpenState(In const API::List<API::Interface::Menu_v1::ButtonID>& state, In size_t index, In size_t maxI)
				{
					API::Helper::Lock lock(m_sync);
					if (state[index] == m_ID)
					{
						m_open = true;
						for (size_t a = 0; a < m_subMenu.size(); a++) 
						{ 
							DropDownMenu::MenuButton* b = m_subMenu[a];
							b->Show(); 
							if (index + 1 < maxI) { b->SetOpenState(state, index + 1, maxI); }
						}
					}
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::CloseLevel(In size_t lvl)
				{
					API::Helper::Lock lock(m_sync);
					if (m_level == lvl) { Close(); }
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::SetSize(In float width, In float height, In_opt API::Interface::SizeType type)
				{
					API::Helper::Lock lock(m_sync);
					Button::SetSize(width, height, type);
					UpdateMenuAnchor();
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::OnClick(const API::Global::Events::onClick& ev)
				{
					API::Helper::Lock lock(m_sync);
					if (IS_CLICK(ev, LEFT))
					{
						API::Helper::Lock parLock(m_menuPar->m_sync);
						if (m_canBeChecked) { m_checked = !m_checked; }
						if (m_open) { Close(); }
						else { Open(); }
						API::Global::Events::onMenuButtonClick mbc;
						mbc.activeKeys = ev.activeKeys;
						mbc.key = ev.key;
						mbc.clickPos = ev.clickPos;
						mbc.menuButtonID = m_ID;
						mbc.moveDir = ev.moveDir;
						mbc.pressType = ev.pressType;
						mbc.previousType = ev.previousType;
						mbc.relativePos = ev.relativePos;
						mbc.isChecked = m_checked;
						m_menuPar->triggerEvent(mbc);
					}
				}

				bool CLOAK_CALL_THIS DropDownMenu::MenuButton::iUpdateDrawInfo(In unsigned long long time)
				{
					m_checkedVertex.Position = m_pos;
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

					const float xoff = m_Width - m_symWidth;
					m_subMenuVertex.Position.X = m_pos.X + (m_rotation.cos_a*xoff);
					m_subMenuVertex.Position.Y = m_pos.Y + (m_rotation.sin_a*xoff);
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

					m_d_checkedInfo.Update(m_imgChecked, m_checked);
					m_d_subMenuOpenInfo.Update(m_imgSubMenuOpen, m_subMenu.size() > 0 && m_open == true);
					m_d_subMenuClosedInfo.Update(m_imgSubMenuClosed, m_subMenu.size() > 0 && m_open == false);

					return Button::iUpdateDrawInfo(time);
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::iDraw(In API::Rendering::Context_v1::IContext2D* context)
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
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::UpdateMenuAnchor()
				{
					API::Helper::Lock lock(m_sync);
					API::Interface::Anchor anch;
					anch.point = m_menAnch.Point;
					anch.relativePoint = m_menAnch.RelativePoint;
					anch.relativeTarget = m_menAnch.RelativeAnchor;
					anch.offset.X = 0;
					anch.offset.Y = m_menAnch.MenuNum*GetHeight();
					SetAnchor(anch);
				}
				void CLOAK_CALL_THIS DropDownMenu::MenuButton::UpdateTextOffset()
				{
					API::Helper::Lock lock(m_sync);
					m_textOffset.X = m_baseTextOff.X + (m_checkSpace ? m_symWidth : 0);
					m_textOffset.Y = m_baseTextOff.Y;
				}
			}
		}
	}
}