#include "stdafx.h"
#include "Implementation/Interface/Frame.h"

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace Frame_v1 {
				CLOAK_CALL_THIS Frame::Frame()
				{
					m_borderWidth = 0;
					m_borderTop = 0;
					m_borderBottom = 0;
					m_enabled = true;
					m_img = nullptr;
					DEBUG_NAME(Frame);
				}
				CLOAK_CALL_THIS Frame::~Frame() 
				{ 
					SAVE_RELEASE(m_img); 
				}
				void CLOAK_CALL_THIS Frame::Enable() { m_enabled = true; }
				void CLOAK_CALL_THIS Frame::Disable() { m_enabled = false; }
				void CLOAK_CALL_THIS Frame::SetBackgroundTexture(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(img, m_img);
				}
				void CLOAK_CALL_THIS Frame::SetBorderSize(In API::Interface::BorderType Border, In float size)
				{
					if (CloakDebugCheckOK(size >= 0 && size <= 0.5f, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::Lock lock(m_sync);
						switch (Border)
						{
							case CloakEngine::API::Interface::Frame_v1::BorderType::SIDE:
								m_borderWidth = size;
								break;
							case CloakEngine::API::Interface::Frame_v1::BorderType::TOP:
								m_borderTop = size;
								break;
							case CloakEngine::API::Interface::Frame_v1::BorderType::BOTTOM:
								m_borderBottom = size;
								break;
							default:
								break;
						}
					}
				}
				bool CLOAK_CALL_THIS Frame::IsEnabled() const { return m_enabled; }

				Success(return==true) bool CLOAK_CALL_THIS Frame::iQueryInterface(In REFIID id, Outptr void** ptr)
				{
					CLOAK_QUERY(id, ptr, BasicGUI, Interface::Frame_v1, Frame);
				}
				void CLOAK_CALL_THIS Frame::iDraw(In API::Rendering::Context_v1::IContext2D* context)
				{
					if (m_d_img.CanDraw())
					{
						iDrawImage(context, m_d_img.GetImage(), 0);
					}
				}
				void CLOAK_CALL_THIS Frame::iDrawImage(In API::Rendering::Context_v1::IContext2D* context, In API::Files::ITexture* img, In UINT Z)
				{
					if (img != nullptr)
					{
						if (img->IsBorderedImage())
						{
							for (size_t a = 0; a < 9; a++) { context->Draw(this, img, m_imgVertex[a + 1], Z); }
						}
						else
						{
							context->Draw(this, img, m_imgVertex[0], Z);
						}
					}
					else { context->Draw(this, (API::Files::ITexture*)nullptr, m_imgVertex[0], Z); }
				}
				bool CLOAK_CALL_THIS Frame::iUpdateDrawInfo(In unsigned long long time)
				{
					API::Helper::Lock lock(m_sync);
					float W[2] = { m_borderWidth,1 - (2 * m_borderWidth) };
					float H[3] = { m_borderTop,1 - (m_borderTop + m_borderBottom),m_borderBottom };
					float XO[3] = { 0,m_borderWidth,1 - m_borderWidth };
					float YO[3] = { 0,m_borderTop,1 - m_borderBottom };
					/*
					vertex index:
					0 = whole image (no border)
					1 = top left corner
					2 = top border
					3 = top right corner
					4 = left border
					5 = center
					6 = right border
					7 = bottom left corner
					8 = bottom border
					9 = bottom right corner
					*/
					for (size_t a = 0; a < 10; a++)
					{
						m_imgVertex[a].Color[0] = API::Helper::Color::White;
						if (a == 0)
						{
							m_imgVertex[a].TexTopLeft.Y = 0;
							m_imgVertex[a].TexBottomRight.Y = 1;
							m_imgVertex[a].TexBottomRight.X = 1;
							m_imgVertex[a].TexTopLeft.X = 0;
							m_imgVertex[a].Size.X = m_Width;
							m_imgVertex[a].Size.Y = m_Height;
							m_imgVertex[a].Position = m_pos;
						}
						else
						{
							const float offset[2] = { (m_Width*XO[(a - 1) / 3]), (m_Height*YO[(a - 1) / 3]) };
							m_imgVertex[a].Position.X = m_pos.X + (offset[0] * m_rotation.cos_a) - (offset[1] * m_rotation.sin_a);
							m_imgVertex[a].Position.Y = m_pos.Y + (offset[0] * m_rotation.sin_a) + (offset[1] * m_rotation.cos_a);
							m_imgVertex[a].Size.X = m_Width*W[(a % 3) >> 1];
							m_imgVertex[a].Size.Y = m_Height*H[(a - 1) % 3];
							m_imgVertex[a].TexTopLeft.X = static_cast<float>((a - 1) % 3) / 3;
							m_imgVertex[a].TexTopLeft.Y = static_cast<float>((a - 1) / 3) / 3;
							m_imgVertex[a].TexBottomRight.X = static_cast<float>(1 + ((a - 1) % 3)) / 3;
							m_imgVertex[a].TexBottomRight.Y = static_cast<float>(1 + ((a - 1) / 3)) / 3;
						}
						m_imgVertex[a].Rotation.Sinus = m_rotation.sin_a;
						m_imgVertex[a].Rotation.Cosinus = m_rotation.cos_a;
						m_imgVertex[a].Flags = API::Rendering::Vertex2DFlags::NONE;
					}
					m_d_img.Update(m_img);
					return BasicGUI::iUpdateDrawInfo(time);
				}
			}
		}
	}
}