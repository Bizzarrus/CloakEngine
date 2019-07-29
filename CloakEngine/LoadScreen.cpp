#include "stdafx.h"
#include "Implementation/Interface/LoadScreen.h"
#include "Implementation/Interface/BasicGUI.h"

#include <cstdlib>

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace LoadScreen_v1 {
#pragma warning(push)
#pragma warning(disable: 4250)
				class TopGUI : public BasicGUI {
					public:
						virtual void CLOAK_CALL_THIS Show() override
						{
							API::Helper::Lock lock(m_sync);
							m_visible = true;
						}
						virtual void CLOAK_CALL_THIS Hide() override
						{
							API::Helper::Lock lock(m_sync);
							m_visible = false;
						}
				};
#pragma warning(pop)

				TopGUI* g_topGUI = nullptr;
				API::Helper::ISyncSection* g_sync = nullptr;
				Engine::LinkedList<LoadScreen*>* g_pool;
				LoadScreen* g_lastRandom = nullptr;

				void CLOAK_CALL LoadScreen::Initialize()
				{
					g_pool = new Engine::LinkedList<LoadScreen*>();
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_sync));
					g_topGUI = new TopGUI();
				}
				void CLOAK_CALL LoadScreen::Terminate()
				{
					API::Helper::Lock lock(g_sync);
					if (g_pool != nullptr)
					{
						g_pool->for_each([=](LoadScreen* ls) {if (ls != nullptr) { ls->Release(); }});
					}
					lock.unlock();
					SAVE_RELEASE(g_lastRandom);
					SAVE_RELEASE(g_sync);
					SAVE_RELEASE(g_topGUI);
					delete g_pool;
				}
				void CLOAK_CALL LoadScreen::ShowRandom(In API::Rendering::Context_v1::IContext2D* context)
				{
					API::Helper::Lock lock(g_sync);
					if (g_lastRandom == nullptr && g_pool->size() > 0)
					{
						size_t rp = (static_cast<size_t>(std::rand()) % g_pool->size()) + 1;
						for (Engine::LinkedList<LoadScreen*>::iterator a = g_pool->begin(); rp > 0; a++, rp--) { g_lastRandom = *a; }
						if (g_lastRandom != nullptr) { g_lastRandom->AddRef(); }
					}
					if (g_lastRandom != nullptr) { g_lastRandom->Draw(context); }
				}
				void CLOAK_CALL LoadScreen::Hide()
				{
					API::Helper::Lock lock(g_sync);
					if (g_lastRandom != nullptr)
					{
						g_lastRandom->iHide();
						g_lastRandom->Release();
						g_lastRandom = nullptr;
					}
				}

				CLOAK_CALL LoadScreen::LoadScreen()
				{
					m_gui = nullptr;
					m_img = nullptr;
					m_updateFlag = false;
					m_poolPos = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					DEBUG_NAME(LoadScreen);
				}
				CLOAK_CALL LoadScreen::~LoadScreen()
				{
					SAVE_RELEASE(m_sync);
					SAVE_RELEASE(m_gui);
					SAVE_RELEASE(m_img);
				}
				void CLOAK_CALL_THIS LoadScreen::SetGUI(In API::Interface::IBasicGUI* gui)
				{
					API::Helper::Lock lock(m_sync);
					SAVE_RELEASE(m_gui);
					m_gui = gui;
					if (m_gui != nullptr) 
					{ 
						m_gui->AddRef();
						m_gui->SetParent(nullptr);
					}
				}
				void CLOAK_CALL_THIS LoadScreen::SetBackgroundImage(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					SAVE_RELEASE(m_img);
					m_img = img;
					if (m_img != nullptr) 
					{ 
						m_img->AddRef(); 
						m_img->RequestLOD(API::Files::LOD::ULTRA);
						m_img->load(API::Files::Priority::HIGH); 
					}
				}
				void CLOAK_CALL_THIS LoadScreen::AddToPool()
				{
					API::Helper::Lock lock(m_sync);
					if (m_poolPos == nullptr && g_pool != nullptr)
					{
						AddRef();
						API::Helper::Lock glock(g_sync);
						m_poolPos = g_pool->push(this);
					}
				}
				void CLOAK_CALL_THIS LoadScreen::RemoveFromPool()
				{
					API::Helper::Lock lock(m_sync);
					if (m_poolPos != nullptr && g_pool != nullptr)
					{
						API::Helper::Lock glock(g_sync);
						g_pool->remove(m_poolPos);
						glock.unlock();
						m_poolPos = nullptr;
						Release();
					}
				}

				void CLOAK_CALL_THIS LoadScreen::Draw(In API::Rendering::Context_v1::IContext2D* context)
				{
					API::Helper::Lock lock(m_sync);
					if (m_img != nullptr && m_img->isLoaded(API::Files::Priority::HIGH))
					{
						context->Draw(this, m_img, m_vertex, 0);
					}
					lock.unlock();
					lock.lock(g_sync);
					g_topGUI->Draw(context);
				}
				void CLOAK_CALL_THIS LoadScreen::UpdateDrawInformations(In API::Global::Time time, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager)
				{
					if (m_updateFlag.exchange(false) == true)
					{
						API::Helper::Lock lock(m_sync);
						m_vertex.Position.X = 0;
						m_vertex.Position.Y = 0;
						m_vertex.Color[0] = API::Helper::Color::White;
						m_vertex.Size.X = 1;
						m_vertex.Size.Y = 1;
						m_vertex.TexBottomRight.X = 1;
						m_vertex.TexBottomRight.Y = 1;
						m_vertex.TexTopLeft.X = 0;
						m_vertex.TexTopLeft.Y = 0;
						m_vertex.Rotation.Sinus = 0;
						m_vertex.Rotation.Cosinus = 1;
						m_vertex.Flags = API::Rendering::Vertex2DFlags::NONE;
					}
				}
				bool CLOAK_CALL_THIS LoadScreen::SetUpdateFlag() { return m_updateFlag.exchange(true); }

				Success(return == true) bool CLOAK_CALL_THIS LoadScreen::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, SavePtr, Interface::LoadScreen_v1, LoadScreen);
				}
				void CLOAK_CALL_THIS LoadScreen::iShow()
				{
					API::Helper::Lock lock(m_sync);
					if (m_gui != nullptr)
					{
						API::Helper::Lock glock(g_sync);
						m_gui->SetParent(g_topGUI);
						glock.unlock();
						m_gui->Show();
					}
				}
				void CLOAK_CALL_THIS LoadScreen::iHide()
				{
					API::Helper::Lock lock(m_sync);
					if (m_gui != nullptr)
					{
						m_gui->Hide();
						m_gui->SetParent(nullptr);
					}
				}
			}
		}
	}
}