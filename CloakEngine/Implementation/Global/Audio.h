#pragma once
#ifndef CE_IMPL_GLOBAL_AUDIO_H
#define CE_IMPL_GLOBAL_AUDIO_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Audio.h"
#include "CloakEngine\Helper\SyncSection.h"
#include "Implementation\Helper\SavePtr.h"

#include <xaudio2.h>
#include <x3daudio.h>
#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Audio {
				void CLOAK_CALL Initialize();
				void CLOAK_CALL OnUpdate();
				void CLOAK_CALL Release();
				void CLOAK_CALL looseFocus();
				void CLOAK_CALL getFocus();

				inline namespace Audio_v1 {
					class CLOAK_UUID("{06D62EDD-585B-4D8B-BF38-91A23A2D0FAB}") Sound : public virtual API::Global::Audio::Audio_v1::ISound, public virtual IXAudio2VoiceCallback, public Impl::Helper::SavePtr_v1::SavePtr {
						public:
							CLOAK_CALL Sound(In const API::Global::Audio::Audio_v1::SOUND_DESC& desc, In_opt API::Global::Audio::Audio_v1::ISoundCallback* callback = nullptr);
							CLOAK_CALL ~Sound();
							void CLOAK_CALL_THIS Start() override;
							void CLOAK_CALL_THIS Pause() override;
							void CLOAK_CALL_THIS Resume() override;
							void CLOAK_CALL_THIS Stop() override;
							void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In_opt bool lastBuffer = false) override;
							void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In void* data, In_opt bool lastBuffer = false) override;
							void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In_opt bool lastBuffer = false) override;
							void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In void* data, In_opt bool lastBuffer = false)  override;
							void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In size_t loopStart, In size_t loopEnd, In_opt bool lastBuffer = false) override;
							void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In size_t loopStart, In size_t loopEnd, In void* data, In_opt bool lastBuffer = false) override;
							void CLOAK_CALL_THIS SetEffectList(In size_t effectCount, In_reads(effectCount) API::Global::Audio::Audio_v1::ISoundEffect* const * list) override;
							void CLOAK_CALL_THIS SetVolume(In float vol) override;
							void CLOAK_CALL_THIS GetState(Out API::Global::Audio::Audio_v1::SOUND_STATE* state) override;
							API::Global::Audio::Audio_v1::SOUND_STATE CLOAK_CALL_THIS GetState() override;

							// Called just before this voice's processing pass begins.
							STDMETHOD_(void, OnVoiceProcessingPassStart) (THIS_ UINT32 BytesRequired) override;

							// Called just after this voice's processing pass ends.
							STDMETHOD_(void, OnVoiceProcessingPassEnd) (THIS) override;

							// Called when this voice has just finished playing a buffer stream
							// (as marked with the XAUDIO2_END_OF_STREAM flag on the last buffer).
							STDMETHOD_(void, OnStreamEnd) (THIS) override;

							// Called when this voice is about to start processing a new buffer.
							STDMETHOD_(void, OnBufferStart) (THIS_ void* pBufferContext) override;

							// Called when this voice has just finished processing a buffer.
							// The buffer can now be reused or destroyed.
							STDMETHOD_(void, OnBufferEnd) (THIS_ void* pBufferContext) override;

							// Called when this voice has just reached the end position of a loop.
							STDMETHOD_(void, OnLoopEnd) (THIS_ void* pBufferContext) override;

							// Called in the event of a critical error during voice processing,
							// such as a failing xAPO or an error from the hardware XMA decoder.
							// The voice may have to be destroyed and re-created to recover from
							// the error.  The callback arguments report which buffer was being
							// processed when the error occurred, and its HRESULT code.
							STDMETHOD_(void, OnVoiceError) (THIS_ void* pBufferContext, HRESULT Error) override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							IXAudio2SourceVoice* m_data;
							API::Helper::ISyncSection* m_sync;
							API::Global::Audio::Audio_v1::ISoundCallback* const m_callback;

#ifdef _DEBUG
							enum class State {
								WAIT,
								PAUSED,
								PLAYING,
							};
							State m_curState;
#endif
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif