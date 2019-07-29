#pragma once
#ifndef CE_IMPL_INTERFACE_PROGRESSBAR_H
#define CE_IMPL_INTERFACE_PROGRESSBAR_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Interface/ProgressBar.h"
#include "Implementation/Interface/Frame.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace ProgressBar_v1 {
				class CLOAK_UUID("{AD42C522-FE65-4B25-8670-BBFA2B94DF72}") ProgressBar : public virtual API::Interface::ProgressBar_v1::IProgressBar, public Interface::Frame_v1::Frame {
					public:
						CLOAK_CALL_THIS ProgressBar();
						virtual CLOAK_CALL_THIS ~ProgressBar();
						virtual void CLOAK_CALL_THIS SetProgressImage(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetProgress(In In_range(0, 1) float progress) override;
						virtual float CLOAK_CALL_THIS GetProgress() const override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual bool CLOAK_CALL_THIS iUpdateDrawInfo(In unsigned long long time) override;
						virtual void CLOAK_CALL_THIS iDraw(In API::Rendering::Context_v1::IContext2D* context) override;

						API::Files::ITexture* m_imgProgress;
						float m_progress;
						API::Rendering::VERTEX_2D m_progressVertex;
						ImageDrawInfo m_d_progress;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif
