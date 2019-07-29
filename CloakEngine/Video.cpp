#include "stdafx.h"
#include "CloakEngine/Global/Video.h"
#include "Implementation/Global/Video.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Video {
				Impl::Rendering::IVideoEncoder* g_encoder = nullptr;

				CLOAKENGINE_API void CLOAK_CALL StartVideo(std::u16string filePath)
				{
					g_encoder->StartPlayback(filePath);
				}
				CLOAKENGINE_API void CLOAK_CALL StopVideo()
				{
					g_encoder->StopPlayback();
				}
				CLOAKENGINE_API bool CLOAK_CALL IsVideoPlaying()
				{
					return g_encoder->IsVideoPlaying();
				}
			}
		}
	}
	namespace Impl {
		namespace Global {
			namespace Video {
				void CLOAK_CALL Initialize(In Impl::Rendering::IVideoEncoder* encoder)
				{
					assert(API::Global::Video::g_encoder == nullptr);
					API::Global::Video::g_encoder = encoder;
					if (encoder != nullptr) { encoder->AddRef(); }
				}
				void CLOAK_CALL Release()
				{
					SAVE_RELEASE(API::Global::Video::g_encoder);
				}
				void CLOAK_CALL OnVideoEvent()
				{
					API::Global::Video::g_encoder->OnVideoEvent();
				}
				void CLOAK_CALL OnSizeChange(In const LPRECT rec)
				{
					API::Global::Video::g_encoder->OnWindowResize(rec);
				}
			}
		}
	}
}

//Depricated, will be replaced with Rendering::VideoEncoder
#ifdef OLD_VIDEO
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Helper/StringConvert.h"

#include "Implementation/Global/Video.h"
#include "Implementation/Global/Game.h"

#include "Engine/LinkedList.h"
#include "Engine/WindowHandler.h"


#include <atomic>
#include <d3d9.h>
#include <dshow.h>
#include <evr.h>
#include <vmr9.h>

namespace CloakEngine {
	namespace API {
		namespace Global {
			namespace Video {
				enum class RdyState { INIT, WAIT, RDY };
				enum class RenderTypes {
					EVR = 0,
					VRM9 = EVR + 1,
					VRM7 = VRM9 + 1,
				};
				RenderTypes CLOAK_CALL operator++(RenderTypes& i) //prefix
				{
					i = static_cast<RenderTypes>(static_cast<unsigned int>(i) + 1);
					return i;
				}
				RenderTypes CLOAK_CALL operator++(RenderTypes& i, int) //postfix
				{
					RenderTypes o = i;
					i = static_cast<RenderTypes>(static_cast<unsigned int>(i) + 1);
					return o;
				}
				class IRenderer {
					public:
						virtual bool CLOAK_CALL_THIS HasVideo() const = 0;
						virtual HRESULT CLOAK_CALL_THIS AddToGraph() = 0;
						virtual HRESULT CLOAK_CALL_THIS FinalizeGraph() = 0;
						virtual HRESULT CLOAK_CALL_THIS UpdateWindow(In const LPRECT rec) = 0;
						virtual HRESULT CLOAK_CALL_THIS UpdateDisplayMode() = 0;
						virtual HRESULT CLOAK_CALL_THIS Render(In Impl::Rendering::IColorBuffer* dst) = 0;
				};

				Helper::ISyncSection* g_syncQueue = nullptr;
				Helper::ISyncSection* g_syncGraph = nullptr;
				API::Queue<std::u16string>* g_playQueue;
				std::atomic<bool> g_isVideoPlaying = false;
				std::atomic<bool> g_hasVideoPlayback = false;
				std::atomic<RdyState> g_rdyState = RdyState::INIT;
				std::u16string* g_nextFile = nullptr;

				IRenderer* g_renderer = nullptr;

				IGraphBuilder* g_graph = nullptr;
				IMediaControl* g_mediaControl = nullptr;
				IMediaEventEx* g_mediaEvent = nullptr;

				HRESULT CLOAK_CALL IsPinConntected(In IPin* pin, Out bool* res)
				{
					IPin* tmp = nullptr;
					HRESULT hRet = pin->ConnectedTo(&tmp);
					if (SUCCEEDED(hRet)) { *res = true; }
					else if (hRet == VFW_E_NOT_CONNECTED) { *res = false; hRet = S_OK; }
					RELEASE(tmp);
					return hRet;
				}
				HRESULT CLOAK_CALL IsPinDirection(In IPin* pin, In PIN_DIRECTION dir, Out bool* res)
				{
					PIN_DIRECTION pinDir;
					HRESULT hRet = pin->QueryDirection(&pinDir);
					if (SUCCEEDED(hRet)) { *res = (pinDir == dir); }
					return hRet;
				}
				HRESULT CLOAK_CALL FindPin(In IBaseFilter* filter, In PIN_DIRECTION dir, Outptr IPin** pin)
				{
					*pin = nullptr;
					IEnumPins* pinEnum = nullptr;
					HRESULT hRet = filter->EnumPins(&pinEnum);
					if (FAILED(hRet)) { return hRet; }
					IPin* p = nullptr;
					bool found = false;
					while (found == false && pinEnum->Next(1, &p, nullptr) == S_OK)
					{
						bool con = false;
						hRet = IsPinConntected(p, &con);
						if (SUCCEEDED(hRet))
						{
							if (con) { hRet = IsPinDirection(p, dir, &found); }
						}
						if (FAILED(hRet))
						{
							found = true;
						}
						else if (found)
						{
							*pin = p;
							p->AddRef();
						}
						RELEASE(p);
					}
					RELEASE(p);
					if (found == false) { hRet = VFW_E_NOT_FOUND; }
					return hRet;
				}

				bool CLOAK_CALL RemoveIfUnconnected(In IBaseFilter* filter)
				{
					IPin* pin = nullptr;
					HRESULT hRet = FindPin(filter, PINDIR_INPUT, &pin);
					RELEASE(pin);
					if (FAILED(hRet))
					{
						CloakCheckOK(g_graph->RemoveFilter(filter), API::Global::Debug::Error::VIDEO_REMOVE_FILTER, false);
						return true;
					}
					return false;
				}
				HRESULT CLOAK_CALL AddFilter(In REFGUID clsid, Outptr IBaseFilter** filter, In_opt LPCWSTR name = nullptr)
				{
					if (filter != nullptr)
					{
						RELEASE(*filter);
						IBaseFilter* res = nullptr;
						HRESULT hRet = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, CE_QUERY_ARGS(&res));
						if (SUCCEEDED(hRet))
						{
							hRet = g_graph->AddFilter(res, name);
							if (SUCCEEDED(hRet)) { *filter = res; res->AddRef(); }
						}
						res->Release();
						return hRet;
					}
					return E_FAIL;
				}
				HRESULT CLOAK_CALL InitVMRControl(In IBaseFilter* vmr, Outptr IVMRWindowlessControl** control)
				{
					IVMRFilterConfig* config = nullptr;
					HRESULT hRet = vmr->QueryInterface(CE_QUERY_ARGS(&config));
					if (SUCCEEDED(hRet))
					{
						hRet = config->SetRenderingMode(VMRMode_Windowless);
						if (SUCCEEDED(hRet))
						{
							IVMRWindowlessControl* con = nullptr;
							hRet = vmr->QueryInterface(CE_QUERY_ARGS(&con));
							if (SUCCEEDED(hRet))
							{
								hRet = con->SetVideoClippingWindow(Engine::WindowHandler::getWindow());
								if (SUCCEEDED(hRet))
								{
									hRet = con->SetAspectRatioMode(VMR_ARMODE_LETTER_BOX);
									if (SUCCEEDED(hRet))
									{
										*control = con;
										con->AddRef();
									}
								}
							}
							RELEASE(con);
						}
					}
					RELEASE(config);
					return hRet;
				}
				HRESULT CLOAK_CALL InitVMRControl(In IBaseFilter* vmr, Outptr IVMRWindowlessControl9** control)
				{
					IVMRFilterConfig9* config = nullptr;
					HRESULT hRet = vmr->QueryInterface(CE_QUERY_ARGS(&config));
					if (SUCCEEDED(hRet))
					{
						hRet = config->SetRenderingMode(VMR9Mode_Windowless);
						if (SUCCEEDED(hRet))
						{
							IVMRWindowlessControl9* con = nullptr;
							hRet = vmr->QueryInterface(CE_QUERY_ARGS(&con));
							if (SUCCEEDED(hRet))
							{
								hRet = con->SetVideoClippingWindow(Engine::WindowHandler::getWindow());
								if (SUCCEEDED(hRet))
								{
									hRet = con->SetAspectRatioMode(VMR9ARMode_LetterBox);
									if (SUCCEEDED(hRet))
									{
										*control = con;
										con->AddRef();
									}
								}
							}
							RELEASE(con);
						}
					}
					RELEASE(config);
					return hRet;
				}
				HRESULT CLOAK_CALL InitEVR(In IBaseFilter* evr, Outptr IMFVideoDisplayControl** control)
				{
					IMFGetService* service = nullptr;
					HRESULT hRet = evr->QueryInterface(CE_QUERY_ARGS(&service));
					if (SUCCEEDED(hRet))
					{
						IMFVideoDisplayControl* display = nullptr;
						hRet = service->GetService(MR_VIDEO_RENDER_SERVICE, CE_QUERY_ARGS(&display));
						if (SUCCEEDED(hRet))
						{
							hRet = display->SetVideoWindow(Engine::WindowHandler::getWindow());
							if (SUCCEEDED(hRet))
							{
								hRet = display->SetAspectRatioMode(MFVideoARMode_PreservePicture);
								if (SUCCEEDED(hRet))
								{
									*control = display;
									display->AddRef();
								}
							}
						}
						RELEASE(service);
					}
					RELEASE(service);
					return hRet;
				}

				class VRM7 : public IRenderer {
					public:
						CLOAK_CALL VRM7()
						{
							m_windowControl = nullptr;
						}
						CLOAK_CALL ~VRM7()
						{
							RELEASE(m_windowControl);
						}
						bool CLOAK_CALL_THIS HasVideo() const override { return m_windowControl != nullptr; }
						HRESULT CLOAK_CALL_THIS AddToGraph() override
						{
							IBaseFilter* vmr = nullptr;
							HRESULT hRet = AddFilter(CLSID_VideoMixingRenderer, &vmr, L"Video: VMR7");
							if (SUCCEEDED(hRet)) { hRet = InitVMRControl(vmr, &m_windowControl); }
							RELEASE(vmr);
							return hRet;
						}
						HRESULT CLOAK_CALL_THIS FinalizeGraph() override
						{
							if (m_windowControl == nullptr) { return S_OK; }
							IBaseFilter* filter = nullptr;
							HRESULT hRet = m_windowControl->QueryInterface(CE_QUERY_ARGS(&filter));
							if (SUCCEEDED(hRet))
							{
								if (RemoveIfUnconnected(filter)) { RELEASE(m_windowControl); }
							}
							RELEASE(filter);
							return hRet;
						}
						HRESULT CLOAK_CALL_THIS UpdateWindow(In const LPRECT rec) override
						{
							if (m_windowControl == nullptr) { return S_OK; }
							if (rec != nullptr)
							{
								return m_windowControl->SetVideoPosition(nullptr, rec);
							}
							else
							{
								RECT rc;
								GetClientRect(Engine::WindowHandler::getWindow(), &rc);
								return m_windowControl->SetVideoPosition(nullptr, &rc);
							}
						}
						HRESULT CLOAK_CALL_THIS UpdateDisplayMode() override
						{
							if (m_windowControl == nullptr) { return S_OK; }
							return m_windowControl->DisplayModeChanged();
						}
						HRESULT CLOAK_CALL_THIS Render(In Impl::Rendering::IColorBuffer* dst) override
						{
							if (m_windowControl == nullptr) { return S_OK; }
							return m_windowControl->RepaintVideo(Engine::WindowHandler::getWindow(), hdc);
						}
						void* CLOAK_CALL operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }
					private:
						IVMRWindowlessControl* m_windowControl;
				};
				class VRM9 : public IRenderer {
					public:
						CLOAK_CALL VRM9()
						{
							m_windowControl = nullptr;
						}
						CLOAK_CALL ~VRM9()
						{
							RELEASE(m_windowControl);
						}
						bool CLOAK_CALL_THIS HasVideo() const override { return m_windowControl != nullptr; }
						HRESULT CLOAK_CALL_THIS AddToGraph() override
						{
							IBaseFilter* vmr = nullptr;
							HRESULT hRet = AddFilter(CLSID_VideoMixingRenderer9, &vmr, L"Video: VMR9");
							if (SUCCEEDED(hRet)) { hRet = InitVMRControl(vmr, &m_windowControl); }
							RELEASE(vmr);
							return hRet;
						}
						HRESULT CLOAK_CALL_THIS FinalizeGraph() override
						{
							if (m_windowControl == nullptr) { return S_OK; }
							IBaseFilter* filter = nullptr;
							HRESULT hRet = m_windowControl->QueryInterface(CE_QUERY_ARGS(&filter));
							if (SUCCEEDED(hRet)) 
							{
								if (RemoveIfUnconnected(filter)) { RELEASE(m_windowControl); }
							}
							RELEASE(filter);
							return hRet;
						}
						HRESULT CLOAK_CALL_THIS UpdateWindow(In const LPRECT rec) override
						{
							if (m_windowControl == nullptr) { return S_OK; }
							if (rec != nullptr)
							{
								return m_windowControl->SetVideoPosition(nullptr, rec);
							}
							else
							{
								RECT rc;
								GetClientRect(Engine::WindowHandler::getWindow(), &rc);
								return m_windowControl->SetVideoPosition(nullptr, &rc);
							}
						}
						HRESULT CLOAK_CALL_THIS UpdateDisplayMode() override
						{
							if (m_windowControl == nullptr) { return S_OK; }
							return m_windowControl->DisplayModeChanged();
						}
						HRESULT CLOAK_CALL_THIS Render(In Impl::Rendering::IColorBuffer* dst) override
						{
							if (m_windowControl == nullptr) { return S_OK; }
							return m_windowControl->RepaintVideo(Engine::WindowHandler::getWindow(), hdc);
						}
						void* CLOAK_CALL operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }
					private:
						IVMRWindowlessControl9* m_windowControl;
				};
				class EVR : public IRenderer {
					public:
						CLOAK_CALL EVR()
						{
							m_filter = nullptr;
							m_displayControl = nullptr;
						}
						CLOAK_CALL ~EVR()
						{
							RELEASE(m_filter);
							RELEASE(m_displayControl);
						}
						bool CLOAK_CALL_THIS HasVideo() const override { return m_displayControl != nullptr; }
						HRESULT CLOAK_CALL_THIS AddToGraph() override
						{
							IBaseFilter* evr = nullptr;
							HRESULT hRet = AddFilter(CLSID_EnhancedVideoRenderer, &evr, L"Video: EVR");
							if (SUCCEEDED(hRet))
							{
								hRet = InitEVR(evr, &m_displayControl);
								if (SUCCEEDED(hRet))
								{
									m_filter = evr;
									evr->AddRef();
								}
							}
							RELEASE(evr);
							return hRet;
						}
						HRESULT CLOAK_CALL_THIS FinalizeGraph() override
						{
							if (m_filter == nullptr) { return S_OK; }
							if (RemoveIfUnconnected(m_filter))
							{
								RELEASE(m_filter);
								RELEASE(m_displayControl);
							}
							return S_OK;
						}
						HRESULT CLOAK_CALL_THIS UpdateWindow(In const LPRECT rec) override
						{
							if (m_displayControl == nullptr) { return S_OK; }
							if (rec != nullptr)
							{
								return m_displayControl->SetVideoPosition(nullptr, rec);
							}
							else
							{
								RECT rc;
								GetClientRect(Engine::WindowHandler::getWindow(), &rc);
								return m_displayControl->SetVideoPosition(nullptr, &rc);
							}
						}
						HRESULT CLOAK_CALL_THIS UpdateDisplayMode() override { return S_OK; }
						HRESULT CLOAK_CALL_THIS Render(In Impl::Rendering::IColorBuffer* dst) override
						{
							if (m_displayControl == nullptr) { return S_OK; }
							return m_displayControl->RepaintVideo();
						}
						void* CLOAK_CALL operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }
					private:
						IBaseFilter* m_filter;
						IMFVideoDisplayControl* m_displayControl;
				};

				HRESULT CLOAK_CALL CreateRenderer()
				{
					HRESULT hRet = E_FAIL;
					for (RenderTypes a = RenderTypes::EVR; a <= RenderTypes::VRM7; a++)
					{
						IRenderer* res = nullptr;
						switch (a)
						{
							case CloakEngine::API::Global::Video::RenderTypes::EVR:
								res = new EVR();
								break;
							case CloakEngine::API::Global::Video::RenderTypes::VRM9:
								res = new VRM9();
								break;
							case CloakEngine::API::Global::Video::RenderTypes::VRM7:
								res = new VRM7();
								break;
							default:
								break;
						}
						if (res == nullptr) { return E_OUTOFMEMORY; }
						hRet = res->AddToGraph();
						if (SUCCEEDED(hRet)) 
						{ 
							g_renderer = res; 
							break;
						}
						delete res;
					}
					return hRet;
				}
				void CLOAK_CALL StartPlayback()
				{
					API::Helper::Lock lock(g_syncGraph);
					g_mediaEvent->SetNotifyWindow((OAHWND)Engine::WindowHandler::getWindow(), WM_VIDEO_EVENT, 0);
					IBaseFilter* filter = nullptr;
					HRESULT hRet = g_graph->AddSourceFilter(Helper::StringConvert::ConvertToW(*g_nextFile).c_str(), L"Video: SrcFilter", &filter);
					if (FAILED(hRet)) { Log::WriteToLog(u"Video not found: '" + *g_nextFile + u"'", Log::Type::Warning); }
					else
					{
						bool didRender = false;
						IFilterGraph3* filterGraph = nullptr;
						hRet = g_graph->QueryInterface(CE_QUERY_ARGS(&filterGraph));
						if (CloakCheckOK(hRet, API::Global::Debug::Error::VIDEO_NO_FILTER, false))
						{
							hRet = CreateRenderer();
							if (CloakCheckOK(hRet, API::Global::Debug::Error::VIDEO_NO_RENDERER, false))
							{
								IBaseFilter* audioFilter = nullptr;
								hRet = AddFilter(CLSID_DSoundRender, &audioFilter, L"Video: AudioFilter");
								if (CloakCheckOK(hRet, API::Global::Debug::Error::VIDEO_NO_AUDIO, false))
								{
									IEnumPins* pinEnum = nullptr;
									hRet = filter->EnumPins(&pinEnum);
									if (CloakCheckOK(hRet, API::Global::Debug::Error::VIDEO_NO_PIN, false))
									{
										IPin* pin = nullptr;
										while (pinEnum->Next(1, &pin, nullptr) == S_OK)
										{
											hRet = filterGraph->RenderEx(pin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, nullptr);
											pin->Release();
											if (SUCCEEDED(hRet)) { didRender = true; }
										}
										hRet = g_renderer->FinalizeGraph();
										if (CloakCheckOK(hRet, API::Global::Debug::Error::VIDEO_FINALIZE, false)) { RemoveIfUnconnected(audioFilter); }
									}
									RELEASE(pinEnum);
								}
								RELEASE(audioFilter);
							}
						}
						RELEASE(filterGraph);
						if (didRender) { g_mediaControl->Run(); }
					}
				}
				bool CLOAK_CALL InitPlayback(const std::u16string& file)
				{
					if (g_hasVideoPlayback.exchange(true) == false)
					{
						RdyState oldRdy = g_rdyState.load();
						RdyState newRdy;
						do {
							newRdy = oldRdy;
							if (newRdy != RdyState::RDY) { newRdy = RdyState::WAIT; }
						} while (!g_rdyState.compare_exchange_strong(oldRdy, newRdy));

						API::Helper::Lock lock(g_syncGraph);
						HRESULT hRet = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, CE_QUERY_ARGS(&g_graph));
						if (CloakCheckOK(hRet, API::Global::Debug::Error::VIDEO_NO_DEVICE, true))
						{
							hRet = g_graph->QueryInterface(CE_QUERY_ARGS(&g_mediaControl));
							if (CloakCheckOK(hRet, API::Global::Debug::Error::VIDEO_NO_CONTROL, true))
							{
								hRet = g_graph->QueryInterface(CE_QUERY_ARGS(&g_mediaEvent));
								if (CloakCheckOK(hRet, API::Global::Debug::Error::VIDEO_NO_EVENT, true))
								{
									*g_nextFile = file;
									if (newRdy == RdyState::RDY) { StartPlayback(); }
									return true;
								}
								RELEASE(g_mediaEvent);
							}
							RELEASE(g_mediaControl);
						}
						RELEASE(g_graph);
					}
					return false;
				}
				void CLOAK_CALL StopPlayback()
				{
					if (g_hasVideoPlayback.exchange(false) == true)
					{
						API::Helper::Lock lock(g_syncGraph);
						if (g_mediaEvent != nullptr) { g_mediaEvent->SetNotifyWindow(NULL, 0, 0); }

						RELEASE(g_graph);
						RELEASE(g_mediaControl);
						RELEASE(g_mediaEvent);

						delete g_renderer;
						g_renderer = nullptr;
						lock.unlock();
						lock.lock(g_syncQueue);
						if (g_playQueue->size() > 0) 
						{ 
							InitPlayback(g_playQueue->front()); 
							g_playQueue->pop();
						}
						else { g_isVideoPlaying = false; }
					}
				}

				CLOAKENGINE_API void CLOAK_CALL StartVideo(std::u16string filePath)
				{
					bool ivp = g_isVideoPlaying.exchange(true);
					if (ivp == false) { ivp = !InitPlayback(filePath); }
					if (ivp == true)
					{
						API::Helper::Lock lock(g_syncQueue);
						g_playQueue->push(filePath);
					}
				}
				CLOAKENGINE_API void CLOAK_CALL StopVideo()
				{
					if (g_isVideoPlaying == true && g_hasVideoPlayback == true)
					{
						API::Helper::Lock lock(g_syncGraph);
						if (g_mediaControl != nullptr) { g_mediaControl->Stop(); }
					}
				}
				CLOAKENGINE_API bool CLOAK_CALL IsVideoPlaying() { return g_isVideoPlaying; }
			}
		}
	}
	namespace Impl {
		namespace Global {
			namespace Video {
				void CLOAK_CALL Initialize()
				{
					API::Global::Video::g_playQueue = new API::Queue<std::u16string>();
					API::Global::Video::g_nextFile = new std::u16string(u"");
					CREATE_INTERFACE(CE_QUERY_ARGS(&API::Global::Video::g_syncQueue));
					CREATE_INTERFACE(CE_QUERY_ARGS(&API::Global::Video::g_syncGraph));
				}
				void CLOAK_CALL Release()
				{
					SAVE_RELEASE(API::Global::Video::g_syncQueue);
					SAVE_RELEASE(API::Global::Video::g_syncGraph);
					delete API::Global::Video::g_playQueue;
					delete API::Global::Video::g_nextFile;
				}
				void CLOAK_CALL OnStartWindow(In HWND wnd)
				{
					API::Global::Video::RdyState oldRdy = API::Global::Video::g_rdyState.exchange(API::Global::Video::RdyState::RDY);
					if (oldRdy == API::Global::Video::RdyState::WAIT) { API::Global::Video::StartPlayback(); }
				}
				void CLOAK_CALL OnVideoEvent()
				{
					API::Helper::Lock lock(API::Global::Video::g_syncGraph);
					if (API::Global::Video::g_mediaEvent != nullptr)
					{
						long ev = 0;
						LONG_PTR p1, p2;
						while (SUCCEEDED(API::Global::Video::g_mediaEvent->GetEvent(&ev, &p1, &p2, 0)))
						{
							API::Global::Video::g_mediaEvent->FreeEventParams(ev, p1, p2);
							switch (ev)
							{
								case EC_ERRORABORT:
									API::Global::Log::WriteToLog("Error during video playback occurred!", API::Global::Log::Type::Warning);
								case EC_COMPLETE:
								case EC_USERABORT:
									API::Global::Video::StopPlayback();
									break;
								default:
									break;
							}
						}
					}
				}
				void CLOAK_CALL RenderVideo(In Impl::Rendering::IColorBuffer* dst)
				{
					if (API::Global::Video::g_isVideoPlaying)
					{
						API::Helper::Lock lock(API::Global::Video::g_syncGraph);
						if (API::Global::Video::g_renderer != nullptr) { API::Global::Video::g_renderer->Render(hdc); }
					}
				}
				void CLOAK_CALL OnDisplayModeChange()
				{
					if (API::Global::Video::g_isVideoPlaying)
					{
						API::Helper::Lock lock(API::Global::Video::g_syncGraph);
						if (API::Global::Video::g_renderer != nullptr) { API::Global::Video::g_renderer->UpdateDisplayMode(); }
					}
				}
				void CLOAK_CALL OnSizeChange(In const LPRECT rec)
				{
					if (API::Global::Video::g_isVideoPlaying)
					{
						API::Helper::Lock lock(API::Global::Video::g_syncGraph);
						if (API::Global::Video::g_renderer != nullptr) { API::Global::Video::g_renderer->UpdateWindow(rec); }
					}
				}
				IMFMediaEngine* g_mediaEngine = nullptr;




			}
		}
	}
	namespace Impl {
		namespace Global {
			namespace Video {
				void CLOAK_CALL Initialize()
				{

				}
				void CLOAK_CALL OnStartWindow(In HWND wnd)
				{

				}
				void CLOAK_CALL OnVideoEvent()
				{

				}
				void CLOAK_CALL Release()
				{

				}
				void CLOAK_CALL RenderVideo(In Impl::Rendering::IColorBuffer* dst)
				{

				}
				void CLOAK_CALL OnDisplayModeChange()
				{

				}
				void CLOAK_CALL OnSizeChange(In const LPRECT rec)
				{

				}
			}
		}
	}
}
#endif