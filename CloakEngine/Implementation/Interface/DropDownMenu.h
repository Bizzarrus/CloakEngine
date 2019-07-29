#pragma once
#ifndef CE_IMPL_INTERFACE_DROPDOWNMENU_H
#define CE_IMPL_INTERFACE_DROPDOWNMENU_H

#include "CloakEngine/Interface/DropDownMenu.h"
#include "Implementation/Interface/Button.h"

#include <unordered_map>

#pragma warning(push)
#pragma warning(disable:4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace DropDownMenu_v1 {
				class CLOAK_UUID("{5E8A65C1-ED1A-44BD-8ABA-ED9F19114D98}") DropDownMenu : public virtual API::Interface::DropDownMenu_v1::IDropDownMenu, public Button_v1::Button, IMPL_EVENT(onMenuButtonClick) {
					public:
						CLOAK_CALL DropDownMenu();
						virtual CLOAK_CALL ~DropDownMenu();
						virtual void CLOAK_CALL_THIS SetMenu(In const API::Interface::Menu_v1::MENU_DESC& desc) override;
						virtual void CLOAK_CALL_THIS SetButtonTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetButtonHighlightTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight = API::Interface::Frame_v1::HighlightType::OVERRIDE) override;
						virtual void CLOAK_CALL_THIS SetButtonCheckTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetButtonSubMenuTexture(In API::Files::ITexture* open, In API::Files::ITexture* closed) override;
						virtual void CLOAK_CALL_THIS SetButtonWidth(In float W) override;
						virtual void CLOAK_CALL_THIS SetButtonHeight(In float H) override;
						virtual void CLOAK_CALL_THIS SetButtonSize(In float W, In float H) override;
						virtual void CLOAK_CALL_THIS SetButtonSymbolWidth(In float W) override;
						virtual void CLOAK_CALL_THIS SetButtonFont(In API::Files::IFont* font) override;
						virtual void CLOAK_CALL_THIS SetButtonFontSize(In API::Files::FontSize size) override;
						virtual void CLOAK_CALL_THIS SetButtonFontColor(In const API::Interface::FontColor& color) override;
						virtual void CLOAK_CALL_THIS SetButtonJustify(In API::Files::Justify hJust) override;
						virtual void CLOAK_CALL_THIS SetButtonJustify(In API::Files::Justify hJust, In API::Files::AnchorPoint vJust) override;
						virtual void CLOAK_CALL_THIS SetButtonJustify(In API::Files::AnchorPoint vJust) override;
						virtual void CLOAK_CALL_THIS SetButtonLetterSpace(In float space) override;
						virtual void CLOAK_CALL_THIS SetButtonLineSpace(In float space) override;
						virtual void CLOAK_CALL_THIS Open() override;
						virtual void CLOAK_CALL_THIS Close() override;
						virtual void CLOAK_CALL_THIS SetButtonTextOffset(In const API::Global::Math::Space2D::Vector& offset) override;
						virtual bool CLOAK_CALL_THIS IsOpen() const override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						void CLOAK_CALL_THIS CloseLevel(In size_t lvl);
						void CLOAK_CALL_THIS RegisterClose();
						void CLOAK_CALL_THIS RegisterOpen(In API::Interface::Menu_v1::ButtonID id, In size_t lvl);

						struct MenuAnchor {
							BasicGUI* RelativeAnchor;
							API::Interface::AnchorPoint Point;
							API::Interface::AnchorPoint RelativePoint;
							size_t MenuNum;
							bool CanCheck;
						};
						class MenuButton : public Impl::Interface::Button_v1::Button {
							public:
								CLOAK_CALL MenuButton(In DropDownMenu* par);
								virtual CLOAK_CALL ~MenuButton();
								void CLOAK_CALL_THIS ResetMenu();
								void CLOAK_CALL_THIS SetMenu(In const API::Interface::Menu_v1::MENU_ITEM& desc, In const MenuAnchor& anchor, In size_t level);
								void CLOAK_CALL_THIS Open();
								void CLOAK_CALL_THIS Close();
								void CLOAK_CALL_THIS SetCheckTexture(In API::Files::ITexture* img);
								void CLOAK_CALL_THIS SetSubMenuTexture(In API::Files::ITexture* open, In API::Files::ITexture* closed);
								void CLOAK_CALL_THIS SetSymbolWidth(In float W);
								void CLOAK_CALL_THIS SetTextOffset(In const API::Global::Math::Space2D::Vector& offset);
								void CLOAK_CALL_THIS CloseLevel(In size_t lvl);
								void CLOAK_CALL_THIS SetOpenState(In const API::List<API::Interface::Menu_v1::ButtonID>& state, In size_t index, In size_t maxI);
								void CLOAK_CALL_THIS SetSize(In float width, In float height, In_opt API::Interface::SizeType type = API::Interface::SizeType::NORMAL) override;
								virtual void CLOAK_CALL_THIS OnClick(const API::Global::Events::onClick& ev) override;
							protected:
								virtual bool CLOAK_CALL_THIS iUpdateDrawInfo(In unsigned long long time) override;
								virtual void CLOAK_CALL_THIS iDraw(In API::Rendering::Context_v1::IContext2D* context) override;
								virtual void CLOAK_CALL_THIS UpdateMenuAnchor();
								virtual void CLOAK_CALL_THIS UpdateTextOffset();

								DropDownMenu* const m_menuPar;
								API::List<MenuButton*> m_subMenu;
								bool m_open;
								bool m_checked;
								bool m_canBeChecked;
								bool m_checkSpace;
								size_t m_level;
								float m_symWidth;
								API::Files::ITexture* m_imgChecked;
								API::Files::ITexture* m_imgSubMenuOpen;
								API::Files::ITexture* m_imgSubMenuClosed;
								MenuAnchor m_menAnch;
								API::Interface::Menu_v1::ButtonID m_ID;
								API::Global::Math::Space2D::Vector m_baseTextOff;

								API::Rendering::VERTEX_2D m_checkedVertex;
								API::Rendering::VERTEX_2D m_subMenuVertex;
								ImageDrawInfo m_d_checkedInfo;
								ImageDrawInfo m_d_subMenuOpenInfo;
								ImageDrawInfo m_d_subMenuClosedInfo;
						};

						virtual MenuButton* CLOAK_CALL_THIS GetBtnFromPool();

						API::List<MenuButton*> m_pool;
						size_t m_usedPoolSize;
						API::List<MenuButton*> m_menu;
						API::List<API::Interface::Menu_v1::ButtonID> m_openState;
						size_t m_stateSize;
						bool m_open;

						API::Global::Math::Space2D::Vector m_btnTextOffset;
						float m_btnWidth;
						float m_btnHeight;
						float m_btnSymbolWidth;
						float m_btnMaxLen;
						float m_btnLetterSpace;
						float m_btnLineSpace;
						API::Files::ITexture* m_btnBackground;
						API::Files::ITexture* m_btnHighlight;
						API::Files::ITexture* m_btnChecked;
						API::Files::ITexture* m_btnSubMenuOpen;
						API::Files::ITexture* m_btnSubMenuClosed;
						API::Files::IFont* m_btnFont;
						API::Files::FontSize m_btnFontSize;
						API::Interface::FontColor m_btnFontColor;
						API::Files::Justify m_btnJustifyH;
						API::Files::AnchorPoint m_btnJustifyV;
						API::Interface::Frame_v1::HighlightType m_btnHighlightType;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif