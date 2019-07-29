#pragma once
#ifndef CE_IMPL_INTERFACE_FRAME_H
#define CE_IMPL_INTERFACE_FRAME_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Interface/Frame.h"
#include "Implementation/Interface/BasicGUI.h"

#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace Frame_v1 {
				struct ImageDrawInfo {
					public:
						API::Interface::HighlightType Type = API::Interface::HighlightType::ADD;
						inline bool CLOAK_CALL_THIS CanDraw()
						{
							if (m_allowDraw == false) { return false; }
							return m_img == nullptr || m_img->isLoaded(API::Files::Priority::NORMAL);
						}
						inline void CLOAK_CALL_THIS Update(In API::Files::ITexture* img, In_opt bool canDraw = true, In_opt API::Interface::HighlightType type = API::Interface::HighlightType::ADD)
						{
							if (img != m_img)
							{
								SAVE_RELEASE(m_img);
								m_img = img;
								if (m_img != nullptr) { m_img->AddRef(); }
							}
							m_allowDraw = canDraw;
							Type = type;
						}
						inline API::Files::ITexture* CLOAK_CALL_THIS GetImage() const { return m_img; }
						CLOAK_CALL ~ImageDrawInfo() { SAVE_RELEASE(m_img); }
					private:
						API::Files::ITexture* m_img = nullptr;
						bool m_allowDraw = true;
				};

				class CLOAK_UUID("{A7970A76-C252-475F-9A1F-C1B7E808F0AB}") Frame : public virtual API::Interface::Frame_v1::IFrame, public virtual BasicGUI_v1::BasicGUI {
					public:
						CLOAK_CALL_THIS Frame();
						virtual CLOAK_CALL_THIS ~Frame();
						virtual void CLOAK_CALL_THIS Enable() override;
						virtual void CLOAK_CALL_THIS Disable() override;
						virtual void CLOAK_CALL_THIS SetBackgroundTexture(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetBorderSize(In API::Interface::BorderType Border, In float size) override;
						virtual bool CLOAK_CALL_THIS IsEnabled() const override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual bool CLOAK_CALL_THIS iUpdateDrawInfo(In unsigned long long time) override;
						virtual void CLOAK_CALL_THIS iDraw(In API::Rendering::Context_v1::IContext2D* context) override;
						virtual void CLOAK_CALL_THIS iDrawImage(In API::Rendering::Context_v1::IContext2D* context, In API::Files::ITexture* img, In UINT Z);

						API::Files::ITexture* m_img;
						std::atomic<bool> m_enabled;
						API::Rendering::VERTEX_2D m_imgVertex[10];
						float m_borderWidth;
						float m_borderTop;
						float m_borderBottom;

						ImageDrawInfo m_d_img;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif