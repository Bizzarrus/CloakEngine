#pragma once
#ifndef CE_E_AUDIO_CORE_H
#define CE_E_AUDIO_CORE_H

#include "CloakEngine\Defines.h"
#include "CloakEngine\Global\Audio.h"
#include "CloakEngine\Global\Memory.h"
#include "CloakEngine\Global\Math.h"
#include "CloakEngine\Helper\SavePtr.h"

#include <intrin.h>
#include <atomic>

namespace CloakEngine {
	namespace Engine {
		namespace Audio {
			namespace Core {
				//Default samplerate (samples per second per channel)
				constexpr uint32_t SAMPLE_RATE = 44100;

				//Just a handsome buffer to manage speaker setup
				class ChannelSetup {
					private:
						API::Global::Audio::SPEAKER* m_subMasks;
						const API::Global::Audio::SPEAKER m_mask;
						const size_t m_speakerCount;
						const size_t m_frameSize;

						CLOAK_CALL ChannelSetup(In const ChannelSetup&) = delete;
					public:
						CLOAK_CALL ChannelSetup(In API::Global::Audio::SPEAKER speaker, In size_t speakerCount) : m_mask(speaker), m_speakerCount(speakerCount), m_frameSize((speakerCount + 3) >> 2)
						{
							m_subMasks = reinterpret_cast<API::Global::Audio::SPEAKER*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(API::Global::Audio::SPEAKER) * m_frameSize));
							API::Global::Audio::SPEAKER m = m_mask;
							for (size_t a = 0; a < m_speakerCount; a += 4)
							{
								const size_t b = a >> 2;
								CLOAK_ASSUME(b < m_frameSize);
								m_subMasks[b] = static_cast<API::Global::Audio::SPEAKER>(0);
								for (size_t c = 0; c < 4 && c + a < m_speakerCount; c++)
								{
									const uint8_t sbm = API::Global::Math::bitScanForward(m);
									CLOAK_ASSUME(sbm < 0xFF);
									const API::Global::Audio::SPEAKER sm = static_cast<API::Global::Audio::SPEAKER>(1 << sbm);
									CLOAK_ASSUME((sm & m) != static_cast<API::Global::Audio::SPEAKER>(0));
									m_subMasks[b] |= sm;
									m ^= sm;
								}
							}
							CLOAK_ASSUME(m == static_cast<API::Global::Audio::SPEAKER>(0));
	#ifdef _DEBUG
							for (size_t a = 0; a < m_frameSize; a++)
							{
								const uint8_t pc = API::Global::Math::popcount(m_subMasks[a]);
								CLOAK_ASSUME(pc == 4 || (a + 1 >= m_frameSize && pc < 4));
								CLOAK_ASSUME((m & m_subMasks[a]) == static_cast<API::Global::Audio::SPEAKER>(0));
								m |= m_subMasks[a];
							}
							CLOAK_ASSUME(m == m_mask);
	#endif
						}
						CLOAK_CALL ~ChannelSetup() { API::Global::Memory::MemoryHeap::Free(m_subMasks); }
						CLOAK_FORCEINLINE size_t CLOAK_CALL_THIS GetChannelCount() const { return m_speakerCount; }
						CLOAK_FORCEINLINE size_t CLOAK_CALL_THIS GetChannelCount(In size_t a) const 
						{
							if (a + 1 < m_frameSize) { return 4; }
							const size_t scm = m_speakerCount & 0x3;
							if (scm == 0) { return 4; }
							return scm;
						}
						CLOAK_FORCEINLINE size_t CLOAK_CALL_THIS GetFrameSize() const { return m_frameSize; }
						CLOAK_FORCEINLINE API::Global::Audio::SPEAKER CLOAK_CALL_THIS GetMask() const { return m_mask; }
						CLOAK_FORCEINLINE API::Global::Audio::SPEAKER CLOAK_CALL_THIS GetMask(In size_t a) const { CLOAK_ASSUME(a < m_frameSize); return m_subMasks[a]; }

						CLOAK_FORCEINLINE API::Global::Audio::SPEAKER CLOAK_CALL_THIS operator[](In size_t a) const { return GetMask(a); }

						void* CLOAK_CALL operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }
				};
				//Just a handsome container class to manage SSE buffer
				class SoundBuffer {
					protected:
						__m128* m_buffer;
						size_t m_frameCount;
						const size_t m_frameSize;
						const size_t m_channelCount;

						CLOAK_CALL SoundBuffer(In const SoundBuffer& o) = delete;
						CLOAK_CALL SoundBuffer(In size_t channelCount, In size_t frameCount) : m_frameSize((channelCount + 3) >> 2), m_frameCount(frameCount), m_channelCount(channelCount) {}
						CLOAK_CALL SoundBuffer(In const ChannelSetup& setup, In size_t frameCount) : SoundBuffer(setup.GetChannelCount(), frameCount) {}
					public:
						CLOAK_FORCEINLINE void CLOAK_CALL_THIS Reset() const
						{
							const size_t size = m_frameCount * m_frameSize;
							for (size_t a = 0; a < size; a++) { m_buffer[a] = _mm_set1_ps(0); }
						}
						//Get n-th frame
						CLOAK_FORCEINLINE __m128* CLOAK_CALL_THIS At(In size_t a) const { CLOAK_ASSUME(a < m_frameCount); return &m_buffer[a * m_frameSize]; }
						CLOAK_FORCEINLINE __m128* CLOAK_CALL_THIS GetBuffer() const { return m_buffer; }
						CLOAK_FORCEINLINE size_t CLOAK_CALL_THIS GetFrameSize() const { return m_frameSize; }
						CLOAK_FORCEINLINE size_t CLOAK_CALL_THIS GetFrameCount() const { return m_frameCount; }
						CLOAK_FORCEINLINE size_t CLOAK_CALL_THIS GetChannelCount()const { return m_channelCount; }

						//Get n-th frame
						CLOAK_FORCEINLINE __m128* CLOAK_CALL_THIS operator[](In size_t a) const { return At(a); }
				};
				class DynamicSoundBuffer : public SoundBuffer {
					private:
						API::Global::Memory::PageStackAllocator* const m_alloc;
					public:
						CLOAK_CALL DynamicSoundBuffer(In API::Global::Memory::PageStackAllocator* alloc, In size_t channelCount, In size_t frameCount) : m_alloc(alloc), SoundBuffer(channelCount, frameCount)
						{
							const size_t size = m_frameCount * m_frameSize;
							m_buffer = reinterpret_cast<__m128*>(m_alloc->Allocate(sizeof(__m128) * size));
							//PageStackAllocator already allocates in 16 byte alignment, so that there is no need to align this array manually
							//But to be on the safe side, we check the alignment anyway:
							CLOAK_ASSUME((reinterpret_cast<uintptr_t>(m_buffer) & 15) == 0);
						}
						CLOAK_CALL DynamicSoundBuffer(In API::Global::Memory::PageStackAllocator* alloc, In const ChannelSetup& setup, In size_t frameCount) : DynamicSoundBuffer(alloc, setup.GetChannelCount(), frameCount) {}
						CLOAK_CALL ~DynamicSoundBuffer() { m_alloc->Free(m_buffer); }
				};
				class RotatingSoundBuffer : public SoundBuffer {
					private:
						const size_t m_fullFrameCount;
						size_t m_readStart;
						size_t m_readSize;
						size_t m_readBreak;
						__m128* m_baseBuffer;
					public:
						CLOAK_CALL RotatingSoundBuffer(In size_t channelCount, In size_t frameCount) : SoundBuffer(channelCount, frameCount), m_fullFrameCount(frameCount << 1), m_readStart(0), m_readSize(0), m_readBreak(frameCount << 1)
						{
							const size_t size = m_fullFrameCount * m_frameSize;
							m_baseBuffer = reinterpret_cast<__m128*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(__m128) * size));
							m_buffer = m_baseBuffer;
							//PageStackAllocator already allocates in 16 byte alignment, so that there is no need to align this array manually
							//But to be on the safe side, we check the alignment anyway:
							CLOAK_ASSUME((reinterpret_cast<uintptr_t>(m_buffer) & 15) == 0);
						}

						CLOAK_CALL ~RotatingSoundBuffer() { API::Global::Memory::MemoryHeap::Free(m_baseBuffer); }
						//Called after buffer has been refilled
						CLOAK_FORCEINLINE void CLOAK_CALL_THIS OnPullComplete()
						{
							const size_t v = (m_readStart + m_readSize) % m_readBreak;
							if (m_buffer != &m_baseBuffer[v]) { m_readBreak = v; CLOAK_ASSUME(m_buffer == m_baseBuffer); }
							else if (m_readStart + m_readSize + m_frameCount < m_readBreak) { m_readBreak = m_fullFrameCount; }

							m_readSize += m_frameCount;
							m_frameCount = 0;
							m_buffer = nullptr;
						}
						//The first n frames were processed and are no longer needed, refill buffer with new n frames
						CLOAK_FORCEINLINE void CLOAK_CALL_THIS Pull(In size_t frames)
						{
							if (frames == 0) { return; }
							CLOAK_ASSUME(frames <= m_readSize);
							CLOAK_ASSUME(m_frameCount == 0);
							m_readSize -= frames;
							m_readStart = (m_readStart + frames) % m_readBreak;
							size_t v = (m_readStart + m_readSize) % m_readBreak;
							if (m_fullFrameCount - v < frames) { v = 0; }
							CLOAK_ASSUME(v + frames <= m_readStart || m_readStart + m_readSize <= v);
							m_buffer = &m_baseBuffer[v * m_frameSize];
							m_frameCount = frames;
						}
						CLOAK_FORCEINLINE bool CLOAK_CALL_THIS HasToPull() const { return m_frameCount > 0; }
						CLOAK_FORCEINLINE const __m128* CLOAK_CALL_THIS ReadFrame(In size_t frame) const
						{
							CLOAK_ASSUME(frame < m_readSize);
							return &m_baseBuffer[((m_readStart + frame) % m_readBreak)*m_frameSize];
						}
						CLOAK_FORCEINLINE size_t CLOAK_CALL_THIS GetAviableFrames() const { return m_readSize; }
				};

				class ISoundProcessor : public API::Helper::IBasicPtr {
					private:
						std::atomic<uint64_t> m_ref = 1;
					protected:
						virtual CLOAK_CALL ~ISoundProcessor() {}
					public:
						uint64_t CLOAK_CALL_THIS AddRef() override
						{
							return m_ref.fetch_add(1) + 1;
						}
						uint64_t CLOAK_CALL_THIS Release() override
						{
							uint64_t r = m_ref.fetch_sub(1);
							if (r == 1) { delete this; }
							else if (r == 0) { m_ref = 0; }
							return r > 0 ? r - 1 : 0;
						}
						//Notice: Process should OVERWRITE all samples in the buffer. The buffer might not include any valid data before calling this function
						virtual void CLOAK_CALL_THIS Process(In API::Global::Memory::PageStackAllocator* alloc, In const SoundBuffer& buffer, In const ChannelSetup& channels) = 0;
						virtual void CLOAK_CALL_THIS OnFocusLost() = 0;
						virtual void CLOAK_CALL_THIS OnFocusGained() = 0;

						void* CLOAK_CALL operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }
				};
				class ISoundEffect : public ISoundProcessor {
					protected:
						std::atomic<ISoundProcessor*> m_parent;
						std::atomic<bool> m_focused = true;
					public:
						CLOAK_CALL ISoundEffect() : m_parent(nullptr) {}
						virtual CLOAK_CALL ~ISoundEffect() { SetParent(nullptr); }
						
						CLOAK_FORCEINLINE void CLOAK_CALL SetParent(In ISoundProcessor* p)
						{
							if (p != nullptr) { p->AddRef(); }
							ISoundProcessor* o = m_parent.exchange(p);
							if (o != nullptr) { o->Release(); }
						}
						CLOAK_FORCEINLINE API::RefPointer<ISoundProcessor> CLOAK_CALL_THIS GetParent() { return m_parent.load(); }
						void CLOAK_CALL_THIS OnFocusLost() override 
						{
							if (m_focused.exchange(false) == true)
							{
								ISoundProcessor* p = GetParent();
								if (p != nullptr) 
								{ 
									p->OnFocusLost(); 
									p->Release(); 
								}
							}
						}
						void CLOAK_CALL_THIS OnFocusGained() override 
						{
							if (m_focused.exchange(true) == false)
							{
								ISoundProcessor* p = GetParent();
								if (p != nullptr)
								{
									p->OnFocusGained();
									p->Release();
								}
							}
						}
				};

				class ChannelMatrix {
					private:
						__m128* m_volume;
						__m128* m_tmp;
						API::Global::Audio::SPEAKER m_inputMask;
						API::Global::Audio::SPEAKER m_outputMask;
						size_t m_inputSize;
						size_t m_outputSize;
					public:
						CLOAK_CALL ChannelMatrix();
						CLOAK_CALL ChannelMatrix(In const ChannelMatrix& x);
						CLOAK_CALL ChannelMatrix(In API::Global::Audio::SPEAKER input, API::Global::Audio::SPEAKER output);
						CLOAK_CALL ~ChannelMatrix();

						ChannelMatrix& CLOAK_CALL_THIS operator=(In const ChannelMatrix& x);

						size_t CLOAK_CALL_THIS InputSize() const;
						size_t CLOAK_CALL_THIS OutputSize() const;
						void CLOAK_CALL_THIS Transform(In_reads(m_inputSize) const __m128* input, Out_writes(m_outputSize) __m128* output) const;
				};

				void CLOAK_CALL Start();
				void CLOAK_CALL Shutdown();
				void CLOAK_CALL Release();
				void CLOAK_CALL FinalRelease();
				void CLOAK_CALL OnFocusLost();
				void CLOAK_CALL OnFocusGained();
			}
		}
	}
}

#endif