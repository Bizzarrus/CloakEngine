#pragma once
#ifndef CE_API_GLOBAL_INPUT_H
#define CE_API_GLOBAL_INPUT_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Global/Math.h"

#undef DELETE

#define IS_CLICK(click,btn) (click.key==CloakEngine::API::Global::Input::Keys::Mouse::btn && click.pressType==CloakEngine::API::Global::Input::MOUSE_UP && (click.previousType==CloakEngine::API::Global::Input::MOUSE_DOWN || click.previousType==CloakEngine::API::Global::Input::MOUSE_DOUBLE))

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Input {
				typedef uint8_t User;

				enum CONNECT_EVENT {
					CONNECTED,
					DISCONNECTED,
				};
				enum KEY_STATE {
					KEY_DOWN,
					KEY_UP,
					KEY_PRESS,
				};
				enum MOUSE_STATE {
					MOUSE_DOWN,
					MOUSE_UP,
					MOUSE_PRESSED,
					MOUSE_DRAGED,
					MOUSE_DOUBLE,
				};
				enum SCROLL_STATE {
					SCROLL_VERTICAL,
					SCROLL_HORIZONTAL,
				};
				enum class KEY_BROADCAST {
					NONE = 0x00,
					GLOBAL = 0x01,
					CURRENT = 0x02,
					INACTIVE = 0x04,
					ALL = 0xFF,
				};
				DEFINE_ENUM_FLAG_OPERATORS(KEY_BROADCAST);
				struct ThumbMoveData {
					API::Global::Math::Space2D::Vector LeftMovement;
					API::Global::Math::Space2D::Vector RightMovement;
					API::Global::Math::Space2D::Point LeftResPos;
					API::Global::Math::Space2D::Point RightResPos;
				};

				struct KeyCode {
					uint64_t Data0;
					uint64_t Data1;
					uint64_t Data2;

					constexpr inline CLOAK_CALL KeyCode(In uint64_t d0, In uint64_t d1, In uint64_t d2) : Data0(d0), Data1(d1), Data2(d2) {}

					constexpr inline CLOAK_CALL KeyCode(In const KeyCode& c) : Data0(c.Data0), Data1(c.Data1), Data2(c.Data2) {}

					constexpr inline CLOAK_CALL KeyCode() : KeyCode(0, 0, 0) {}

					constexpr inline KeyCode& CLOAK_CALL_THIS operator=(In const KeyCode& c) {
						Data0 = c.Data0;
						Data1 = c.Data1;
						Data2 = c.Data2;
						return *this;
					}

					constexpr inline KeyCode& CLOAK_CALL_THIS operator|=(In const KeyCode& c) {
						Data0 |= c.Data0;
						Data1 |= c.Data1;
						Data2 |= c.Data2;
						return *this;
					}

					constexpr inline KeyCode& CLOAK_CALL_THIS operator&=(In const KeyCode& c) {
						Data0 &= c.Data0;
						Data1 &= c.Data1;
						Data2 &= c.Data2;
						return *this;
					}

					constexpr inline KeyCode& CLOAK_CALL_THIS operator^=(In const KeyCode& c) {
						Data0 ^= c.Data0;
						Data1 ^= c.Data1;
						Data2 ^= c.Data2;
						return *this;
					}

					constexpr inline KeyCode& CLOAK_CALL_THIS operator<<=(In const uint64_t c) {
						if (c != 0)
						{
							const uint64_t b = c & 0x3F;
							const uint64_t bi = 64 - b;
							CLOAK_ASSUME(b == c);
							Data2 = (Data2 << b) | (Data1 >> bi);
							Data1 = (Data1 << b) | (Data0 >> bi);
							Data0 = (Data0 << b);
						}
						return *this;
					}


					constexpr inline KeyCode& CLOAK_CALL_THIS operator>>=(In uint64_t c) {
						if (c != 0)
						{
							const uint64_t b = c & 0x3F;
							const uint64_t bi = 64 - b;
							CLOAK_ASSUME(b == c);
							Data0 = (Data0 >> b) | (Data1 << bi);
							Data1 = (Data1 >> b) | (Data2 << bi);
							Data2 = (Data2 >> b);
						}
						return *this;
					}


					constexpr inline KeyCode CLOAK_CALL_THIS operator|(In const KeyCode& c) const { return KeyCode(*this) |= c; }

					constexpr inline KeyCode CLOAK_CALL_THIS operator&(In const KeyCode& c) const { return KeyCode(*this) &= c; }

					constexpr inline KeyCode CLOAK_CALL_THIS operator^(In const KeyCode& c) const { return KeyCode(*this) ^= c; }

					constexpr inline KeyCode CLOAK_CALL_THIS operator<<(In uint64_t c) const { return KeyCode(*this) <<= c; }

					constexpr inline KeyCode CLOAK_CALL_THIS operator>>(In uint64_t c) const { return KeyCode(*this) >>= c; }

					constexpr inline KeyCode CLOAK_CALL_THIS operator~() const { return KeyCode(~Data0, ~Data1, ~Data2); }

					constexpr inline bool CLOAK_CALL_THIS operator==(In const KeyCode& c) const { return Data0 == c.Data0 && Data1 == c.Data1 && Data2 == c.Data2; }

					constexpr inline bool CLOAK_CALL_THIS operator!=(In const KeyCode& c) const { return !operator==(c); }
				};

				typedef std::function<void(bool, User)> KeyBindingFunc;
				typedef std::function<void(bool, User)> KeyPageFunc;
				CLOAK_INTERFACE IKeyBinding {
					public:
						virtual void CLOAK_CALL_THIS SetKeys(In size_t slot, In const KeyCode& keys) = 0;
						virtual const KeyCode& CLOAK_CALL_THIS GetKeys(In size_t slot) const = 0;
				};
				CLOAK_INTERFACE_ID("{858C1DF7-C432-4A32-9B61-65EDDEB158B3}") IKeyBindingPage : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual CE::RefPointer<IKeyBindingPage> CLOAK_CALL_THIS CreateNewPage(In_opt API::Global::Time timeout = 0, In_opt KeyPageFunc func = nullptr) = 0;
						virtual void CLOAK_CALL_THIS Enter(In User user) = 0;
						//if exact is set to true, no keys other than the required ones are allowed to be pressed. If false, there might be other keys pressed as well.
						virtual IKeyBinding* CLOAK_CALL_THIS CreateKeyBinding(In size_t slotCount, In bool exact, In KeyBindingFunc onKeyCode) = 0;
				};

				namespace KeyCodeGeneration {
					enum class KeyCodeLength {
						KEYBOARD = 0x80,
						MOUSE = 0x10,
						GAMEPAD = 0x20,
					};
					enum class KeyCodeType {
						KEYBOARD = 0,
						MOUSE = KEYBOARD + static_cast<size_t>(KeyCodeLength::KEYBOARD),
						GAMEPAD = MOUSE + static_cast<size_t>(KeyCodeLength::MOUSE),
					};
					constexpr CLOAK_FORCEINLINE KeyCode GenerateKeyCode(In KeyCodeType type, In uint8_t bitPos)
					{
						uint64_t Data[3] = { 0,0,0 };
						const uint32_t t = static_cast<uint32_t>(type) + bitPos;
						CLOAK_ASSUME(t < 192);
						const uint32_t a = t / 64;
						const uint32_t b = t % 64;
						Data[a] = 1Ui64 << b;
						return KeyCode(Data[0], Data[1], Data[2]);
					}
					constexpr CLOAK_FORCEINLINE KeyCode GenerateKeyMask(In KeyCodeType type)
					{
						uint64_t Data[3] = { 0,0,0 };
						const uint32_t a = static_cast<uint32_t>(type);
						KeyCodeLength bk = static_cast<KeyCodeLength>(0);
						switch (type)
						{
							case KeyCodeType::GAMEPAD:	bk = KeyCodeLength::GAMEPAD; break;
							case KeyCodeType::KEYBOARD:	bk = KeyCodeLength::KEYBOARD; break;
							case KeyCodeType::MOUSE:	bk = KeyCodeLength::MOUSE; break;
						}
						uint32_t b = static_cast<uint32_t>(bk);
						CLOAK_ASSUME(b != 0);
						const uint32_t e = a + b;
						const uint32_t begA = a >> 6;
						const uint32_t begB = a & 0x3F;
						const uint32_t endA = e >> 6;
						const uint32_t endB = e & 0x3F;
						CLOAK_ASSUME(begA < ARRAYSIZE(Data) && endA < ARRAYSIZE(Data));
						CLOAK_ASSUME(begA <= endA);
						CLOAK_ASSUME(begA < endA || begB < endB);
						for (uint32_t x = begA; x < endA; x++) { Data[x] = ~static_cast<uint64_t>(0); }
						Data[endA] = (1Ui64 << endB) - 1;
						Data[begA] &= ~((1Ui64 << begB) - 1);
						return KeyCode(Data[0], Data[1], Data[2]);
					}
				}

				namespace Keys {
					constexpr KeyCode NONE = KeyCode(0, 0, 0);
					namespace Masks {
						constexpr KeyCode MOUSE = KeyCodeGeneration::GenerateKeyMask(KeyCodeGeneration::KeyCodeType::MOUSE);
						constexpr KeyCode KEYBOARD = KeyCodeGeneration::GenerateKeyMask(KeyCodeGeneration::KeyCodeType::KEYBOARD);
						constexpr KeyCode GAMEPAD = KeyCodeGeneration::GenerateKeyMask(KeyCodeGeneration::KeyCodeType::GAMEPAD);
					}
					namespace Mouse {
						constexpr KeyCode LEFT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::MOUSE, 0x00);// 0x01
						constexpr KeyCode RIGHT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::MOUSE, 0x01);// 0x02
						constexpr KeyCode MIDDLE = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::MOUSE, 0x02);// 0x04
						constexpr KeyCode X1 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::MOUSE, 0x03);// 0x05
						constexpr KeyCode X2 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::MOUSE, 0x04);// 0x06
					}
					namespace Gamepad {
						constexpr KeyCode X = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x00);// 0x00
						constexpr KeyCode Y = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x01);// 0x01
						constexpr KeyCode A = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x02);// 0x02
						constexpr KeyCode B = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x03);// 0x03
						namespace DPad {
							constexpr KeyCode UP = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x04);// 0x04
							constexpr KeyCode DOWN = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x05);// 0x05
							constexpr KeyCode LEFT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x06);// 0x06
							constexpr KeyCode RIGHT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x07);// 0x07
						}
						constexpr KeyCode START = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x08);// 0x08
						constexpr KeyCode BACK = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x09);// 0x09
						namespace Thumb {
							constexpr KeyCode LEFT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x0A);// 0x0A
							constexpr KeyCode RIGHT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x0B);// 0x0B
						}
						namespace Shoulder {
							constexpr KeyCode LEFT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x0C);// 0x0C
							constexpr KeyCode RIGHT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x0D);// 0x0D
						}
						namespace Trigger {
							constexpr KeyCode LEFT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x0E);// 0x0E
							constexpr KeyCode RIGHT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x0F);// 0x0F
						}
						namespace Special {
							constexpr KeyCode LEFT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x10);// 0x10
							constexpr KeyCode RIGHT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::GAMEPAD, 0x11);// 0x11
						}
					}
					namespace Keyboard {
						constexpr KeyCode BACKSPACE = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x00);// 0x08
						constexpr KeyCode TAB = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x01);// 0x09
						constexpr KeyCode CLEAR = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x02);// 0x0C
						constexpr KeyCode RETURN = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x03);// 0x0D
						constexpr KeyCode PAUSE = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x04);// 0x13
						constexpr KeyCode CAPSLOCK = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x05);// 0x14
						constexpr KeyCode ESCAPE = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x06);// 0x1B
						constexpr KeyCode SPACE = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x07);// 0x20
						constexpr KeyCode PAGE_UP = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x08);// 0x21
						constexpr KeyCode PAGE_DOWN = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x09);// 0x22
						constexpr KeyCode END = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x0A);// 0x23
						constexpr KeyCode HOME = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x0B);// 0x24
						namespace Arrow {
							constexpr KeyCode LEFT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x0C);// 0x25
							constexpr KeyCode UP = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x0D);// 0x26
							constexpr KeyCode RIGHT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x0E);// 0x27
							constexpr KeyCode DOWN = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x0F);// 0x28
						}
						constexpr KeyCode SELECT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x10);// 0x29
						constexpr KeyCode PRINT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x11);// 0x2A
						constexpr KeyCode EXECUTE = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x12);// 0x2B
						constexpr KeyCode SNAPSHOT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x13);// 0x2C
						constexpr KeyCode INSERT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x14);// 0x2D
						constexpr KeyCode DELETE = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x15);// 0x2E
						constexpr KeyCode HELP = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x16);// 0x2F
						constexpr KeyCode KEY_0 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x17);// 0x30
						constexpr KeyCode KEY_1 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x18);// 0x31
						constexpr KeyCode KEY_2 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x19);// 0x32
						constexpr KeyCode KEY_3 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x1A);// 0x33
						constexpr KeyCode KEY_4 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x1B);// 0x34
						constexpr KeyCode KEY_5 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x1C);// 0x35
						constexpr KeyCode KEY_6 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x1D);// 0x36
						constexpr KeyCode KEY_7 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x1E);// 0x37
						constexpr KeyCode KEY_8 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x1F);// 0x38
						constexpr KeyCode KEY_9 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x20);// 0x39
						constexpr KeyCode A = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x21);// 0x41
						constexpr KeyCode B = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x22);// 0x42
						constexpr KeyCode C = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x23);// 0x43
						constexpr KeyCode D = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x24);// 0x44
						constexpr KeyCode E = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x25);// 0x45
						constexpr KeyCode F = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x26);// 0x46
						constexpr KeyCode G = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x27);// 0x47
						constexpr KeyCode H = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x28);// 0x48
						constexpr KeyCode I = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x29);// 0x49
						constexpr KeyCode J = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x2A);// 0x4A
						constexpr KeyCode K = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x2B);// 0x4B
						constexpr KeyCode L = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x2C);// 0x4C
						constexpr KeyCode M = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x2D);// 0x4D
						constexpr KeyCode N = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x2E);// 0x4E
						constexpr KeyCode O = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x2F);// 0x4F
						constexpr KeyCode P = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x30);// 0x50
						constexpr KeyCode Q = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x31);// 0x51
						constexpr KeyCode R = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x32);// 0x52
						constexpr KeyCode S = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x33);// 0x53
						constexpr KeyCode T = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x34);// 0x54
						constexpr KeyCode U = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x35);// 0x55
						constexpr KeyCode V = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x36);// 0x56
						constexpr KeyCode W = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x37);// 0x57
						constexpr KeyCode X = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x38);// 0x58
						constexpr KeyCode Y = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x39);// 0x59
						constexpr KeyCode Z = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x3A);// 0x5A
						constexpr KeyCode WIN_LEFT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x3B);// 0x5B
						constexpr KeyCode WIN_RIGHT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x3C);// 0x5C
						constexpr KeyCode WIN = WIN_LEFT | WIN_RIGHT;
						constexpr KeyCode APPS = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x3D);// 0x5D
						constexpr KeyCode SLEEP = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x3E);// 0x5F
						constexpr KeyCode MULTIPLY = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x3F);// 0x6A
						constexpr KeyCode ADD = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x40);// 0x6B
						constexpr KeyCode SEPARATOR = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x41);// 0x6C
						constexpr KeyCode SUBTRACT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x42);// 0x6D
						constexpr KeyCode DECIMAL = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x43);// 0x6E
						constexpr KeyCode DIVIDE = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x44);// 0x6F
						constexpr KeyCode F1 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x45);// 0x70
						constexpr KeyCode F2 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x46);// 0x71
						constexpr KeyCode F3 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x47);// 0x72
						constexpr KeyCode F4 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x48);// 0x73
						constexpr KeyCode F5 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x49);// 0x74
						constexpr KeyCode F6 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x4A);// 0x75
						constexpr KeyCode F7 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x4B);// 0x76
						constexpr KeyCode F8 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x4C);// 0x77
						constexpr KeyCode F9 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x4D);// 0x78
						constexpr KeyCode F10 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x4E);// 0x79
						constexpr KeyCode F11 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x4F);// 0x7A
						constexpr KeyCode F12 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x50);// 0x7B
						constexpr KeyCode F13 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x51);// 0x7C
						constexpr KeyCode F14 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x52);// 0x7D
						constexpr KeyCode F15 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x53);// 0x7E
						constexpr KeyCode F16 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x54);// 0x7F
						constexpr KeyCode F17 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x55);// 0x80
						constexpr KeyCode F18 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x56);// 0x81
						constexpr KeyCode F19 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x57);// 0x82
						constexpr KeyCode F20 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x58);// 0x83
						constexpr KeyCode F21 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x59);// 0x84
						constexpr KeyCode F22 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x5A);// 0x85
						constexpr KeyCode F23 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x5B);// 0x86
						constexpr KeyCode F24 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x5C);// 0x87
						constexpr KeyCode NUMLOCK = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x5D);// 0x90
						constexpr KeyCode SCROLL = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x5E);// 0x91
						constexpr KeyCode SHIFT_LEFT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x5F);// 0xA0
						constexpr KeyCode SHIFT_RIGHT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x60);// 0xA1
						constexpr KeyCode CONTROL_LEFT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x61);// 0xA2
						constexpr KeyCode CONTROL_RIGHT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x62);// 0xA3
						constexpr KeyCode ALT_LEFT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x63);// 0xA4
						constexpr KeyCode ALT_RIGHT = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x64);// 0xA5
						constexpr KeyCode SHIFT = SHIFT_LEFT | SHIFT_RIGHT;
						constexpr KeyCode CONTROL = CONTROL_LEFT | CONTROL_RIGHT;
						constexpr KeyCode ALT = ALT_LEFT | ALT_RIGHT;
						constexpr KeyCode OEM1 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x65);// 0xBA
						constexpr KeyCode OEM2 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x66);// 0xBF
						constexpr KeyCode OEM3 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x67);// 0xC0
						constexpr KeyCode OEM4 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x68);// 0xDB
						constexpr KeyCode OEM5 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x69);// 0xDC
						constexpr KeyCode OEM6 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x6A);// 0xDD
						constexpr KeyCode OEM7 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x6B);// 0xDE
						constexpr KeyCode OEM8 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x6C);// 0xDF
						constexpr KeyCode OEM102 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x6D);// 0xE2
						constexpr KeyCode OEM_PLUS = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x6E);// 0xBB
						constexpr KeyCode OEM_COMMA = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x6F);// 0xBC
						constexpr KeyCode OEM_MINUS = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x70);// 0xBD
						constexpr KeyCode OEM_PERIOD = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x71);// 0xBE
						constexpr KeyCode OEM_CLEAR = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x72);// 0xFE

						namespace Numpad {
							constexpr KeyCode KEY_0 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x73);// 0x60
							constexpr KeyCode KEY_1 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x74);// 0x61
							constexpr KeyCode KEY_2 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x75);// 0x62
							constexpr KeyCode KEY_3 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x76);// 0x63
							constexpr KeyCode KEY_4 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x77);// 0x64
							constexpr KeyCode KEY_5 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x78);// 0x65
							constexpr KeyCode KEY_6 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x79);// 0x66
							constexpr KeyCode KEY_7 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x7A);// 0x67
							constexpr KeyCode KEY_8 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x7B);// 0x68
							constexpr KeyCode KEY_9 = KeyCodeGeneration::GenerateKeyCode(KeyCodeGeneration::KeyCodeType::KEYBOARD, 0x7C);// 0x69
						}
					}
				}

				CLOAKENGINE_API void CLOAK_CALL SetLeftVibration(In User user, In float power);
				CLOAKENGINE_API void CLOAK_CALL SetRightVibration(In User user, In float power);
				CLOAKENGINE_API void CLOAK_CALL SetVibration(In User user, In float left, In float right);
				CLOAKENGINE_API void CLOAK_CALL SetBroadcastMode(In KEY_BROADCAST mode);
				CLOAKENGINE_API IKeyBinding* CLOAK_CALL CreateGlobalKeyBinding(In size_t slotCount, In bool exact, In KeyBindingFunc onKeyCode);
				CLOAKENGINE_API Debug::Error CLOAK_CALL GetBasePage(In REFIID riid, Outptr void** ptr);
			}
		}
	}
}

#endif
