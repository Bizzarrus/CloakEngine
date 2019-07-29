#pragma once
#ifndef CE_IMPL_RENDERING_DX12_VIDEOENCODER_H
#define CE_IMPL_RENDERING_DX12_VIDEOENCODER_H

#include "Implementation/Rendering/VideoEncoder.h"
#include "Implementation/Helper/SavePtr.h"

#include <mfmediaengine.h>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace VideoEncoder_v1 {
					class CLOAK_UUID("{BEF22FE2-B25E-4E1D-AEBA-568B87BB3F4C}") VideoEncoder : public virtual Impl::Rendering::IVideoEncoder, public Impl::Helper::SavePtr {
						public:
							CLOAK_CALL VideoEncoder();
							virtual CLOAK_CALL ~VideoEncoder();
							virtual bool CLOAK_CALL_THIS StartPlayback(In const std::u16string& file) override;
							virtual void CLOAK_CALL_THIS StopPlayback() override;
							virtual void CLOAK_CALL_THIS RenderVideo(In API::Rendering::IContext* context, In API::Rendering::IColorBuffer* dst) override;
							virtual bool CLOAK_CALL_THIS IsVideoPlaying() const override;
							virtual void CLOAK_CALL_THIS OnVideoEvent() override;
							virtual void CLOAK_CALL_THIS OnWindowResize(In LPRECT rct) override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							IMFMediaEngine* m_engine;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif