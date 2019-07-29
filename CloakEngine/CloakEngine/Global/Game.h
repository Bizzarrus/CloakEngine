#pragma once
#ifndef CE_API_GLOBAL_GAME_H
#define CE_API_GLOBAL_GAME_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Global/GameEvents.h"
#include "CloakEngine/Helper/SavePtr.h"

#include <functional>

#if WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
#ifdef _WINDOWS
#define CE_MAIN_FUNCTION() int CALLBACK WinMain(HINSTANCE hInst, HINSTANCE hPrev,LPSTR cmds,int cmdShow)
#else
#define CE_MAIN_FUNCTION() int main()
#endif
#else
#define CE_MAIN_FUNCTION() int main()
#endif
#define CLOAKENGINE_START(gameClass,gameInfo) CE_MAIN_FUNCTION(){return CloakEngine::API::Global::Game::StartGame(gameClass(),gameInfo());}

#ifdef _DEBUG
#define CREATE_INTERFACE(...) CloakEngine::API::Global::Game::CreateInterface(std::string(__FILE__)+" @ "+std::to_string(__LINE__),__VA_ARGS__)
#else
#define CREATE_INTERFACE(...) CloakEngine::API::Global::Game::CreateInterface(__VA_ARGS__)
#endif

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			constexpr size_t THREAD_COUNT = 7;
			struct GameInfo {
				bool useWindow = false;
				bool useConsole = true;
				bool useSteam = false;
				bool debugMode = true;
			};
			struct FPSInfo {
				struct {
					float FPS = 0;
					unsigned int ID = 0;
				} Thread[THREAD_COUNT];
				uint32_t Used = 0;
			};
			class CLOAKENGINE_API Date {
				private:
					//Time in seconds since 1.1.0000 00:00:00 
					uint64_t m_time;

					static constexpr uint32_t DAYS_IN_MONTH[][13] = {
						{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
						{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }, //Leap Year
					};
					static constexpr uint64_t TIME_TO_MINUTES = 60;
					static constexpr uint64_t TIME_TO_HOURS = TIME_TO_MINUTES * 60;
					static constexpr uint64_t TIME_TO_DAYS = TIME_TO_HOURS * 24;

					inline constexpr bool CLOAK_CALL_THIS isLeap(In uint32_t year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }
					inline constexpr uint64_t CLOAK_CALL_THIS iToTime(In uint32_t year, In uint32_t month, In uint32_t day, In uint32_t hour, In uint32_t minute, In uint32_t second)
					{
						CLOAK_ASSUME((year == 0) == (month == 0));
						CLOAK_ASSUME(day > 0 || (year == 0 && month == 0));
						if (year > 0 && month > 0)
						{
							const size_t dim = isLeap(year) ? 1 : 0;
							const uint32_t py = year - 1;
							const uint32_t diy = (py * 365) + (py / 4) + (py / 400) - (py / 100);
							day += (DAYS_IN_MONTH[dim][month - 1] - 1) + diy;
						}
						return second + (60ULL * (minute + (60ULL * (hour + (24ULL * day)))));
					}
				public:
					constexpr CLOAK_CALL Date(In_opt uint64_t time = 0) : m_time(time) {}
					constexpr CLOAK_CALL Date(In const Date& o) : m_time(o.m_time) {}
					constexpr CLOAK_CALL Date(In uint32_t hour, In uint32_t minute, In uint32_t second) : m_time(iToTime(0, 0, 0, hour, minute, second)) {}
					constexpr CLOAK_CALL Date(In uint32_t day, In uint32_t hour, In uint32_t minute, In uint32_t second) : m_time(iToTime(0, 0, day, hour, minute, second)) {}
					constexpr CLOAK_CALL Date(In uint32_t year, In uint32_t month, In uint32_t day, In uint32_t hour, In uint32_t minute, In uint32_t second) : m_time(iToTime(year, month, day, hour, minute, second)) {}

					inline constexpr operator uint64_t() const { return m_time; }

					inline constexpr Date& CLOAK_CALL_THIS operator=(In uint64_t time) { m_time = time; return *this; }
					inline constexpr Date& CLOAK_CALL_THIS operator=(In const Date& o) { m_time = o.m_time; return *this; }
					inline constexpr Date& CLOAK_CALL_THIS operator-=(In const Date& o) { m_time = m_time > o.m_time ? m_time - o.m_time : 0; return *this; }
					inline constexpr Date& CLOAK_CALL_THIS operator+=(In const Date& o) { m_time += o.m_time; return *this; }
					inline constexpr Date CLOAK_CALL_THIS operator-(In const Date& o) const { return Date(*this) -= o; }
					inline constexpr Date CLOAK_CALL_THIS operator+(In const Date& o) const { return Date(*this) += o; }

					inline constexpr bool CLOAK_CALL_THIS operator==(In const Date& o) const { return m_time == o.m_time; }
					inline constexpr bool CLOAK_CALL_THIS operator!=(In const Date& o) const { return m_time != o.m_time; }
					inline constexpr bool CLOAK_CALL_THIS operator>=(In const Date& o) const { return m_time >= o.m_time; }
					inline constexpr bool CLOAK_CALL_THIS operator<=(In const Date& o) const { return m_time <= o.m_time; }
					inline constexpr bool CLOAK_CALL_THIS operator>(In const Date& o) const { return m_time > o.m_time; }
					inline constexpr bool CLOAK_CALL_THIS operator<(In const Date& o) const { return m_time < o.m_time; }

					inline constexpr bool CLOAK_CALL_THIS operator==(In uint64_t time) const { return m_time == time; }
					inline constexpr bool CLOAK_CALL_THIS operator!=(In uint64_t time) const { return m_time != time; }
					inline constexpr bool CLOAK_CALL_THIS operator>=(In uint64_t time) const { return m_time >= time; }
					inline constexpr bool CLOAK_CALL_THIS operator<=(In uint64_t time) const { return m_time <= time; }
					inline constexpr bool CLOAK_CALL_THIS operator>(In uint64_t time) const { return m_time > time; }
					inline constexpr bool CLOAK_CALL_THIS operator<(In uint64_t time) const { return m_time < time; }

					inline constexpr uint64_t CLOAK_CALL_THIS TotalDays() const { return m_time / TIME_TO_DAYS; }
					inline constexpr uint64_t CLOAK_CALL_THIS TotalHours() const { return m_time / TIME_TO_HOURS; }
					inline constexpr uint64_t CLOAK_CALL_THIS TotalMinutes() const { return m_time / TIME_TO_MINUTES; }
					inline constexpr uint64_t CLOAK_CALL_THIS TotalSeconds() const { return m_time; }

					inline constexpr uint32_t CLOAK_CALL_THIS Day() const { return static_cast<uint32_t>(m_time / TIME_TO_DAYS); }
					inline constexpr uint32_t CLOAK_CALL_THIS Hour() const { return static_cast<uint32_t>((m_time / TIME_TO_HOURS) % 24); }
					inline constexpr uint32_t CLOAK_CALL_THIS Minute() const { return static_cast<uint32_t>((m_time / TIME_TO_MINUTES) % 60); }
					inline constexpr uint32_t CLOAK_CALL_THIS Second() const { return static_cast<uint32_t>(m_time % 60); }
			};
			namespace Game {
				CLOAKENGINE_API void CLOAK_CALL StartEngine(In const IGameEventFactory* factory, In const GameInfo& info);
				CLOAKENGINE_API void CLOAK_CALL StartEngineAsync(In const IGameEventFactory* factory, In const GameInfo& info);
				CLOAKENGINE_API void CLOAK_CALL StartEngine(In const IGameEventFactory* factory, In const GameInfo& info, In HWND window);
				CLOAKENGINE_API void CLOAK_CALL StartEngineAsync(In const IGameEventFactory* factory, In const GameInfo& info, In HWND window);
				CLOAKENGINE_API void CLOAK_CALL Stop();
				CLOAKENGINE_API void CLOAK_CALL WaitForStop();
				CLOAKENGINE_API void CLOAK_CALL SetFPS(In_opt float fps = 0.0f);
				CLOAKENGINE_API void CLOAK_CALL GetFPS(Out FPSInfo* info);
				CLOAKENGINE_API bool CLOAK_CALL IsRunning();
				CLOAKENGINE_API bool CLOAK_CALL IsDebugMode();
				CLOAKENGINE_API bool CLOAK_CALL HasWindow();
				CLOAKENGINE_API Debug::Error CLOAK_CALL CreateInterface(In REFIID riid, Outptr void** ppvObject);
				CLOAKENGINE_API Debug::Error CLOAK_CALL CreateInterface(In REFIID classRiid, In REFIID interfRiid, Outptr void** ppvObject);
				CLOAKENGINE_API Debug::Error CLOAK_CALL CreateInterface(In const std::string& file, In REFIID riid, Outptr void** ppvObject);
				CLOAKENGINE_API Debug::Error CLOAK_CALL CreateInterface(In const std::string& file, In REFIID classRiid, In REFIID interfRiid, Outptr void** ppvObject);
				CLOAKENGINE_API Debug::Error CLOAK_CALL GetDebugLayer(In REFIID riid, Outptr void** ppvObject);
				CLOAKENGINE_API void CLOAK_CALL Pause();
				CLOAKENGINE_API void CLOAK_CALL Resume();
				CLOAKENGINE_API bool CLOAK_CALL IsPaused();
				CLOAKENGINE_API Time CLOAK_CALL GetGameTimeStamp();
				CLOAKENGINE_API Time CLOAK_CALL GetTimeStamp();
				CLOAKENGINE_API Date CLOAK_CALL GetLocalDate();
				CLOAKENGINE_API Date CLOAK_CALL GetUTCDate();
			}
		}
	}
}

#endif