#pragma once
#ifndef CE_API_GLOBAL_DEBUG_H
#define CE_API_GLOBAL_DEBUG_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Log.h"
#include "CloakEngine/Global/Memory.h"
#include "CloakEngine/Helper/StringConvert.h"

#include <string>
#include <functional>
#include <Windows.h>
#include <comdef.h>

#define CloakCheckOK(toCheck,toThrow,critical,...) CloakEngine::API::Global::Debug::CheckError<critical,decltype(toCheck)>::Check(toCheck,toThrow,__FILE__,__LINE__,__VA_ARGS__)
#define CloakCheckError(toCheck,toThrow,critical,...) (!CloakCheckOK(toCheck,toThrow,critical,__VA_ARGS__))

#define CloakError(toThrow,critical,...) CloakCheckOK(false,toThrow,critical,__VA_ARGS__)

#ifdef _DEBUG
	#define CloakDebugCheckOK(toCheck,toThrow,critical,...) CloakCheckOK(toCheck,toThrow,critical,__VA_ARGS__)
	
	#define CloakDebugStringStart(msg) std::string __dbgStr=(msg)
	#define CloakDebugStringAttach(msg) __dbgStr+=(msg)
	#define CloakDebugStringPrint() CloakDebugLog(__dbgStr); __dbgStr=""

	#define CloakCreateTimer(name) CloakEngine::API::Global::Debug::PerformanceTimer name
	#define CloakStartTimer(name) name.Start()
	#define CloakStopTimer(name) name.Stop()
	#define CloakPrintTimer(name, text, maxSamples) {if(name.GetSamples()>=(maxSamples)){CloakDebugLog((text) + std::to_string(name.GetPerformanceMicroSeconds()/1000.0)+" Milli Seconds"); name.Reset();}}
#else
	#define CloakDebugCheckOK(toCheck,toThrow,critical,...) true
	
	#define CloakDebugStringStart(msg)
	#define CloakDebugStringAttach(msg)
	#define CloakDebugStringPrint()

	#define CloakCreateTimer(name)
	#define CloakStartTimer(name)
	#define CloakStopTimer(name)
	#define CloakPrintTimer(name, text, maxSamples)
#endif

#define CloakDebugCheckError(toCheck,toThrow,critical,...) (!CloakDebugCheckOK(toCheck,toThrow,critical,__VA_ARGS__))

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Debug {
				Success_Type(return==Error::OK)
				enum class Error {
					OK							= 0x00000000,

					//0x40000XXX
					NO_INTERFACE				= 0x40000000,
					NO_CLASS					= 0x40000001,
					SERIALIZE_ERROR				= 0x40000002,
					COMPONENT_ALREADY_EXISTS	= 0x40000003,
					NOT_IMPLEMENTED				= 0x40000004,
					FILE_IO_ERROR				= 0x40000005,
					//0x40001XXX
					ILLEGAL_ARGUMENT			= 0x40001000,
					ILLEGAL_FUNC_CALL			= 0x40001001,
					ILLEGAL_DISABLED			= 0x40001002,
					//0x40002XXX
					GAME_ALREADY_STARTED		= 0x40002000,
					GAME_NOT_STARTED			= 0x40002001,
					GAME_INIT					= 0x40002002,
					GAME_MEMORY					= 0x40002003,
					GAME_NO_EVENT_HANDLER		= 0x40002004,
					GAME_MEMORY_ALLOC			= 0x40002005,
					GAME_OS_INIT				= 0x40002006,
					GAME_DEBUG_LAYER			= 0x40002007,
					//0x40003XXX
					LOCK_ALREADY_LOCKED			= 0x40003000,
					//0x40004XXX
					THREAD_VOID_ERROR			= 0x40004000,
					THREAD_NO_RETURN			= 0x40004001,
					THREAD_INIT					= 0x40004002,
					THREAD_TOO_SLOW				= 0x40004003,
					THREAD_NO_RESPONDE			= 0x40004004,
					THREAD_AFFINITY				= 0x40004005,
					//0x40005XXX
					WINDOW_CREATE				= 0x40005000,
					//0x40006XXX
					FWRITER_NO_FILE				= 0x40006000,
					FWRITER_COMPRESSION_FAILED	= 0x40006001,
					//0x40007XXX
					FREADER_NO_FILE				= 0x40007000,
					FREADER_OLD_VERSION			= 0x40007001,
					FREADER_FAILED_FILETYPE		= 0x40007002,
					FREADER_FAILED_FILEKEY		= 0x40007003,
					FREADER_FAILED_DECODING		= 0x40007004,
					FREADER_FAILED_CRYPTID		= 0x40007005,
					FREADER_FILE_CORRUPTED		= 0x40007006,
					//0x40008XXX
					INTERFACE_ILLEGAL_PARENT	= 0x40008000,
					INTERFACE_MENU_EXISTS		= 0x40008001,
					INTERFACE_MENU_NOT_EXIST	= 0x40008002,
					INTERFACE_MENU_ILLEGAL_SIZE	= 0x40008003,
					INTERFACE_MENU_NOT_VALID	= 0x40008004,
					//0x40009XXX
					BINDING_NOT_AVAILABLE		= 0x40009000,
					//0x4000aXXX
					GRAPHIC_ENABLE_DEBUG		= 0x4000a000,
					GRAPHIC_INIT_FACTORY		= 0x4000a001,
					GRAPHIC_NO_ADAPTER			= 0x4000a002,
					GRAPHIC_NO_DEVICE			= 0x4000a003,
					GRAPHIC_NO_QUEUE			= 0x4000a004,
					GRAPHIC_NO_SWAPCHAIN		= 0x4000a005,
					GRAPHIC_NO_ALLOCATOR		= 0x4000a006,
					GRAPHIC_NO_LIST				= 0x4000a007,
					GRAPHIC_LIST_CLOSE			= 0x4000a008,
					GRAPHIC_FENCE				= 0x4000a009,
					GRAPHIC_RESET				= 0x4000a00a,
					GRAPHIC_NO_RESSOURCES		= 0x4000a00b,
					GRAPHIC_SWITCH_MODE			= 0x4000a00c,
					GRAPHIC_CREATE_TEXTURE		= 0x4000a00d,
					GRAPHIC_DESCRIPTOR			= 0x4000a00e,
					GRAPHIC_NO_ROOT				= 0x4000a00f,
					GRAPHIC_NO_PSO				= 0x4000a010,
					GRAPHIC_ROOT_SERIALIZE		= 0x4000a011,
					GRAPHIC_NO_UPLOAD			= 0x4000a012,
					GRAPHIC_MAP_ERROR			= 0x4000a013,
					GRAPHIC_RESIZE				= 0x4000a014,
					GRAPHIC_NO_WARP_DEVICE		= 0x4000a015,
					GRAPHIC_FORMAT_UNSUPPORTED	= 0x4000a016,
					GRAPHIC_FEATURE_CHECK		= 0x4000a017,
					GRAPHIC_NO_MANAGER			= 0x4000a018,
					GRAPHIC_FEATURE_UNSUPPORETD	= 0x4000a019,
					GRAPHIC_WRONG_NODE			= 0x4000a01a,
					GRAPHIC_PARAMETER_NOT_SET	= 0x4000a01b,
					GRAPHIC_PSO_CREATION		= 0x4000a01c,
					GRAPIC_NO_ANSWER			= 0x4000a01d,
					//0x4000bXXX
					CMD_TOO_SHORT				= 0x4000b000,
					CMD_UNKNOWN_PARSE			= 0x4000b001,
					//0x4000cXXX
					INPUT_WRONG_SIZE			= 0x4000c000,
					INPUT_REGISTER_FAIL			= 0x4000c001,
					//0x4000dXXX
					AUDIO_NO_DEVICE				= 0x4000d000,
					AUDIO_NO_CLIENT				= 0x4000d001,
					AUDIO_FORMAT_SUPPORT		= 0x4000d002,
					AUDIO_NO_BUFFER				= 0x4000d003,
					AUDIO_NO_PLAYBACK			= 0x4000d004,
					AUDIO_EVENT_ERROR			= 0x4000d005,
					//0x4000eXXX
					EVENT_NOT_SUPPORTED			= 0x4000e000,
					//0x4000fXXX
					VIDEO_NO_DEVICE				= 0x4000f000,
					VIDEO_NO_CONTROL			= 0x4000f001,
					VIDEO_NO_EVENT				= 0x4000f002,
					VIDEO_NO_FILTER				= 0x4000f003,
					VIDEO_NO_RENDERER			= 0x4000f004,
					VIDEO_NO_AUDIO				= 0x4000f005,
					VIDEO_NO_PIN				= 0x4000f006,
					VIDEO_FINALIZE				= 0x4000f007,
					VIDEO_REMOVE_FILTER			= 0x4000f008,
					//0x40010XXX
					STATE_TRANSITION_ILLEGAL	= 0x40010000,
					STATE_TRANSITION_ENTER		= 0x40010001,
					STATE_TRANSITION_LEAVE		= 0x40010002,
					//0x40011XXX
					STEAM_COULD_NOT_LOAD		= 0x40011000,
					//0x40012XXX
					CONNECT_STARTUP				= 0x40012000,
					CONNECT_BIND				= 0x40012001,
					CONNECT_SOCKET				= 0x40012002,
					CONNECT_LISTEN				= 0x40012003,
					CONNECT_LOBBY_IS_OPEN		= 0x40012004,
					CONNECT_INVALID_IPTYPE		= 0x40012005,
					CONNECT_FAILED				= 0x40012006,
					CONNECT_EVENT_REGISTRATION	= 0x40012007,
					CONNECT_ACCEPT				= 0x40012008,
					CONNECT_READ_BUFFER_FULL	= 0x40012009,
					CONNECT_ADDRESS_RESOLVE		= 0x4001200a,
					CONNECT_HTTP_HOST			= 0x4001200b,
					CONNECT_HTTP_ERROR			= 0x4001200c,
					//0x40013XXX
					HARDWARE_NO_SSE_SUPPORT		= 0x40013000,
				};
				enum class CommandArgumentType { STRING, OPTION, INT, FLOAT, FIXED, NOT_SET };
				struct CommandArgument {
					CommandArgumentType Type;
					struct {
						std::string String = "";
						size_t Option = 0;
						int Integer = 0;
						float Float = 0;
					} Value;
				};

				CLOAKENGINE_API void CLOAK_CALL SendError(In Error toThrow, In bool critical, In_opt Log::Source source = CloakDefaultLogSource, In_opt std::string file = "", In_opt int line = 0, In_opt unsigned int framesToSkip = 0);
				CLOAKENGINE_API void CLOAK_CALL SendError(In Error toThrow, In bool critical, In std::string info, In_opt Log::Source source = CloakDefaultLogSource, In_opt std::string file = "", In_opt int line = 0, In_opt unsigned int framesToSkip = 0);

				template<typename A> CLOAK_FORCEINLINE constexpr bool CLOAK_CALL IsError(In const A& t) { return true; }
				template<> CLOAK_FORCEINLINE constexpr bool CLOAK_CALL IsError<bool>(In const bool& t) { return !t; }
				template<> CLOAK_FORCEINLINE constexpr bool CLOAK_CALL IsError<HRESULT>(In const HRESULT& t) { return FAILED(t); }
				template<> CLOAK_FORCEINLINE constexpr bool CLOAK_CALL IsError<Error>(In const Error& t) { return (static_cast<unsigned int>(t) & 0x40000000) != 0; }

				template<bool critical, typename ErrorType> struct CheckError {
					static CLOAK_FORCEINLINE bool CLOAK_CALL Check(In ErrorType t, In Error toThrow, In std::string file, In int line, In_opt Log::Source source = CloakDefaultLogSource)
					{
						if (IsError(t))
						{
							SendError(toThrow, critical, source, file, line, 1U);
							return false;
						}
						return true;
					}
					static CLOAK_FORCEINLINE bool CLOAK_CALL Check(In ErrorType t, In Error toThrow, In std::string file, In int line, In std::string info, In_opt Log::Source source = CloakDefaultLogSource)
					{
						if (IsError(t))
						{
							SendError(toThrow, critical, info, source, file, line, 1U);
							return false;
						}
						return true;
					}
				};
#ifndef _DEBUG
				template<typename ErrorType> struct CheckError<false, ErrorType> {
					static constexpr bool critical = false;
					static constexpr CLOAK_FORCEINLINE bool CLOAK_CALL Check(In ErrorType t, In Error toThrow, In std::string file, In int line, In_opt Log::Source source = CloakDefaultLogSource)
					{
						return true;
					}
					static constexpr CLOAK_FORCEINLINE bool CLOAK_CALL Check(In ErrorType t, In Error toThrow, In std::string file, In int line, In std::string info, In_opt Log::Source source = CloakDefaultLogSource)
					{
						return true;
					}
				};
				template<> struct CheckError<true, HRESULT> {
					static constexpr bool critical = true;
#else
				template<bool critical> struct CheckError<critical, HRESULT> {
#endif
					typedef HRESULT ErrorType;
					static CLOAK_FORCEINLINE bool CLOAK_CALL Check(In ErrorType t, In Error toThrow, In std::string file, In int line, In_opt Log::Source source = CloakDefaultLogSource)
					{
						if (IsError(t))
						{
							_com_error err(t);
							auto msg = err.ErrorMessage();
							SendError(toThrow, critical, API::Helper::StringConvert::ConvertToU8(msg), source, file, line, 1U);
							return false;
						}
						return true;
					}
					static CLOAK_FORCEINLINE bool CLOAK_CALL Check(In ErrorType t, In Error toThrow, In std::string file, In int line, In std::string info, In_opt Log::Source source = CloakDefaultLogSource)
					{
						if (IsError(t))
						{
							_com_error err(t);
							auto msg = err.ErrorMessage();
							SendError(toThrow, critical, info + "\n" + API::Helper::StringConvert::ConvertToU8(msg), source, file, line, 1U);
							return false;
						}
						return true;
					}
				};
				class CLOAKENGINE_API PerformanceTimer {
					private:
						API::Global::Time m_start;
						uint64_t m_acc;
						uint64_t m_samples;
					public:
						CLOAK_CALL PerformanceTimer();
						void CLOAK_CALL_THIS Start();
						void CLOAK_CALL_THIS Stop();
						void CLOAK_CALL_THIS Reset();
						double CLOAK_CALL_THIS GetPerformanceMicroSeconds() const;
						uint64_t CLOAK_CALL_THIS GetSamples() const;
				};
				/*
				Registers a console command. Syntax:
				- You can optionally add a "(desc)" to any parameter (except %o, where you have to) to give the parameter the name "desc". (No spaces are allowed within that name)
				- You can specify a parameter as optional by writing it in [] brakets. Notice that there can not be an optional parameter followed by a non-optional one.
				- "text %%text" translates to "text %text"
				- %s is a text parameter
				- %d is a integer parameter
				- %f is a float parameter
				- %o(a|b|c) is a options-parameter, where only "a", "b" or "c" are valid inputs (number of valid inputs can be anything greater or equal to one)
				*/
				CLOAKENGINE_API Error CLOAK_CALL RegisterConsoleArgument(In std::string parseCmd, In const std::string& description, Inout std::function<bool(In const CommandArgument* args, In size_t count)>&& func);
			}
		}
	}
}

#endif
