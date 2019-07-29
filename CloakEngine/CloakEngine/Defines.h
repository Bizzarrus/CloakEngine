#pragma once
#ifndef CE_API_DEFINES_H
#define CE_API_DEFINES_H

	#ifndef __cplusplus
		#error A C++ compiler is required!
	#endif

	#include "CloakEngine/OS.h"
	#include "CloakEngine/Helper/Types.h"

	#define CLOAKENGINE_VERSION 1000

	#if defined(__GLIBC__) || defined(__GNU_LIBRARY__)
		#define CLOAK_GNU_LIB_C
	#endif

	#if CHECK_COMPILER(Clang)
		#define CLOAK_UUID(id) __declspec(uuid(id))
		#define CLOAK_INTERFACE_BASIC class
		#define CLOAK_DEPRECATED 
		#define CLOAK_ALIGN
		#define CLOAK_FORCEINLINE inline
		#define CLOAK_CALL 
		#define CLOAK_CALL_THIS 
		#define CLOAK_CALL_FAST 
		#define CLOAK_CALL_MATH 
	#elif CHECK_COMPILER(GCC)
		#define CLOAK_UUID(id)
		#define CLOAK_INTERFACE_BASIC class
		#define CLOAK_DEPRECATED 
		#define CLOAK_ALIGN
		#define CLOAK_FORCEINLINE inline
		#define CLOAK_CALL 
		#define CLOAK_CALL_THIS 
		#define CLOAK_CALL_FAST 
		#define CLOAK_CALL_MATH 
	#elif CHECK_COMPILER(MicrosoftVisualCpp)
		#define CLOAK_UUID(id) __declspec(uuid(id))
		#define CLOAK_INTERFACE_BASIC class __declspec(novtable)
		#define CLOAK_DEPRECATED __declspec(deprecated)
		#define CLOAK_ALIGN alignas(16)
		#define CLOAK_FORCEINLINE __forceinline
		#ifdef X64
			#define CLOAK_CALL __fastcall
			#define CLOAK_CALL_THIS CLOAK_CALL
			#define CLOAK_CALL_FAST CLOAK_CALL
			#define CLOAK_CALL_MATH __vectorcall
		#else
			#define CLOAK_CALL __stdcall
			#define CLOAK_CALL_THIS __thiscall
			#define CLOAK_CALL_FAST __fastcall
			#define CLOAK_CALL_MATH __vectorcall
		#endif
	#else
		#define CLOAK_UUID(id)
		#define CLOAK_INTERFACE_BASIC class
		#define CLOAK_DEPRECATED 
		#define CLOAK_ALIGN
		#define CLOAK_FORCEINLINE inline
		#define CLOAK_CALL 
		#define CLOAK_CALL_THIS 
		#define CLOAK_CALL_FAST 
		#define CLOAK_CALL_MATH 
	#endif

	#ifdef _DEBUG
		#include <cassert>
		#if CHECK_COMPILER(MicrosoftVisualCpp)
			#define CLOAK_ASSUME(condition) assert(condition)
		#else
			#define CLOAK_ASSUME(condition) assert(condition)
		#endif
	#else
		#if CHECK_COMPILER(MicrosoftVisualCpp)
			#define CLOAK_ASSUME(condition) __assume(condition)
		#elif CHECK_COMPILER(GCC)
			#define CLOAK_ASSUME(condition) if(!(condition)){__builtin_unreachable();}
		#else
			#define CLOAK_ASSUME(condition) 
		#endif
	#endif

	#ifdef CLOAKENGINE_EXPORTS

		#define CLOAKENGINE_API_NAMESPACE

		#if CHECK_COMPILER(MicrosoftVisualCpp)
			#define CLOAKENGINE_API __declspec(dllexport)
		#else
			#define CLOAKENGINE_API
		#endif

	#else

		#define CLOAKENGINE_API_NAMESPACE inline

		#if CHECK_COMPILER(MicrosoftVisualCpp)
			#define CLOAKENGINE_API __declspec(dllimport)
		#else
			#define CLOAKENGINE_API
		#endif

	#endif

	#define CLOAK_INTERFACE CLOAK_INTERFACE_BASIC CLOAKENGINE_API
	#define CLOAK_INTERFACE_ID(id) CLOAK_INTERFACE CLOAK_UUID(id)
	#define RELEASE(p) {if((p)!=nullptr){(p)->Release(); p=nullptr;}}
	#define SWAP(p,n) {if((p)!=nullptr){(p)->Release();} (p)=(n); if((p)!=nullptr){(p)->AddRef();}}
	#define CE_QUERY_ARGS(ptr) __uuidof(**(ptr)), reinterpret_cast<void**>(ptr)

	#ifndef KB
		#define KB *(1<<10)
	#endif
	#ifndef MB
		#define MB *(1<<20)
	#endif
	#ifndef GB
		#define GB *(1<<30)
	#endif

	#ifndef CEIL_DIV
		#define CEIL_DIV(a,b) (((a)+(b)-1)/(b))
	#endif

	#define In
	#define Out
	#define Inout
	#define Inout_updates(size)
	#define In_opt
	#define Inptr
	#define Inptr_opt
	#define Inref
	#define Inref_opt
	#define Out_opt
	#define Outptr
	#define Outptr_opt
	#define Outref
	#define Outref_opt
	#define Inout_opt
	#define In_reads(size) In
	#define In_reads_opt(size) In_opt
	#define In_reads_bytes(size) In
	#define In_reads_bytes_opt(size) In_opt
	#define Out_writes(size) Out
	#define Out_writes_opt(size) Out_opt
	#define Out_writes_bytes(size) Out
	#define Out_writes_bytes_opt(size) Out_opt
	#define Out_writes_to(size, count) Out
	#define Out_writes_bytes_to(size, count) Out
	#define Success(expr)
	#define Success_Type(expr)
	#define Use_annotations 
	#define Ret_notnull
	#define Ret_maybenull
	#define In_range(min,max)
	#define Post_satisfies(expr)
	#define When(expr, sal)
	#define Field_size(len)
	#define Filed_size_bytes(len)

	//Should only be called once per program, to enable NVidia Optimus (Laptop integrated GPU)
	#define ENABLE_NV_OPTIMUS() extern "C" {_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;}

	#if defined(CLOAK_GNU_LIB_C) || CHECK_OS(ANDROID,0)
		#include <endian.h>
	#elif CHECK_OS(APPLE,0) || CHECK_OS_SUBTYPE(UNIX,BSD_OPEN)
		#include <machine/endian.h>
	#elif CHECK_OS_SUBTYPE(UNIX,BSD)
		#include <sys/endian.h>
	#endif

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			typedef unsigned long long Time;
			extern const uint32_t CLOAKENGINE_API VERSION;
			CLOAK_FORCEINLINE bool CLOAK_CALL CheckVersion() { return VERSION == CLOAKENGINE_VERSION; }

			enum class Endian {
				Unknown = 0,
				Big = 4321,
				Little = 1234,
				PDP = 2134,

#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && __BYTE_ORDER == __BIG_ENDIAN
				Native = Big,
#elif defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && __BYTE_ORDER == __LITTLE_ENDIAN
				Native = Little,
#elif defined(__BYTE_ORDER) && defined(__PDP_ENDIAN) && __BYTE_ORDER == __PDP_ENDIAN
				Native = PDP,
#elif defined(_BYTE_ORDER) && defined(_BIG_ENDIAN) && _BYTE_ORDER == _BIG_ENDIAN
				Native = Big,
#elif defined(_BYTE_ORDER) && defined(_LITTLE_ENDIAN) && _BYTE_ORDER == _LITTLE_ENDIAN
				Native = Little,
#elif defined(_BYTE_ORDER) && defined(_PDP_ENDIAN) && _BYTE_ORDER == _PDP_ENDIAN
				Native = PDP,
#elif (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) || (defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)) || defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)
				Native = Big,
#elif (!defined(__BIG_ENDIAN__) && defined(__LITTLE_ENDIAN__)) || (!defined(_BIG_ENDIAN) && defined(_LITTLE_ENDIAN)) || defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
				Native = Little,
#elif CHECK_ARCHITECTURE(M68K) || CHECK_ARCHITECTURE(PARISC) || CHECK_ARCHITECTURE(SPARC) || CHECK_ARCHITECTURE(SYS370) || CHECK_ARCHITECTURE(SYS390) || CHECK_ARCHITECTURE(Z)
				Native = Big,
#elif CHECK_ARCHITECTURE(M68K) || CHECK_ARCHITECTURE(AMD64) || CHECK_ARCHITECTURE(IA64) || CHECK_ARCHITECTURE(X86) || CHECK_ARCHITECTURE(BLACKFIN)
				Native = Little,
#elif CHECK_OS(WINDOWS,0)
				Native = Little,
#else
				Native = Unknown,
#	if CHECK_COMPILER(MicrosoftVisualCpp)
#		pragma message("Warning : Failed to identifiy endianess")
#	else
#		warning("Warning : Failed to identifiy endianess")
#	endif
#endif
			};
		}
	}
}
//Convinient namespace alias
namespace CE = CloakEngine::API;

//User defined literals:
#ifndef CLOAKENGINE_NO_LITERALS

#pragma warning(push)
#pragma warning(disable: 4455)

constexpr unsigned long long int operator ""KB(unsigned long long int i) { return i KB; }
constexpr unsigned long long int operator ""MB(unsigned long long int i) { return i MB; }
constexpr unsigned long long int operator ""GB(unsigned long long int i) { return i GB; }

#pragma warning(pop)

#endif

//Recursive Macro Helper:

#define _EXPAND(X) X
#define _FIRSTARG(X,...) (X)
#define _RESTARGS(X,...) (__VA_ARGS__)

#if CHECK_COMPILER(MicrosoftVisualCpp)

#define _NUM_ARGS_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, N, ...) N
#define _NUM_ARGS_C(...) _EXPAND(_NUM_ARGS_N(__VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define _NUM_ARGS_A(...) unused, __VA_ARGS__
#define NUM_ARGS(...) _NUM_ARGS_C(_NUM_ARGS_A(__VA_ARGS__))

#else

#define _NUM_ARGS_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, N, ...) N
#define NUM_ARGS(...) _NUM_ARGS_N(0, ## __VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#endif

#define _FOREACH_0(M,L)
#define _FOREACH_1(M,L) M L
#define _FOREACH_2(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_1(M, _RESTARGS L)
#define _FOREACH_3(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_2(M, _RESTARGS L)
#define _FOREACH_4(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_3(M, _RESTARGS L)
#define _FOREACH_5(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_4(M, _RESTARGS L)
#define _FOREACH_6(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_5(M, _RESTARGS L)
#define _FOREACH_7(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_6(M, _RESTARGS L)
#define _FOREACH_8(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_7(M, _RESTARGS L)
#define _FOREACH_9(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_8(M, _RESTARGS L)
#define _FOREACH_10(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_9(M, _RESTARGS L)
#define _FOREACH_11(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_10(M, _RESTARGS L)
#define _FOREACH_12(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_11(M, _RESTARGS L)
#define _FOREACH_13(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_12(M, _RESTARGS L)
#define _FOREACH_14(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_13(M, _RESTARGS L)
#define _FOREACH_15(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_14(M, _RESTARGS L)
#define _FOREACH_16(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_15(M, _RESTARGS L)
#define _FOREACH_17(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_16(M, _RESTARGS L)
#define _FOREACH_18(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_17(M, _RESTARGS L)
#define _FOREACH_19(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_18(M, _RESTARGS L)
#define _FOREACH_20(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_19(M, _RESTARGS L)
#define _FOREACH_21(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_20(M, _RESTARGS L)
#define _FOREACH_22(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_21(M, _RESTARGS L)
#define _FOREACH_23(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_22(M, _RESTARGS L)
#define _FOREACH_24(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_23(M, _RESTARGS L)
#define _FOREACH_25(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_24(M, _RESTARGS L)
#define _FOREACH_26(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_25(M, _RESTARGS L)
#define _FOREACH_27(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_26(M, _RESTARGS L)
#define _FOREACH_28(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_27(M, _RESTARGS L)
#define _FOREACH_29(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_28(M, _RESTARGS L)
#define _FOREACH_30(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_29(M, _RESTARGS L)
#define _FOREACH_31(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_30(M, _RESTARGS L)
#define _FOREACH_32(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_31(M, _RESTARGS L)
#define _FOREACH_33(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_32(M, _RESTARGS L)
#define _FOREACH_34(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_33(M, _RESTARGS L)
#define _FOREACH_35(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_34(M, _RESTARGS L)
#define _FOREACH_36(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_35(M, _RESTARGS L)
#define _FOREACH_37(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_36(M, _RESTARGS L)
#define _FOREACH_38(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_37(M, _RESTARGS L)
#define _FOREACH_39(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_38(M, _RESTARGS L)
#define _FOREACH_40(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_39(M, _RESTARGS L)
#define _FOREACH_41(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_40(M, _RESTARGS L)
#define _FOREACH_42(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_41(M, _RESTARGS L)
#define _FOREACH_43(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_42(M, _RESTARGS L)
#define _FOREACH_44(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_43(M, _RESTARGS L)
#define _FOREACH_45(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_44(M, _RESTARGS L)
#define _FOREACH_46(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_45(M, _RESTARGS L)
#define _FOREACH_47(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_46(M, _RESTARGS L)
#define _FOREACH_48(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_47(M, _RESTARGS L)
#define _FOREACH_49(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_48(M, _RESTARGS L)
#define _FOREACH_50(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_49(M, _RESTARGS L)
#define _FOREACH_51(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_50(M, _RESTARGS L)
#define _FOREACH_52(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_51(M, _RESTARGS L)
#define _FOREACH_53(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_52(M, _RESTARGS L)
#define _FOREACH_54(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_53(M, _RESTARGS L)
#define _FOREACH_55(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_54(M, _RESTARGS L)
#define _FOREACH_56(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_55(M, _RESTARGS L)
#define _FOREACH_57(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_56(M, _RESTARGS L)
#define _FOREACH_58(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_57(M, _RESTARGS L)
#define _FOREACH_59(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_58(M, _RESTARGS L)
#define _FOREACH_60(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_59(M, _RESTARGS L)
#define _FOREACH_61(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_60(M, _RESTARGS L)
#define _FOREACH_62(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_61(M, _RESTARGS L)
#define _FOREACH_63(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_62(M, _RESTARGS L)
#define _FOREACH_64(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_63(M, _RESTARGS L)
#define _FOREACH_65(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_64(M, _RESTARGS L)
#define _FOREACH_66(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_65(M, _RESTARGS L)
#define _FOREACH_67(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_66(M, _RESTARGS L)
#define _FOREACH_68(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_67(M, _RESTARGS L)
#define _FOREACH_69(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_68(M, _RESTARGS L)
#define _FOREACH_70(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_69(M, _RESTARGS L)
#define _FOREACH_71(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_70(M, _RESTARGS L)
#define _FOREACH_72(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_71(M, _RESTARGS L)
#define _FOREACH_73(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_72(M, _RESTARGS L)
#define _FOREACH_74(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_73(M, _RESTARGS L)
#define _FOREACH_75(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_74(M, _RESTARGS L)
#define _FOREACH_76(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_75(M, _RESTARGS L)
#define _FOREACH_77(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_76(M, _RESTARGS L)
#define _FOREACH_78(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_77(M, _RESTARGS L)
#define _FOREACH_79(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_78(M, _RESTARGS L)
#define _FOREACH_80(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_79(M, _RESTARGS L)
#define _FOREACH_81(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_80(M, _RESTARGS L)
#define _FOREACH_82(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_81(M, _RESTARGS L)
#define _FOREACH_83(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_82(M, _RESTARGS L)
#define _FOREACH_84(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_83(M, _RESTARGS L)
#define _FOREACH_85(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_84(M, _RESTARGS L)
#define _FOREACH_86(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_85(M, _RESTARGS L)
#define _FOREACH_87(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_86(M, _RESTARGS L)
#define _FOREACH_88(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_87(M, _RESTARGS L)
#define _FOREACH_89(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_88(M, _RESTARGS L)
#define _FOREACH_90(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_89(M, _RESTARGS L)
#define _FOREACH_91(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_90(M, _RESTARGS L)
#define _FOREACH_92(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_91(M, _RESTARGS L)
#define _FOREACH_93(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_92(M, _RESTARGS L)
#define _FOREACH_94(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_93(M, _RESTARGS L)
#define _FOREACH_95(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_94(M, _RESTARGS L)
#define _FOREACH_96(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_95(M, _RESTARGS L)
#define _FOREACH_97(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_96(M, _RESTARGS L)
#define _FOREACH_98(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_97(M, _RESTARGS L)
#define _FOREACH_99(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_98(M, _RESTARGS L)
#define _FOREACH_100(M,L) _EXPAND(M _FIRSTARG L) _FOREACH_99(M, _RESTARGS L)

#define __FOREACH(N,C,L) _FOREACH_##N(C,L)
#define _FOREACH(N,C,L) __FOREACH(N,C,L)
#define FOREACH(command, list) _FOREACH(NUM_ARGS list,command,list)

#ifdef __has_builtin
#	define CE_HAS_BUILTIN(x) __has_builtin(x)
#else
#	define CE_HAS_BUILTIN(x) 0
#endif
#ifdef __has_attribute
#	define CE_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#	define CE_HAS_ATTRIBUTE(x) 0
#endif

#endif


