#pragma once
#ifndef CE_IMPL_INTERFACE_LISTMENU_H
#define CE_IMPL_INTERFACE_LISTMENU_H

#include "CloakEngine/Interface/ListMenu.h"
#include "CloakEngine/Interface/MoveableFrame.h"
#include "CloakEngine/Interface/Slider.h"

#include "Implementation/Interface/Frame.h"
#include "Implementation/Interface/Button.h"

#include <unordered_set>

#pragma warning(push)
#pragma warning(disable:4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace ListMenu_v1 {
				class CLOAK_UUID("{B76B50A8-58BF-4EEE-B2F7-780619135F72}") ListMenu : public API::Interface::ListMenu_v1::IListMenu, public Frame_v1::Frame, IMPL_EVENT(onMenuButtonClick), IMPL_EVENT(onClick), IMPL_EVENT(onEnterKey), IMPL_EVENT(onScroll) {
					public:
						CLOAK_CALL ListMenu();
						virtual CLOAK_CALL ~ListMenu();
						virtual void CLOAK_CALL_THIS SetMenu(In const API::Interface::Menu_v1::MENU_DESC& desc) override;
						virtual void CLOAK_CALL_THIS SetButtonTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetButtonHighlightTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight = API::Interface::Frame_v1::HighlightType::OVERRIDE) override;
						virtual void CLOAK_CALL_THIS SetButtonCheckTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetButtonSubMenuTexture(In API::Files::ITexture* open, In API::Files::ITexture* closed) override;
						virtual void CLOAK_CALL_THIS SetButtonWidth(In float W) override;
						virtual void CLOAK_CALL_THIS SetButtonHeight(In float H) override;
						virtual void CLOAK_CALL_THIS SetButtonSize(In float W, In float H) override;
						virtual void CLOAK_CALL_THIS SetButtonSymbolWidth(In float W) override;
						virtual void CLOAK_CALL_THIS SetButtonTextOffset(In const API::Global::Math::Space2D::Vector& offset) override;
						virtual void CLOAK_CALL_THIS SetButtonFont(In API::Files::IFont* font) override;
						virtual void CLOAK_CALL_THIS SetButtonFontSize(In API::Files::FontSize size) override;
						virtual void CLOAK_CALL_THIS SetButtonFontColor(In const API::Interface::FontColor& color) override;
						virtual void CLOAK_CALL_THIS SetButtonJustify(In API::Files::Justify hJust) override;
						virtual void CLOAK_CALL_THIS SetButtonJustify(In API::Files::Justify hJust, In API::Files::AnchorPoint vJust) override;
						virtual void CLOAK_CALL_THIS SetButtonJustify(In API::Files::AnchorPoint vJust) override;
						virtual void CLOAK_CALL_THIS SetButtonLetterSpace(In float space) override;
						virtual void CLOAK_CALL_THIS SetButtonLineSpace(In float space) override;
						virtual void CLOAK_CALL_THIS SetLevelStepSize(In float s) override;
						virtual void CLOAK_CALL_THIS SetSliderWidth(In float w) override;
						virtual void CLOAK_CALL_THIS SetSliderTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetSliderThumbTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetSliderLeftArrowTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetSliderRightArrowTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetSliderArrowHeight(In float w) override;
						virtual void CLOAK_CALL_THIS SetSliderThumbHeight(In float w) override;
						virtual void CLOAK_CALL_THIS SetSliderStepSize(In float ss) override;

						virtual bool CLOAK_CALL_THIS IsClickable() const override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS iSetOpenState(In API::Interface::ButtonID id, In bool state);
						virtual void CLOAK_CALL_THIS iUpdateItemPositions();
						virtual void CLOAK_CALL_THIS iCheckSlider();
						virtual float CLOAK_CALL_THIS GetStepSize() const;

						class MenuButton : public Button_v1::Button {
							public:
								CLOAK_CALL MenuButton(In ListMenu* par, In size_t poolPos);
								virtual CLOAK_CALL ~MenuButton();
								void CLOAK_CALL_THIS Reset();
								void CLOAK_CALL_THIS Open();
								void CLOAK_CALL_THIS Close();
								void CLOAK_CALL_THIS SetOffset(In size_t pos);
								void CLOAK_CALL_THIS UpdateListAnchor();
								void CLOAK_CALL_THIS SetCheckedTexture(In API::Files::ITexture* img);
								void CLOAK_CALL_THIS SetSubMenuTexture(In API::Files::ITexture* open, In API::Files::ITexture* closed);
								void CLOAK_CALL_THIS SetSymbolWidth(In float w);
								void CLOAK_CALL_THIS SetTextOffset(In const API::Global::Math::Space2D::Vector& offset);
								bool CLOAK_CALL_THIS TryHide(In size_t lvl);
								size_t CLOAK_CALL_THIS TryShow(In size_t lvl);
								size_t CLOAK_CALL_THIS SetMenu(In const API::Interface::MENU_ITEM& menu, In size_t pos, In size_t lvl, In const std::unordered_set<API::Interface::ButtonID>& state);
							protected:
								virtual bool CLOAK_CALL_THIS iUpdateDrawInfo(In unsigned long long time) override;
								virtual void CLOAK_CALL_THIS iDraw(In API::Rendering::Context_v1::IContext2D* context) override;
								virtual void CLOAK_CALL_THIS iUpdateTextPos();

								ListMenu* const m_menuPar;
								const size_t m_poolPos;
								size_t m_listPos;
								size_t m_lvl;
								size_t m_listOffset;
								bool m_open;
								bool m_subMenu;
								bool m_checked;
								bool m_canChecked;
								float m_symWidth;
								API::Interface::ButtonID m_ID;
								API::Files::ITexture* m_imgChecked;
								API::Files::ITexture* m_imgSubMenuOpen;
								API::Files::ITexture* m_imgSubMenuClosed;
								API::Global::Math::Space2D::Vector m_baseTextOff;

								API::Rendering::VERTEX_2D m_checkedVertex;
								API::Rendering::VERTEX_2D m_subMenuVertex;
								ImageDrawInfo m_d_checkedInfo;
								ImageDrawInfo m_d_subMenuOpenInfo;
								ImageDrawInfo m_d_subMenuClosedInfo;
						};

						virtual MenuButton* CLOAK_CALL_THIS GetBtnFromPool();
						virtual MenuButton* CLOAK_CALL_THIS GetBtnFromPool(In size_t index);
						virtual API::Interface::IMoveableFrame* CLOAK_CALL_THIS GetMovableFrame() const;

						API::List<MenuButton*> m_pool;
						float m_stepSize;
						float m_btnW;
						float m_btnH;
						float m_sliderW;
						float m_btnSymbW;
						float m_btnLetterSpace;
						float m_btnLineSpace;
						std::unordered_set<API::Interface::ButtonID> m_openState;
						size_t m_usedPoolSize;
						API::Interface::IMoveableFrame* m_frame;
						API::Interface::ISlider* m_slider;
						API::Files::ITexture* m_imgBtnChecked;
						API::Files::ITexture* m_imgBtnSubMenuOpen;
						API::Files::ITexture* m_imgBtnSubMenuClosed;
						API::Files::ITexture* m_imgBtnHighlight;
						API::Files::ITexture* m_imgBtnBackground;
						API::Files::IFont* m_btnFont;
						API::Files::FontSize m_btnFontSize;
						API::Interface::FontColor m_btnFontColor;
						API::Files::Justify m_btnJustifyH;
						API::Files::AnchorPoint m_btnJustifyV;
						API::Interface::Frame_v1::HighlightType m_btnHighlightType;
						API::Global::Math::Space2D::Vector m_btnTextOffset;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif
