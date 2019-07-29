#pragma once
#ifndef CE_IMPL_INTERFACE_MOVEABLEFRAME_H
#define CE_IMPL_INTERFACE_MOVEABLEFRAME_H

#include "CloakEngine/Interface/MoveableFrame.h"
#include "Implementation/Interface/Frame.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace MoveableFrame_v1 {
				class CLOAK_UUID("{2AA4BF07-124B-4E3B-9CFC-717D0526A17C}") MoveableFrame : public API::Interface::MoveableFrame_v1::IMoveableFrame, public Frame_v1::Frame {
					public:
						CLOAK_CALL MoveableFrame();
						virtual CLOAK_CALL ~MoveableFrame();
						virtual void CLOAK_CALL_THIS Move(In float X, In float Y) override;
						virtual void CLOAK_CALL_THIS ResetPos() override;
						virtual void CLOAK_CALL_THIS Draw(In API::Rendering::Context_v1::IContext2D* context) override;
						virtual API::Global::Math::Space2D::Point CLOAK_CALL_THIS GetPositionOnScreen() const override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual bool CLOAK_CALL_THIS iUpdateDrawInfo(In unsigned long long time) override;
						virtual void CLOAK_CALL_THIS iDraw(In API::Rendering::Context_v1::IContext2D* context) override;

						float m_movedX;
						float m_movedY;
						API::Rendering::SCALED_SCISSOR_RECT m_d_sr;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif