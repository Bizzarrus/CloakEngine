#include "stdafx.h"
#include "CloakEngine/Global/Log.h"
#include "Implementation/Global/Log.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/StringConvert.h"
#include "Implementation/Global/Debug.h"
#include "Implementation/Global/Game.h"

#include "Engine/FlipVector.h"

#include <fstream>
#include <codecvt>
#include <sstream>

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Log {
				struct MSG {
					std::string text;
					API::Global::Log::Type typ;
					API::Global::Log::Source src;
				};
				const int g_timeStampLength = 9;

				Engine::FlipVector<MSG>* g_msg;
				API::Helper::ISyncSection* g_syncMsg = nullptr;
				API::Helper::ISyncSection* g_syncFilePath = nullptr;
				std::u16string* g_filePath = nullptr;
				void CLOAK_CALL onStart(In size_t threadID)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncMsg));
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncFilePath));
				}
				void CLOAK_CALL onUpdate(In size_t threadID, In API::Global::Time etime)
				{
					API::Helper::Lock msgLock(g_syncMsg);
					API::List<MSG>& m = g_msg->flip();
					msgLock.unlock();
					if (m.size() > 0)
					{
						API::Helper::Lock fileLock(g_syncFilePath);
						std::fstream mf;
						if (g_filePath->length() > 0) { mf.open(API::Helper::StringConvert::ConvertToU8(*g_filePath), std::ios::app | std::ios::out | std::ios::binary); }
						for (size_t a = 0; a < m.size(); a++)
						{
							const MSG& msg = m[a];
							if (msg.typ != API::Global::Log::Type::Console && mf.is_open())
							{
								//write to file
								mf.write(msg.text.c_str(), msg.text.length());
							}
							if (msg.typ != API::Global::Log::Type::File)
							{
								//write to console
								Impl::Global::Debug::writeToConsole(msg.text, msg.src);
							}
#ifdef _MSC_VER
							if (API::Global::Game::IsDebugMode())
							{
								OutputDebugStringA(msg.text.c_str());
							}
#endif
						}
						if (mf.is_open()) { mf.close(); }
						m.clear();
					}
				}
				void CLOAK_CALL onStop(In size_t threadID)
				{
					onUpdate(0, threadID);
					SAVE_RELEASE(g_syncFilePath);
					SAVE_RELEASE(g_syncMsg);
				}
				void CLOAK_CALL Release()
				{
					delete g_msg;
					delete g_filePath;
					g_msg = nullptr;
					g_filePath = nullptr;
				}
				void CLOAK_CALL Initialize()
				{
					g_msg = new Engine::FlipVector<MSG>();
					g_filePath = new std::u16string(u"");
				}
			}
		}
	}
	namespace API {
		namespace Global {
			namespace Log {
				CLOAKENGINE_API void CLOAK_CALL SetLogFilePath(In const std::u16string& path)
				{
					if (path.length() > 0)
					{
						std::u16string tpath = API::Files::GetFullFilePath(path);
						Helper::Lock lock(Impl::Global::Log::g_syncFilePath);
						//Delete old log file, if any:
						if (Impl::Global::Log::g_filePath->compare(tpath) != 0 && Impl::Global::Log::g_filePath->length() > 0)
						{ 
							std::string fp = API::Helper::StringConvert::ConvertToU8(*Impl::Global::Log::g_filePath);
							std::remove(fp.c_str());
						}
						//Create and clear log file:
						std::fstream mf(API::Helper::StringConvert::ConvertToU8(tpath), std::ios::out);
						mf.close();
						*Impl::Global::Log::g_filePath = tpath;
					}
				}
				void CLOAK_CALL LogWrite(In const std::string& str, In Type typ, In Source src)
				{
					if (str.length() > 0 && (typ != Type::Debug || Game::IsDebugMode()))
					{
						std::stringstream strs;
						Impl::Global::Log::MSG m;
						std::string time = std::to_string(Impl::Global::Game::getCurrentTimeMilliSeconds());
						while (time.length() < Impl::Global::Log::g_timeStampLength) { time = "0" + time; }
						if (time.length() > Impl::Global::Log::g_timeStampLength) { time = time.substr(time.length() - Impl::Global::Log::g_timeStampLength); }
						strs << "[" << time << "]:[";
						switch (src)
						{
							case Source::Modification:
								strs << "MOD";
								break;
							case Source::Engine:
								strs << "ENGINE";
								break;
							case Source::Game:
								strs << "GAME";
								break;
							default:
								strs << "UNKNOWN";
								break;
						}
						strs << " ";
						switch (typ)
						{
							case Type::Console:
								strs << "CONSOLE";
								break;
							case Type::Debug:
								strs << "DEBUG";
								break;
							case Type::Error:
								strs << "ERROR";
								break;
							case Type::File:
								strs << "LOG";
								break;
							case Type::Info:
								strs << "INFO";
								break;
							case Type::Warning:
								strs << "WARNING";
								break;
							default:
								strs << "UNKNOWN";
								break;
						}
						strs << "]: " << str << "\r\n";
						m.text = strs.str();
						m.typ = typ;
						m.src = src;
						Helper::Lock lock(Impl::Global::Log::g_syncMsg);
						Impl::Global::Log::g_msg->get()->push_back(m);
						lock.unlock();
						if (typ == Type::Error) { Impl::Global::Log::onUpdate(0, 0); }
					}
				}
				CLOAKENGINE_API void CLOAK_CALL WriteToLog(In const std::string& str, In Source source, In_opt Type typ) { WriteToLog(str, typ, source); }
				CLOAKENGINE_API void CLOAK_CALL WriteToLog(In const std::u16string& str, In Source source, In_opt Type typ) { WriteToLog(str, typ, source); }
				CLOAKENGINE_API void CLOAK_CALL WriteToLog(In const std::string& str, In_opt Type typ, In_opt Source source) { LogWrite(str, typ, source); }
				CLOAKENGINE_API void CLOAK_CALL WriteToLog(In const std::u16string& str, In_opt Type typ, In_opt Source source) { LogWrite(API::Helper::StringConvert::ConvertToU8(str), typ, source); }
			}
		}
	}
}