#include "stdafx.h"
#include "Engine/Input/Gamepad.h"
#include "Engine/FileLoader.h"
#include "Engine/WindowHandler.h"

#include "Implementation/Global/Game.h"
#include "Implementation/Global/Mouse.h"
#include "Implementation/Global/Input.h"

#include "CloakEngine/Helper/SyncSection.h"

#include <atomic>
#include <Xinput.h>

#pragma comment(lib,"xinput.lib")

namespace CloakEngine {
	namespace Engine {
		namespace Input {
			namespace Gamepad {
				constexpr API::Global::Time CONNECTION_CHECK_TIME = 2000;
				constexpr uint32_t MAX_THUMB_VALUE = 32767;
				constexpr uint32_t MAX_VIBRATION_VALUE = 65535;
				constexpr uint32_t LEFT_THUMB_DEADZONE = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
				constexpr uint32_t RIGHT_THUMB_DEADZONE = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

				struct ThumbData {
					int32_t X = 0;
					int32_t Y = 0;
				};
				struct Gamepad {
					bool Connected = false;
					API::Global::Time lastChecked = 0;
					DWORD lastPacked = 0;
					uint32_t leftVibrationSpeed = 0;
					uint32_t rightVibrationSpeed = 0;
					ThumbData lThumb;
					ThumbData rThumb;
				} g_gamepads[XUSER_MAX_COUNT];

				API::Helper::ISyncSection* g_sync = nullptr;
				std::atomic<bool> g_enabled = true;

				inline bool CLOAK_CALL isButtonPressed(In const XINPUT_GAMEPAD& state, In WORD btn) { return (state.wButtons & btn) != 0; }
				inline bool CLOAK_CALL isTriggerPressed(In const XINPUT_GAMEPAD& state, In bool left) { return (left ? state.bLeftTrigger : state.bRightTrigger) > XINPUT_GAMEPAD_TRIGGER_THRESHOLD; }
				inline bool CLOAK_CALL CheckThumbPos(Inout ThumbData* thumb, In const XINPUT_GAMEPAD& state, In bool left, Out API::Global::Math::Space2D::Point* res)
				{
					res->X = 0;
					res->Y = 0;
					const int32_t tx = left ? state.sThumbLX : state.sThumbRX;
					const int32_t ty = left ? state.sThumbLY : state.sThumbRY;
					const uint32_t deadzone = left ? LEFT_THUMB_DEADZONE : RIGHT_THUMB_DEADZONE;
					const double l = std::sqrt((tx*tx) + (ty*ty));
					if (l > deadzone)
					{
						const float nl = static_cast<float>((min(l, MAX_THUMB_VALUE) - deadzone) / (l*(MAX_THUMB_VALUE - deadzone)));
						res->X = tx * nl;
						res->Y = ty * nl;
						if (thumb->X != tx || thumb->Y != ty)
						{
							thumb->X = tx;
							thumb->Y = ty;
							return true;
						}
					}
					else if (thumb->X != 0 || thumb->Y != 0)
					{
						thumb->X = 0;
						thumb->Y = 0;
						return true;
					}
					return false;
				}
				inline void CLOAK_CALL UpdateThumbPos(Inout Gamepad* g, In const XINPUT_GAMEPAD& state, In DWORD user)
				{
					API::Global::Math::Space2D::Point lt;
					API::Global::Math::Space2D::Point rt;
					const bool ul = CheckThumbPos(&g->lThumb, state, true, &lt);
					const bool ur = CheckThumbPos(&g->rThumb, state, false, &rt);
					if (ul || ur) { Impl::Global::Input::OnThumbMoveEvent(lt, rt, user); }
				}
				inline void CLOAK_CALL SetVibration(In DWORD user, Inout Gamepad* g, In uint32_t left, In uint32_t right)
				{
					g->leftVibrationSpeed = left;
					g->rightVibrationSpeed = right;
					XINPUT_VIBRATION vib;
					vib.wLeftMotorSpeed = static_cast<WORD>(left);
					vib.wRightMotorSpeed = static_cast<WORD>(right);
					XInputSetState(user, &vib);
				}

				inline void CLOAK_CALL ResetGamepad(Inout Gamepad* g, In DWORD user)
				{
					Impl::Global::Input::OnResetKeys(user);
					if (g->lThumb.X != 0 || g->lThumb.Y != 0 || g->rThumb.X != 0 || g->rThumb.Y != 0)
					{
						g->lThumb.X = 0;
						g->lThumb.Y = 0;
						g->rThumb.X = 0;
						g->rThumb.Y = 0;
						API::Global::Math::Space2D::Point lt(0, 0);
						Impl::Global::Input::OnThumbMoveEvent(lt, lt, user);
					}

					//Reset vibration
					XINPUT_VIBRATION vib;
					vib.wLeftMotorSpeed = 0;
					vib.wRightMotorSpeed = 0;
					XInputSetState(user, &vib);
					/*
					
						if (isTriggerPressed(*pad, true)) { gamepadButtonEvent(API::Global::Input::Keys::Gamepad::Trigger::Left, API::Global::Input::KeyPressType::UP, user); }
						if (isTriggerPressed(*pad, false)) { gamepadButtonEvent(API::Global::Input::Keys::Gamepad::Trigger::Left, API::Global::Input::KeyPressType::UP, user); }
						API::Global::Math::Space2D::Point thumbL = API::Global::Math::Space2D::Point(0, 0);
						API::Global::Math::Space2D::Point thumbR = API::Global::Math::Space2D::Point(0, 0);
						gamepadThumbEvent(thumbL, thumbR, user);
						if (useVibrator)
						{
							XINPUT_VIBRATION vib;
							ZeroMemory(&vib, sizeof(vib));
							XInputSetState(user, &vib);
						}
					}
					ZeroMemory(pad, sizeof(XINPUT_GAMEPAD));
					*/
				}


				void CLOAK_CALL Initialize()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_sync));
				}
				void CLOAK_CALL Release()
				{
					SAVE_RELEASE(g_sync);
				}
				void CLOAK_CALL Update(In unsigned long long etime)
				{
					if (g_enabled == true && Engine::FileLoader::isLoading(API::Files::Priority::NORMAL) == false && Engine::WindowHandler::isFocused() && Impl::Global::Mouse::IsInClip())
					{
						API::Helper::Lock lock(g_sync);
						const API::Global::Time ct = Impl::Global::Game::getCurrentTimeMilliSeconds();
						XINPUT_STATE state;
						for (DWORD a = 0; a < XUSER_MAX_COUNT; a++)
						{
							Gamepad* g = &g_gamepads[a];
							if (g->Connected || (etime > 0 && g->lastChecked == 0) || (g->lastChecked != 0 && ct - g->lastChecked > CONNECTION_CHECK_TIME))
							{
								ZeroMemory(&state, sizeof(state));
								const DWORD hRet = XInputGetState(a, &state);
								const bool connected = (hRet == ERROR_SUCCESS);
								const bool updated = (connected != g->Connected);
								g->Connected = connected;
								g->lastChecked = ct;
								if (updated)
								{
									if (connected == false) { ResetGamepad(g, a); }
									else { SetVibration(a, g, g->leftVibrationSpeed, g->rightVibrationSpeed); }
									Impl::Global::Input::OnConnect(a, connected);
								}
								if (connected && (updated || g->lastPacked != state.dwPacketNumber))
								{
									if (!updated)
									{
										API::Global::Input::KeyCode down = API::Global::Input::Keys::NONE;
										API::Global::Input::KeyCode up = API::Global::Input::Keys::NONE;
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_DPAD_UP)) { down |= API::Global::Input::Keys::Gamepad::DPad::UP; } else { up |= API::Global::Input::Keys::Gamepad::DPad::UP; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_DPAD_DOWN)) { down |= API::Global::Input::Keys::Gamepad::DPad::DOWN; } else { up |= API::Global::Input::Keys::Gamepad::DPad::DOWN; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_DPAD_LEFT)) { down |= API::Global::Input::Keys::Gamepad::DPad::LEFT; } else { up |= API::Global::Input::Keys::Gamepad::DPad::LEFT; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_DPAD_RIGHT)) { down |= API::Global::Input::Keys::Gamepad::DPad::RIGHT; } else { up |= API::Global::Input::Keys::Gamepad::DPad::RIGHT; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_X)) { down |= API::Global::Input::Keys::Gamepad::X; } else { up |= API::Global::Input::Keys::Gamepad::X; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_Y)) { down |= API::Global::Input::Keys::Gamepad::Y; } else { up |= API::Global::Input::Keys::Gamepad::Y; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_A)) { down |= API::Global::Input::Keys::Gamepad::A; } else { up |= API::Global::Input::Keys::Gamepad::A; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_B)) { down |= API::Global::Input::Keys::Gamepad::B; } else { up |= API::Global::Input::Keys::Gamepad::B; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_LEFT_THUMB)) { down |= API::Global::Input::Keys::Gamepad::Thumb::LEFT; } else { up |= API::Global::Input::Keys::Gamepad::Thumb::LEFT; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_RIGHT_THUMB)) { down |= API::Global::Input::Keys::Gamepad::Thumb::RIGHT; } else { up |= API::Global::Input::Keys::Gamepad::Thumb::RIGHT; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_LEFT_SHOULDER)) { down |= API::Global::Input::Keys::Gamepad::Shoulder::LEFT; } else { up |= API::Global::Input::Keys::Gamepad::Shoulder::LEFT; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_RIGHT_SHOULDER)) { down |= API::Global::Input::Keys::Gamepad::Shoulder::RIGHT; } else { up |= API::Global::Input::Keys::Gamepad::Shoulder::RIGHT; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_START)) { down |= API::Global::Input::Keys::Gamepad::START; } else { up |= API::Global::Input::Keys::Gamepad::START; }
										if (isButtonPressed(state.Gamepad, XINPUT_GAMEPAD_BACK)) { down |= API::Global::Input::Keys::Gamepad::BACK; } else { up |= API::Global::Input::Keys::Gamepad::BACK; }
										if (isTriggerPressed(state.Gamepad, true)) { down |= API::Global::Input::Keys::Gamepad::Trigger::LEFT; } else { up |= API::Global::Input::Keys::Gamepad::Trigger::LEFT; }
										if (isTriggerPressed(state.Gamepad, false)) { down |= API::Global::Input::Keys::Gamepad::Trigger::RIGHT; } else { up |= API::Global::Input::Keys::Gamepad::Trigger::RIGHT; }
										Impl::Global::Input::OnKeyEvent(down, up, a);
									}
									UpdateThumbPos(g, state.Gamepad, a);
									g->lastPacked = state.dwPacketNumber;
								}
							}
						}
					}
				}
				void CLOAK_CALL SetEnable(In bool enabled)
				{
					g_enabled = enabled;
				}
				void CLOAK_CALL SetVibrationLeft(In DWORD user, In float power)
				{
					if (user < XUSER_MAX_COUNT)
					{
						const uint32_t lvalue = static_cast<uint32_t>(power * (MAX_VIBRATION_VALUE + 1));
						API::Helper::Lock lock(g_sync);
						Gamepad* g = &g_gamepads[user];
						SetVibration(user, g, min(lvalue, MAX_VIBRATION_VALUE), g->rightVibrationSpeed);
					}
				}
				void CLOAK_CALL SetVibrationRight(In DWORD user, In float power)
				{
					if (user < XUSER_MAX_COUNT)
					{
						const uint32_t rvalue = static_cast<uint32_t>(power * (MAX_VIBRATION_VALUE + 1));
						API::Helper::Lock lock(g_sync);
						Gamepad* g = &g_gamepads[user];
						SetVibration(user, g, g->leftVibrationSpeed, min(rvalue, MAX_VIBRATION_VALUE));
					}
				}
				void CLOAK_CALL SetVibration(In DWORD user, In float left, In float right)
				{
					if (user < XUSER_MAX_COUNT)
					{
						const uint32_t lvalue = static_cast<uint32_t>(left * (MAX_VIBRATION_VALUE + 1));
						const uint32_t rvalue = static_cast<uint32_t>(right * (MAX_VIBRATION_VALUE + 1));
						API::Helper::Lock lock(g_sync);
						Gamepad* g = &g_gamepads[user];
						SetVibration(user, g, min(lvalue, MAX_VIBRATION_VALUE), min(rvalue, MAX_VIBRATION_VALUE));
					}
				}
			}
		}
	}
}