#include "stdafx.h"
#include "Implementation/Interface/RayTestContext.h"
#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Interface/BasicGUI.h"

#include "CloakEngine/Rendering/Context.h"
#include "CloakEngine/Helper/SyncSection.h"

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace RayTestContext {
#pragma warning(push)
#pragma warning(disable: 4250)

				class Context : public API::Rendering::IContext2D, public Impl::Helper::SavePtr {
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override
						{
							return SavePtr::iQueryInterface(riid, ptr);
						}

						API::Global::Memory::PageStackAllocator m_pageAlloc;
						API::PagedStack<API::Rendering::SCALED_SCISSOR_RECT> m_scissors;
						API::PagedStack<UINT> m_zPos;
						API::Global::Math::Space2D::Point m_checkPos;
						UINT m_maxZ;
						API::Rendering::IDrawable2D* m_res;
					public:
						CLOAK_CALL Context() : m_scissors(&m_pageAlloc), m_zPos(&m_pageAlloc), m_res(nullptr) {}
						CLOAK_CALL ~Context() {}

						API::Interface::IBasicGUI* CLOAK_CALL RunTest(In const API::Global::Math::Space2D::Point& pos)
						{
							m_checkPos = pos;
							m_maxZ = 0;
							CLOAK_ASSUME(m_res == nullptr);

							size_t guiSize = 0;
							API::Interface::IBasicGUI** guis = Impl::Interface::GetVisibleGUIs(&m_pageAlloc, &guiSize);

							for (size_t a = 0; a < guiSize; a++) { guis[a]->Draw(this); guis[a]->Release(); }

							m_scissors.clear();
							m_zPos.clear();
							m_pageAlloc.Clear();
							API::Interface::IBasicGUI* r = dynamic_cast<API::Interface::IBasicGUI*>(m_res);
							if (r != nullptr) { r->AddRef(); }
							SAVE_RELEASE(m_res);
							return r;
						}

						void CLOAK_CALL_THIS Draw(In API::Rendering::IDrawable2D* drawing, In API::Files::ITexture* img, In const API::Rendering::VERTEX_2D& vertex, In UINT Z, In_opt size_t bufferNum = 0) override
						{
							if (img == nullptr || drawing == nullptr) { return; }
							if (m_scissors.empty() == false)
							{
								const API::Rendering::SCALED_SCISSOR_RECT& rc = m_scissors.top();
								if (vertex.Position.X + vertex.Size.X < rc.TopLeftX || vertex.Position.Y + vertex.Size.Y < rc.TopLeftY || vertex.Position.X > rc.TopLeftX + rc.Width || vertex.Position.Y > rc.TopLeftY + rc.Height) { return; }
								if (m_checkPos.X<rc.TopLeftX || m_checkPos.Y<rc.TopLeftY || m_checkPos.X>rc.TopLeftX + rc.Width || m_checkPos.Y>rc.TopLeftY + rc.Height) { return; }
							}
							if (m_checkPos.X<vertex.Position.X || m_checkPos.Y<vertex.Position.Y || m_checkPos.X>vertex.Position.X + vertex.Size.X || m_checkPos.Y>vertex.Position.Y + vertex.Size.Y) { return; }
							UINT zpos = Z;
							if (m_zPos.empty() == false) { zpos += m_zPos.top(); }
							if (zpos < m_maxZ) { return; }

							API::Rendering::FLOAT2 sp(m_checkPos.X - vertex.Position.X, m_checkPos.Y - vertex.Position.Y);
							sp.X /= vertex.Size.X;
							sp.Y /= vertex.Size.Y;
							CLOAK_ASSUME(sp.X >= 0 && sp.X <= 1 && sp.Y >= 0 && sp.Y <= 1);
							API::Global::Math::Space2D::Point relPos;
							relPos.X = (sp.X*vertex.Rotation.Cosinus) - (sp.Y*vertex.Rotation.Sinus);
							relPos.Y = (sp.X*vertex.Rotation.Sinus) + (sp.Y*vertex.Rotation.Cosinus);
							sp.X = vertex.TexTopLeft.X + (relPos.X*(vertex.TexBottomRight.X - vertex.TexTopLeft.X));
							sp.Y = vertex.TexTopLeft.Y + (relPos.Y*(vertex.TexBottomRight.Y - vertex.TexTopLeft.Y));
							if (img->IsTransparentAt(sp.X, sp.Y, bufferNum)) { return; }

							m_maxZ = zpos;
							SWAP(m_res, drawing);
						}
						void CLOAK_CALL_THIS Draw(In API::Rendering::IDrawable2D* drawing, In API::Rendering::IColorBuffer* img, In const API::Rendering::VERTEX_2D& vertex, In UINT Z) override
						{
							if (img == nullptr || drawing == nullptr) { return; }
							if (m_scissors.empty() == false)
							{
								const API::Rendering::SCALED_SCISSOR_RECT& rc = m_scissors.top();
								if (vertex.Position.X + vertex.Size.X < rc.TopLeftX || vertex.Position.Y + vertex.Size.Y < rc.TopLeftY || vertex.Position.X > rc.TopLeftX + rc.Width || vertex.Position.Y > rc.TopLeftY + rc.Height) { return; }
								if (m_checkPos.X<rc.TopLeftX || m_checkPos.Y<rc.TopLeftY || m_checkPos.X>rc.TopLeftX + rc.Width || m_checkPos.Y>rc.TopLeftY + rc.Height) { return; }
							}
							if (m_checkPos.X<vertex.Position.X || m_checkPos.Y<vertex.Position.Y || m_checkPos.X>vertex.Position.X + vertex.Size.X || m_checkPos.Y>vertex.Position.Y + vertex.Size.Y) { return; }
							UINT zpos = Z;
							if (m_zPos.empty() == false) { zpos += m_zPos.top(); }
							if (zpos < m_maxZ) { return; }
							m_maxZ = zpos;
							SWAP(m_res, drawing);
						}
						void CLOAK_CALL_THIS StartNewPatch(In UINT Z) override
						{
							UINT zpos = Z;
							if (m_zPos.empty() == false) { zpos += m_zPos.top(); }
							m_zPos.push(zpos);
						}
						void CLOAK_CALL_THIS EndPatch() override
						{
							m_zPos.pop();
						}
						void CLOAK_CALL_THIS SetScissor(In const API::Rendering::SCALED_SCISSOR_RECT& rct) override
						{
							API::Rendering::SCALED_SCISSOR_RECT last;
							if (m_scissors.empty())
							{
								last.TopLeftX = 0;
								last.TopLeftY = 0;
								last.Width = 1;
								last.Height = 1;
							}
							else { last = m_scissors.top(); }
							API::Rendering::SCALED_SCISSOR_RECT rc;
							rc.TopLeftX = max(last.TopLeftX, rct.TopLeftX);
							rc.TopLeftY = max(last.TopLeftY, rct.TopLeftY);
							rc.Width = min(last.Width, rct.Width);
							rc.Height = min(last.Height, rct.Height);
							m_scissors.push(rc);
						}
						void CLOAK_CALL_THIS ResetScissor() override
						{
							if (m_scissors.empty() == false) { m_scissors.pop(); }
						}
						void CLOAK_CALL_THIS Execute() override {}
				};

#pragma warning(pop)

				API::Queue<Context*>* g_aviableContexts = nullptr;
				API::List<Context*>* g_contexts = nullptr;
				API::Helper::ISyncSection* g_sync = nullptr;

				void CLOAK_CALL Initialize()
				{
					g_aviableContexts = new API::Queue<Context*>();
					g_contexts = new API::List<Context*>();
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_sync));
				}
				void CLOAK_CALL Release()
				{
					SAVE_RELEASE(g_sync);
					for (size_t a = 0; a < g_contexts->size(); a++) { SAVE_RELEASE(g_contexts->at(a)); }
					delete g_contexts;
					delete g_aviableContexts;
				}
				API::Interface::IBasicGUI* CLOAK_CALL FindGUIAt(In const API::Global::Math::Space2D::Point& pos)
				{
					Context* context = nullptr;
					API::Helper::Lock lock(g_sync);
					if (g_aviableContexts->empty() == false) 
					{ 
						context = g_aviableContexts->front(); 
						g_aviableContexts->pop(); 
					}
					else
					{
						context = new Context();
						g_contexts->push_back(context);
					}
					lock.unlock();

					API::Interface::IBasicGUI* res = context->RunTest(pos);

					lock.lock(g_sync);
					g_aviableContexts->push(context);
					lock.unlock();

					return res;
				}
			}
		}
	}
}