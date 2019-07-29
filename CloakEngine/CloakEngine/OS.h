#pragma once
#ifndef CE_API_OS_H
#define CE_API_OS_H

#define CE_OS_WINDOWS 0x10000
#define CE_OS_LINUX 0x20000
#define CE_OS_APPLE 0x30000
#define CE_OS_UNIX 0x40000
#define CE_OS_ANDROID 0x50000

#define CE_OS_WINDOWS_10 (CE_OS_WINDOWS | 0x0A00)
#define CE_OS_WINDOWS_8_1 (CE_OS_WINDOWS | 0x0801)
#define CE_OS_WINDOWS_8 (CE_OS_WINDOWS | 0x0800)
#define CE_OS_WINDOWS_7 (CE_OS_WINDOWS | 0x0700)
#define CE_OS_WINDOWS_0 (CE_OS_WINDOWS | 0x0000)

#define CE_OS_LINUX_0 (CE_OS_LINUX | 0x0000)

#define CE_OS_APPLE_0 (CE_OS_APPLE | 0x0000)
#define CE_OS_APPLE_IOS (CE_OS_APPLE | 0x1000)

#define CE_OS_UNIX_0 (CE_OS_UNIX | 0x0000)
#define CE_OS_UNIX_BSD (CE_OS_UNIX | 0x1000)
#define CE_OS_UNIX_BSD_OPEN (CE_OS_UNIX | 0x2000)

#define CE_OS_ANDROID_0 (CE_OS_ANDROID | 0x0000)

//Identify OS
#if defined(_WINDOWS) || defined(_WIN32) || defined(_WIN64)
#	include <Windows.h>
#	define CE_OS_TYPE CE_OS_WINDOWS
#	if WINVER >= _WIN32_WINNT_WIN10
#		define CE_OS_VERSION CE_OS_WINDOWS_10
#	elif WINVER >= _WIN32_WINNT_WINBLUE
#		define CE_OS_VERSION CE_OS_WINDOWS_8_1
#	elif WINVER >=_WIN32_WINNT_WIN8
#		define CE_OS_VERSION CE_OS_WINDOWS_8
#	elif WINVER >= NTDDI_WIN7
#		define CE_OS_VERSION CE_OS_WINDOWS_7
#	endif
#elif defined(__APPLE__) || defined(__MACH__) || defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || defined(macintosh) || defined(Macintosh)
#	include <TargetConditionals.h>
#	define CE_OS_TYPE CE_OS_APPLE
#	if (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE == 1) || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR == 1)
#		define CE_OS_SUBTYPE CE_OS_APPLE_IOS
#	endif
#elif defined(__ANDROID__)
#	define CE_OS_TYPE CE_OS_ANDROID
#elif defined(unix) || defined(__unix__) || defined(__unix)
#	include <sys/param.h>
#	define CE_OS_TYPE CE_OS_UNIX
#elif defined(__linux__) || defined(linux) || defined(__linux)
#	define CE_OS_TYPE CE_OS_LINUX
#else
#	error Target OS is not supported
#endif

//Identify Compiler
//List from https://sourceforge.net/p/predef/wiki/Compilers/
#define CE_COMPILER_ACC 0x000001
#define CE_COMPILER_AltiumMicroBlazeC 0x000002
#define CE_COMPILER_AltiumCtoHardware 0x000003
#define CE_COMPILER_AmsterdamCompilerKit 0x000004
#define CE_COMPILER_ARMCompiler 0x000005
#define CE_COMPILER_AztecC 0x000006
#define CE_COMPILER_BorlandCpp 0x000007
#define CE_COMPILER_CC65 0x000008
#define CE_COMPILER_Clang 0x000009
#define CE_COMPILER_ComeauCpp 0x00000A
#define CE_COMPILER_Compaq 0x00000B
#define CE_COMPILER_ConvexC 0x00000C
#define CE_COMPILER_CompCert 0x00000D
#define CE_COMPILER_CoverityStaticAnalyzer 0x00000E
#define CE_COMPILER_Diab 0x00000F
#define CE_COMPILER_DICEC 0x000010
#define CE_COMPILER_DigitalMars 0x000011
#define CE_COMPILER_DignusSystemsCpp 0x000012
#define CE_COMPILER_DJGPP 0x000013
#define CE_COMPILER_EKOPath 0x000015
#define CE_COMPILER_GCC 0x000016
#define CE_COMPILER_GreenHill 0x000017
#define CE_COMPILER_HPaCpp 0x000018
#define CE_COMPILER_IBMXL 0x000019
#define CE_COMPILER_IBMzOS 0x00001A
#define CE_COMPILER_ImageCraftC 0x00001B
#define CE_COMPILER_Intel 0x00001C
#define CE_COMPILER_KEILCARM 0x00001D
#define CE_COMPILER_KEILC166 0x00001E
#define CE_COMPILER_KEILC51 0x00001F
#define CE_COMPILER_LCC 0x000020
#define CE_COMPILER_LLVM 0x000021
#define CE_COMPILER_MetrowerksCodeWarrior 0x000022
#define CE_COMPILER_MicrosoftVisualCpp 0x000023
#define CE_COMPILER_Microtec 0x000024
#define CE_COMPILER_MicrowayNDPC 0x000025
#define CE_COMPILER_MinGW 0x000026
#define CE_COMPILER_MIPSpro 0x000027
#define CE_COMPILER_MiracleC 0x000028
#define CE_COMPILER_MPWCpp 0x000029
#define CE_COMPILER_NorcroftC 0x00002A
#define CE_COMPILER_NWCC 0x00002B
#define CE_COMPILER_Open64 0x00002C
#define CE_COMPILER_OracleProCPrecompiler 0x00002D
#define CE_COMPILER_OracleSolarisStudio 0x00002E
#define CE_COMPILER_PellesC 0x00002F
#define CE_COMPILER_PortlandGroup 0x000030
#define CE_COMPILER_SASC 0x000031
#define CE_COMPILER_SCOOpenServer 0x000032
#define CE_COMPILER_SmallDeviceCCompiler 0x000033
#define CE_COMPILER_SNCompiler 0x000034
#define CE_COMPILER_StratusVOSC 0x000035
#define CE_COMPILER_TenDRA 0x000036
#define CE_COMPILER_THINKC 0x000037
#define CE_COMPILER_TinyC 0x000038
#define CE_COMPILER_Turbo 0x000039
#define CE_COMPILER_USLC 0x00003A
#define CE_COMPILER_VBCC 0x00003B
#define CE_COMPILER_WatcomCpp 0x00003C
#define CE_COMPILER_ZortechCpp 0x00003D


#if defined(_ACC_)
#	define CE_COMPILER_TYPE CE_COMPILER_ACC
#elif defined(__CMB__)
#	define CE_COMPILER_TYPE CE_COMPILER_AltiumMicroBlazeC
#	define CE_COMPILER_VERSION __VERSION__
#	define CE_COMPILER_REVISION __REVISION__
#	define CE_COMPILER_BUILD __BUILD__
#elif defined(__CHC__)
#	define CE_COMPILER_TYPE CE_COMPILER_AltiumCtoHardware
#	define CE_COMPILER_VERSION __VERSION__
#	define CE_COMPILER_REVISION __REVISION__
#	define CE_COMPILER_BUILD __BUILD__
#elif defined(__ACK__)
#	define CE_COMPILER_TYPE CE_COMPILER_AmsterdamCompilerKit
#elif defined(__CC_ARM)
#	define CE_COMPILER_TYPE CE_COMPILER_ARMCompiler
#	define CE_COMPILER_VERSION __ARMCC_VERSION
#elif defined(AZTEC_C)
#	define CE_COMPILER_TYPE CE_COMPILER_AztecC
#	define CE_COMPILER_VERSION __VERSION
#elif defined(__BORLANDC__) || defined(__CODEGEARC__)
#	define CE_COMPILER_TYPE CE_COMPILER_BorlandCpp
#	if defined(__BORLANDC__)
#		define CE_COMPILER_VERSION __BORLANDC__
#	elif defined(__CODEGEARC__)
#		define CE_COMPILER_VERSION __CODEGEARC__
#	endif
#elif defined(__CC65__)
#	define CE_COMPILER_TYPE CE_COMPILER_CC65
#	define CE_COMPILER_VERSION __CC65__
#elif defined(__clang__)
#	define CE_COMPILER_TYPE CE_COMPILER_Clang
#	define CE_COMPILER_VERSION __clang_major__
#	define CE_COMPILER_REVISION __clang_minor__
#	define CE_COMPILER_BUILD __clang_patchlevel__
#elif defined(__COMO__)
#	define CE_COMPILER_TYPE CE_COMPILER_ComeauCpp
#	define CE_COMPILER_VERSION __COMO_VERSION__
#elif defined(__DECC) || defined(__DECCXX) || defined(__VAXC) || defined(VAXC)
#	define CE_COMPILER_TYPE CE_COMPILER_Compaq
#	if defined(__DECC)
#		define CE_COMPILER_VERSION __DECC_VER
#	elif defined(__DECCXX)
#		define CE_COMPILER_VERSION __DECCXX_VER
#	endif
#elif defined(__convexc__)
#	define CE_COMPILER_TYPE CE_COMPILER_ConvexC
#elif defined(__COMPCERT__)
#	define CE_COMPILER_TYPE CE_COMPILER_CompCert
#elif defined(__COVERITY__) || defined(_CRAYC)
#	define CE_COMPILER_TYPE CE_COMPILER_CoverityStaticAnalyzer
#	define CE_COMPILER_VERSION _RELEASE
#	define CE_COMPILER_REVISION _RELEASE_MINOR
#elif defined(__DCC__)
#	define CE_COMPILER_TYPE CE_COMPILER_Diab
#	define CE_COMPILER_VERSION __VERSION_NUMBER__
#elif defined(_DICE)
#	define CE_COMPILER_TYPE CE_COMPILER_DICEC
#elif defined(__DMC__)
#	define CE_COMPILER_TYPE CE_COMPILER_DigitalMars
#	define CE_COMPILER_VERSION __DMC__
#elif defined(__SYSC__)
#	define CE_COMPILER_TYPE CE_COMPILER_DignusSystemsCpp
#	define CE_COMPILER_VERSION __SYSC_VER__
#elif defined(__DJGPP__) || defined(__GO32__)
#	define CE_COMPILER_TYPE CE_COMPILER_DJGPP
#	define CE_COMPILER_VERSION __DJGPP__
#	define CE_COMPILER_REVISION __DJGPP_MINOR__
#elif defined(__PATHCC__) || defined(__FCC_VERSION)
#	define CE_COMPILER_TYPE CE_COMPILER_EKOPath
#	define CE_COMPILER_VERSION __PATHCC__
#	define CE_COMPILER_REVISION __PATHCC_MINOR__
#	define CE_COMPILER_BUILD __PATHCC_PATCHLEVEL__
#elif defined(__GNUC__)
#	define CE_COMPILER_TYPE CE_COMPILER_GCC
#	define CE_COMPILER_VERSION __GNUC__
#	define CE_COMPILER_REVISION __GNUC_MINOR__
#	define CE_COMPILER_BUILD __GNUC_PATCHLEVEL__
#	define CE_LANG_VERSION (__cplusplus / 100)
#elif defined(__ghs__) || defined(__HP_cc)
#	define CE_COMPILER_TYPE CE_COMPILER_GreenHill
#	define CE_COMPILER_VERSION __GHS_VERSION_NUMBER__
#	define CE_COMPILER_REVISION __GHS_REVISION_DATE__
#elif defined(__HP_aCC) || defined(__IAR_SYSTEMS_ICC__)
#	define CE_COMPILER_TYPE CE_COMPILER_HPaCpp
#	if defined(__HP_aCC)
#		define CE_COMPILER_VERSION __HP_aCC
#	elif defined(__VER__)
#		define CE_COMPILER_VERSION __VER__
#	endif
#elif defined(__xlc__) || defined(__xlC__) || defined(__IBMC__) || defined(__IBMCPP__)
#	define CE_COMPILER_TYPE CE_COMPILER_IBMXL
#	if defined(__xlC__)
#		define CE_COMPILER_VERSION __xlC__
#	elif defined(__xlC_ver__)
#		define CE_COMPILER_VERSION __xlC_ver__
#	elif defined(__IBMC__)
#		define CE_COMPILER_VERSION __IBMC__
#	endif
#elif defined(__IBMC__) || defined(__IBMCPP__)
#	define CE_COMPILER_TYPE CE_COMPILER_IBMzOS
#	if defined(__IBMC__)
#		define CE_COMPILER_VERSION __IBMC__
#	elif defined(__COMPILER_VER__)
#		define CE_COMPILER_VERSION __COMPILER_VER__
#	endif
#elif defined(__IMAGECRAFT__)
#	define CE_COMPILER_TYPE CE_COMPILER_ImageCraftC
#elif defined(__INTEL_COMPILER) || defined(__ICC) || defined(__ECC) || defined(__ICL) || defined(__KCC)
#	define CE_COMPILER_TYPE CE_COMPILER_Intel
#	if defined(__INTEL_COMPILER)
#		define CE_COMPILER_VERSION __INTEL_COMPILER
#		define CE_COMPILER_REVISION __INTEL_COMPILER_BUILD_DATE
#	elif defined(__KCC_VERSION)
#		define CE_COMPILER_VERSION __KCC_VERSION
#	endif
#elif defined(__CA__) || defined(__KEIL__)
#	define CE_COMPILER_TYPE CE_COMPILER_KEILCARM
#	define CE_COMPILER_VERSION __CA__
#elif defined(__C166__)
#	define CE_COMPILER_TYPE CE_COMPILER_KEILC166
#	define CE_COMPILER_VERSION __C166__
#elif defined(__C51__) || defined(__CX51__)
#	define CE_COMPILER_TYPE CE_COMPILER_KEILC51
#	define CE_COMPILER_VERSION __C51__
#elif defined(__LCC__)
#	define CE_COMPILER_TYPE CE_COMPILER_LCC
#elif defined(__llvm__) || defined(__HIGHC__)
#	define CE_COMPILER_TYPE CE_COMPILER_LLVM
#elif defined(__MWERKS__) || defined(__CWCC__)
#	define CE_COMPILER_TYPE CE_COMPILER_MetrowerksCodeWarrior
#	if defined(__MWERKS__)
#		define CE_COMPILER_VERSION __MWERKS__
#	elif defined(__CWCC__)
#		define CE_COMPILER_VERSION __CWCC__
#	endif
#elif defined(_MSC_VER)
#	define CE_COMPILER_TYPE CE_COMPILER_MicrosoftVisualCpp
#	define CE_COMPILER_VERSION _MSC_VER
#	define CE_COMPILER_REVISION _MSC_BUILD
#	define CE_LANG_VERSION (_MSVC_LANG / 100)
#elif defined(_MRI)
#	define CE_COMPILER_TYPE CE_COMPILER_Microtec
#elif defined(__NDPC__)
#	define CE_COMPILER_TYPE CE_COMPILER_MicrowayNDPC
#elif defined(__MINGW32__) || defined(__MINGW64__)
#	define CE_COMPILER_TYPE CE_COMPILER_MinGW
#	if defined(__MINGW32__)
#		define CE_COMPILER_VERSION __MINGW32_MAJOR_VERSION
#		define CE_COMPILER_REVISION __MINGW32_MINOR_VERSION
#	elif defined(__MINGW64__)
#		define CE_COMPILER_VERSION __MINGW64_VERSION_MAJOR
#		define CE_COMPILER_REVISION __MINGW64_VERSION_MINOR
#	endif
#elif defined(__sgi) || defined(sgi)
#	define CE_COMPILER_TYPE CE_COMPILER_MIPSpro
#	if defined(_COMPILER_VERSION)
#		define CE_COMPILER_VERSION _COMPILER_VERSION
#	elif defined(_SGI_COMPILER_VERSION)
#		define CE_COMPILER_VERSION _SGI_COMPILER_VERSION
#	endif
#elif defined(MIRACLE)
#	define CE_COMPILER_TYPE CE_COMPILER_MiracleC
#elif defined(__MRC__) || defined(MPW_C) || defined(MPW_CPLUS)
#	define CE_COMPILER_TYPE CE_COMPILER_MPWCpp
#	define CE_COMPILER_VERSION __MRC__
#elif defined(__CC_NORCROFT)
#	define CE_COMPILER_TYPE CE_COMPILER_NorcroftC
#	define CE_COMPILER_VERSION __ARMCC_VERSION
#elif defined(__NWCC__)
#	define CE_COMPILER_TYPE CE_COMPILER_NWCC
#elif defined(__OPEN64__) || defined(__OPENCC__)
#	define CE_COMPILER_TYPE CE_COMPILER_Open64
#	define CE_COMPILER_VERSION __OPENCC__
#	define CE_COMPILER_REVISION __OPENCC_MINOR__
#	define CE_COMPILER_BUILD __OPENCC_PATCHLEVEL__
#elif defined(ORA_PROC)
#	define CE_COMPILER_TYPE CE_COMPILER_OracleProCPrecompiler
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC) || defined(__PACIFIC__) || defined(_PACC_VER)
#	define CE_COMPILER_TYPE CE_COMPILER_OracleSolarisStudio
#	if defined(__SUNPRO_C)
#		define CE_COMPILER_VERSION __SUNPRO_C
#	elif defined(__SUNPRO_CC)
#		define CE_COMPILER_VERSION __SUNPRO_CC
#	elif defined(_PACC_VER)
#		define CE_COMPILER_VERSION _PACC_VER
#	endif
#elif defined(__POCC__)
#	define CE_COMPILER_TYPE CE_COMPILER_PellesC
#	define CE_COMPILER_VERSION __POCC__
#elif defined(__PGI) || defined(__RENESAS__) || defined(__HITACHI__)
#	define CE_COMPILER_TYPE CE_COMPILER_PortlandGroup
#	if defined(__PGI)
#		define CE_COMPILER_VERSION __PGIC__
#		define CE_COMPILER_REVISION __PGIC_MINOR__
#		define CE_COMPILER_BUILD __PGIC_PATCHLEVEL__
#	elif defined(__RENESAS__)
#		define CE_COMPILER_VERSION __RENESAS_VERSION__
#	elif defined(__HITACHI__)
#		define CE_COMPILER_VERSION __HITACHI_VERSION__
#	endif
#elif defined(SASC) || defined(__SASC) || defined(__SASC__)
#	define CE_COMPILER_TYPE CE_COMPILER_SASC
#	if defined(__VERSION__)
#		define CE_COMPILER_VERSION __VERSION__
#		define CE_COMPILER_REVISION __REVISION__
#	elif defined(__SASC__)
#		define CE_COMPILER_VERSION __SASC__
#	endif
#elif defined(_SCO_DS)
#	define CE_COMPILER_TYPE CE_COMPILER_SCOOpenServer
#elif defined(SDCC)
#	define CE_COMPILER_TYPE CE_COMPILER_SmallDeviceCCompiler
#	define CE_COMPILER_VERSION SDCC
#elif defined(__SNC__)
#	define CE_COMPILER_TYPE CE_COMPILER_SNCompiler
#elif defined(__VOSC__) || defined(__SC__)
#	define CE_COMPILER_TYPE CE_COMPILER_StratusVOSC
#	if defined(__VOSC__)
#		define CE_COMPILER_VERSION __VOSC__
#	elif defined(__SC__)
#		define CE_COMPILER_VERSION __SC__
#	endif
#elif defined(__TenDRA__) || defined(__TI_COMPILER_VERSION__) || defined(_TMS320C6X)
#	define CE_COMPILER_TYPE CE_COMPILER_TenDRA
#	define CE_COMPILER_VERSION __TI_COMPILER_VERSION__
#elif defined(THINKC3) || defined(THINKC4)
#	define CE_COMPILER_TYPE CE_COMPILER_THINKC
#	if defined(THINKC3)
#		define CE_COMPILER_VERSION THINKC3
#	elif defined(THINKC4)
#		define CE_COMPILER_VERSION THINKC4
#	endif
#elif defined(__TINYC__)
#	define CE_COMPILER_TYPE CE_COMPILER_TinyC
#elif defined(__TURBOC__) || defined(_UCC)
#	define CE_COMPILER_TYPE CE_COMPILER_Turbo
#	define CE_COMPILER_VERSION __TURBOC__
#	define CE_COMPILER_REVISION _MAJOR_REV
#elif defined(__USLC__)
#	define CE_COMPILER_TYPE CE_COMPILER_USLC
#	define CE_COMPILER_VERSION __SCO_VERSION__
#elif defined(__VBCC__)
#	define CE_COMPILER_TYPE CE_COMPILER_VBCC
#elif defined(__WATCOMC__)
#	define CE_COMPILER_TYPE CE_COMPILER_WatcomCpp
#	define CE_COMPILER_VERSION __WATCOMC__
#elif defined(__ZTC__)
#	define CE_COMPILER_TYPE CE_COMPILER_ZortechCpp
#	define CE_COMPILER_VERSION __ZTC__
#else
#	error Failed to identify compiler!
#endif

#ifndef CE_COMPILER_VERSION
#	define CE_COMPILER_VERSION 0
#endif
#ifndef CE_COMPILER_REVISION
#	define CE_COMPILER_REVISION 0
#endif
#ifndef CE_COMPILER_BUILD
#	define CE_COMPILER_BUILD 0
#endif

//Identify Architecture
#define CLOAK_ARX_ALPHA 0x01000
#define CLOAK_ARX_ARM 0x02000
#define CLOAK_ARX_BLACKFIN 0x03000
#define CLOAK_ARX_CONVEX 0x04000
#define CLOAK_ARX_IA64 0x05000
#define CLOAK_ARX_M68K 0x06000
#define CLOAK_ARX_MIPS 0x07000
#define CLOAK_ARX_PARISC 0x08000
#define CLOAK_ARX_POWERPC 0x09000
#define CLOAK_ARX_PYRAMID 0x0a000
#define CLOAK_ARX_RS6000 0x0b000
#define CLOAK_ARX_SPARC 0x0c000
#define CLOAK_ARX_SUPERH 0x0d000
#define CLOAK_ARX_SYS370 0x0e000
#define CLOAK_ARX_SYS390 0x0f000
#define CLOAK_ARX_x86 0x10000
#define CLOAK_ARX_Z 0x20000
#define CLOAK_ARX_AMD64 0x30000

#if defined(__alpha__) || defined(__alpha) || defined(_M_ALPHA)
#	define CLOAK_ARX CLOAK_ARX_ALPHA
#elif defined(__arm__) || defined(__arm64) || defined(__thumb__) || defined(__TARGET_ARCH_ARM) || defined(__TARGET_ARCH_THUMB) || defined(_M_ARM)
#	define CLOAK_ARX CLOAK_ARX_ARM
#	ifdef __arm64
#		define X64 1
#	endif
#elif defined(__bfin__) || defined(__BFIN__) || defined(bfin) || defined(BFIN)
#	define CLOAK_ARX CLOAK_ARX_BLACKFIN
#elif defined(__convex__)
#	define CLOAK_ARX CLOAK_ARX_CONVEX
#elif defined(__ia64__) || defined(_IA64) || defined(__IA64__) || defined(__ia64) || defined(_M_IA64) || defined(__itanium__)
#	define X64 1
#	define CLOAK_ARX CLOAK_ARX_IA64
#elif defined(__m68k__) || defined(M68000)
#	define CLOAK_ARX CLOAK_ARX_M68K
#elif defined(__mips__) || defined(__mips) || defined(__MIPS__)
#	define CLOAK_ARX CLOAK_ARX_MIPS
#elif defined(__hppa__) || defined(__hppa) || defined(__HPPA__)
#	define CLOAK_ARX CLOAK_ARX_PARISC
#elif defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__) || defined(__ppc__) || defined(_M_PPC) || defined(_ARCH_PPC) || defined(__PPCGECKO__) || defined(__PPCBROADWAY__) || defined(_XENON)
#	define CLOAK_ARX CLOAK_ARX_POWERPC
#elif defined(pyr)
#	define CLOAK_ARX CLOAK_ARX_PYRAMID
#elif defined(__THW_RS6000) || defined(_IBMR2) || defined(_POWER) || defined(_ARCH_PWR) ||defined(_ARCH_PWR2)
#	define CLOAK_ARX CLOAK_ARX_RS6000
#elif defined(__sparc__) || defined(__sparc)
#	define CLOAK_ARX CLOAK_ARX_SPARC
#elif defined(__sh__)
#	define CLOAK_ARX CLOAK_ARX_SUPERH
#elif defined(__370__) || defined(__THW_370__)
#	define CLOAK_ARX CLOAK_ARX_SYS370
#elif defined(__s390__) || defined(__s390x__)
#	define CLOAK_ARX CLOAK_ARX_SYS390
#elif defined(i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__i386) || defined(_M_IX86) || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || defined(__INTEL__)
#	define CLOAK_ARX CLOAK_ARX_x86
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
#	define CLOAK_ARX CLOAK_ARX_x86
#	define X64 1
#elif defined(__SYSC_ZARCH__)
#	define CLOAK_ARX CLOAK_ARX_Z
#else
#	error Failed to identify architecture!
#endif

#ifndef CE_OS_VERSION
#	define CE_OS_VERSION 0
#endif
#ifndef CE_OS_SUBTYPE
#	define CE_OS_SUBTYPE 0
#endif
#if !defined(X64) && !defined(X32)
#	define X32 1
#endif

#define CHECK_OS(type,version) (CE_OS_TYPE==CE_OS_##type && CE_OS_VERSION>=CE_OS_##type##_##version)
#define CHECK_OS_SUBTYPE(type, sub) (CE_OS_TYPE==CE_OS_##type && CE_OS_SUBTYPE==CE_OS_##type##_##sub)
#define CHECK_ARCHITECTURE(arx) (CLOAK_ARX == CLOAK_ARX_##arx)
#define CHECK_COMPILER(type) (CE_COMPILER_TYPE == CE_COMPILER_##type)
#define CHECK_COMPILER_VERSION(type, major, minor, build) (CHECK_COMPILER(type) && (CE_COMPILER_VERSION > major || (CE_COMPILER_VERSION == major && (CE_COMPILER_REVISION > minor || (CE_COMPILER_REVISION == minor && CE_COMPILER_BUILD >= build)))))

#ifndef CE_LANG_VERSION
#	define CE_LANG_VERSION 1997L
#	if CECK_COMPILER(MicrosoftVisualCpp)
#		pragma message("Warning: C++ Version macro could not be found")
#	else
#		warning "Warning: C++ Version macro could not be found"
#	endif
#endif

#endif