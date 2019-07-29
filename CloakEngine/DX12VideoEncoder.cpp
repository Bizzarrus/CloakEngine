#include "stdafx.h"
#include "Implementation/Rendering/DX12/VideoEncoder.h"
#include "Implementation/Rendering/DX12/Defines.h"

#include "CloakEngine/Helper/StringConvert.h"

#include "Engine/WindowHandler.h"

#include <mfapi.h>

#pragma comment(lib,"mfplat.lib")

#define DEL(x) delete x; x=nullptr

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace VideoEncoder_v1 {

					class VideoNotify : public virtual IMFMediaEngineNotify, public Impl::Helper::SavePtr{
						public:
							CLOAK_CALL VideoNotify()
							{

							}
							CLOAK_CALL ~VideoNotify()
							{

							}
							HRESULT STDMETHODCALLTYPE EventNotify(In DWORD event, In DWORD_PTR arg1, In DWORD arg2) override
							{

							}
						protected:
							Success(return == true) bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override
							{
								if (riid == __uuidof(IMFMediaEngineNotify)) { *ptr = (IMFMediaEngineNotify*)this; return true; }
								return SavePtr::iQueryInterface(riid, ptr);
							}
					};



					CLOAK_CALL VideoEncoder::VideoEncoder()
					{
						DEBUG_NAME(VideoEncoder);
						IMFMediaEngineClassFactory* factory = nullptr;
						HRESULT hRet = CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER, CE_QUERY_ARGS(&factory));
						if (CloakCheckOK(hRet, API::Global::Debug::Error::VIDEO_NO_DEVICE, true))
						{
							IMFAttributes* attr = nullptr;
							hRet = MFCreateAttributes(&attr, 1);
							if (CloakCheckOK(hRet, API::Global::Debug::Error::VIDEO_NO_EVENT, true))
							{
								/*
								
								MEDIA::ThrowIfFailed(
								spAttributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, (IUnknown*) m_spDXGIManager.Get())
								);

								MEDIA::ThrowIfFailed(
								spAttributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, (IUnknown*) spNotify.Get())
								);

								MEDIA::ThrowIfFailed(
								spAttributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, m_d3dFormat)
								);

								// Create the Media Engine.
								const DWORD flags = MF_MEDIA_ENGINE_WAITFORSTABLE_STATE;
								MEDIA::ThrowIfFailed(
								spFactory->CreateInstance(flags, spAttributes.Get(), &m_spMediaEngine)
								);

								MEDIA::ThrowIfFailed(
								m_spMediaEngine.Get()->QueryInterface(__uuidof(IMFMediaEngine), (void**) &m_spEngineEx)
								);
								
								*/
							}
							SAVE_RELEASE(attr);
						}
						SAVE_RELEASE(factory);
					}
					CLOAK_CALL VideoEncoder::~VideoEncoder()
					{
						if (m_engine != nullptr) { m_engine->Shutdown(); }
						SAVE_RELEASE(m_engine);
					}

					bool CLOAK_CALL_THIS VideoEncoder::StartPlayback(In const std::u16string& file)
					{
						
						return false;
					}
					void CLOAK_CALL_THIS VideoEncoder::StopPlayback()
					{
						
					}
					void CLOAK_CALL_THIS VideoEncoder::RenderVideo(In API::Rendering::IContext* context, In API::Rendering::IColorBuffer* dst)
					{
						/*
						 if (m_spMediaEngine->OnVideoStreamTick(&pts) == S_OK)
        {
            // new frame available at the media engine so get it 
            ComPtr<ID3D11Texture2D> spTextureDst;
            MEDIA::ThrowIfFailed(
                m_spDX11SwapChain->GetBuffer(0, CE_QUERY_ARGS(&spTextureDst))
                );

            MEDIA::ThrowIfFailed(
                m_spMediaEngine->TransferVideoFrame(spTextureDst.Get(), nullptr, &m_rcTarget, &m_bkgColor)
                );

            // and the present it to the screen
            MEDIA::ThrowIfFailed(
                m_spDX11SwapChain->Present(1, 0)
                );            
        }
						*/
					}
					bool CLOAK_CALL_THIS VideoEncoder::IsVideoPlaying() const
					{
						
						return false;
					}
					void CLOAK_CALL_THIS VideoEncoder::OnVideoEvent()
					{
						
					}
					void CLOAK_CALL_THIS VideoEncoder::OnWindowResize(In LPRECT rct)
					{
						
					}

					Success(return == true) bool CLOAK_CALL_THIS VideoEncoder::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY_IMPL(ptr, riid, VideoEncoder, VideoEncoder_v1, SavePtr);
					}
				}
			}
		}
	}
}