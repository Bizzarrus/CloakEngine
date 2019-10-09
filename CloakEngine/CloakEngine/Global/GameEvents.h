#pragma once
#ifndef CE_API_GLOBAL_GAMEEVENTS_H
#define CE_API_GLOBAL_GAMEEVENTS_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Global/Input.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Global/Connection.h"
#include "CloakEngine/Launcher/Launcher.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			inline namespace GameEvents_v1 {
				CLOAK_INTERFACE IGameEventBase{
					public:
						virtual void CLOAK_CALL_THIS Delete() = 0;
				};
				CLOAK_INTERFACE IGameEvent : public virtual IGameEventBase{
					public:
						virtual void CLOAK_CALL_THIS OnInit() = 0;
						virtual void CLOAK_CALL_THIS OnStart() = 0;
						virtual void CLOAK_CALL_THIS OnUpdate(In Global::Time etime, In Global::Time scaledGameTime, In bool inLoadingScreen) = 0;
						virtual void CLOAK_CALL_THIS OnStop() = 0;
						virtual void CLOAK_CALL_THIS OnPause() = 0;
						virtual void CLOAK_CALL_THIS OnResume() = 0;
						virtual void CLOAK_CALL_THIS OnCrash(In Global::Debug::Error errMsg) = 0;
						virtual void CLOAK_CALL_THIS OnUpdateLanguage() = 0;
				};
				CLOAK_INTERFACE ILauncherEvent : public virtual IGameEventBase{
					public:
						virtual void CLOAK_CALL_THIS OnOpenLauncher(In Launcher::ILauncher* manager) = 0;
				};
				CLOAK_INTERFACE IInputEvent : public virtual IGameEventBase{
					public:
						virtual void CLOAK_CALL_THIS OnMouseMove(In Global::Input::User user, In const Global::Math::Space2D::Vector& direction, In const Global::Math::Space2D::Point& resPos, In Global::Time etime) = 0;
						virtual void CLOAK_CALL_THIS OnGamepadConnect(In Global::Input::User user, In Global::Input::CONNECT_EVENT connect) = 0;
						virtual void CLOAK_CALL_THIS OnGamepadThumbMove(In Global::Input::User user, In const Global::Input::ThumbMoveData& data, In Global::Time etime) = 0;
						virtual void CLOAK_CALL_THIS OnKeyEnter(In API::Global::Input::User user, In const Global::Input::KeyCode& down, In const Global::Input::KeyCode& up) = 0;
						virtual void CLOAK_CALL_THIS OnScroll(In API::Global::Input::User user, In API::Global::Input::SCROLL_STATE button, In float delta) = 0;
						virtual void CLOAK_CALL_THIS OnWindowClose() = 0;
				};
				CLOAK_INTERFACE ILobbyEvent : public virtual IGameEventBase{
					public:
						virtual void CLOAK_CALL_THIS OnConnect(In IConnection* con) = 0;
						virtual void CLOAK_CALL_THIS OnDisconnect(In IConnection* con) = 0;
						virtual void CLOAK_CALL_THIS OnReceivedData(In IConnection* con) = 0;
						virtual void CLOAK_CALL_THIS OnConnectionFailed(In IConnection* con) = 0;
				};
				CLOAK_INTERFACE IDebugEvent : public virtual IGameEventBase{
					public:
						virtual void CLOAK_CALL_THIS OnLogWrite(In const std::string& msg) = 0;
				};
				CLOAK_INTERFACE IGameEventFactory{
					public:
						virtual IGameEvent* CLOAK_CALL_THIS CreateGame() = 0;
						virtual ILauncherEvent* CLOAK_CALL_THIS CreateLauncher() = 0;
						virtual IInputEvent* CLOAK_CALL_THIS CreateInput() = 0;
						virtual ILobbyEvent* CLOAK_CALL_THIS CreateLobby() = 0;
						virtual IDebugEvent* CLOAK_CALL_THIS CreateDebug() = 0;
				};
			}
		}
	}
}

#endif