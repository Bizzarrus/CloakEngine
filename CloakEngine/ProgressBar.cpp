#include "stdafx.h"
#include "Implementation/Interface/ProgressBar.h"

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace ProgressBar_v1 {
				CLOAK_CALL_THIS ProgressBar::ProgressBar()
				{
					m_ZSize = 1;
					m_imgProgress = nullptr;
					m_progress = 0;
					DEBUG_NAME(ProgressBar);
				}
				CLOAK_CALL_THIS ProgressBar::~ProgressBar()
				{
					SAVE_RELEASE(m_imgProgress);
				}
				void CLOAK_CALL_THIS ProgressBar::SetProgressImage(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(img, m_imgProgress);
					m_imgProgress->SetDebugName("Progress Bar image");
				}
				void CLOAK_CALL_THIS ProgressBar::SetProgress(In In_range(0, 1) float progress)
				{
					if (CloakDebugCheckOK(progress >= 0 && progress <= 1, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						m_progress = progress;
						API::Interface::registerForUpdate(this);
					}
				}
				float CLOAK_CALL_THIS ProgressBar::GetProgress() const
				{
					API::Helper::Lock lock(m_sync);
					return m_progress;
				}

				Success(return == true) bool CLOAK_CALL_THIS ProgressBar::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, Frame, Interface::ProgressBar_v1, ProgressBar);
				}
				bool CLOAK_CALL_THIS ProgressBar::iUpdateDrawInfo(In unsigned long long time)
				{
					API::Helper::Lock lock(m_sync);
					m_progressVertex.Position = m_pos;
					m_progressVertex.Color[0] = API::Helper::Color::White;
					m_progressVertex.Size.X = m_Width*m_progress;
					m_progressVertex.Size.Y = m_Height;
					m_progressVertex.TexBottomRight.X = m_progress;
					m_progressVertex.TexBottomRight.Y = 1;
					m_progressVertex.TexTopLeft.X = 0;
					m_progressVertex.TexTopLeft.Y = 0;
					m_progressVertex.Rotation.Sinus = m_rotation.sin_a;
					m_progressVertex.Rotation.Cosinus = m_rotation.cos_a;
					m_progressVertex.Flags = API::Rendering::Vertex2DFlags::NONE;

					m_d_progress.Update(m_imgProgress);

					return Frame::iUpdateDrawInfo(time);
				}
				void CLOAK_CALL_THIS ProgressBar::iDraw(In API::Rendering::Context_v1::IContext2D* context)
				{
					Frame::iDraw(context);
					if (m_d_progress.CanDraw())
					{
						context->Draw(this, m_d_progress.GetImage(), m_progressVertex, 1);
					}
				}
			}
		}
	}
}