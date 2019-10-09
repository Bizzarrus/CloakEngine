#include "stdafx.h"
#include "Engine\Audio\Core.h"

#include "CloakEngine\Global\Debug.h"
#include "Implementation/Global/Task.h"
#include "Implementation\Helper\SyncSection.h"
#include "Engine\Audio\Resampler.h"

#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <atomic>

//Enable sound processing
#define SOUND_ENABLED

//Play continious C#4 for debugging
//#define PLAY_DEBUG_SOUND

//Channel Matrix Flags:

//Global Volume Level Adjustent to account for speaker count
//#define CM_GLOBAL_VOLUME_ADJUSTMENT

//Bleeding reduction by rescaling linear falloff between speakers
#define CM_REDUCED_BLEEDING_FOCUS

//Bleeding reduction by using squared functions of different strength
//#define CM_FOCUS_FUNCTION_1
//#define CM_FOCUS_FUNCTION_2


#undef SPEAKER_FRONT_LEFT				
#undef SPEAKER_FRONT_RIGHT				
#undef SPEAKER_FRONT_CENTER			
#undef SPEAKER_LOW_FREQUENCY			
#undef SPEAKER_BACK_LEFT				
#undef SPEAKER_BACK_RIGHT				
#undef SPEAKER_FRONT_LEFT_OF_CENTER	
#undef SPEAKER_FRONT_RIGHT_OF_CENTER	
#undef SPEAKER_BACK_CENTER				
#undef SPEAKER_SIDE_LEFT				
#undef SPEAKER_SIDE_RIGHT				
#undef SPEAKER_TOP_CENTER				
#undef SPEAKER_TOP_FRONT_LEFT			
#undef SPEAKER_TOP_FRONT_CENTER		
#undef SPEAKER_TOP_FRONT_RIGHT			
#undef SPEAKER_TOP_BACK_LEFT			
#undef SPEAKER_TOP_BACK_CENTER			
#undef SPEAKER_TOP_BACK_RIGHT	

namespace CloakEngine {
	namespace Engine {
		namespace Audio {
			namespace Core {
				constexpr REFERENCE_TIME BUFFER_PLAY_TIME = 20*10000; //20 ms of buffer play time
				constexpr DWORD MAX_WAIT_TIME = 250; //wait not more than 250 ms in main audio thread
				constexpr DWORD FADEOUT_TIME = 500; //take 500 ms to fade volume up/down during startup/shutdown phases
				constexpr uint32_t MAX_PROCESSING_FRAMES = 500; //Max number of frames processed at once
				constexpr uint32_t AUDIO_BUFFERING_FACTOR = 3;

				constexpr API::Global::Audio::SPEAKER SPEAKER_POSITION_MASK = ~API::Global::Audio::SPEAKER_LOW_FREQUENCY;
				constexpr uint8_t LOW_FREQUENCY_INDEX = API::Global::Math::CompileTime::bitScanForward(API::Global::Audio::SPEAKER_LOW_FREQUENCY);

				const __m128 ALL_ONE = _mm_set1_ps(1);
				const __m128 ALL_NEG_ONE = _mm_set1_ps(-1);

				struct LongLat 
				{
					struct AngleData {
						const long double Value;
						const long double Cos;
						const long double Sin;

						constexpr CLOAK_CALL AngleData(long double v) : Value(v), Cos(API::Global::Math::CompileTime::cos(v)), Sin(API::Global::Math::CompileTime::sin(v)){}
					};
					const AngleData Longitude; 
					const AngleData Latitude;

					constexpr CLOAK_CALL LongLat() : Longitude(0), Latitude(0) {}
					constexpr CLOAK_CALL LongLat(In long double longitude, In long double latitude) : Longitude(longitude), Latitude(latitude) {}

					API::Global::Math::Vector Position() const { return API::Global::Math::Vector(Longitude.Sin * Latitude.Cos, Latitude.Sin, Longitude.Cos * Latitude.Cos); }
				};

				//Speaker positions based on Dolby Speaker Placement Guides
				constexpr LongLat SPEAKER_POSITION[] = {
					/*SPEAKER_FRONT_LEFT			*/LongLat(-0.45378560551852569L	, 0 ),
					/*SPEAKER_FRONT_RIGHT			*/LongLat( 0.45378560551852569L	, 0 ),
					/*SPEAKER_FRONT_CENTER			*/LongLat( 0					, 0 ),
					/*SPEAKER_LOW_FREQUENCY			*/LongLat( 0					, 0 ),
					/*SPEAKER_BACK_LEFT				*/LongLat(-2.48709418409191964L	, 0 ),
					/*SPEAKER_BACK_RIGHT			*/LongLat( 2.48709418409191964L	, 0 ),
					/*SPEAKER_FRONT_LEFT_OF_CENTER	*/LongLat(-1.04719755119659774L	, 0 ),
					/*SPEAKER_FRONT_RIGHT_OF_CENTER	*/LongLat( 1.04719755119659774L	, 0 ),
					/*SPEAKER_BACK_CENTER			*/LongLat( 3.14159265358979323L	, 0 ),
					/*SPEAKER_SIDE_LEFT				*/LongLat(-1.57079632679489661L	, 0 ),
					/*SPEAKER_SIDE_RIGHT			*/LongLat( 1.57079632679489661L	, 0 ),
					/*SPEAKER_TOP_CENTER			*/LongLat( 0					, 1.39626340159546366L ),
					/*SPEAKER_TOP_FRONT_LEFT		*/LongLat(-0.66867951494624364L	, 0.78539816339744830L ),
					/*SPEAKER_TOP_FRONT_CENTER		*/LongLat( 0					, 0.78539816339744830L ),
					/*SPEAKER_TOP_FRONT_RIGHT		*/LongLat( 0.66867951494624364L	, 0.78539816339744830L ),
					/*SPEAKER_TOP_BACK_LEFT			*/LongLat(-2.23947584174114025L	, 0.78539816339744830L ),
					/*SPEAKER_TOP_BACK_CENTER		*/LongLat( 3.14159265358979323L	, 0.78539816339744830L ),
					/*SPEAKER_TOP_BACK_RIGHT		*/LongLat( 2.23947584174114025L	, 0.78539816339744830L ),
				};

				enum class ShutdownState {
					FadingUp,
					Running,
					Shutdown,
					FadingDown,
					Finished,
					Closed,
				};

				IAudioClient* g_client = nullptr;
				IAudioRenderClient* g_playback = nullptr;

				struct {
					struct {
						API::Global::Audio::SPEAKER Mask = static_cast<API::Global::Audio::SPEAKER>(0);
						size_t Count = 0;
					} Speaker;
					struct {
						WORD BitCount;
						WORD ValidBits;
						DWORD PerSecond;
					} Samples;
					size_t FrameByteSize;
				} g_formatSettings;
				class {
					private:
						bool m_noReads = false;
						uint32_t m_slots = 1;
						uint32_t m_regSlots = 0;
						uint32_t m_usedSlots = 0;
						uint32_t m_readSlots = 0;
						Impl::Helper::WAIT_EVENT m_fullSlots = false;
						SRWLOCK m_srw = SRWLOCK_INIT;
					public:
						uint32_t CLOAK_CALL_THIS LockWrite(In uint32_t maxSlots, Out bool* hasMore)
						{
							CLOAK_ASSUME(hasMore != nullptr);
							*hasMore = false;
							AcquireSRWLockExclusive(&m_srw);
							if (m_noReads == true) { return 0; }
							CLOAK_ASSUME(m_slots >= m_regSlots);
							uint32_t slots = m_slots - m_regSlots;
							if (slots > maxSlots) { slots = maxSlots; *hasMore = true; }
							m_regSlots += slots;
							ReleaseSRWLockExclusive(&m_srw);
							return slots;
						}
						void CLOAK_CALL_THIS UnlockWrite(In uint32_t slots)
						{
							AcquireSRWLockExclusive(&m_srw);
							m_usedSlots += slots;
							m_fullSlots.Set();
							ReleaseSRWLockExclusive(&m_srw);
						}
						uint32_t CLOAK_CALL_THIS LockRead(In uint32_t maxSlots)
						{
							AcquireSRWLockExclusive(&m_srw);
							while (m_readSlots == m_usedSlots)
							{
								ReleaseSRWLockExclusive(&m_srw);
								m_fullSlots.Wait();
								AcquireSRWLockExclusive(&m_srw);
							}
							uint32_t slots = m_usedSlots - m_readSlots;
							if (slots > maxSlots) { slots = maxSlots; }
							m_readSlots += slots;
							if (m_readSlots == m_usedSlots) { m_fullSlots.Reset(); }
							ReleaseSRWLockExclusive(&m_srw);
							return slots;
						}
						void CLOAK_CALL_THIS UnlockRead(In uint32_t slots, In bool readSuccess)
						{
							AcquireSRWLockExclusive(&m_srw);
							m_readSlots -= slots;
							if (readSuccess == true)
							{
								m_usedSlots -= slots;
								m_regSlots -= slots;
							}
							ReleaseSRWLockExclusive(&m_srw);
						}
						void CLOAK_CALL_THIS Resize(In uint32_t slots)
						{
							m_slots = slots;
						}
						void CLOAK_CALL_THIS Shutdown()
						{
							AcquireSRWLockExclusive(&m_srw);
							m_noReads = true;
							ReleaseSRWLockExclusive(&m_srw);
						}
				} g_semaphore;
				
				UINT32 g_audioFrames = 0;
				HANDLE g_bufferEvent = INVALID_HANDLE_VALUE;
				HANDLE g_finishedEvent = INVALID_HANDLE_VALUE;
				uint64_t g_processedSamples = 0;
				Resampler* g_audioSource = nullptr;
				ChannelSetup* g_channelSetup = nullptr;
				std::atomic<ShutdownState> g_running = ShutdownState::FadingUp;
				std::atomic<float> g_volume = 1;
				std::atomic<bool> g_hasTask = false;
				API::Global::Memory::PageStackAllocator* g_allocs = nullptr;
				size_t g_allocCount = 0;
				uint64_t g_shutdownStart = 0;
				uint8_t* g_buffer = nullptr;
				size_t g_bufferSize = 0;
				size_t g_bufferFrames = 0;
				std::atomic<uint64_t> g_writePos = 0;
				size_t g_readPos = 0;

#ifdef PLAY_DEBUG_SOUND
				class DebugSoundSource : public ISoundProcessor {
					private:
						static constexpr double TONE_FREQUENCY = 261.626; //C#4
						uint64_t m_proccessedFrames;
					public:
						void CLOAK_CALL_THIS Process(In API::Global::Memory::PageStackAllocator* alloc, In const SoundBuffer& buffer, In const ChannelSetup& channels) override
						{
							for (size_t a = 0; a < buffer.GetFrameCount(); a++)
							{
								const double p = static_cast<double>(m_proccessedFrames + a) / SAMPLE_RATE;
								const float v = static_cast<float>(0.5 * std::sin(p * API::Global::Math::Tau * TONE_FREQUENCY));
								for (size_t b = 0; b < buffer.GetFrameSize(); b++) { buffer[a][b] = _mm_set_ps1(v); }
							}
							m_proccessedFrames += buffer.GetFrameCount();
						}
						void CLOAK_CALL_THIS OnFocusLost() override {}
						void CLOAK_CALL_THIS OnFocusGained() override {}
				};
#endif

				CLOAK_FORCEINLINE size_t CLOAK_CALL transformType(In uint8_t* dst, In size_t pos, In float value)
				{
					switch (g_formatSettings.Samples.BitCount)
					{
						case 8:
						{
							dst[pos] = static_cast<uint8_t>((value * 0x7F) + 0x80);
							return pos + 1;
						}
						case 16:
						{
							int16_t* d = reinterpret_cast<int16_t*>(&dst[pos]);
							*d = static_cast<int16_t>(0x7FFF * value);
							return pos + 2;
						}
						case 24:
						{
							int32_t* d = reinterpret_cast<int32_t*>(&dst[pos]);
							const int32_t mv = static_cast<int32_t>((1 << (g_formatSettings.Samples.ValidBits - 1)) - 1);
							const int32_t iv = static_cast<int32_t>(mv * value);
							if constexpr (API::Global::Endian::Native == API::Global::Endian::Big)
							{
								*d &= 0x000000FF;
								*d |= iv << (8 + g_formatSettings.Samples.BitCount - g_formatSettings.Samples.ValidBits);
							}
							else if constexpr (API::Global::Endian::Native == API::Global::Endian::Little)
							{
								*d &= 0xFF000000;
								*d |= iv << (g_formatSettings.Samples.BitCount - g_formatSettings.Samples.ValidBits);
							}
							else if constexpr (API::Global::Endian::Native == API::Global::Endian::PDP)
							{
								*d &= 0xFF000000;
								*d |= iv << (g_formatSettings.Samples.BitCount - g_formatSettings.Samples.ValidBits);
							}
							else 
							{ 
								CLOAK_ASSUME(false);
								*d = 0;
							}
							return pos + 3;
						}
						case 32:
						{
							int32_t* d = reinterpret_cast<int32_t*>(&dst[pos]);
							const int32_t mv = static_cast<int32_t>((1 << (g_formatSettings.Samples.ValidBits - 1)) - 1);
							const int32_t iv = static_cast<int32_t>(mv * value);
							*d = iv << (g_formatSettings.Samples.BitCount - g_formatSettings.Samples.ValidBits);
							return pos + 4;
						}
						default: CLOAK_ASSUME(false);
					}
					return pos;
				}
				inline void CLOAK_CALL processBuffer(API::Global::Memory::PageStackAllocator* alloc, In uint32_t frameCount, In size_t startFrame)
				{
					//Volume used to reduce volume during startup and shutdown to prevent sudden buffer ups/downs
					float fadeVolume = 1;
					ShutdownState exp = ShutdownState::Shutdown;
					if (g_running.compare_exchange_strong(exp, ShutdownState::FadingDown)) { g_shutdownStart = g_processedSamples; }
					else
					{
						switch (exp)
						{
							case CloakEngine::Engine::Audio::Core::ShutdownState::FadingUp:
							{
								const uint32_t dt = static_cast<uint32_t>((g_processedSamples * 1000) / g_formatSettings.Samples.PerSecond);
								if (dt >= FADEOUT_TIME) { fadeVolume = 1; g_running = ShutdownState::Running; }
								else { fadeVolume = static_cast<float>(dt) / FADEOUT_TIME; }
								break;
							}
							case CloakEngine::Engine::Audio::Core::ShutdownState::FadingDown:
							{
								const uint32_t dt = static_cast<uint32_t>(((g_processedSamples - g_shutdownStart) * 1000) / g_formatSettings.Samples.PerSecond);
								if (dt >= FADEOUT_TIME) { fadeVolume = 0; g_running = ShutdownState::Finished; }
								else { fadeVolume = 1 - (static_cast<float>(dt) / FADEOUT_TIME); }
								break;
							}
							default:
								break;
						}
					}

					//Calculate audio stream:
					DynamicSoundBuffer sb(alloc, *g_channelSetup, frameCount);
					if (g_audioSource == nullptr) { sb.Reset(); }
					else { g_audioSource->Process(alloc, sb, *g_channelSetup); }

					__m128 globalVolume = _mm_set1_ps(fadeVolume * g_volume);
					for (size_t a = 0, pos = startFrame * g_formatSettings.FrameByteSize; a < frameCount; a++)
					{
						__m128* frame = sb[a];
						for (size_t b = 0; b < sb.GetFrameSize(); b++)
						{
							__m128 value = frame[b];
							//Apply volume:
							value = _mm_mul_ps(value, globalVolume);
							//Clamp:
							value = _mm_min_ps(ALL_ONE, _mm_max_ps(ALL_NEG_ONE, value));
							//Set to buffer:
							CLOAK_ALIGN float frameValue[4];
							_mm_store_ps(frameValue, value);
							for (size_t c = 0; c < g_channelSetup->GetChannelCount(b); c++) 
							{ 
								pos = transformType(g_buffer, pos, frameValue[c]) % g_bufferSize; 
							}
						}
						CLOAK_ASSUME(a + 1 < frameCount || pos == ((startFrame + frameCount) % g_bufferFrames)*g_formatSettings.FrameByteSize);
					}
					g_processedSamples += frameCount;
				}
				inline void CLOAK_CALL fillBuffer(In UINT32 frameCount, In size_t startFrame, Out uint8_t* buffer)
				{
					const size_t byteCount = frameCount * g_formatSettings.FrameByteSize;
					const size_t byteStart = startFrame * g_formatSettings.FrameByteSize;
					for (size_t a = 0, b = byteStart; a < byteCount; a++, b = (b + 1) % g_bufferSize) 
					{ 
						buffer[a] = g_buffer[b]; 
						CLOAK_ASSUME(a + 1 < byteCount || (b + 1) % g_bufferSize == ((startFrame + frameCount) % g_bufferFrames)*g_formatSettings.FrameByteSize);
					}
				}
				inline void CLOAK_CALL refillBuffer(In size_t id)
				{
					CLOAK_ASSUME(id < g_allocCount);
					if (g_running != ShutdownState::Closed)
					{
						bool hasMore = false;
						const uint32_t frames = g_semaphore.LockWrite(MAX_PROCESSING_FRAMES, &hasMore);
						if (frames > 0)
						{
							const uint64_t wp = g_writePos.fetch_add(frames) % g_bufferFrames;
							processBuffer(&g_allocs[id], frames, static_cast<size_t>(wp));
							g_allocs[id].Clear();
							g_semaphore.UnlockWrite(frames);
							if (hasMore == true) 
							{ 
								API::Global::Task t = API::Global::PushTask([](In size_t id) { refillBuffer(id); });
								t.Schedule(CE::Global::Threading::Priority::High);
								return;
							}
						}
						g_hasTask = false;
						if (g_running == ShutdownState::Closed) { SetEvent(g_finishedEvent); }
					}
					else { SetEvent(g_finishedEvent); }
				}


				DWORD WINAPI runThreadFuncMain(LPVOID p)
				{
					while (g_running != ShutdownState::Finished)
					{
						if (WaitForSingleObject(g_bufferEvent, MAX_WAIT_TIME) == WAIT_OBJECT_0)
						{
							UINT32 padding = 0;
							HRESULT hRet = g_client->GetCurrentPadding(&padding);
							if (SUCCEEDED(hRet))
							{
								CLOAK_ASSUME(padding <= g_audioFrames);
								const UINT frames = g_audioFrames - padding;

								const UINT nframes = static_cast<UINT>(g_semaphore.LockRead(static_cast<uint32_t>(frames)));
#ifdef _DEBUG
								if (frames != nframes) { CloakDebugLog("Audio Starving! Required " + std::to_string(frames) + " samples, but only got " + std::to_string(nframes)); }
#endif
								BYTE* buffer = nullptr;
								HRESULT hRet = g_playback->GetBuffer(static_cast<UINT32>(nframes), &buffer);
								if (SUCCEEDED(hRet))
								{
									fillBuffer(static_cast<UINT32>(nframes), g_readPos, reinterpret_cast<uint8_t*>(buffer));
									g_playback->ReleaseBuffer(static_cast<UINT32>(nframes), 0);
									g_readPos = (g_readPos + nframes) % g_bufferFrames;
								}
								g_semaphore.UnlockRead(static_cast<uint32_t>(nframes), SUCCEEDED(hRet));
								if (g_hasTask.exchange(true) == false) 
								{
									API::Global::Task t = API::Global::PushTask([](In size_t id) { refillBuffer(id); });
									t.Schedule(CE::Global::Threading::Priority::High);
								}
							}
						}
					}
					g_running = ShutdownState::Closed;
					g_semaphore.Shutdown();
					if (g_hasTask.exchange(true) == false) { API::Global::PushTask([](In size_t id) { refillBuffer(id); }); }
					return 0;
				}

				void CLOAK_CALL Start()
				{
#ifdef SOUND_ENABLED
					//Create required resources:
					g_bufferEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
					g_finishedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

					//Create audio client:
					WAVEFORMATEX* mix = nullptr;
					WAVEFORMATEX* cform = nullptr;
					IMMDeviceEnumerator* enumerator = nullptr;
					IMMDevice* device = nullptr;
					HRESULT hRet = CoCreateInstance(__uuidof(MMDeviceEnumerator), 0, CLSCTX_ALL, CE_QUERY_ARGS(&enumerator));
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_DEVICE, true)) { goto initialize_cleanup; }
					hRet = enumerator->GetDefaultAudioEndpoint(EDataFlow::eRender, ERole::eConsole, &device);
					//No error if no device could be found
					if (hRet == E_NOTFOUND || CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_DEVICE, true)) { goto initialize_cleanup; }
					hRet = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&g_client));
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_CLIENT, true)) { goto initialize_cleanup; }

					//Query current audio setup:
					hRet = g_client->GetMixFormat(&mix);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_FORMAT_SUPPORT, true)) { goto initialize_cleanup; }
					if (mix->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
					{
						WAVEFORMATEXTENSIBLE me;
						ZeroMemory(&me, sizeof(me));
						memcpy(&me.Format, mix, min(sizeof(me), mix->cbSize));
						g_formatSettings.Speaker.Mask = static_cast<API::Global::Audio::SPEAKER>(me.dwChannelMask);
						g_formatSettings.Speaker.Count = me.Format.nChannels;
						g_formatSettings.Samples.PerSecond = me.Format.nSamplesPerSec;
						g_formatSettings.Samples.BitCount = me.Format.wBitsPerSample;
						g_formatSettings.Samples.ValidBits = me.Samples.wValidBitsPerSample;
					}
					else
					{
						if (mix->nChannels <= 1)
						{
							g_formatSettings.Speaker.Mask = API::Global::Audio::SPEAKER_MONO;
							g_formatSettings.Speaker.Count = 1;
						}
						else
						{
							g_formatSettings.Speaker.Mask = API::Global::Audio::SPEAKER_STEREO;
							g_formatSettings.Speaker.Count = 2;
						}
						g_formatSettings.Samples.PerSecond = mix->nSamplesPerSec;
						g_formatSettings.Samples.BitCount = mix->wBitsPerSample;
						g_formatSettings.Samples.ValidBits = mix->wBitsPerSample;
					}
					g_formatSettings.FrameByteSize = g_formatSettings.Speaker.Count * (g_formatSettings.Samples.BitCount >> 3);

					//Create audio format structure:
					WAVEFORMATEXTENSIBLE fd;
					fd.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);
					fd.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
					fd.Format.nChannels = static_cast<WORD>(g_formatSettings.Speaker.Count);
					fd.Format.nSamplesPerSec = static_cast<DWORD>(g_formatSettings.Samples.PerSecond);
					fd.Format.wBitsPerSample = static_cast<WORD>(g_formatSettings.Samples.BitCount);
					fd.Format.nBlockAlign = static_cast<WORD>((fd.Format.wBitsPerSample * g_formatSettings.Speaker.Count) >> 3);
					fd.Format.nAvgBytesPerSec = static_cast<DWORD>(fd.Format.nBlockAlign * g_formatSettings.Samples.PerSecond);
					fd.dwChannelMask = static_cast<DWORD>(g_formatSettings.Speaker.Mask);
					fd.Samples.wValidBitsPerSample = g_formatSettings.Samples.ValidBits;
					fd.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

					//Initialize client:
					hRet = g_client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, BUFFER_PLAY_TIME, 0, &fd.Format, nullptr);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_CLIENT, true)) { goto initialize_cleanup; }
					hRet = g_client->GetBufferSize(&g_audioFrames);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_BUFFER, true)) { goto initialize_cleanup; }
					hRet = g_client->GetService(CE_QUERY_ARGS(&g_playback));
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_NO_PLAYBACK, true)) { goto initialize_cleanup; }
					hRet = g_client->SetEventHandle(g_bufferEvent);
					if (CloakCheckError(hRet, API::Global::Debug::Error::AUDIO_EVENT_ERROR, true)) { goto initialize_cleanup; }

					//Initialize mixing sub system:
					g_channelSetup = new ChannelSetup(g_formatSettings.Speaker.Mask, g_formatSettings.Speaker.Count);
					g_audioSource = new Resampler(SAMPLE_RATE, g_formatSettings.Samples.PerSecond, g_formatSettings.Speaker.Count);
#ifdef PLAY_DEBUG_SOUND
					DebugSoundSource* dss = new DebugSoundSource();
					g_audioSource->SetParent(dss);
					dss->Release();
#endif

					API::Global::Log::WriteToLog("Audio Setup: " + std::to_string(g_formatSettings.Speaker.Mask) + " with " + std::to_string(g_formatSettings.Samples.PerSecond) + " Hz at " + std::to_string(g_formatSettings.Samples.ValidBits) + (g_formatSettings.Samples.ValidBits != g_formatSettings.Samples.BitCount ? "(" + std::to_string(g_formatSettings.Samples.BitCount) + ") " : "") + "bps");

					//Initial buffer setup:
					BYTE* buffer = nullptr;
					hRet = g_playback->GetBuffer(g_audioFrames, &buffer);
					if (SUCCEEDED(hRet)) { g_playback->ReleaseBuffer(g_audioFrames, AUDCLNT_BUFFERFLAGS_SILENT); }
					g_bufferFrames = g_audioFrames * AUDIO_BUFFERING_FACTOR;
					g_bufferSize = g_bufferFrames * g_formatSettings.FrameByteSize;
					g_buffer = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(uint8_t)*g_bufferSize));
					g_semaphore.Resize(static_cast<uint32_t>(g_bufferFrames));

					//Initialize Task handling:
					g_allocCount = API::Global::GetExecutionThreadCount();
					g_allocs = reinterpret_cast<API::Global::Memory::PageStackAllocator*>(API::Global::Memory::MemoryHeap::Allocate(g_allocCount * sizeof(API::Global::Memory::PageStackAllocator)));
					for (size_t a = 0; a < g_allocCount; a++) { new(&g_allocs[a])API::Global::Memory::PageStackAllocator(); }

					//Start thread:
					DWORD threadID = 0;
					HANDLE thread = CreateThread(nullptr, 0, runThreadFuncMain, nullptr, 0, &threadID);
					SetThreadPriority(thread, 11);

					//Start task:
					if (g_hasTask.exchange(true) == false)
					{
						API::Global::Task t = API::Global::PushTask([](In size_t id) { refillBuffer(id); });
						t.Schedule(CE::Global::Threading::Priority::High);
					}

					//Start playback:
					hRet = g_client->Start();

				initialize_cleanup:
					if (cform != nullptr) { CoTaskMemFree(cform); }
					if (mix != nullptr) { CoTaskMemFree(mix); }
					RELEASE(device);
					RELEASE(enumerator);

#endif
				}
				void CLOAK_CALL Shutdown()
				{
#ifdef SOUND_ENABLED
					ShutdownState exp = ShutdownState::Running;
					g_running.compare_exchange_strong(exp, ShutdownState::Shutdown);
#endif
				}
				void CLOAK_CALL Release()
				{
#ifdef SOUND_ENABLED
					WaitForSingleObject(g_finishedEvent, INFINITE);
#endif
				}
				void CLOAK_CALL FinalRelease()
				{
#ifdef SOUND_ENABLED
					delete g_channelSetup;
					RELEASE(g_audioSource);
					RELEASE(g_playback);
					RELEASE(g_client);
					CloseHandle(g_bufferEvent);
					CloseHandle(g_finishedEvent);
					for (size_t a = 0; a < g_allocCount; a++) { g_allocs[a].~PageStackAllocator(); }
					API::Global::Memory::MemoryHeap::Free(g_buffer);
					API::Global::Memory::MemoryHeap::Free(g_allocs);
#endif
				}
				void CLOAK_CALL OnFocusLost()
				{
					if (g_audioSource != nullptr) { g_audioSource->OnFocusLost(); }
				}
				void CLOAK_CALL OnFocusGained()
				{
					if (g_audioSource != nullptr) { g_audioSource->OnFocusGained(); }
				}

				CLOAK_CALL ChannelMatrix::ChannelMatrix()
				{
					m_volume = nullptr;
					m_tmp = nullptr;
					m_inputMask = static_cast<API::Global::Audio::SPEAKER>(0);
					m_inputSize = 0;
					m_outputSize = 0;
				}
				CLOAK_CALL ChannelMatrix::ChannelMatrix(In const ChannelMatrix& x)
				{
					m_inputSize = x.m_inputSize;
					m_outputSize = x.m_outputSize;
					m_inputMask = x.m_inputMask;
					if (m_outputSize > 0)
					{
						if (m_inputSize > 0)
						{
							const size_t ts = m_inputSize << 2;
							const size_t ds = ts * m_outputSize;
							m_volume = reinterpret_cast<__m128*>(API::Global::Memory::MemoryHeap::Allocate((ds + ts) * sizeof(__m128)));
							for (size_t a = 0; a < ds; a++) { m_volume[a] = x.m_volume[a]; }
							m_tmp = &m_volume[ds];
						}
						else
						{
							m_tmp = nullptr;
							m_volume = nullptr;
						}
					}
					else
					{
						m_tmp = nullptr;
						m_volume = nullptr;
					}
				}
				CLOAK_CALL ChannelMatrix::ChannelMatrix(In API::Global::Audio::SPEAKER input, API::Global::Audio::SPEAKER output)
				{
					const size_t is = API::Global::Math::popcount(static_cast<uint64_t>(input));
					const size_t os = API::Global::Math::popcount(static_cast<uint64_t>(output));
					m_inputSize = (is + 3) >> 2;
					m_outputSize = (os + 3) >> 2;
					m_inputMask = input;
					m_outputMask = output;
					if (m_outputSize > 0)
					{
						if (m_inputSize > 0)
						{
							const size_t ts = m_inputSize << 2;
							const size_t ds = ts * m_outputSize;
							m_volume = reinterpret_cast<__m128*>(API::Global::Memory::MemoryHeap::Allocate((ds + ts) * sizeof(__m128)));
							for (size_t a = 0; a < ds; a++) { m_volume[a] = _mm_set1_ps(0); }
							m_tmp = &m_volume[ds];

#ifdef CM_GLOBAL_VOLUME_ADJUSTMENT
							float giv = 0;
							float gov = 0;
							if (is > 1 || os > 1)
							{
								for (size_t a = 0; a < ARRAYSIZE(SPEAKER_POSITION); a++)
								{
									if (a != LOW_FREQUENCY_INDEX)
									{
										if ((static_cast<API::Global::Audio::SPEAKER>(1 << a) & m_inputMask) != static_cast<API::Global::Audio::SPEAKER>(0))
										{
											const API::Global::Math::Point p = static_cast<API::Global::Math::Point>(SPEAKER_POSITION[a].Position());
											giv += (p.Z + 1) / 2;
										}
										if ((static_cast<API::Global::Audio::SPEAKER>(1 << a) & m_outputMask) != static_cast<API::Global::Audio::SPEAKER>(0))
										{
											const API::Global::Math::Point p = static_cast<API::Global::Math::Point>(SPEAKER_POSITION[a].Position());
											gov += (p.Z + 1) / 2;
										}
									}
								}
							}
							if (is == 1) { giv = 1; }
							if (os == 1) { gov = 1; }
							const float gv = giv / gov;
#else
							const float gv = 1;
#endif
#ifdef CM_REDUCED_BLEEDING_FOCUS
							float focus = 1;
							if (os == 1) { focus = 0; }
							else
							{
								for (size_t in = 0; in < ARRAYSIZE(SPEAKER_POSITION); in++)
								{
									if ((static_cast<API::Global::Audio::SPEAKER>(1 << in) & m_outputMask) != static_cast<API::Global::Audio::SPEAKER>(0) && in != LOW_FREQUENCY_INDEX)
									{
										float mf = -1;
										const API::Global::Math::Vector inV = SPEAKER_POSITION[in].Position();
										for (size_t out = 0; out < ARRAYSIZE(SPEAKER_POSITION); out++)
										{
											if ((static_cast<API::Global::Audio::SPEAKER>(1 << out) & m_outputMask) != static_cast<API::Global::Audio::SPEAKER>(0) && out != LOW_FREQUENCY_INDEX && out != in)
											{
												const float f = inV.Dot(SPEAKER_POSITION[out].Position());
												mf = max(mf, f);
											}
										}
										focus = min(focus, mf);
									}
								}
							}
#else
							const float focus = -1;
#endif

							for (size_t in = 0, inP = 0; in < ARRAYSIZE(SPEAKER_POSITION); in++)
							{
								if ((static_cast<API::Global::Audio::SPEAKER>(1 << in) & m_inputMask) != static_cast<API::Global::Audio::SPEAKER>(0))
								{
									const API::Global::Math::Vector inV = SPEAKER_POSITION[in].Position();
									for (size_t out = 0, outP = 0; out < ARRAYSIZE(SPEAKER_POSITION); out++)
									{
										if ((static_cast<API::Global::Audio::SPEAKER>(1 << out) & m_outputMask) != static_cast<API::Global::Audio::SPEAKER>(0))
										{
											float volume = (inV.Dot(SPEAKER_POSITION[out].Position()) - focus) / (1 - focus);
											CLOAK_ASSUME(volume <= 1);
											if (volume < 0) { volume = 0; }
											if (os == 1) { volume = 1; }
											if (in == LOW_FREQUENCY_INDEX || out == LOW_FREQUENCY_INDEX) { volume = in == out ? 1.0f : 0.0f; }

#ifdef CM_FOCUS_FUNCTION_1
											volume = ((volume * volume) + volume) / 2;
#endif
#ifdef CM_FOCUS_FUNCTION_2
											volume = volume * volume;
#endif

											volume *= gv;
											const size_t p = (inP * m_outputSize) + (outP >> 2);
											__m128 d;
											switch (outP % 4)
											{
												case 0: d = _mm_set_ps(0, 0, 0, volume); break;
												case 1: d = _mm_set_ps(0, 0, volume, 0); break;
												case 2: d = _mm_set_ps(0, volume, 0, 0); break;
												case 3: d = _mm_set_ps(volume, 0, 0, 0); break;
												default: CLOAK_ASSUME(false); break;
											}
											m_volume[p] = _mm_add_ps(m_volume[p], d);
											outP++;
										}
									}
									inP++;
								}
							}
						}
						else
						{
							m_volume = nullptr;
							m_tmp = nullptr;
						}
					}
					else
					{
						m_volume = nullptr;
						m_tmp = nullptr;
					}
				}
				CLOAK_CALL ChannelMatrix::~ChannelMatrix()
				{
					API::Global::Memory::MemoryHeap::Free(m_volume);
				}

				ChannelMatrix& CLOAK_CALL_THIS ChannelMatrix::operator=(In const ChannelMatrix& x)
				{
					if (this != &x)
					{
						API::Global::Memory::MemoryHeap::Free(m_volume);
						m_inputSize = x.m_inputSize;
						m_outputSize = x.m_outputSize;
						m_inputMask = x.m_inputMask;
						if (m_outputSize > 0)
						{
							if (m_inputSize > 0)
							{
								const size_t ts = m_inputSize << 2;
								const size_t ds = ts * m_outputSize;
								m_volume = reinterpret_cast<__m128*>(API::Global::Memory::MemoryHeap::Allocate((ds + ts) * sizeof(__m128)));
								for (size_t a = 0; a < ds; a++) { m_volume[a] = x.m_volume[a]; }
								m_tmp = &m_volume[ds];
							}
							else
							{
								m_volume = nullptr;
								m_tmp = nullptr;
							}
						}
						else
						{
							m_volume = nullptr;
							m_tmp = nullptr;
						}
					}
					return *this;
				}
				size_t CLOAK_CALL_THIS ChannelMatrix::InputSize() const { return m_inputSize; }
				size_t CLOAK_CALL_THIS ChannelMatrix::OutputSize() const { return m_outputSize; }
				void CLOAK_CALL_THIS ChannelMatrix::Transform(In_reads(m_inputSize) const __m128* input, Out_writes(m_outputSize) __m128* output) const
				{
					if (m_inputSize > 0 && m_outputSize > 0 && input != nullptr && output != nullptr)
					{
						const size_t ris = m_inputSize << 2;
						for (size_t a = 0; a < m_inputSize; a++)
						{
							const size_t b = a << 2;
							m_tmp[b + 0] = _mm_shuffle_ps(input[a], input[a], _MM_SHUFFLE(0, 0, 0, 0));
							m_tmp[b + 1] = _mm_shuffle_ps(input[a], input[a], _MM_SHUFFLE(1, 1, 1, 1));
							m_tmp[b + 2] = _mm_shuffle_ps(input[a], input[a], _MM_SHUFFLE(2, 2, 2, 2));
							m_tmp[b + 3] = _mm_shuffle_ps(input[a], input[a], _MM_SHUFFLE(3, 3, 3, 3));
						}
						for (size_t a = 0; a < m_outputSize; a++)
						{
							output[a] = _mm_set1_ps(0);
							for (size_t b = 0; b < ris; b++)
							{
								const size_t p = (b*m_outputSize) + a;
								output[a] = _mm_add_ps(output[a], _mm_mul_ps(m_tmp[b], m_volume[p]));
							}
						}
					}
				}

				/*
				3D Audio Transformation:

				Pos = Input Position
				M = Camera-LookMatrix
				P = Pos * M
				G = 0
				For each output channel:
					G += ([Output Channel Position].Z + 1) / 2
				if(Output Channel Count == 1):
					G = 1
				For each output channel:
					V = (P.Dot(Output Channel Position) + 1) / (2 * G)
					Output[Channel] = Input * V
				*/
			}
		}
	}
}