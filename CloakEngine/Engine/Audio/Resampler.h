#pragma once
#ifndef CE_E_AUDIO_RESAMPLER_H
#define CE_E_AUDIO_RESAMPLER_H

#include "CloakEngine\Defines.h"
#include "Engine\Audio\Core.h"

namespace CloakEngine {
	namespace Engine {
		namespace Audio {
			class Resampler : public Engine::Audio::Core::ISoundEffect {
				private:
					struct TABLE_DESC {
						size_t BufferFrameSize;
						size_t ChannelCount;
						size_t FIRBufferSize;

						uint32_t InputBuffer;
						uint32_t InputFrames;
						uint32_t OutputFrames;
						double KernelPower;
						uint32_t KernelHalfSize;
					};
					class Table {
						private:
#ifdef _DEBUG
							size_t m_firSize;
#endif
							//Finite Impulse Response (FIR) coefficients for each phase offset
							float* const m_fir;
						public:
							Engine::Audio::Core::RotatingSoundBuffer SoundBuffer;
							const uint32_t ReadBuffer;
							const uint32_t InputFrames;
							const uint32_t OutputFrames;
							const uint32_t KernelSize;
							const uint32_t KernelHalf;
							const uint32_t SoundBufferSize;

							CLOAK_CALL Table(In const TABLE_DESC& desc);
							CLOAK_CALL ~Table();

							const float& CLOAK_CALL_THIS operator[](In size_t a) const;
					};

					CLOAK_FORCEINLINE static TABLE_DESC CLOAK_CALL iCreateTable(In uint32_t inputFrequency, In uint32_t outputFrequency, In size_t channelCount);

					Table m_table;
					const bool m_noOp;
					size_t m_readPos;
					size_t m_emptyFrames;
					uint32_t m_phaseOffset;
				public:
					CLOAK_CALL Resampler(In uint32_t inputFrequency, In uint32_t outputFrequency, In size_t channelCount);

					void CLOAK_CALL_THIS Process(In API::Global::Memory::PageStackAllocator* alloc, In const Engine::Audio::Core::SoundBuffer& buffer, In const Engine::Audio::Core::ChannelSetup& channels) override;
			};
		}
	}
}

#endif