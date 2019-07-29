#include "stdafx.h"
#include "Engine\Audio\Resampler.h"

#include <type_traits>
#include <functional>
//Resampling algorithmus based of "Zita-Resmpler" v1.6.0 (https://kokkinizita.linuxaudio.org/linuxaudio/zita-resampler/resampler.html) from Fons Adriaensen  <fons@linuxaudio.org>

namespace CloakEngine {
	namespace Engine {
		namespace Audio {
			//Valid range is 16 to 96
			constexpr uint32_t KERNEL_HALF = 48;
			constexpr uint32_t KERNEL_SIZE = KERNEL_HALF << 1;
			constexpr uint32_t BUFFERED_FRAMES = 250;

			opt_in_constexpr CLOAK_FORCEINLINE double CLOAK_CALL sinc(In double x)
			{
				x = API::Global::Math::abs(x);
				if (x < 1e-6) { return 1.0; }
				x *= API::Global::Math::PI;
				return API::Global::Math::sin(x) / x;
			}
			opt_in_constexpr CLOAK_FORCEINLINE double CLOAK_CALL wind(In double x)
			{
				x = API::Global::Math::abs(x);
				if (x >= 1.0) { return 0.0; }
				x *= API::Global::Math::PI;
				return 0.384 + (0.500 * API::Global::Math::cos(x)) + (0.116 * API::Global::Math::cos(2 * x));
			}

			CLOAK_FORCEINLINE Resampler::TABLE_DESC CLOAK_CALL Resampler::iCreateTable(In uint32_t inputFrequency, In uint32_t outputFrequency, In size_t channelCount)
			{
				CLOAK_ASSUME(outputFrequency > 0 && inputFrequency > 0);
				if (inputFrequency == outputFrequency)
				{
					Resampler::TABLE_DESC desc;
					desc.ChannelCount = channelCount;
					desc.BufferFrameSize = 0; //We are going to pass through, so no buffer needed
					desc.FIRBufferSize = 0;

					desc.InputBuffer = 0;
					desc.InputFrames = 0;
					desc.OutputFrames = 0;
					desc.KernelPower = 0;
					desc.KernelHalfSize = 0;
					return desc;
				}
				else
				{
					const double otir = static_cast<double>(outputFrequency) / inputFrequency;
					const uint32_t gcd = API::Global::Math::gcd(outputFrequency, inputFrequency);
					//Algorithm generates #outFrames frames for each #inFrames frames it reads
					const uint32_t inFrames = inputFrequency / gcd;
					const uint32_t outFrames = outputFrequency / gcd;
					//Check if values are within the expected range of the algorithm
					CLOAK_ASSUME(otir * 16 >= 1 && outFrames <= 1000);
					uint32_t kh = KERNEL_HALF;
					uint32_t inBuffer = BUFFERED_FRAMES;
					double kf = 1.0 - (2.6 / kh);
					if (otir < 1)
					{
						kf *= otir;
						kh = static_cast<uint32_t>(std::ceil(kh / otir));
						inBuffer = static_cast<uint32_t>(std::ceil(inBuffer / otir));
					}

					Resampler::TABLE_DESC desc;
					desc.ChannelCount = channelCount;
					desc.BufferFrameSize = (kh << 1) + inBuffer - 1; //Minus one as we never reach the end of that area
					desc.FIRBufferSize = kh * (outFrames + 1);

					desc.InputBuffer = inBuffer;
					desc.InputFrames = inFrames;
					desc.OutputFrames = outFrames;
					desc.KernelPower = kf;
					desc.KernelHalfSize = kh;
					return desc;
				}
			}

			CLOAK_CALL Resampler::Table::Table(In const TABLE_DESC& desc) : SoundBuffer(desc.ChannelCount, desc.BufferFrameSize), m_fir(reinterpret_cast<float*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(float)*desc.FIRBufferSize))), ReadBuffer(desc.InputBuffer), InputFrames(desc.InputFrames), OutputFrames(desc.OutputFrames), KernelSize(desc.KernelHalfSize << 1), KernelHalf(desc.KernelHalfSize), SoundBufferSize(static_cast<uint32_t>(desc.BufferFrameSize))
			{
#ifdef _DEBUG
				m_firSize = desc.FIRBufferSize;
#endif
				if (desc.FIRBufferSize > 0)
				{
					for (size_t a = 0; a <= desc.OutputFrames; a++) //Calculate FIR values for all possible phase offsets
					{
						const size_t p = (a + 1) * desc.KernelHalfSize;
						double b = static_cast<double>(a) / desc.OutputFrames;
						for (size_t c = 0; c < desc.KernelHalfSize; c++, b += 1) //Calculate FIR kernel with specific phase offset
						{
#ifdef _DEBUG
							CLOAK_ASSUME(p - (c + 1) < m_firSize);
#endif
							const double s = sinc(b * desc.KernelPower);
							const double w = wind(b / desc.KernelHalfSize);
							m_fir[p - (c + 1)] = static_cast<float>(desc.KernelPower * s * w);
						}
					}
				}
			}
			CLOAK_CALL Resampler::Table::~Table()
			{
				API::Global::Memory::MemoryHeap::Free(m_fir);
			}
			const float& CLOAK_CALL_THIS Resampler::Table::operator[](In size_t a) const
			{
#ifdef _DEBUG
				CLOAK_ASSUME(a < m_firSize);
#endif
				return m_fir[a];
			}

			CLOAK_CALL Resampler::Resampler(In uint32_t inputFrequency, In uint32_t outputFrequency, In size_t channelCount) : m_table(iCreateTable(inputFrequency, outputFrequency, channelCount)), m_noOp(inputFrequency == outputFrequency)
			{
				m_readPos = 0;
				m_emptyFrames = 0;
			}

			void CLOAK_CALL_THIS Resampler::Process(In API::Global::Memory::PageStackAllocator* alloc, In const Engine::Audio::Core::SoundBuffer& buffer, In const Engine::Audio::Core::ChannelSetup& channels)
			{
				Engine::Audio::Core::ISoundProcessor* parent = GetParent();
				if (m_noOp == true)
				{
					//Pass through (no operation needed)
					if (parent != nullptr) { parent->Process(alloc, buffer, channels); }
					else { buffer.Reset(); }
				}
				else
				{
					const size_t outFrameCount = buffer.GetFrameCount();
					const size_t frameSize = buffer.GetFrameSize();
					CLOAK_ASSUME(frameSize == m_table.SoundBuffer.GetFrameSize());
					for (size_t outPos = 0; outPos < outFrameCount; outPos++)
					{
						//Query new data
						if (m_table.SoundBuffer.HasToPull())
						{
							if (parent != nullptr) 
							{ 
								parent->Process(alloc, m_table.SoundBuffer, channels); 
								m_emptyFrames = 0;
							}
							else if (m_emptyFrames < m_table.SoundBufferSize)
							{
								m_emptyFrames += m_table.SoundBuffer.GetFrameCount();
								m_table.SoundBuffer.Reset();
							}
							m_table.SoundBuffer.OnPullComplete();
						}
						//Calculate output:
						for (size_t a = 0; a < frameSize; a++) { buffer[outPos][a] = _mm_set1_ps(0); }
						if (m_emptyFrames < m_table.SoundBufferSize - m_readPos)
						{
							const uint32_t firID1 = m_table.KernelHalf * m_phaseOffset;
							const uint32_t firID2 = m_table.KernelHalf * (m_table.OutputFrames - m_phaseOffset);
							for (size_t a = 0; a < m_table.KernelHalf; a++)
							{
								const __m128 fir[] = { _mm_set1_ps(m_table[firID1 + a]), _mm_set1_ps(m_table[firID2 + a]) };
								const __m128* rf[] = { m_table.SoundBuffer.ReadFrame(m_readPos + a),m_table.SoundBuffer.ReadFrame(m_readPos + m_table.KernelSize - (a + 1)) };

								for (size_t b = 0; b < frameSize; b++)
								{
									__m128 c = buffer[outPos][b];
									c = _mm_add_ps(c, _mm_add_ps(_mm_mul_ps(fir[0], rf[0][b]), _mm_mul_ps(fir[1], rf[1][b])));
									buffer[outPos][b] = c;
								}
							}
						}
						//Update:
						m_phaseOffset += m_table.InputFrames;
						if (m_phaseOffset >= m_table.OutputFrames)
						{
							const uint32_t steps = m_phaseOffset / m_table.OutputFrames;
							m_phaseOffset -= steps * m_table.OutputFrames;
							m_readPos += steps;
							if (m_readPos >= m_table.ReadBuffer)
							{
								m_table.SoundBuffer.Pull(m_readPos);
								m_readPos = 0;
							}
						}
					}
				}
				if (parent != nullptr) { parent->Release(); }
			}
		}
	}
}