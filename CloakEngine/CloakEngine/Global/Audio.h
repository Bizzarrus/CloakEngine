#pragma once
#ifndef CE_API_GLOBAL_AUDIO_H
#define CE_API_GLOBAL_AUDIO_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Audio {
				inline namespace Audio_v1 {
					constexpr size_t LOOP_INFINITE = 255;
					constexpr size_t LOOP_MAX_COUNT = 254;

					enum class SOUND_TYPE {
						EFFECTS = 0,
						MUSIC = 1,
						SPEECH = 2,
					};
					enum SPEAKER {
						SPEAKER_FRONT_LEFT				= 0x00001,
						SPEAKER_FRONT_RIGHT				= 0x00002,
						SPEAKER_FRONT_CENTER			= 0x00004,
						SPEAKER_LOW_FREQUENCY			= 0x00008,
						SPEAKER_BACK_LEFT				= 0x00010,
						SPEAKER_BACK_RIGHT				= 0x00020,
						SPEAKER_FRONT_LEFT_OF_CENTER	= 0x00040,
						SPEAKER_FRONT_RIGHT_OF_CENTER	= 0x00080,
						SPEAKER_BACK_CENTER				= 0x00100,
						SPEAKER_SIDE_LEFT				= 0x00200,
						SPEAKER_SIDE_RIGHT				= 0x00400,
						SPEAKER_TOP_CENTER				= 0x00800,
						SPEAKER_TOP_FRONT_LEFT			= 0x01000,
						SPEAKER_TOP_FRONT_CENTER		= 0x02000,
						SPEAKER_TOP_FRONT_RIGHT			= 0x04000,
						SPEAKER_TOP_BACK_LEFT			= 0x08000,
						SPEAKER_TOP_BACK_CENTER			= 0x10000,
						SPEAKER_TOP_BACK_RIGHT			= 0x20000,

						//Predefined defaults:
						SPEAKER_MONO					= SPEAKER_FRONT_CENTER,
						SPEAKER_STEREO					= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
						SPEAKER_QUAD					= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
						SPEAKER_SURROUND				= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER,
						//Configurations based on Dolby:
						SPEAKER_1_0						= SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY,
						SPEAKER_2_1						= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY,
						SPEAKER_3_0						= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER,
						SPEAKER_3_1						= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY,
						SPEAKER_5_0						= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT,
						SPEAKER_5_1						= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT,
						SPEAKER_5_0_4					= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT | SPEAKER_TOP_BACK_LEFT | SPEAKER_TOP_BACK_RIGHT,
						SPEAKER_5_1_4					= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY| SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT | SPEAKER_TOP_BACK_LEFT | SPEAKER_TOP_BACK_RIGHT,
						SPEAKER_7_0						= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT,
						SPEAKER_7_1						= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT,
						SPEAKER_7_0_4					= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT | SPEAKER_TOP_BACK_LEFT | SPEAKER_TOP_BACK_RIGHT,
						SPEAKER_7_1_4					= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT | SPEAKER_TOP_BACK_LEFT | SPEAKER_TOP_BACK_RIGHT,
						SPEAKER_9_0						= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER,
						SPEAKER_9_1						= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER,
						SPEAKER_9_0_4					= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT | SPEAKER_TOP_BACK_LEFT | SPEAKER_TOP_BACK_RIGHT,
						SPEAKER_9_1_4					= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT | SPEAKER_TOP_BACK_LEFT | SPEAKER_TOP_BACK_RIGHT,
					};
					DEFINE_ENUM_FLAG_OPERATORS(SPEAKER);
					struct SOUND_DESC {
						SOUND_TYPE Type;
						SPEAKER ActiveChannels;		//Mask of all active Channels. Samples should be present in the same order as the set bits (from least to most significant)
						uint32_t SamplesPerSecond;	//Samplerate in Samples per Second per Channel
						uint8_t BitsPerSample;		//Size of the container that holds a sample, must be byte-aligned (8, 16, 24, or 32)
						uint8_t ValidBitsPerSample;	//Size of the actual sample (may be 8, 16, 20, 24, or 32). Samples are written in the leftmost (most significant) bits - the remaining (least significant) bits should be set to zero
					};
					struct SOUND_STATE {
						uint32_t QueuedBuffers;
						uint64_t ProcessedSamples;
						void* CurrentData;
					};
					CLOAK_INTERFACE ISoundEffect : public virtual API::Helper::SavePtr_v1::IBasicPtr {
						public:

					};
					CLOAK_INTERFACE ISoundCallback : public virtual API::Helper::SavePtr_v1::IBasicPtr{
						public:
							virtual void CLOAK_CALL_THIS OnBufferStart(In void* data) = 0;
							virtual void CLOAK_CALL_THIS OnBufferEnd(In void* data) = 0;
							virtual void CLOAK_CALL_THIS OnLoopEnd(In void* data) = 0;
							virtual void CLOAK_CALL_THIS OnSoundFinished() = 0;
							//Called right before processing a buffer. requiredBytes is the number of bytes that must be submitted right now to keep playing (allows just-in-time streaming)
							virtual void CLOAK_CALL_THIS OnProcessingStart(In uint32_t requiredBytes) = 0;
							virtual void CLOAK_CALL_THIS OnProcessingEnd() = 0;
					};
					CLOAK_INTERFACE_ID("{ADF4BF76-A3EA-4B43-9F73-0FE2FF68AA11}") ISound : public virtual API::Helper::SavePtr_v1::ISavePtr{
						public:
							virtual void CLOAK_CALL_THIS Start() = 0;
							virtual void CLOAK_CALL_THIS Pause() = 0;
							virtual void CLOAK_CALL_THIS Resume() = 0;
							virtual void CLOAK_CALL_THIS Stop() = 0;
							//Submits an (interleaved) audio buffer to the playback queue. The buffer must stay valid until fully played
							virtual void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In_opt bool lastBuffer = false) = 0;
							virtual void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In void* data, In_opt bool lastBuffer = false) = 0;
							virtual void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In_opt bool lastBuffer = false) = 0;
							virtual void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In void* data, In_opt bool lastBuffer = false) = 0;
							virtual void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In size_t loopStart, In size_t loopEnd, In_opt bool lastBuffer = false) = 0;
							virtual void CLOAK_CALL_THIS SubmitAudioBuffer(In size_t bufferSize, In_reads(bufferSize) const uint8_t* buffer, In size_t loopCount, In size_t loopStart, In size_t loopEnd, In void* data, In_opt bool lastBuffer = false) = 0;
							virtual void CLOAK_CALL_THIS SetEffectList(In size_t effectCount, In_reads(effectCount) ISoundEffect* const * list) = 0;
							virtual void CLOAK_CALL_THIS SetVolume(In float vol) = 0;
							virtual void CLOAK_CALL_THIS GetState(Out SOUND_STATE* state) = 0;
							virtual SOUND_STATE CLOAK_CALL_THIS GetState() = 0;
					};
				}

				enum class Typ { Music, Effect, Speech };
				struct Settings {
					float volumeSpeech = 1;
					float volumeMusic = 1;
					float volumeEffects = 1;
					float volumeMaster = 1;
				};

				

				CLOAKENGINE_API void CLOAK_CALL SetSettings(In const Settings& set);
			}
		}
	}
}

#endif