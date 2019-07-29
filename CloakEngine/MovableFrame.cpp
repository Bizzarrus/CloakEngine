#include "stdafx.h"
#include "Implementation/Interface/MovableFrame.h"

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace MoveableFrame_v1 {
				CLOAK_CALL MoveableFrame::MoveableFrame()
				{
					m_movedX = 0;
					m_movedY = 0;
				}
				CLOAK_CALL MoveableFrame::~MoveableFrame()
				{

				}
				void CLOAK_CALL_THIS MoveableFrame::Move(In float X, In float Y)
				{
					API::Helper::Lock lock(m_sync);
					m_movedX += X;
					m_movedY += Y;
				}
				void CLOAK_CALL_THIS MoveableFrame::ResetPos()
				{
					API::Helper::Lock lock(m_sync);
					m_movedX = 0;
					m_movedY = 0;
				}
				void CLOAK_CALL_THIS MoveableFrame::Draw(In API::Rendering::Context_v1::IContext2D* context)
				{
					API::Helper::ReadLock lock(m_syncDraw);
					context->SetScissor(m_d_sr);
					lock.unlock();
					Frame::Draw(context);
					context->ResetScissor();
				}
				API::Global::Math::Space2D::Point CLOAK_CALL_THIS MoveableFrame::GetPositionOnScreen() const
				{
					API::Helper::Lock lock(m_sync);
					API::Global::Math::Space2D::Point r = m_pos;
					r.X += m_movedX;
					r.Y += m_movedY;
					return r;
				}

				Success(return == true) bool CLOAK_CALL_THIS MoveableFrame::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, Frame, Interface::MoveableFrame_v1, MoveableFrame);
				}
				bool CLOAK_CALL_THIS MoveableFrame::iUpdateDrawInfo(In unsigned long long time)
				{
					Frame::iUpdateDrawInfo(time);
					m_d_sr.TopLeftX = m_pos.X;
					m_d_sr.TopLeftY = m_pos.Y;
					m_d_sr.Width = m_Width;
					m_d_sr.Height = m_Height;

					float posOff[2] = { m_movedX,m_movedY };
					if (abs(posOff[0]) > m_Width) { posOff[0] -= m_Width*static_cast<long>(posOff[0] / m_Width); }
					if (abs(posOff[1]) > m_Height) { posOff[1] -= m_Height*static_cast<long>(posOff[1] / m_Height); }
					posOff[0] -= m_Width;
					posOff[1] -= m_Height;

					m_imgVertex[0].TexTopLeft.X = 0;
					m_imgVertex[0].TexTopLeft.Y = 0;
					m_imgVertex[0].TexBottomRight.X = 3;
					m_imgVertex[0].TexBottomRight.X = 3;
					m_imgVertex[0].Size.X = 3 * m_Width;
					m_imgVertex[0].Size.Y = 3 * m_Height;
					m_imgVertex[0].Position.X = m_pos.X + (m_rotation.cos_a*posOff[0]) - (m_rotation.sin_a*posOff[1]);
					m_imgVertex[0].Position.Y = m_pos.Y + (m_rotation.sin_a*posOff[0]) + (m_rotation.cos_a*posOff[1]);
					return false;
				}
				void CLOAK_CALL_THIS MoveableFrame::iDraw(In API::Rendering::Context_v1::IContext2D* context)
				{
					if (m_d_img.CanDraw())
					{
						context->Draw(this, m_d_img.GetImage(), m_imgVertex[0], 0);
					}
				}
			}
		}
	}
}
