#include "stdafx.h"
#include "Implementation/OS.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Global/Memory.h"
#include "CloakEngine/Global/Math.h"
#include "Implementation/Global/Math.h"

#include <atomic>
#include <thread>

#if CHECK_OS(Linux, 0)
#define _GNU_SOURCE
#include <sched.h>
#endif

struct CPUInfo {
	union {
		struct {
			uint32_t EAX;
			uint32_t EBX;
			uint32_t ECX;
			uint32_t EDX;
		};
		uint32_t Data[4];
	};
};

#if CHECK_COMPILER(MicrosoftVisualCpp)
#include <roapi.h>
CLOAK_FORCEINLINE void CLOAK_CALL cpuidex(Out CPUInfo* info, In uint32_t type, In uint32_t subtype)
{
	__cpuidex(reinterpret_cast<int*>(&info->Data[0]), static_cast<int>(type), static_cast<int>(subtype));
}
#elif CHECK_COMPILER(GCC)
#include <cpuid.h>
CLOAK_FORCEINLINE void CLOAK_CALL cpuidex(Out CPUInfo* info, In uint32_t type, In uint32_t subtype)
{
	__get_cpuid_count(type, subtype, &info->EAX, &info->EBX, &info->ECX, &info->EDX);
}
#else
#error "Compiler-specific CPUID implementation not specified for the current compiler"
#endif

#define cpuid(info,type) cpuidex(info,type,0)

namespace CloakEngine {
	namespace Impl {
		namespace OS {
			constexpr uint32_t CACHE_TYPE_DATA = 1;
			constexpr uint32_t CACHE_TYPE_INSTRUCTION = 2;
			constexpr uint32_t CACHE_TYPE_UNIFIED = 3;

			std::atomic<size_t> g_physProcCount = 0;
			std::atomic<size_t> g_logiProcCount = 0;
			CPU_INFORMATION g_cpuSupport;

			CLOAK_FORCEINLINE constexpr uint32_t CLOAK_CALL CalculateCachePosition(In uint32_t level, In uint32_t type)
			{
				if (level > MAX_CACHE_LEVEL || type > MAX_CACHE_TYPES || level == 0 || type == 0) { return MAX_CACHE_COUNT; }
				return GetCacheID(static_cast<CacheType>(type - 1), static_cast<CacheLevel>(level - 1));
			}
			inline bool CLOAK_CALL BindToCPU(In uint32_t cpuID)
			{
#if CHECK_OS(WINDOWS, 0)
				const DWORD_PTR curCPU = static_cast<DWORD_PTR>(1) << cpuID;
				return SetThreadAffinityMask(GetCurrentThread(), curCPU) != 0;
#elif CHECK_OS(LINUX, 0)
				cpu_set_t curCPU;
				CPU_ZERO(&curCPU);
				CPU_SET(cpuID, &curCPU);
				return !!sched_setaffinity(0, sizeof(curCPU), &curCPU);
#else
#error "BindToCPU is not implemented for current OS!"
#endif
			}
			inline void CLOAK_CALL ResetCPUBinding()
			{
#if CHECK_OS(WINDOWS, 0)
				const DWORD_PTR curCPU = ~static_cast<DWORD_PTR>(1);
				SetThreadAffinityMask(GetCurrentThread(), curCPU);
#elif CHECK_OS(LINUX, 0)
				cpu_set_t curCPU;
				CPU_ZERO(&curCPU);
				for (uint32_t a = 0; a < CPU_SETSIZE; a++) { CPU_SET(a, &curCPU); }
				sched_setaffinity(0, sizeof(curCPU), &curCPU);
#else
#error "ResetCPUBinding is not implemented for current OS!"
#endif
			}
			inline uint32_t CLOAK_CALL GetMaxProcessorSupport()
			{
#if CHECK_OS(WINDOWS, 0)
				SYSTEM_INFO si;
				GetSystemInfo(&si);
				return static_cast<uint32_t>(si.dwNumberOfProcessors);
#elif CHECK_OS(LINUX, 0)
				return sysconf(_SC_NPROCESSORS_CONF);
#else
#error "GetMaxProcessorSupport is not implemented for current OS!"
#endif
			}
			inline uint32_t CLOAK_CALL CreateMask(In uint32_t numEntries, Out_opt uint32_t* width = nullptr)
			{
				const uint64_t k = (static_cast<uint64_t>(numEntries) << 1) - 1;
				const uint8_t i = API::Global::Math::bitScanReverse(k);
				if (i == 0xFF)
				{
					if (width != nullptr) { *width = 0; }
					return 0;
				}
				if (width != nullptr) { *width = i; }
				return ~((~0Ui32) << i);
			}
			CLOAK_FORCEINLINE uint32_t CLOAK_CALL GetBits(In uint32_t value, In_range(0,31) uint32_t first, In_range(0,31) uint32_t last)
			{
				CLOAK_ASSUME(first <= last);
				const uint32_t m = static_cast<uint32_t>(~((~(0Ui64)) << (1 + last)));
				return (value & m) >> first;
			}
			CLOAK_FORCEINLINE bool CLOAK_CALL GetBit(In uint32_t value, In uint32_t pos) { return GetBits(value, pos, pos) != 0; }

			void CLOAK_CALL LoadCPUSupport()
			{
				//At this point in time, memory management is not yet initialized, so we can't use it to allocate the array
				g_cpuSupport.Topology.ProcessorCount = GetMaxProcessorSupport();
				g_cpuSupport.Topology.Processors = new PROCESSOR_DATA[g_cpuSupport.Topology.ProcessorCount];

				CPUInfo info;
				//Read Vendor and Standard Command Support
				cpuid(&info, 0);
				const uint32_t maxID = info.EAX;
				char vendor[] = {
					static_cast<char>((info.EBX >> 0x00) & 0xFF),
					static_cast<char>((info.EBX >> 0x08) & 0xFF),
					static_cast<char>((info.EBX >> 0x10) & 0xFF),
					static_cast<char>((info.EBX >> 0x18) & 0xFF),
					static_cast<char>((info.EDX >> 0x00) & 0xFF),
					static_cast<char>((info.EDX >> 0x08) & 0xFF),
					static_cast<char>((info.EDX >> 0x10) & 0xFF),
					static_cast<char>((info.EDX >> 0x18) & 0xFF),
					static_cast<char>((info.ECX >> 0x00) & 0xFF),
					static_cast<char>((info.ECX >> 0x08) & 0xFF),
					static_cast<char>((info.ECX >> 0x10) & 0xFF),
					static_cast<char>((info.ECX >> 0x18) & 0xFF),
					'\0',
				};
				g_cpuSupport.Vendor = vendor;

				cpuid(&info, 0x80000000);
				const uint32_t maxED = info.EAX;
				if (maxED >= 0x80000004)
				{
					char brand[48];
					for (uint32_t subT = 0x80000002, b = 0; subT <= 0x80000004; subT++)
					{
						cpuid(&info, subT);
						for (size_t a = 0; a < 4; a++)
						{
							for (size_t c = 0; c < 4; c++, b++) { brand[b] = static_cast<char>((info.Data[a] >> (c << 3)) & 0xFF); }
						}
					}
					g_cpuSupport.Brand = brand;
				}

				uint32_t maxCoreCount = 0;
				//Determine supported instruction set:
				if (maxID >= 0x00000001)
				{
					cpuid(&info, 0x00000001);
					g_cpuSupport.Instructions.SSE.SSE3 = GetBit(info.ECX, 0);
					g_cpuSupport.Instructions.SSE.SSSE3 = GetBit(info.ECX, 9);
					g_cpuSupport.Instructions.SSE.SSE4_1 = GetBit(info.ECX, 19);
					g_cpuSupport.Instructions.SSE.SSE4_2 = GetBit(info.ECX, 20);
					g_cpuSupport.Instructions.Other.AES = GetBit(info.ECX, 25);
					g_cpuSupport.Instructions.AVX.AVX = GetBit(info.ECX, 28);

					g_cpuSupport.Instructions.MMX.MMX1 = GetBit(info.EDX, 23);
					g_cpuSupport.Instructions.SSE.SSE1 = GetBit(info.EDX, 25);
					g_cpuSupport.Instructions.SSE.SSE2 = GetBit(info.EDX, 26);


					if (GetBit(info.EDX, 28)) { maxCoreCount = GetBits(info.EBX, 16, 23); }
				}
				if (maxID >= 0x00000007)
				{
					cpuid(&info, 0x00000007);
					g_cpuSupport.Instructions.AVX.AVX2											= GetBit(info.EBX, 5);
					g_cpuSupport.Instructions.AVX.AVX512.Enabled								= GetBit(info.EBX, 16);
					g_cpuSupport.Instructions.AVX.AVX512.DoubleQuadWords						= GetBit(info.EBX, 17);
					g_cpuSupport.Instructions.AVX.AVX512.IntegerMAD								= GetBit(info.EBX, 21);
					g_cpuSupport.Instructions.AVX.AVX512.Prefetch								= GetBit(info.EBX, 27);
					g_cpuSupport.Instructions.AVX.AVX512.ConflictDetection						= GetBit(info.EBX, 28);
					g_cpuSupport.Instructions.Other.SHA											= GetBit(info.EBX, 29);
					g_cpuSupport.Instructions.AVX.AVX512.ByteWord								= GetBit(info.EBX, 30);
					g_cpuSupport.Instructions.AVX.AVX512.VectorLength							= GetBit(info.EBX, 31);

					g_cpuSupport.Instructions.AVX.AVX512.VectorBitManipulation					= GetBit(info.ECX, 1);
					g_cpuSupport.Instructions.AVX.AVX512.VectorBitManipulation2					= GetBit(info.ECX, 6);
					g_cpuSupport.Instructions.AVX.AVX512.NeuralNetwork.Vector					= GetBit(info.ECX, 11);
					g_cpuSupport.Instructions.AVX.AVX512.BITALG									= GetBit(info.ECX, 12);
					g_cpuSupport.Instructions.AVX.AVX512.VectorPopulationCoundDoubleQuadWords	= GetBit(info.ECX, 14);

					g_cpuSupport.Instructions.AVX.AVX512.NeuralNetwork.Register4				= GetBit(info.EDX, 2);
					g_cpuSupport.Instructions.AVX.AVX512.FloatMAD								= GetBit(info.EDX, 3);
				}

				if (g_cpuSupport.Vendor == "GenuineIntel")
				{
					//See https://www.scss.tcd.ie/Jeremy.Jones/CS4021/processor-identification-cpuid-instruction-note.pdf
					bool support2xAPIC = true;
					
					//Determine CPU topology:
					uint32_t coreMask = 0;
					uint32_t coreMaskWidth = 0;
					uint32_t threadMaskWidth = 0;
					uint32_t threadMask = 0;
					uint32_t pkgMask = ~0Ui32;
					if (maxCoreCount > 0)
					{
						if (maxID >= 0x0000000B)
						{
							uint32_t subLeaf = 0;
							do {
								cpuidex(&info, 0x0000000B, subLeaf);
								if (info.EBX == 0) 
								{
									if (subLeaf == 0) { goto intel_topology_detection_legacy; }
									break;
								}
								const uint32_t type = GetBits(info.ECX, 8, 15);
								const uint32_t shift = GetBits(info.EAX, 0, 4);
								switch (type)
								{
									case 0: break;
									case 1:
										//Thread
										threadMask = ~((~0Ui32) << shift);
										threadMaskWidth = shift;
										break;
									case 2:
										//Core
										coreMask = ~((~0Ui32) << shift);
										coreMaskWidth = shift;
										break;
									default: break;
								}
								subLeaf++;
							} while (true);
							pkgMask = ~coreMask;
							coreMask ^= threadMask;
							goto intel_topology_detection_finished;
						}
					intel_topology_detection_legacy:
						support2xAPIC = false;
						uint32_t maxCoreId = 1;
						if (maxID >= 0x00000004)
						{
							cpuid(&info, 0x00000004);
							maxCoreId = GetBits(info.EAX, 26, 31) + 1;
						}
						const uint32_t threadPerCore = maxCoreCount / maxCoreId;
						threadMask = CreateMask(threadPerCore, &threadMaskWidth);
						coreMask = CreateMask(maxCoreId, &coreMaskWidth);
						coreMaskWidth += threadMaskWidth;
						coreMask <<= threadMaskWidth;
						pkgMask = ~(coreMask | threadMask);
					}
				intel_topology_detection_finished:

					//Determine cache sizes:
					if (maxID >= 0x00000004)
					{
						for (uint32_t a = 0; true; a++)
						{
							cpuidex(&info, 0x00000004, a);
							const uint32_t type			= GetBits(info.EAX, 0, 4);
							const uint32_t level		= GetBits(info.EAX, 5, 7);
							const uint32_t ways			= GetBits(info.EBX, 22, 31) + 1;
							const uint32_t partitions	= GetBits(info.EBX, 12, 21) + 1;
							const uint32_t line			= GetBits(info.EBX, 0, 11) + 1;
							const uint32_t sets			= GetBits(info.ECX, 0, 31) + 1;
							if (type == 0) { break; } //No more caches
							if (level > MAX_CACHE_LEVEL || type > MAX_CACHE_TYPES) { continue; }
							const uint32_t p = CalculateCachePosition(level, type);
							CLOAK_ASSUME(p < MAX_CACHE_COUNT);
							g_cpuSupport.Topology.Cache[p].LineSize = line;
							g_cpuSupport.Topology.Cache[p].FullSize = ways * partitions * line * sets;
							g_cpuSupport.Topology.Cache[p].Mask = ~CreateMask(GetBits(info.EAX, 14, 25) + 1);
						}
					}

					//Query processor APIC IDs and building topology informations:
					uint32_t mappedCPUs = 0;
					for (uint32_t a = 0; a < g_cpuSupport.Topology.ProcessorCount; a++)
					{
						if (BindToCPU(a) == false) { break; }
						if (support2xAPIC == true)
						{
							cpuid(&info, 0x0000000B);
							g_cpuSupport.Topology.Processors[a].APICID = GetBits(info.EDX, 0, 31);
						}
						else
						{
							cpuid(&info, 0x00000001);
							g_cpuSupport.Topology.Processors[a].APICID = GetBits(info.EBX, 24, 31);
						}
						g_cpuSupport.Topology.Processors[a].PackageID = (g_cpuSupport.Topology.Processors[a].APICID & pkgMask) >> coreMaskWidth;
						g_cpuSupport.Topology.Processors[a].CoreID = (g_cpuSupport.Topology.Processors[a].APICID & coreMask) >> threadMaskWidth;
						g_cpuSupport.Topology.Processors[a].ThreadID = g_cpuSupport.Topology.Processors[a].APICID & threadMask;
						for (uint32_t b = 0; b < MAX_CACHE_COUNT; b++)
						{
							g_cpuSupport.Topology.Processors[a].CacheID[b] = g_cpuSupport.Topology.Processors[a].APICID & g_cpuSupport.Topology.Cache[b].Mask;
						}
						mappedCPUs++;
					}
					g_cpuSupport.Topology.ProcessorCount = mappedCPUs;
				}
				else if (g_cpuSupport.Vendor == "AuthenticAMD")
				{
					//See https://www.amd.com/system/files/TechDocs/25481.pdf
					//Determine supported instruction set:
					if (maxED >= 0x80000001)
					{
						cpuid(&info, 0x80000001);
						g_cpuSupport.Instructions.SSE.SSE4_a			= GetBit(info.ECX, 6);

						g_cpuSupport.Instructions.MMX.MMX2				= GetBit(info.EDX, 22);
						g_cpuSupport.Instructions.AMD3DNow.Extension	= GetBit(info.EDX, 30);
						g_cpuSupport.Instructions.AMD3DNow.Enabled		= GetBit(info.EDX, 31);
					}

					//Determine CPU topology:
					uint32_t coreMask = 0;
					uint32_t coreMaskWidth = 0;
					if (maxCoreCount > 0)
					{
						if (maxED >= 0x80000008)
						{
							cpuid(&info, 0x80000008);
							coreMaskWidth = GetBits(info.ECX, 12, 15);
							if (coreMaskWidth == 0) 
							{
								const uint32_t coreCount = GetBits(info.ECX, 0, 7) + 1;
								coreMask = CreateMask(coreCount, &coreMaskWidth);
							}
							else { coreMask = ~((~0Ui32) << coreMaskWidth); }
						}
						else { coreMask = CreateMask(maxCoreCount, &coreMaskWidth); }
					}

					//Determine cache sizes:
					if (maxED >= 0x80000005)// L1 Cache
					{
						cpuid(&info, 0x80000005);
						const uint32_t dp = CalculateCachePosition(1, CACHE_TYPE_DATA);
						const uint32_t ip = CalculateCachePosition(1, CACHE_TYPE_INSTRUCTION);
						g_cpuSupport.Topology.Cache[dp].FullSize = GetBits(info.ECX, 24, 31) KB;
						g_cpuSupport.Topology.Cache[dp].LineSize = GetBits(info.ECX, 0, 7);
						g_cpuSupport.Topology.Cache[dp].Mask = ~0;
						g_cpuSupport.Topology.Cache[ip].FullSize = GetBits(info.EDX, 24, 31) KB;
						g_cpuSupport.Topology.Cache[ip].LineSize = GetBits(info.EDX, 0, 7);
						g_cpuSupport.Topology.Cache[ip].Mask = ~0;
					}
					if (maxED >= 0x80000006)// L2 & L3 Cache
					{
						cpuid(&info, 0x80000006);
						const uint32_t pl2 = CalculateCachePosition(2, CACHE_TYPE_UNIFIED);
						const uint32_t pl3 = CalculateCachePosition(3, CACHE_TYPE_UNIFIED);
						g_cpuSupport.Topology.Cache[pl2].FullSize = GetBits(info.ECX, 16, 31) KB;
						g_cpuSupport.Topology.Cache[pl2].LineSize = GetBits(info.ECX, 0, 7);
						g_cpuSupport.Topology.Cache[pl2].Mask = ~0;
						g_cpuSupport.Topology.Cache[pl3].FullSize = GetBits(info.EDX, 18, 31) * 512 KB; //L3 cache is at least this size. May be up to 512 KB larger
						g_cpuSupport.Topology.Cache[pl3].LineSize = GetBits(info.EDX, 0, 7);
						g_cpuSupport.Topology.Cache[pl3].Mask = ~((~0Ui32) << coreMaskWidth);//According to the AMD doku, L3 cache is shared between all cores of a processor
					}

					//Query processor APIC IDs and building topology informations:
					uint32_t mappedCPUs = 0;
					for (uint32_t a = 0; a < g_cpuSupport.Topology.ProcessorCount; a++)
					{
						if (BindToCPU(a) == false) { break; }
						cpuid(&info, 0x00000001);
						g_cpuSupport.Topology.Processors[a].APICID = GetBits(info.EBX, 24, 31);
						g_cpuSupport.Topology.Processors[a].PackageID = g_cpuSupport.Topology.Processors[a].APICID >> coreMaskWidth;
						g_cpuSupport.Topology.Processors[a].CoreID = g_cpuSupport.Topology.Processors[a].APICID & coreMask;
						g_cpuSupport.Topology.Processors[a].ThreadID = 0;
						for (uint32_t b = 0; b < MAX_CACHE_COUNT; b++)
						{
							g_cpuSupport.Topology.Processors[a].CacheID[b] = g_cpuSupport.Topology.Processors[a].APICID & g_cpuSupport.Topology.Cache[b].Mask;
						}
						mappedCPUs++;
					}
					g_cpuSupport.Topology.ProcessorCount = mappedCPUs;
				}

				//Build topology:
				CLOAK_ASSUME(g_cpuSupport.Topology.ProcessorCount <= static_cast<uint16_t>(~0));
				uint32_t counting[16];
				uint16_t* heap = reinterpret_cast<uint16_t*>(_malloca(sizeof(uint16_t) * g_cpuSupport.Topology.ProcessorCount * 2));
				uint16_t* sorted[2] = { &heap[0], &heap[g_cpuSupport.Topology.ProcessorCount] };
				for (uint32_t a = 0; a < g_cpuSupport.Topology.ProcessorCount; a++) { sorted[0][a] = static_cast<uint16_t>(a); }
				for (UINT b = 0; b < 32; b += 4)
				{
					memset(counting, 0, sizeof(uint32_t) * ARRAYSIZE(counting));
					for (uint32_t a = 0; a < g_cpuSupport.Topology.ProcessorCount; a++) { counting[(g_cpuSupport.Topology.Processors[a].APICID >> b) & 0xF]++; }
					for (uint32_t a = 1; a < ARRAYSIZE(counting); a++) { counting[a] += counting[a - 1]; }
					for (uint32_t a = g_cpuSupport.Topology.ProcessorCount; a > 0; a--)
					{
						const uint32_t p = (g_cpuSupport.Topology.Processors[a - 1].APICID >> b) & 0xF;
						sorted[1][--counting[p]] = sorted[0][a - 1];
					}
					std::swap(sorted[0], sorted[1]);
				}
				for (uint32_t a = 0, pkgID = 0, coreID = 0, threadID = 0; a < g_cpuSupport.Topology.ProcessorCount; a++)
				{
					const uint32_t sl = sorted[0][a];

					const uint32_t mp = pkgID;
					const uint32_t mc = coreID;
					const uint32_t mt = threadID;
					if (a + 1 < g_cpuSupport.Topology.ProcessorCount)
					{
						const uint32_t sn = sorted[0][a + 1];
						if (g_cpuSupport.Topology.Processors[sl].ThreadID != g_cpuSupport.Topology.Processors[sn].ThreadID)
						{
							threadID++;
						}
						if (g_cpuSupport.Topology.Processors[sl].CoreID != g_cpuSupport.Topology.Processors[sn].CoreID)
						{
							threadID = 0;
							coreID++;
						}
						if (g_cpuSupport.Topology.Processors[sl].PackageID != g_cpuSupport.Topology.Processors[sn].PackageID)
						{
							threadID = 0;
							coreID = 0;
							pkgID++;
						}
					}
					g_cpuSupport.Topology.Processors[sl].ThreadID = mt;
					g_cpuSupport.Topology.Processors[sl].CoreID = mc;
					g_cpuSupport.Topology.Processors[sl].PackageID = mp;
				}
				for (uint32_t a = 0; a < MAX_CACHE_COUNT; a++)
				{
					for (uint32_t b = 0, cacheID = 0; b < g_cpuSupport.Topology.ProcessorCount; b++)
					{
						const uint32_t sl = sorted[0][b];

						const uint32_t mc = cacheID;
						if (b + 1 < g_cpuSupport.Topology.ProcessorCount)
						{
							const uint32_t sn = sorted[0][b + 1];
							if (g_cpuSupport.Topology.Processors[sl].CacheID[a] != g_cpuSupport.Topology.Processors[sn].CacheID[a])
							{
								cacheID++;
							}
						}
						g_cpuSupport.Topology.Processors[sl].CacheID[a] = mc;
					}
				}
				_freea(heap);
				ResetCPUBinding();
				Impl::Global::Math::Initialize(g_cpuSupport.Instructions.SSE.SSE4_1, g_cpuSupport.Instructions.AVX.AVX);
			}
			const CPU_INFORMATION& GetCPUInformation() { return g_cpuSupport; }

#ifdef _WINDOWS
			typedef BOOL(WINAPI *LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

			size_t CLOAK_CALL GetProcessorCount()
			{
				size_t CoreCount = 0;
				HMODULE hMod = GetModuleHandle(L"kernel32");
				if (hMod == INVALID_HANDLE_VALUE || hMod == NULL) { return 0; }
				auto GLPI = reinterpret_cast<LPFN_GLPI>(GetProcAddress(hMod, "GetLogicalProcessorInformation"));
				if (GLPI == nullptr) { return 0; }
				bool done = false;
				PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = nullptr;
				DWORD resLen = 0;
				do {
					BOOL rc = GLPI(buffer, &resLen);
					if (rc == FALSE)
					{
						if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
						{
							if (buffer != nullptr) { API::Global::Memory::MemoryPool::Free(buffer); }
							buffer = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(API::Global::Memory::MemoryPool::Allocate(resLen));
							if (buffer == nullptr) { return 0; }
						}
						else { return 0; }
					}
					else { done = true; }
				} while (done == false);
				if (buffer == nullptr) { return 0; }
				DWORD offset = 0;
				PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = buffer;
				while (offset < resLen)
				{
					if (ptr->Relationship == RelationProcessorCore)
					{
						if (ptr->ProcessorCore.Flags != 0) { CoreCount++; }
						else { CoreCount += API::Global::Math::popcount(ptr->ProcessorMask); }
					}
					offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
					ptr++;
				}
				API::Global::Memory::MemoryPool::Free(buffer);
				return CoreCount;
			}

			void CLOAK_CALL Initialize()
			{
				HRESULT hRet = Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
				if(CloakCheckError(hRet, API::Global::Debug::Error::GAME_OS_INIT, true)) { return; }
				LoadCPUSupport();
			}
			void CLOAK_CALL PrintCPUInformation()
			{
#ifdef _DEBUG
				API::Global::Log::WriteToLog("CPU: " + g_cpuSupport.Vendor + " " + g_cpuSupport.Brand);
#else
				API::Global::Log::WriteToLog("CPU: " + g_cpuSupport.Vendor + " " + g_cpuSupport.Brand, API::Global::Log::Type::File);
#endif
				CloakDebugLog("SSE 1:" + std::to_string(g_cpuSupport.Instructions.SSE.SSE1));
				CloakDebugLog("SSE 2:" + std::to_string(g_cpuSupport.Instructions.SSE.SSE2));
				CloakDebugLog("SSE 3:" + std::to_string(g_cpuSupport.Instructions.SSE.SSE3));
				CloakDebugLog("SSE 4.1:" + std::to_string(g_cpuSupport.Instructions.SSE.SSE4_1));
				CloakDebugLog("SSE 4.2:" + std::to_string(g_cpuSupport.Instructions.SSE.SSE4_2));
				CloakDebugLog("SSE 4.a:" + std::to_string(g_cpuSupport.Instructions.SSE.SSE4_a));
				CloakDebugLog("AVX:" + std::to_string(g_cpuSupport.Instructions.AVX.AVX));
				CloakDebugLog("AVX 2:" + std::to_string(g_cpuSupport.Instructions.AVX.AVX2));
				CloakCheckOK(g_cpuSupport.Instructions.SSE.SSE3, API::Global::Debug::Error::HARDWARE_NO_SSE_SUPPORT, true);
			}
			void CLOAK_CALL Terminate()
			{
				Windows::Foundation::Uninitialize();
				delete[] g_cpuSupport.Topology.Processors;
			}
			size_t CLOAK_CALL GetPhysicalProcessorCount()
			{
				size_t res = g_physProcCount.load();
				if (res == 0) 
				{
					res = GetProcessorCount();
					g_physProcCount = max(1, res);
				}
				return res;
			}
			size_t CLOAK_CALL GetLogicalProcessorCount()
			{
				size_t res = g_logiProcCount.load();
				if (res == 0)
				{
					const size_t shc = std::thread::hardware_concurrency();
					res = static_cast<size_t>(max(shc, g_cpuSupport.Topology.ProcessorCount));
					if (res == 0) { res = GetMaxProcessorSupport(); }
					g_logiProcCount = max(1, res);
				}
				return res;
			}
#endif
		}
	}
}