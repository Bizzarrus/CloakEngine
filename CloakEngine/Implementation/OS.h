#pragma once
#ifndef CE_IMPL_OS_H
#define CE_IMPL_OS_H

#include "CloakEngine/OS.h"
#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/Types.h"

#include <string>

#ifdef _WINDOWS
#pragma comment(lib,"runtimeobject")
#endif

#if CE_LANG_VERSION > 2017L
#define __CE_BITFIELD(type, name, defValue, bitField) type name : bitField = defValue
#else
#define __CE_BITFIELD(type, name, defValue, bitField) type name = defValue
#endif

namespace CloakEngine {
	namespace Impl {
		namespace OS {
			constexpr uint32_t MAX_CACHE_LEVEL = 3;
			constexpr uint32_t MAX_CACHE_TYPES = 3;
			constexpr uint32_t MAX_CACHE_COUNT = MAX_CACHE_LEVEL * MAX_CACHE_TYPES;
			enum class CacheType { Data = 0, Instruction = 1, Unified = 2 };
			enum class CacheLevel { L1 = 0, L2 = 1, L3 = 2 };
			constexpr CLOAK_FORCEINLINE uint32_t CLOAK_CALL GetCacheID(In CacheType t, In CacheLevel l) { return (static_cast<uint32_t>(l) * MAX_CACHE_TYPES) + static_cast<uint32_t>(t); }

			struct PROCESSOR_DATA {
				uint32_t APICID = 0; // APIC ID (Mask) set by hardware
				uint32_t PackageID = 0; // Zero based Package Index
				uint32_t CoreID = 0; // Zero based Core Index within the same Package
				uint32_t ThreadID = 0; // Zero based Thread Index within the same Core
				uint32_t CacheID[MAX_CACHE_COUNT] = { 0 }; // Zero based cache identifier for specified cache connected with this logical processor
			};
			struct CACHE_DATA {
				uint32_t FullSize = 0;
				uint32_t LineSize = 0;
				uint32_t Mask = 0;
			};
			struct CPU_INFORMATION {
				std::string Vendor = "";
				std::string Brand = "";
				struct {
					struct {
						__CE_BITFIELD(bool, MMX1 , false, 1);
						__CE_BITFIELD(bool, MMX2 , false, 1);
					} MMX;
					struct {
						__CE_BITFIELD(bool, SSE1 , false, 1);
						__CE_BITFIELD(bool, SSE2 , false, 1);
						__CE_BITFIELD(bool, SSE3 , false, 1);
						__CE_BITFIELD(bool, SSE4_1 , false, 1);
						__CE_BITFIELD(bool, SSE4_2 , false, 1);
						__CE_BITFIELD(bool, SSSE3 , false, 1);
						__CE_BITFIELD(bool, SSE4_a , false, 1);
					} SSE;
					struct {
						__CE_BITFIELD(bool, AVX , false, 1);
						__CE_BITFIELD(bool, AVX2 , false, 1);
						struct {
							__CE_BITFIELD(bool, Enabled , false, 1);
							__CE_BITFIELD(bool, DoubleQuadWords , false, 1);
							__CE_BITFIELD(bool, IntegerMAD , false, 1);
							__CE_BITFIELD(bool, FloatMAD , false, 1);
							__CE_BITFIELD(bool, Prefetch , false, 1);
							__CE_BITFIELD(bool, ExponentialReciprocal , false, 1);
							__CE_BITFIELD(bool, ConflictDetection , false, 1);
							__CE_BITFIELD(bool, ByteWord , false, 1);
							__CE_BITFIELD(bool, VectorLength , false, 1);
							__CE_BITFIELD(bool, VectorBitManipulation , false, 1);
							__CE_BITFIELD(bool, VectorBitManipulation2 , false, 1);
							__CE_BITFIELD(bool, BITALG , false, 1);
							__CE_BITFIELD(bool, VectorPopulationCoundDoubleQuadWords , false, 1);
							struct {
								__CE_BITFIELD(bool, Register4 , false, 1);
								__CE_BITFIELD(bool, Vector , false, 1);
							} NeuralNetwork;
						} AVX512;
					} AVX;
					struct {
						__CE_BITFIELD(bool, Enabled , false, 1);
						__CE_BITFIELD(bool, Extension , false, 1);
					} AMD3DNow;
					struct {
						__CE_BITFIELD(bool, SHA , false, 1);
						__CE_BITFIELD(bool, AES , false, 1);
					} Other;
				} Instructions;
				struct {
					uint32_t ProcessorCount = 0;
					PROCESSOR_DATA* Processors = nullptr;
					CACHE_DATA Cache[MAX_CACHE_COUNT]; //Ordered in (Level * MAX_CACHE_TYPES) + Type
				} Topology;
				CLOAK_FORCEINLINE const CACHE_DATA& CLOAK_CALL GetCache(In CacheType type, In CacheLevel level) const
				{
					const size_t p = GetCacheID(type, level);
					if (Topology.Cache[p].FullSize > 0) { return Topology.Cache[p]; }
					return Topology.Cache[GetCacheID(CacheType::Unified, level)];
				}
			};

			void CLOAK_CALL Initialize();
			void CLOAK_CALL PrintCPUInformation();
			void CLOAK_CALL Terminate();
			size_t CLOAK_CALL GetPhysicalProcessorCount();
			size_t CLOAK_CALL GetLogicalProcessorCount();
			const CPU_INFORMATION& GetCPUInformation();
		}
	}
}

#undef  __CE_BITFIELD

#endif