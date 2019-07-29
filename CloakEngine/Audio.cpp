#include "stdafx.h"
#include "Implementation/Global/Audio.h"

#include "CloakEngine\Global\Debug.h"
#include "CloakEngine\Global\Math.h"
#include "CloakEngine\World\ComponentManager.h"

#pragma comment(lib,"xaudio2.lib")
/*
db to volume percentage:
	return pow(10, 0.05 * db);
volume percentage to db:
	return 20 * log10(volume);
*/


namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Audio {
				constexpr uint32_t SUBMIX_SAMPLERATE = 44100;
				constexpr size_t SUBMIX_COUNT = 3;

				IXAudio2* g_engine = nullptr;
				IXAudio2MasteringVoice* g_master = nullptr;
				struct {
					union {
						IXAudio2SubmixVoice* Data[SUBMIX_COUNT];
						struct {
							IXAudio2SubmixVoice* Effects;
							IXAudio2SubmixVoice* Music;
							IXAudio2SubmixVoice* Speech;
						};
					};
					IXAudio2SubmixVoice*& CLOAK_CALL_THIS operator[](In size_t a) { return Data[a]; }
				} g_submix;
				uint8_t g_channelCount = 0;
				DWORD g_channelMask = 0;
				X3DAUDIO_HANDLE g_audio3D;

				void CLOAK_CALL Initialize()
				{
#ifdef DEPRECATED
					g_submix[0] = g_submix[1] = g_submix[2] = nullptr;
					//Initialize XAudio2:
					HRESULT hRet = XAudio2Create(&g_engine, 0, XAUDIO2_ANY_PROCESSOR);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_DEVICE, true)) { return; }
					//Create Master Voice:
					hRet = g_engine->CreateMasteringVoice(&g_master, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, nullptr, nullptr, AudioCategory_GameEffects);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_MASTER_VOICE, true)) { return; }
					hRet = g_master->GetChannelMask(&g_channelMask);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_QUERY_CHANNELS, true)) { return; }
					g_channelCount = API::Global::Math::popcount(g_channelMask);
					//Create SFX voices:
					for (size_t a = 0; a < SUBMIX_COUNT; a++)
					{
						hRet = g_engine->CreateSubmixVoice(&g_submix[a], g_channelMask, SUBMIX_SAMPLERATE, 0, 256, nullptr, nullptr);
						if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_SUBMIX_VOICE, true)) { return; }
						g_submix[a]->SetOutputVoices(nullptr);
					}
					//Initialize X3DAudio:
					hRet = X3DAudioInitialize(g_channelMask, X3DAUDIO_SPEED_OF_SOUND, g_audio3D);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_3D, true)) { return; }
#endif
				}
				void CLOAK_CALL OnUpdate()
				{

				}
				void CLOAK_CALL Release()
				{
					if (g_engine != nullptr) { g_engine->StopEngine(); }

					for (size_t a = 0; a < 3; a++) { if (g_submix[a] != nullptr) { g_submix[a]->DestroyVoice(); } }
					if (g_master != nullptr) { g_master->DestroyVoice(); }
					RELEASE(g_engine);
				}
				void CLOAK_CALL looseFocus()
				{

				}
				void CLOAK_CALL getFocus()
				{

				}


				namespace Audio_v1 {
					CLOAK_CALL Sound::Sound(In const API::Global::Audio::Audio_v1::SOUND_DESC& desc, In_opt API::Global::Audio::Audio_v1::ISoundCallback* callback) : m_callback(callback)
					{
#ifdef DEPRECATED
						CLOAK_ASSUME(static_cast<size_t>(desc.Type) < SUBMIX_COUNT);

						if (m_callback != nullptr) { m_callback->AddRef(); }
						m_data = nullptr;
						m_sync = nullptr;
#ifdef _DEBUG
						m_curState = State::WAIT;
#endif

						WAVEFORMATEXTENSIBLE fd;
						fd.Format.cbSize = sizeof(fd);
						fd.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
						fd.Format.nChannels = API::Global::Math::popcount(desc.ActiveChannels);
						fd.Format.nSamplesPerSec = static_cast<DWORD>(desc.SamplesPerSecond);
						fd.Format.wBitsPerSample = static_cast<WORD>(desc.BitsPerSample);
						fd.Format.nBlockAlign = (fd.Format.nChannels*fd.Format.wBitsPerSample) / 8;
						fd.Format.nAvgBytesPerSec = fd.Format.nBlockAlign * fd.Format.nSamplesPerSec;
						fd.dwChannelMask = static_cast<DWORD>(desc.ActiveChannels);
						fd.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

						XAUDIO2_SEND_DESCRIPTOR sd;
						sd.Flags = 0;
						sd.pOutputVoice = g_submix[static_cast<size_t>(desc.Type)];

						XAUDIO2_VOICE_SENDS sl;
						sl.SendCount = 1;
						sl.pSends = &sd;

						UINT32 flags = XAUDIO2_VOICE_NOPITCH;
						if (desc.SamplesPerSecond == SUBMIX_SAMPLERATE) { flags |= XAUDIO2_VOICE_NOSRC; } //No SampleRateConversion

						HRESULT hRet = g_engine->CreateSourceVoice(&m_data, &fd.Format, flags, XAUDIO2_DEFAULT_FREQ_RATIO, this, &sl, nullptr/*TODO: Effect Chain*/);
						if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_SOUND_CREATION, false)) { m_data = nullptr; }

						CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
#endif
					}
					CLOAK_CALL Sound::~Sound()
					{
						if (m_data != nullptr) { m_data->DestroyVoice(); }
						SAVE_RELEASE(m_sync);
						m_callback->Release();
					}
					void CLOAK_CALL_THIS Sound::Start()
					{
						API::Helper::Lock lock(m_sync);
#ifdef _DEBUG
						CLOAK_ASSUME(m_curState == State::WAIT);
#endif
						m_data->Start();
					}
					void CLOAK_CALL_THIS Sound::Pause()
					{
						API::Helper::Lock lock(m_sync);
#ifdef _DEBUG
						CLOAK_ASSUME(m_curState == State::PLAYING);
#endif
						m_data->Stop();
					}
					void CLOAK_CALL_THIS Sound::Resume()
					{
						API::Helper::Lock lock(m_sync);
#ifdef _DEBUG
						CLOAK_ASSUME(m_curState == State::PAUSED);
#endif
						m_data->Start();
					}
					void CLOAK_CALL_THIS Sound::Stop()
					{
						API::Helper::Lock lock(m_sync);
#ifdef _DEBUG
						CLOAK_ASSUME(m_curState == State::PLAYING);
#endif
						m_data->Stop();
						m_data->FlushSourceBuffers();
					}
					void CLOAK_CALL_THIS Sound::SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In_opt bool lastBuffer)
					{
						SubmitAudioBuffer(bufferSize, buffer, 0, 0, 0, nullptr, lastBuffer);
					}
					void CLOAK_CALL_THIS Sound::SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In void* data, In_opt bool lastBuffer)
					{
						SubmitAudioBuffer(bufferSize, buffer, 0, 0, 0, data, lastBuffer);
					}
					void CLOAK_CALL_THIS Sound::SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In_opt bool lastBuffer)
					{
						SubmitAudioBuffer(bufferSize, buffer, loopCount, 0, 0, nullptr, lastBuffer);
					}
					void CLOAK_CALL_THIS Sound::SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In void* data, In_opt bool lastBuffer)
					{
						SubmitAudioBuffer(bufferSize, buffer, loopCount, 0, 0, data, lastBuffer);
					}
					void CLOAK_CALL_THIS Sound::SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In size_t loopStart, In size_t loopEnd, In_opt bool lastBuffer)
					{
						SubmitAudioBuffer(bufferSize, buffer, loopCount, loopStart, loopEnd, nullptr, lastBuffer);
					}
					void CLOAK_CALL_THIS Sound::SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In size_t loopStart, In size_t loopEnd, In void* data, In_opt bool lastBuffer)
					{
						CLOAK_ASSUME(loopCount > 0 || (loopStart == 0 && loopEnd == 0));
						CLOAK_ASSUME(loopEnd > loopStart || (loopStart == 0 && loopEnd == 0));
						CLOAK_ASSUME(loopEnd <= bufferSize);
						CLOAK_ASSUME(loopCount <= XAUDIO2_MAX_LOOP_COUNT || loopCount == XAUDIO2_LOOP_INFINITE);
						CLOAK_ASSUME(bufferSize > 0);

						API::Helper::Lock lock(m_sync);
						XAUDIO2_BUFFER xb;
						xb.Flags = lastBuffer ? XAUDIO2_END_OF_STREAM : 0;
						xb.AudioBytes = static_cast<UINT32>(bufferSize);
						xb.pAudioData = reinterpret_cast<const BYTE*>(buffer);
						xb.PlayBegin = 0;
						xb.PlayLength = 0;
						xb.LoopBegin = loopCount == 0 ? XAUDIO2_NO_LOOP_REGION : static_cast<UINT32>(loopStart);
						xb.LoopLength = static_cast<UINT32>(loopEnd - loopStart);
						xb.LoopCount = static_cast<UINT32>(loopCount);
						xb.pContext = data;
						m_data->SubmitSourceBuffer(&xb);
					}
					void CLOAK_CALL_THIS Sound::SetEffectList(In size_t effectCount, In_reads(effectCount) API::Global::Audio::Audio_v1::ISoundEffect* const * list)
					{
						//TODO
					}
					void CLOAK_CALL_THIS Sound::SetVolume(In float vol)
					{
						API::Helper::Lock lock(m_sync);
						m_data->SetVolume(vol);
					}
					void CLOAK_CALL_THIS Sound::GetState(Out API::Global::Audio::Audio_v1::SOUND_STATE* state)
					{
						CLOAK_ASSUME(state != nullptr);
						API::Helper::Lock lock(m_sync);
						XAUDIO2_VOICE_STATE s;
						m_data->GetState(&s);
						state->CurrentData = s.pCurrentBufferContext;
						state->ProcessedSamples = static_cast<uint64_t>(s.SamplesPlayed);
						state->QueuedBuffers = static_cast<uint32_t>(s.BuffersQueued);
					}
					API::Global::Audio::Audio_v1::SOUND_STATE CLOAK_CALL_THIS Sound::GetState()
					{
						API::Global::Audio::Audio_v1::SOUND_STATE r;
						GetState(&r);
						return r;
					}

					COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE Sound::OnVoiceProcessingPassStart(THIS_ UINT32 BytesRequired)
					{
						if (m_callback != nullptr)
						{
							m_callback->OnProcessingStart(static_cast<uint32_t>(BytesRequired));
						}
					}
					COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE Sound::OnVoiceProcessingPassEnd(THIS)
					{
						if (m_callback != nullptr)
						{
							m_callback->OnProcessingEnd();
						}
					}
					COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE Sound::OnStreamEnd(THIS)
					{
						if (m_callback != nullptr)
						{
							m_callback->OnSoundFinished();
						}
					}
					COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE Sound::OnBufferStart(THIS_ void* pBufferContext)
					{
						if (m_callback != nullptr)
						{
							m_callback->OnBufferStart(pBufferContext);
						}
					}
					COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE Sound::OnBufferEnd(THIS_ void* pBufferContext)
					{
						if (m_callback != nullptr)
						{
							m_callback->OnBufferEnd(pBufferContext);
						}
					}
					COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE Sound::OnLoopEnd(THIS_ void* pBufferContext)
					{
						if (m_callback != nullptr)
						{
							m_callback->OnLoopEnd(pBufferContext);
						}
					}
					COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE Sound::OnVoiceError(THIS_ void* pBufferContext, HRESULT Error)
					{
						//TODO
					}

					Success(return == true) bool CLOAK_CALL_THIS Sound::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						CLOAK_QUERY(riid, ptr, SavePtr, Global::Audio::Audio_v1, Sound);
					}
				}
#ifdef DEPRECATED
#define DESTROY(p)if(p!=nullptr){p->DestroyVoice();p=nullptr;}

#define SUBMIX_SAMPLERATE 44100

				IXAudio2* g_device = nullptr;
				IXAudio2MasteringVoice* g_masterVoice = nullptr;
				IXAudio2SubmixVoice* g_submixEffects = nullptr;
				IXAudio2SubmixVoice* g_submixSpeech = nullptr;
				IXAudio2SubmixVoice* g_submixMusic = nullptr;
				std::atomic<DWORD> g_usedChannels = 0;

				void CLOAK_CALL SetVolume(In IXAudio2Voice* voice, In float volume)
				{
					if (voice != nullptr)
					{
						voice->SetVolume(min(max(volume, 0), XAUDIO2_MAX_VOLUME_LEVEL));
					}
				}

				void CLOAK_CALL Initialize()
				{
					HRESULT hRet = XAudio2Create(&g_device, 0, XAUDIO2_DEFAULT_PROCESSOR);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_DEVICE, true)) { return; }
					hRet = g_device->CreateMasteringVoice(&g_masterVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, nullptr, nullptr, AudioCategory_GameEffects);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_MASTER_VOICE, true)) { return; }
					//Get used channel num:
					UINT32 chanNum = 0;
					DWORD chanMask = 0;
					g_masterVoice->GetChannelMask(&chanMask);
					while (chanMask != 0)
					{
						chanNum++;
						chanMask = chanMask >> 1;
					}
					g_usedChannels = chanNum;
					//Create submix voices:
					hRet = g_device->CreateSubmixVoice(&g_submixEffects, g_usedChannels, SUBMIX_SAMPLERATE, 0, 256, nullptr, nullptr);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_SUBMIX_VOICE, true)) { return; }
					hRet = g_device->CreateSubmixVoice(&g_submixMusic, g_usedChannels, SUBMIX_SAMPLERATE, 0, 256, nullptr, nullptr);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_SUBMIX_VOICE, true)) { return; }
					hRet = g_device->CreateSubmixVoice(&g_submixSpeech, g_usedChannels, SUBMIX_SAMPLERATE, 0, 256, nullptr, nullptr);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_SUBMIX_VOICE, true)) { return; }
				}
				void CLOAK_CALL Release()
				{
					if (g_device != nullptr) { g_device->StopEngine(); }

					DESTROY(g_submixEffects);
					DESTROY(g_submixMusic);
					DESTROY(g_submixSpeech);
					DESTROY(g_masterVoice);
					RELEASE(g_device);
				}
				void CLOAK_CALL looseFocus()
				{
					//g_device->StopEngine();
				}
				void CLOAK_CALL getFocus()
				{
					//g_device->StartEngine();
				}
#endif
			}
		}
	}
	namespace API {
		namespace Global {
			namespace Audio {

				CLOAKENGINE_API void CLOAK_CALL SetSettings(In const Settings& set)
				{
					//Impl::Global::Audio::SetVolume(Impl::Global::Audio::g_masterVoice, set.volumeMaster);
					//Impl::Global::Audio::SetVolume(Impl::Global::Audio::g_submixEffects, set.volumeEffects);
					//Impl::Global::Audio::SetVolume(Impl::Global::Audio::g_submixMusic, set.volumeMusic);
					//Impl::Global::Audio::SetVolume(Impl::Global::Audio::g_submixSpeech, set.volumeSpeech);
				}
			}
		}
	}
}