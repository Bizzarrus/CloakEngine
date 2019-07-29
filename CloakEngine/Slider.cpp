#include "stdafx.h"
#include "Implementation/Interface/Slider.h"

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace Slider_v1 {
				CLOAK_CALL_THIS Slider::Slider()
				{
					INIT_EVENT(onClick, m_sync);
					INIT_EVENT(onEnterKey, m_sync);
					INIT_EVENT(onValueChange, m_sync);
					INIT_EVENT(onScroll, m_sync);

					m_arrW = 0;
					m_thumbW = 0;
					m_min = 0;
					m_max = 0;
					m_val = 0;
					m_step = 0;
					//Notice: m_min,m_max and m_val are scaled differently: m_min is the minimal offset, m_max is the difference between
					//the actual minimal and maximal values, and m_val is a percentage value, which slides between the actual minimal (0%) and 
					//maximal (100%) values. These scalings shall reduce the overall amount of calculations, as the values in that scale are more
					//often used than the values in "normal" scale. Also, that way the values will be kept at the lowest minimum, which allowes
					//higher precition.
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_thumb));
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_lArrow));
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_rArrow));
					API::Interface::Anchor anchor;
					anchor.offset.X = 0;
					anchor.offset.Y = 0;
					anchor.point = API::Interface::AnchorPoint::LEFT;
					anchor.relativePoint = API::Interface::AnchorPoint::LEFT;
					anchor.relativeTarget = this;
					m_lArrow->SetAnchor(anchor);
					anchor.point = API::Interface::AnchorPoint::RIGHT;
					anchor.relativePoint = API::Interface::AnchorPoint::RIGHT;
					m_rArrow->SetAnchor(anchor);
					anchor.point = API::Interface::AnchorPoint::LEFT;
					anchor.relativePoint = API::Interface::AnchorPoint::RIGHT;
					anchor.relativeTarget = m_lArrow;
					m_thumb->SetAnchor(anchor);
					m_lArrow->registerListener([=](const API::Global::Events::onClick& ev) {
						if (ev.key == API::Global::Input::Keys::Mouse::LEFT)
						{
							if (ev.pressType == API::Global::Input::KEY_DOWN || ev.pressType == API::Global::Input::KEY_PRESS)
							{
								SetValue(GetValue() - GetStepSize());
							}
						}
					});
					m_rArrow->registerListener([=](const API::Global::Events::onClick& ev) {
						if (ev.key == API::Global::Input::Keys::Mouse::LEFT)
						{
							if (ev.pressType == API::Global::Input::KEY_DOWN || ev.pressType == API::Global::Input::KEY_PRESS)
							{
								SetValue(GetValue() + GetStepSize());
							}
						}
					});
					m_thumb->registerListener([=](const API::Global::Events::onClick& ev)
					{
						if (ev.key == API::Global::Input::Keys::Mouse::LEFT && ev.pressType == API::Global::Input::MOUSE_DRAGED)
						{
							DragThumb(ev);
						}
					});
					m_thumb->registerListener([=](const API::Global::Events::onScroll& ev) {
						OnScroll(ev);
					});
					DEBUG_NAME(Slider);
				}
				CLOAK_CALL_THIS Slider::~Slider()
				{
					SAVE_RELEASE(m_thumb);
					SAVE_RELEASE(m_lArrow);
					SAVE_RELEASE(m_rArrow);
				}
				void CLOAK_CALL_THIS Slider::Initialize()
				{
					Frame::Initialize();
					registerListener([=](const API::Global::Events::onScroll& ev) {
						OnScroll(ev);
					});
					registerListener([=](const API::Global::Events::onClick& ev) {
						if (ev.key == API::Global::Input::Keys::Mouse::LEFT && ev.pressType == API::Global::Input::MOUSE_UP && ev.previousType == API::Global::Input::MOUSE_DOWN)
						{
							SetThumbPos(ev);
						}
					});
				}
				void CLOAK_CALL_THIS Slider::SetMin(In float minV)
				{
					API::Helper::Lock lock(m_sync);
					if (CloakDebugCheckOK(minV <= m_max + m_min, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						if (minV == m_max + m_min) { m_val = 0; }
						m_max += m_min - minV;
						m_min = minV;
					}
				}
				void CLOAK_CALL_THIS Slider::SetMax(In float maxV)
				{
					API::Helper::Lock lock(m_sync);
					if (CloakDebugCheckOK(maxV >= m_min, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						if (maxV == m_min) { m_val = 0; }
						m_max = maxV - m_min;
					}
				}
				void CLOAK_CALL_THIS Slider::SetMinMax(In float minV, In float maxV)
				{
					API::Helper::Lock lock(m_sync);
					if (CloakDebugCheckOK(minV <= maxV, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						if (minV == maxV) { m_val = 0; }
						m_min = minV;
						m_max = maxV - minV;
					}
				}
				void CLOAK_CALL_THIS Slider::SetValue(In float val)
				{
					API::Helper::Lock lock(m_sync);
					val = min(m_max, min(m_min, val));
					const float nv = (val - m_min) / m_max;
					if (nv != m_val)
					{
						API::Global::Events::onValueChange ovc;
						ovc.newValue = val;
						triggerEvent(ovc);
						UpdatePosition();
					}
				}
				void CLOAK_CALL_THIS Slider::SetThumbTexture(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					m_thumb->SetBackgroundTexture(img);
				}
				void CLOAK_CALL_THIS Slider::SetLeftArrowTexture(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					m_lArrow->SetBackgroundTexture(img);
				}
				void CLOAK_CALL_THIS Slider::SetRightArrowTexture(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					m_rArrow->SetBackgroundTexture(img);
				}
				void CLOAK_CALL_THIS Slider::SetArrowWidth(In float w)
				{
					API::Helper::Lock lock(m_sync);
					m_arrW = min(1, max(0, w));
					const float aw = (m_Width*(1 - m_thumbW)) / 2;
					m_lArrow->SetSize(m_arrW*aw, m_Height);
					m_rArrow->SetSize(m_arrW*aw, m_Height);
				}
				void CLOAK_CALL_THIS Slider::SetThumbWidth(In float w)
				{
					API::Helper::Lock lock(m_sync);
					m_thumbW = min(1, max(0, w));
					m_thumb->SetSize(m_thumbW*m_Width, m_Height);
					const float aw = (m_Width*(1 - m_thumbW)) / 2;
					m_lArrow->SetSize(m_arrW*aw, m_Height);
					m_rArrow->SetSize(m_arrW*aw, m_Height);
				}
				void CLOAK_CALL_THIS Slider::SetStepSize(In float ss)
				{
					API::Helper::Lock lock(m_sync);
					m_step = ss;
				}
				float CLOAK_CALL_THIS Slider::GetStepSize() const
				{
					API::Helper::Lock lock(m_sync);
					return m_step;
				}
				float CLOAK_CALL_THIS Slider::GetValue() const
				{
					API::Helper::Lock lock(m_sync);
					return m_min + (m_val*m_max);
				}
				float CLOAK_CALL_THIS Slider::GetMin() const
				{
					API::Helper::Lock lock(m_sync);
					return m_min;
				}
				float CLOAK_CALL_THIS Slider::GetMax() const
				{
					API::Helper::Lock lock(m_sync);
					return m_max + m_min;
				}
				bool CLOAK_CALL_THIS Slider::IsClickable() const { return true; }

				void CLOAK_CALL_THIS Slider::Draw(In API::Rendering::Context_v1::IContext2D* context)
				{
					API::Helper::Lock lock(m_sync);
					Frame::Draw(context);
					context->StartNewPatch(m_d_Z + m_d_ZSize + 1);
					m_lArrow->Draw(context);
					m_rArrow->Draw(context);
					m_thumb->Draw(context);
					context->EndPatch();
				}
				void CLOAK_CALL_THIS Slider::DragThumb(In const API::Global::Events::onClick& click)
				{
					API::Helper::Lock lock(m_sync);
					const float move = (click.moveDir.X*m_rotation.cos_a) + (click.moveDir.Y*m_rotation.sin_a);
					m_val += move / (m_Width*(1-(m_arrW-(m_arrW*m_thumbW)+m_thumbW)));
					m_val = min(1, max(0, m_val));
					UpdatePosition();
				}
				void CLOAK_CALL_THIS Slider::SetThumbPos(In const API::Global::Events::onClick& click)
				{
					API::Helper::Lock lock(m_sync);
					m_val = click.relativePos.X / (1 - (m_arrW - (m_arrW*m_thumbW) + m_thumbW));
					m_val = min(1, max(0, m_val));
					UpdatePosition();
				}
				void CLOAK_CALL_THIS Slider::OnScroll(In const API::Global::Events::onScroll& ev)
				{
					if (ev.button == API::Global::Input::SCROLL_VERTICAL)
					{
						SetValue(GetValue() + (ev.delta*GetStepSize()));
					}
				}

				Success(return==true) bool CLOAK_CALL_THIS Slider::iQueryInterface(In REFIID id, Outptr void** ptr)
				{
					CLOAK_QUERY(id, ptr, Frame, Interface::Slider_v1, Slider);
				}
				/*
				void CLOAK_CALL_THIS Slider::iUpdatePosition()
				{
					m_thumb->SetSize(m_thumbW*m_Width, m_Height);
					const float aw = m_arrW*(m_Width*(1 - m_thumbW)) / 2;
					m_lArrow->SetSize(aw, m_Height);
					m_rArrow->SetSize(aw, m_Height);
					API::Interface::Anchor anchor;
					anchor.offset.X = m_val*(m_Width - ((2 * aw) + (m_thumbW*m_Width)));
					anchor.offset.Y = 0;
					anchor.point = API::Interface::AnchorPoint::LEFT;
					anchor.relativePoint = API::Interface::AnchorPoint::RIGHT;
					anchor.relativeTarget = m_lArrow;
					m_thumb->SetAnchor(anchor);
					m_thumb->SetRotation(m_rotation.alpha);
					m_lArrow->SetRotation(m_rotation.alpha);
					m_rArrow->SetRotation(m_rotation.alpha);
				}
				*/
			}
		}
	}
}