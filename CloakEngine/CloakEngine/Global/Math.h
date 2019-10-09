#pragma once
#ifndef CE_API_GLOBAL_MATH_H
#define CE_API_GLOBAL_MATH_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/OS.h"
#include "CloakEngine/Global/Memory.h"
#include "CloakEngine/Helper/Predicate.h"
#include "CloakEngine/Helper/Types.h"

#include <type_traits>
#include <intrin.h>
#include <limits>
#include <random>

//Popcount ASM support check:
#if (CHECK_ARCHITECTURE(x86) && (defined(__cplusplus) || CHECK_COMPILER(MicrosoftVisualCpp) || CHECK_COMPILER_VERSION(GCC,4,2,0) || CE_HAS_BUILTIN(__sync_val_compare_and_swap)))
#	define HAS_CPUID
#endif
#if (CHECK_COMPILER_VERSION(GCC,4,2,0) || CE_HAS_BUILTIN(__builtin_popcount))
#	define HAS_POPCOUNT_BUILTIN
#endif
#if (CHECK_COMPILER_VERSION(GCC,4,2,0) || CHECK_COMPILER_VERSION(Clang,3,0,0))
#	define HAS_POPCOUNT_ASM
#endif
#if (defined(HAS_CPUID) && (defined(HAS_POPCOUNT_ASM) || CHECK_COMPILER(MicrosoftVisualCpp)))
#	define HAS_POPCNT
#endif
#if (CHECK_COMPILER(MicrosoftVisualCpp) || (CHECK_COMPILER(Clang) && defined(__c2__)))
#	pragma intrinsic(_BitScanForward, _BitScanReverse)
#	ifdef X64
#		pragma intrinsic(_BitScanForward64, _BitScanReverse64)
#	endif
#endif

#ifdef min
#	pragma push_macro("min")
#	undef min
#	define POP_MACRO_MIN
#endif
#ifdef max
#	pragma push_macro("max")
#	undef max
#	define POP_MACRO_MAX
#endif

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Math {
				constexpr long double PI	= 3.141592653589793238L;
				//Tau equals 2 PI or 360°
				constexpr long double Tau	= 6.283185307179586477L;
				constexpr long double E		= 2.718281828459045235L;

				namespace CompileTime {
					//Calculates the absolute (non-negative) value of x. Returns an unsigned type, if aviable
					template<typename T, typename std::enable_if<std::is_floating_point<T>::value || std::is_unsigned<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL abs(In T x) { return x < 0 ? -x : x; }
					template<typename T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE std::make_unsigned_t<T> CLOAK_CALL abs(In T x) { return static_cast<std::make_unsigned_t<T>>(x < 0 ? -x : x); }
					//Calculates the min/max values of a and b
					template<typename T, typename std::enable_if<std::is_floating_point<T>::value || std::is_unsigned<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL min(In T a, In T b) { return a < b ? a : b; }
					template<typename T, typename std::enable_if<std::is_floating_point<T>::value || std::is_unsigned<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL max(In T a, In T b) { return a < b ? b : a; }
					// Calculates the factorial (= x!)
					template<typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr> constexpr T CLOAK_CALL Factorial(In T x)
					{
						if constexpr (std::is_signed<T>::value) { if (x < 0) { return 0; } }
						T r = static_cast<T>(1);
						for (T a = static_cast<T>(2); a <= x; a++) { r *= a; }
						return r;
					}
					//Rounds up to next power of two
					template<typename T, typename std::enable_if<std::is_integral<T>::value&& std::is_unsigned<T>::value>::type* = nullptr> constexpr T CLOAK_CALL CeilPower2(In T x)
					{
						if (x == 0) { return 0; }
						x -= 1;
						x |= x >> 1;
						x |= x >> 2;
						x |= x >> 4;
						if constexpr (sizeof(T) > 1) { x |= x >> 8; }
						if constexpr (sizeof(T) > 2) { x |= x >> 16; }
						if constexpr (sizeof(T) > 4) { x |= x >> 32; }
						return x + 1;
					}
					//Rotates all bits of the given value 'shift' bits towards the left (most significant) bit, warping bits around
					constexpr CLOAK_FORCEINLINE unsigned long CLOAK_CALL rotateLeft(In unsigned long value, In int shift)
					{
						if (shift < 0)
						{
							return (value >> -shift) | (value << ((sizeof(unsigned long) << 3) + shift));
						}
						return (value << shift) | (value >> ((sizeof(unsigned long) << 3) - shift));
					}
					//Rotates all bits of the given value 'shift' bits towards the right (least significant) bit, warping bits around
					constexpr CLOAK_FORCEINLINE unsigned long CLOAK_CALL rotateRight(In unsigned long value, In int shift)
					{
						return rotateLeft(value, -shift);
					}
					//Calculates the number of set bits
					constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL popcount(In uint64_t x)
					{
						x -= (x >> 1) & 0x5555555555555555Ui64;
						x = (x & 0x3333333333333333Ui64) + ((x >> 2) & 0x3333333333333333Ui64);
						x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FUi64;
						return static_cast<uint8_t>((x * 0x0101010101010101Ui64) >> 56);
					}
					//Calculates index of first set bit (or 0xFF if no bit is set)
					constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanForward(In uint32_t mask)
					{
						if (mask == 0) { return 0xFF; }
						uint8_t r = 0;
						while ((mask & 0x1) == 0)
						{
							mask >>= 1;
							r++;
						}
						return r;
					}
					constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanForward(In uint64_t mask)
					{
						if (mask == 0) { return 0xFF; }
						uint8_t r = 0;
						while ((mask & 0x1) == 0)
						{
							mask >>= 1;
							r++;
						}
						return r;
					}
					template<typename T, typename std::enable_if<sizeof(T) <= sizeof(uint64_t)>::type* = nullptr> constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanForward(In T x)
					{
						typedef typename type_if<sizeof(T) <= sizeof(uint32_t), uint32_t, uint64_t>::type C;
						return bitScanForward(static_cast<C>(x));
					}

					//Calculates index of last set bit (or 0xFF if no bit is set)
					constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanReverse(In uint32_t mask)
					{
						if (mask == 0) { return 0xFF; }
						uint8_t r = 0;
						mask >>= 1;
						while (mask != 0)
						{
							mask >>= 1;
							r++;
						}
						return r;
						
					}
					constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanReverse(In uint64_t mask)
					{
						if (mask == 0) { return 0xFF; }
						uint8_t r = 0;
						mask >>= 1;
						while (mask != 0)
						{
							mask >>= 1;
							r++;
						}
						return r;
					}
					template<typename T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanReverse(In T x)
					{
						typedef typename type_if<sizeof(T) <= sizeof(uint32_t), uint32_t, uint64_t>::type C;
						return bitScanReverse(static_cast<C>(x));
					}

					//Calculates the greated common devisor
					template<typename T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL gcd(In T a, In T b)
					{
						if (a == 0) { return b; }
						else if (b == 0) { return a; }
						while (true)
						{
							if (a > b)
							{
								a = a % b;
								if (a == 0) { return b; }
								else if (a == 1) { return 1; }
							}
							else
							{
								b = b % a;
								if (b == 0) { return a; }
								else if (b == 1) { return 1; }
							}
						}
						CLOAK_ASSUME(false);
						return 1;
					}

					//Calculates the square root of x
					template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL sqrt(In T x)
					{
						if constexpr (std::is_floating_point<T>::value)
						{
							if (x < 0 || x != x) { return std::numeric_limits<T>::quiet_NaN(); }
							if (x >= std::numeric_limits<T>::infinity()) { return std::numeric_limits<T>::infinity(); }
							T c = x; //Initial guess
							T p = 0;
							while (abs(c - p) > std::numeric_limits<T>::epsilon())
							{
								p = c;
								c = (c + (x / c))*static_cast<T>(0.5); //Newton approximation
							}
							return c;
						}
						else
						{
							T c = x; //Initial guess
							T p = 0;
							T d = 0; //Prevent hopping between two values
							while (c != p && c != d)
							{
								d = p;
								p = c;
								c = (c + (x / c)) >> 1;
							}
							//Select closest result:
							const T cs = c * c;
							const T cd = cs > x ? (cs - x) : (x - cs);
							const T n = cs > x ? (c - 1) : (c + 1);
							const T ns = n * n;
							const T nd = ns > x ? (ns - x) : (x - ns);
							return nd > cd ? c : n;
						}
					}

					//Calculates a to the power of b, where a must be positive and b must be a whole number
					template<typename A, typename B, typename std::enable_if<std::is_integral<B>::value && (std::is_integral<A>::value || std::is_floating_point<A>::value)>::type* = nullptr> constexpr CLOAK_FORCEINLINE A CLOAK_CALL pow(In A a, In B b)
					{
						if constexpr (std::is_floating_point<A>::value)
						{
							if (a < 0 || a != a || (a == 0 && b == 0)) { return std::numeric_limits<A>::quiet_NaN(); }
							if (a >= std::numeric_limits<A>::infinity()) { return std::numeric_limits<A>::quiet_NaN(); }
							if (b < 0) { return 1 / pow(a, -b); }
							A x = 1;
							while (b > 0)
							{
								if ((b & 0x1) != 0) { x *= a; }
								a *= a;
								b >>= 1;
							}
							return x;
						}
						else
						{
							if (a <= 0 || b < 0) { return 0; }
							A x = 1;
							while (b > 0)
							{
								if ((b & 0x1) != 0) { x *= a; }
								a *= a;
								b >>= 1;
							}
							return x;
						}
					}
					//Calculates E to the power of x
					template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL exp(In T x)
					{
						if (std::numeric_limits<T>::epsilon() > abs(x)) { return static_cast<T>(1); }
						T fact = static_cast<T>(1);
						T frac = x;
						if (abs(x) >= static_cast<T>(2))
						{
							const uint64_t fl = static_cast<uint64_t>(x);
							const uint64_t sgn = (x > static_cast<T>(0) ? 1 : (x < static_cast<T>(0) ? -1 : 0));
							const uint64_t whole = abs(x - static_cast<T>(fl)) > static_cast<T>(0.5) ? (fl + sgn) : fl;
							fact = pow(static_cast<T>(E), whole);
							frac = x - static_cast<T>(whole);
						}
						T lr = static_cast<T>(1);
						for (uint64_t d = 200; d > 1; d--)
						{
							lr = static_cast<T>(1) + (frac / static_cast<T>(d - 1)) - (frac / (static_cast<T>(d)*lr));
						}
						return fact / (static_cast<T>(1) - (frac / lr));
					}

					//Trigonometric functions:
					template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL atan(In T x)
					{
						const T ra = x / (static_cast<T>(1) + (x*x));
						const T x2 = x * x;
						T sum = 1;
						uint64_t n = 1;
						while (true)
						{
							T prod = 1;
							for (uint64_t k = n; k > 0; k--)
							{
								T term = (static_cast<T>(k << 1)*x2) / ((static_cast<T>(k << 1) + static_cast<T>(1))*(static_cast<T>(1) + x2));
								prod *= term;
							}
							if (sum + prod == sum) { break; }
							sum += prod;
							n++;
						}
						return ra * sum;
					}
					template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL asin(In T x)
					{
						T i = abs(x);
						if (x != x || i > static_cast<T>(1)) { return std::numeric_limits<T>::quiet_NaN(); }
						T r = 0;
						if (std::numeric_limits<T>::epsilon() > abs(i - static_cast<T>(1))) { r = static_cast<T>(PI / 2); }
						else if (std::numeric_limits<T>::epsilon() > i) { r = 0; }
						else { r = atan(i / sqrt(static_cast<T>(1) - (i*i))); }
						return x < 0 ? -r : r;
					}
					template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL acos(In T x)
					{
						T i = abs(x);
						if (x != x || i > static_cast<T>(1)) { return std::numeric_limits<T>::quiet_NaN(); }
						T r = 0;
						if (std::numeric_limits<T>::epsilon() > abs(i - static_cast<T>(1))) { r = 0; }
						else if (std::numeric_limits<T>::epsilon() > i) { r = static_cast<T>(PI / 2); }
						else { r = atan(sqrt(static_cast<T>(1) - (i*i)) / i); }
						return x < 0 ? -r : r;
					}
					template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL tan(In T x)
					{
						if (std::numeric_limits<T>::epsilon() > abs(x)) { return 0; }
						T i = abs(x);
						uint64_t count = 0;
						while (i > static_cast<T>(PI))
						{
							if (count > 1)
							{
								return x > 0 ? std::numeric_limits<T>::quiet_NaN() : -std::numeric_limits<T>::quiet_NaN();
							}
							i = i - static_cast<T>(static_cast<T>(PI) * static_cast<uint64_t>(i / static_cast<T>(PI)));
							count++;
						}

						if (x > static_cast<T>(1.55) && x < static_cast<T>(1.60))
						{
							if (abs(x - static_cast<T>(PI / 2)) < std::numeric_limits<T>::epsilon()) { return static_cast<T>(PI / 2); }
							const T z = x - static_cast<T>(PI / 2);
							return -(1 / z) + ((z / 3) + ((pow(z, 3) / 45) + ((2 * pow(z, 5) / 945) + (pow(z, 7) / 4725))));
						}
						const uint64_t md = i > static_cast<T>(1.4) ? 45 : (i > static_cast<T>(1) ? 35 : 25);
						const T ii = i * i;
						T lr = static_cast<T>((2 * md) - 1);
						for (uint64_t d = md - 1; d > 0; d--) { lr = static_cast<T>((2 * d) - (1 + (ii / lr))); }
						return (x > 0 ? i : -i) / lr;
					}
					template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL sin(In T x)
					{
						if (std::numeric_limits<T>::epsilon() > abs(x)) { return 0; }
						const T i = tan(x / static_cast<T>(2));
						if (abs(i) == std::numeric_limits<T>::infinity()) { return 0; }
						return (static_cast<T>(2)*i) / (static_cast<T>(1) + (i*i));
					}
					template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> constexpr CLOAK_FORCEINLINE T CLOAK_CALL cos(In T x)
					{
						if (std::numeric_limits<T>::epsilon() > abs(x)) { return static_cast<T>(1); }
						const T i = tan(x / static_cast<T>(2));
						if (abs(i) == std::numeric_limits<T>::infinity()) { return -1; }
						return (static_cast<T>(1) - (i*i)) / (static_cast<T>(1) + (i*i));
					}
				}
				//Rounds up to next power of two
				template<typename T, typename std::enable_if<std::is_integral<T>::value&& std::is_unsigned<T>::value>::type* = nullptr> constexpr T CLOAK_CALL CeilPower2(In T x)
				{
					return CompileTime::CeilPower2(x);
				}
				//Rotates all bits of the given value 'shift' bits towards the left (most significant) bit, warping bits around
				opt_in_constexpr CLOAK_FORCEINLINE unsigned long CLOAK_CALL rotateLeft(In unsigned long value, int shift)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::rotateLeft(value, shift);
					}
					else
					{
						return _lrotl(value, shift);
					}
				}
				//Rotates all bits of the given value 'shift' bits towards the right (least significant) bit, warping bits around
				opt_in_constexpr CLOAK_FORCEINLINE unsigned long CLOAK_CALL rotateRight(In unsigned long value, int shift)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::rotateRight(value, shift);
					}
					else
					{
						return _lrotr(value, shift);
					}
				}
				//Calculates the absolute (non-negative) value of x. Returns an unsigned type, if aviable
				template<typename T, typename std::enable_if<std::is_floating_point<T>::value || std::is_unsigned<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL abs(In T x) { return x < 0 ? -x : x; }
				template<typename T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE std::make_unsigned_t<T> CLOAK_CALL abs(In T x) { return static_cast<std::make_unsigned_t<T>>(x < 0 ? -x : x); }
				//Calculates the min/max values of a and b
				template<typename T, typename std::enable_if<std::is_floating_point<T>::value || std::is_unsigned<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL min(In T a, In T b) { return a < b ? a : b; }
				template<typename T, typename std::enable_if<std::is_floating_point<T>::value || std::is_unsigned<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL max(In T a, In T b) { return a < b ? b : a; }
				//Calculates the number of set bits
				opt_in_constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL popcount(In uint64_t x)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::popcount(x);
					}
					else
					{
#if (defined(HAS_POPCOUNT_ASM) && (defined(X64) || CHECK_ARCHITECTURE(x86)))
#	if defined(X64)
						__asm__("popcnt %1, %0" : "=r" (x) : "0" (x));
						return static_cast<uint8_t>(x);
#	elif CHECK_ARCHITECTURE(x86)
						uint32_t a = static_cast<uint32_t>(x);
						uint32_t b = static_cast<uint32_t>(x >> 32);
						__asm__("popcnt %1, %0" : "=r" (a) : "0" (a));
						__asm__("popcnt %1, %0" : "=r" (b) : "0" (b));
						return static_cast<uint8_t>(a + b);
#	endif
#elif (CHECK_COMPILER(MicrosoftVisualCpp) && (defined(X64) || CHECK_ARCHITECTURE(x86)))
#	if defined(X64)
						return static_cast<uint8_t>(_mm_popcnt_u64(x));
#	else
						return static_cast<uint8_t>(_mm_popcnt_u32(static_cast<uint32_t>(x)) + _mm_popcnt_u32(static_cast<uint32_t>(x >> 32)));
#	endif
#elif defined(HAS_POPCOUNT_BUILTIN)
						return static_cast<uint8_t>(__builtin_popcountll(x));
#else
						x -= (x >> 1) & 0x5555555555555555Ui64;
						x = (x & 0x3333333333333333Ui64) + ((x >> 2) & 0x3333333333333333Ui64);
						x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FUi64;
						return static_cast<uint8_t>((x * 0x0101010101010101Ui64) >> 56);
#endif
					}
				}
				//Calculates index of first (lowest) set bit (or 0xFF if no bit is set)
				opt_in_constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanForward(In uint32_t mask)
				{
					if (mask == 0) { return 0xFF; }
					if constexpr (in_constexpr())
					{
						return CompileTime::bitScanForward(mask);
					}
					else
					{
#if (CHECK_COMPILER(MicrosoftVisualCpp) || (CHECK_COMPILER(Clang) && defined(__c2__)))
						DWORD r = 0;
						static_assert(sizeof(DWORD) >= sizeof(mask),"Invalid type");
						_BitScanForward(&r, mask);
						return static_cast<uint8_t>(r);
#elif (CHECK_COMPILER(GCC) || CHECK_COMPILER(Clang))
						return static_cast<uint8_t>(__builtin_ctz(mask));
#else
						uint8_t r = 0;
						while ((mask & 0x1) == 0)
						{
							mask >>= 1;
							r++;
						}
						return r;
#endif
					}
				}
				opt_in_constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanForward(In uint64_t mask)
				{
					if (mask == 0) { return 0xFF; }
					if constexpr (in_constexpr())
					{
						return CompileTime::bitScanForward(mask);
					}
					else
					{
#if (CHECK_COMPILER(MicrosoftVisualCpp) || (CHECK_COMPILER(Clang) && defined(__c2__)))
#	ifdef X64
						DWORD r = 0;
						_BitScanForward64(&r, mask);
						return static_cast<uint8_t>(r);
#	else
						DWORD r = 0;
						auto c = _BitScanForward(&r, static_cast<DWORD>(mask & 0xFFFFFFFF));
						if (c == 0)
						{
							_BitScanForward(&r, static_cast<DWORD>(mask >> 32));
							r += 32;
						}
						return static_cast<uint8_t>(r);
#	endif
#elif (CHECK_COMPILER(GCC) || CHECK_COMPILER(Clang))
						return static_cast<uint8_t>(__builtin_ctzll(mask));
#else
						uint8_t r = 0;
						while ((mask & 0x1) == 0)
						{
							mask >>= 1;
							r++;
						}
						return r;
#endif
					}
				}
				template<typename T, typename std::enable_if<sizeof(T) <= sizeof(uint64_t)>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanForward(In T x)
				{
					typedef typename type_if<sizeof(T) <= sizeof(uint32_t), uint32_t, uint64_t>::type C;
					return bitScanForward(static_cast<C>(x));
				}

				//Calculates index of last (highest) set bit (or 0xFF if no bit is set)
				opt_in_constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanReverse(In uint32_t mask)
				{
					if (mask == 0) { return 0xFF; }
					if constexpr (in_constexpr())
					{
						return CompileTime::bitScanReverse(mask);
					}
					else
					{
#if (CHECK_COMPILER(MicrosoftVisualCpp) || (CHECK_COMPILER(Clang) && defined(__c2__)))
						DWORD r = 0;
						static_assert(sizeof(DWORD) >= sizeof(mask), "Invalid type");
						_BitScanReverse(&r, mask);
						return static_cast<uint8_t>(r);
#elif (CHECK_COMPILER(GCC) || CHECK_COMPILER(Clang))
						return 31 - static_cast<uint8_t>(__builtin_clz(mask));
#else
						uint8_t r = 0;
						mask >>= 1;
						while (mask != 0)
						{
							mask >>= 1;
							r++;
						}
						return r;
#endif
					}
				}
				opt_in_constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanReverse(In uint64_t mask)
				{
					if (mask == 0) { return 0xFF; }
					if constexpr (in_constexpr())
					{
						return CompileTime::bitScanReverse(mask);
					}
					else
					{
#if (CHECK_COMPILER(MicrosoftVisualCpp) || (CHECK_COMPILER(Clang) && defined(__c2__)))
#	ifdef X64
						DWORD r = 0;
						_BitScanReverse64(&r, mask);
						return static_cast<uint8_t>(r);
#	else
						DWORD r = 0;
						auto c = _BitScanReverse(&r, static_cast<DWORD>(mask & 0xFFFFFFFF));
						if (c == 0)
						{
							_BitScanReverse(&r, static_cast<DWORD>(mask >> 32));
							r += 32;
						}
						return static_cast<uint8_t>(r);
#	endif
#elif (CHECK_COMPILER(GCC) || CHECK_COMPILER(Clang))
						return 63 - static_cast<uint8_t>(__builtin_clzll(mask));
#else
						uint8_t r = 0;
						mask >>= 1;
						while (mask != 0)
						{
							mask >>= 1;
							r++;
						}
						return r;
#endif
					}
				}
				template<typename T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE uint8_t CLOAK_CALL bitScanReverse(In T x)
				{
					typedef typename type_if<sizeof(T) <= sizeof(uint32_t), uint32_t, uint64_t>::type C;
					return bitScanReverse(static_cast<C>(x));
				}

				//Calculates the greated common devisor
				template<typename T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL gcd(In T a, In T b)
				{
					return CompileTime::gcd(a, b);
				}

				//Calculates the square root of x
				template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL sqrt(In T x)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::sqrt(x);
					}
					else
					{
						return std::sqrt(x);
					}
				}

				//Calculates a to the power of b, where a must be positive and b must be a whole number
				template<typename A, typename B, typename std::enable_if<std::is_integral<B>::value && (std::is_integral<A>::value || std::is_floating_point<A>::value)>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE A CLOAK_CALL pow(In A a, In B b)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::pow(a, b);
					}
					else
					{
						return std::pow(a, b);
					}
				}
				//Calculates E to the power of x
				template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL exp(In T x)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::exp(x);
					}
					else
					{
						return std::exp(x);
					}
				}

				//Trigonometric functions:
				template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL atan(In T x)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::atan(x);
					}
					else
					{
						return std::atan(x);
					}
				}
				template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL asin(In T x)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::asin(x);
					}
					else
					{
						return std::asin(x);
					}
				}
				template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL acos(In T x)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::acos(x);
					}
					else
					{
						return std::acos(x);
					}
				}
				template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL tan(In T x)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::tan(x);
					}
					else
					{
						return std::tan(x);
					}
				}
				template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL sin(In T x)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::sin(x);
					}
					else
					{
						return std::sin(x);
					}
				}
				template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> opt_in_constexpr CLOAK_FORCEINLINE T CLOAK_CALL cos(In T x)
				{
					if constexpr (in_constexpr())
					{
						return CompileTime::cos(x);
					}
					else
					{
						return std::cos(x);
					}
				}

				class CLOAKENGINE_API PerlinNoise {
					public:
						CLOAK_CALL PerlinNoise();
						CLOAK_CALL PerlinNoise(In const std::seed_seq& seed);
						CLOAK_CALL PerlinNoise(In const unsigned long long seed);
						float CLOAK_CALL_THIS Generate(In float x, In_opt float y = 0, In_opt float z = 0);
						float CLOAK_CALL_THIS operator()(In float x, In_opt float y = 0, In_opt float z = 0);
					private:
						uint8_t m_P[256];
				};
				//Prime noise uses an simple, fast noise without uniform distribution or any other mathematical reasoning, based on the first few prime numbers
				class CLOAKENGINE_API PrimeNoise {
					public:
						CLOAK_CALL PrimeNoise();
						CLOAK_CALL PrimeNoise(In const std::seed_seq& seed);
						CLOAK_CALL PrimeNoise(In const unsigned long long seed);
						float CLOAK_CALL_THIS Generate(In float x);
						float CLOAK_CALL_THIS operator()(In float x);
						float CLOAK_CALL_THIS Generate(In float x, In float y);
						float CLOAK_CALL_THIS operator()(In float x, In float y);
						float CLOAK_CALL_THIS Generate(In float x, In float y, In float z);
						float CLOAK_CALL_THIS operator()(In float x, In float y, In float z);
					private:
						float m_W[48];
				};

				struct Point;
				struct IntPoint;
				struct WorldPosition;
				struct IntVector;
				struct Vector;
				struct Matrix;
				struct Quaternion;
				struct DualQuaternion;

				enum class RotationOrder { XYZ, XZY, YXZ, YZX, ZXY, ZYX };

				struct CLOAK_ALIGN Plane {
					__m128 Data;
					CLOAKENGINE_API CLOAK_CALL Plane();
					CLOAKENGINE_API CLOAK_CALL Plane(In const __m128& d);
					CLOAKENGINE_API CLOAK_CALL Plane(In const Plane& p);
					CLOAKENGINE_API CLOAK_CALL Plane(In const Vector& normal, In float d);
					CLOAKENGINE_API CLOAK_CALL Plane(In const Vector& normal, In const Vector& point);
					CLOAKENGINE_API CLOAK_CALL Plane(In const Vector& p1, In const Vector& p2, In const Vector& p3);
					CLOAKENGINE_API Plane& CLOAK_CALL_THIS operator=(In const Plane& p);
					CLOAKENGINE_API float CLOAK_CALL_THIS SignedDistance(In const Vector& point) const;
					CLOAKENGINE_API Vector CLOAK_CALL_THIS GetNormal() const;

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);
				};
				struct CLOAK_ALIGN Frustum {
					/*
					Order:
					0 = Left
					1 = Right
					2 = Top
					3 = Bottom
					4 = Near
					5 = Far

					Normals are pointing outwards
					*/
					Plane Data[6];
					//Input should be WorldToProjection Matrix
					CLOAKENGINE_API CLOAK_CALL Frustum();
					CLOAKENGINE_API CLOAK_CALL Frustum(In const Matrix& m);
					CLOAKENGINE_API CLOAK_CALL Frustum(In const Frustum& f);
					//Input should be WorldToProjection Matrix
					CLOAKENGINE_API Frustum& CLOAK_CALL_THIS operator=(In const Matrix& m);
					CLOAKENGINE_API Frustum& CLOAK_CALL_THIS operator=(In const Frustum& f);
					CLOAKENGINE_API const Plane& CLOAK_CALL_THIS operator[](In size_t p) const;

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);
				};
				struct CLOAK_ALIGN OctantFrustum {
					Frustum Base;
					/*
					0 = Left\Right (Positive = Left)
					1 = Top/Down (Positive = Top)
					2 = Near/Far (Positive = Near)
					*/
					Plane Octants[3];
					float CenterDistance;

					static constexpr size_t OCTANT_PLANES[8][3] = {
						{ 4,2,0 },// Near	Top		Left
						{ 4,2,1 },// Near	Top		Right
						{ 4,3,0 },// Near	Bottom	Left
						{ 4,3,1 },// Near	Bottom	Right
						{ 5,2,0 },// Far	Top		Left
						{ 5,2,1 },// Far	Top		Right
						{ 5,3,0 },// Far	Bottom	Left
						{ 5,3,1 },// Far	Bottom	Right
					};

					CLOAKENGINE_API CLOAK_CALL OctantFrustum();
					CLOAKENGINE_API CLOAK_CALL OctantFrustum(In const Matrix& m);
					CLOAKENGINE_API CLOAK_CALL OctantFrustum(In const Frustum& m);
					CLOAKENGINE_API CLOAK_CALL OctantFrustum(In const OctantFrustum& o);

					CLOAKENGINE_API OctantFrustum& CLOAK_CALL_THIS operator=(In const Matrix& m);
					CLOAKENGINE_API OctantFrustum& CLOAK_CALL_THIS operator=(In const Frustum& m);
					CLOAKENGINE_API OctantFrustum& CLOAK_CALL_THIS operator=(In const OctantFrustum& o);

					CLOAKENGINE_API size_t CLOAK_CALL_THIS LocateOctant(In const Vector& center) const;

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);
				};
				struct CLOAK_ALIGN IntPoint {
					union {
						struct { int32_t X, Y, Z, W; };
						int32_t Data[4];
					};
					CLOAKENGINE_API CLOAK_CALL IntPoint();
					CLOAKENGINE_API CLOAK_CALL IntPoint(In int32_t X, In int32_t Y, In int32_t Z, In_opt int32_t W = 1);
					CLOAKENGINE_API CLOAK_CALL IntPoint(In const IntPoint& p);
					CLOAKENGINE_API CLOAK_CALL IntPoint(In const __m128i& d);
					CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH operator+(In const IntVector& v) const;
					CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH operator-(In const IntVector& v) const;
					CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH operator=(In const IntPoint& p);
					CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH operator+=(In const IntVector& v);
					CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH operator-=(In const IntVector& v);
					CLOAKENGINE_API const Point CLOAK_CALL_MATH operator*(In float a) const;
					CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH operator*(In int32_t a) const;
					CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH operator*(In const IntVector& v) const;
					CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH operator*=(In int32_t a);
					CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH operator*=(In const IntVector& v);
					CLOAKENGINE_API const Point CLOAK_CALL_MATH operator/(In float a) const;
					CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH operator/(In int32_t a) const;
					CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH operator/(In const IntVector& v) const;
					CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH operator/=(In int32_t a);
					CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH operator/=(In const IntVector& v);
					CLOAKENGINE_API int32_t& CLOAK_CALL_MATH operator[](In size_t a);
					CLOAKENGINE_API const int32_t& CLOAK_CALL_MATH operator[](In size_t a) const;

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);
				};
				struct CLOAK_ALIGN Point {
					union {
						struct { float X, Y, Z, W; };
						float Data[4];
					};
					CLOAKENGINE_API CLOAK_CALL Point();
					CLOAKENGINE_API CLOAK_CALL Point(In float X, In float Y, In float Z, In_opt float W = 1);
					CLOAKENGINE_API CLOAK_CALL Point(In const Point& p);
					CLOAKENGINE_API CLOAK_CALL Point(In const __m128& d);
					CLOAKENGINE_API const Point CLOAK_CALL_MATH operator+(In const Vector& v) const;
					CLOAKENGINE_API const Point CLOAK_CALL_MATH operator-(In const Vector& v) const;
					CLOAKENGINE_API Point& CLOAK_CALL_MATH operator=(In const Point& p);
					CLOAKENGINE_API Point& CLOAK_CALL_MATH operator+=(In const Vector& v);
					CLOAKENGINE_API Point& CLOAK_CALL_MATH operator-=(In const Vector& v);
					CLOAKENGINE_API const Point CLOAK_CALL_MATH operator*(In float a) const;
					CLOAKENGINE_API const Point CLOAK_CALL_MATH operator*(In const Vector& v) const;
					CLOAKENGINE_API Point& CLOAK_CALL_MATH operator*=(In float a);
					CLOAKENGINE_API Point& CLOAK_CALL_MATH operator*=(In const Vector& v);
					CLOAKENGINE_API const Point CLOAK_CALL_MATH operator/(In float a) const;
					CLOAKENGINE_API const Point CLOAK_CALL_MATH operator/(In const Vector& v) const;
					CLOAKENGINE_API Point& CLOAK_CALL_MATH operator/=(In float a);
					CLOAKENGINE_API Point& CLOAK_CALL_MATH operator/=(In const Vector& v);
					CLOAKENGINE_API float& CLOAK_CALL_MATH operator[](In size_t a);
					CLOAKENGINE_API const float& CLOAK_CALL_MATH operator[](In size_t a) const;

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);
				};
				struct CLOAK_ALIGN EulerAngles {
					union {
						struct { 
							float Roll; //Rotation about X-Axis
							float Pitch; //Rotation about Y-Axis
							float Yaw; //Rotation about Z-Axis
						};
						struct {
							float AngleX;
							float AngleY;
							float AngleZ;
						};
						float Data[3];
					};
					CLOAKENGINE_API CLOAK_CALL EulerAngles();
					CLOAKENGINE_API CLOAK_CALL EulerAngles(In float roll, In float pitch, In float yaw);
					CLOAKENGINE_API CLOAK_CALL EulerAngles(In const EulerAngles& e);
					CLOAKENGINE_API Quaternion CLOAK_CALL ToQuaternion(In RotationOrder order) const;
					CLOAKENGINE_API Matrix CLOAK_CALL ToMatrix(In RotationOrder order) const;
					CLOAKENGINE_API Vector CLOAK_CALL ToAngularVelocity() const;
					CLOAKENGINE_API float& CLOAK_CALL_MATH operator[](In size_t a);
					CLOAKENGINE_API const float& CLOAK_CALL_MATH operator[](In size_t a) const;

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);

					CLOAKENGINE_API static Quaternion CLOAK_CALL ToQuaternion(In const EulerAngles& e, In RotationOrder order);
					CLOAKENGINE_API static Matrix CLOAK_CALL ToMatrix(In const EulerAngles& e, In RotationOrder order);
				};
				struct CLOAK_ALIGN IntVector {
					__m128i Data;
					template<typename A, typename B, typename C> CLOAK_CALL IntVector(In A X, In B Y, In C Z) : IntVector(static_cast<int32_t>(X), static_cast<int32_t>(Y), static_cast<int32_t>(Z)) {}
					template<typename A, typename B, typename C, typename D> CLOAK_CALL IntVector(In A X, In B Y, In C Z, In D W) : IntVector(static_cast<int32_t>(X), static_cast<int32_t>(Y), static_cast<int32_t>(Z), static_cast<int32_t>(W)) {}
					CLOAKENGINE_API CLOAK_CALL IntVector();
					CLOAKENGINE_API CLOAK_CALL IntVector(In int32_t X, In int32_t Y, In int32_t Z);
					CLOAKENGINE_API CLOAK_CALL IntVector(In int32_t X, In int32_t Y, In int32_t Z, In int32_t W);
					CLOAKENGINE_API CLOAK_CALL IntVector(In const IntPoint& p);
					CLOAKENGINE_API CLOAK_CALL IntVector(In const IntVector& v);
					CLOAKENGINE_API CLOAK_CALL IntVector(In const __m128i& d);
					CLOAKENGINE_API explicit CLOAK_CALL_MATH operator IntPoint() const;
					CLOAKENGINE_API explicit CLOAK_CALL_MATH operator Point() const;
					CLOAKENGINE_API explicit CLOAK_CALL_MATH operator Quaternion() const;
					CLOAKENGINE_API const IntVector CLOAK_CALL_MATH operator+(In const IntVector& v) const;
					CLOAKENGINE_API const IntVector CLOAK_CALL_MATH operator-(In const IntVector& v) const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator+(In const Vector& v) const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator-(In const Vector& v) const;
					CLOAKENGINE_API const IntVector CLOAK_CALL_MATH operator*(In const IntVector& v) const;
					CLOAKENGINE_API const IntVector CLOAK_CALL_MATH operator*(In int32_t v) const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator*(In const Vector& v) const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator*(In float v) const;
					CLOAKENGINE_API const IntVector CLOAK_CALL_MATH operator/(In const IntVector& v) const;
					CLOAKENGINE_API const IntVector CLOAK_CALL_MATH operator/(In int32_t v) const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator/(In const Vector& v) const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator/(In float v) const;
					CLOAKENGINE_API const IntVector CLOAK_CALL_MATH operator-() const;
					CLOAKENGINE_API IntVector& CLOAK_CALL_MATH operator=(In const IntVector& v);
					CLOAKENGINE_API IntVector& CLOAK_CALL_MATH operator=(In const IntPoint& v);
					CLOAKENGINE_API IntVector& CLOAK_CALL_MATH operator+=(In const IntVector& v);
					CLOAKENGINE_API IntVector& CLOAK_CALL_MATH operator-=(In const IntVector& v);
					CLOAKENGINE_API IntVector& CLOAK_CALL_MATH operator*=(In const IntVector& v);
					CLOAKENGINE_API IntVector& CLOAK_CALL_MATH operator*=(In int32_t v);
					CLOAKENGINE_API IntVector& CLOAK_CALL_MATH operator/=(In const IntVector& v);
					CLOAKENGINE_API IntVector& CLOAK_CALL_MATH operator/=(In int32_t v);
					CLOAKENGINE_API bool CLOAK_CALL_MATH operator==(In const IntVector& v) const;
					CLOAKENGINE_API bool CLOAK_CALL_MATH operator!=(In const IntVector& v) const;

					CLOAKENGINE_API float CLOAK_CALL_MATH Length() const;
					CLOAKENGINE_API int32_t CLOAK_CALL_MATH Dot(In const IntVector& v) const;
					CLOAKENGINE_API IntVector CLOAK_CALL_MATH Cross(In const IntVector& v) const;
					CLOAKENGINE_API IntVector& CLOAK_CALL_MATH Set(In int32_t X, In int32_t Y, In int32_t Z);

					CLOAKENGINE_API static float CLOAK_CALL_MATH Length(In const IntVector& v);
					CLOAKENGINE_API static int32_t CLOAK_CALL_MATH Dot(In const IntVector& a, In const IntVector& b);
					CLOAKENGINE_API static IntVector CLOAK_CALL_MATH Cross(In const IntVector& a, In const IntVector& b);
					CLOAKENGINE_API static IntVector CLOAK_CALL_MATH Min(In const IntVector& a, In const IntVector& b);
					CLOAKENGINE_API static IntVector CLOAK_CALL_MATH Max(In const IntVector& a, In const IntVector& b);
					CLOAKENGINE_API static IntVector CLOAK_CALL_MATH Clamp(In const IntVector& a, In const IntVector& min, In const IntVector& max);

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);
				};
				struct CLOAK_ALIGN Vector {
					__m128 Data;
					template<typename A, typename B, typename C> CLOAK_CALL Vector(In A X, In B Y, In C Z) : Vector(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z)) {}
					template<typename A, typename B, typename C, typename D> CLOAK_CALL Vector(In A X, In B Y, In C Z, In D W) : Vector(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z), static_cast<float>(W)) {}
					CLOAKENGINE_API CLOAK_CALL Vector();
					CLOAKENGINE_API CLOAK_CALL Vector(In float X, In float Y, In float Z);
					CLOAKENGINE_API CLOAK_CALL Vector(In float X, In float Y, In float Z, In float W);
					CLOAKENGINE_API CLOAK_CALL Vector(In const Point& p);
					CLOAKENGINE_API CLOAK_CALL Vector(In const Vector& v);
					CLOAKENGINE_API CLOAK_CALL Vector(In const __m128& d);
					CLOAKENGINE_API CLOAK_CALL Vector(In const IntVector& v);
					CLOAKENGINE_API explicit CLOAK_CALL_MATH operator Point() const;
					CLOAKENGINE_API explicit CLOAK_CALL_MATH operator Quaternion() const;
					CLOAKENGINE_API explicit CLOAK_CALL_MATH operator IntVector() const;
					CLOAKENGINE_API explicit CLOAK_CALL_MATH operator IntPoint() const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator+(In const Vector& v) const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator-(In const Vector& v) const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator*(In const Vector& v) const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator*(In float f) const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator/(In const Vector& v) const;
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator/(In float f) const;
					CLOAKENGINE_API Vector& CLOAK_CALL_MATH operator=(In const Vector& v);
					CLOAKENGINE_API Vector& CLOAK_CALL_MATH operator=(In const Point& p);
					CLOAKENGINE_API Vector& CLOAK_CALL_MATH operator=(In const IntVector& v);
					CLOAKENGINE_API Vector& CLOAK_CALL_MATH operator+=(In const Vector& v);
					CLOAKENGINE_API Vector& CLOAK_CALL_MATH operator-=(In const Vector& v);
					CLOAKENGINE_API Vector& CLOAK_CALL_MATH operator*=(In const Vector& v);
					CLOAKENGINE_API Vector& CLOAK_CALL_MATH operator*=(In float f);
					CLOAKENGINE_API Vector& CLOAK_CALL_MATH operator/=(In const Vector& v);
					CLOAKENGINE_API Vector& CLOAK_CALL_MATH operator/=(In float f);
					CLOAKENGINE_API Vector CLOAK_CALL_MATH operator*(In const Matrix& m) const;
					CLOAKENGINE_API Vector& CLOAK_CALL_MATH operator*=(In const Matrix& m);
					CLOAKENGINE_API const Vector CLOAK_CALL_MATH operator-() const;
					CLOAKENGINE_API bool CLOAK_CALL_MATH operator==(In const Vector& v) const;
					CLOAKENGINE_API bool CLOAK_CALL_MATH operator!=(In const Vector& v) const;

					CLOAKENGINE_API float CLOAK_CALL_MATH Length() const;
					CLOAKENGINE_API float CLOAK_CALL_MATH Dot(In const Vector& v) const;
					CLOAKENGINE_API Vector CLOAK_CALL_MATH Cross(In const Vector& v) const;
					CLOAKENGINE_API Vector CLOAK_CALL_MATH Normalize() const;
					CLOAKENGINE_API Vector CLOAK_CALL_MATH Abs() const;
					CLOAKENGINE_API Vector& CLOAK_CALL_MATH Set(In float X, In float Y, In float Z);
					CLOAKENGINE_API Vector& CLOAK_CALL_THIS ProjectOnPlane(In const Plane& p);
					CLOAKENGINE_API std::pair<Vector, Vector> CLOAK_CALL_MATH GetOrthographicBasis() const;

					CLOAKENGINE_API static Vector CLOAK_CALL_MATH Abs(In const Vector& a);
					CLOAKENGINE_API static Vector CLOAK_CALL_MATH Normalize(In const Vector& m);
					CLOAKENGINE_API static Vector CLOAK_CALL_MATH Cross(In const Vector& a, In const Vector& b);
					CLOAKENGINE_API static float CLOAK_CALL_MATH Dot(In const Vector& a, In const Vector& b);
					CLOAKENGINE_API static float CLOAK_CALL_MATH Length(In const Vector& a);
					CLOAKENGINE_API static Vector CLOAK_CALL_MATH Min(In const Vector& a, In const Vector& b);
					CLOAKENGINE_API static Vector CLOAK_CALL_MATH Max(In const Vector& a, In const Vector& b);
					CLOAKENGINE_API static Vector CLOAK_CALL_MATH Clamp(In const Vector& value, In const Vector& min, In const Vector& max);
					CLOAKENGINE_API static Vector CLOAK_CALL_MATH Lerp(In const Vector& a, In const Vector& b, In float c);
					CLOAKENGINE_API static Vector CLOAK_CALL_MATH Floor(In const Vector& a);
					CLOAKENGINE_API static Vector CLOAK_CALL_MATH Ceil(In const Vector& a);
					CLOAKENGINE_API static Vector CLOAK_CALL_MATH Barycentric(In const Vector& a, In const Vector& b, In const Vector& c, In const Vector& p);

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);

					CLOAKENGINE_API static const Vector Forward;
					CLOAKENGINE_API static const Vector Back;
					CLOAKENGINE_API static const Vector Left;
					CLOAKENGINE_API static const Vector Right;
					CLOAKENGINE_API static const Vector Top;
					CLOAKENGINE_API static const Vector Bottom;
					CLOAKENGINE_API static const Vector Zero;
				};
				struct CLOAK_ALIGN WorldPosition {
					Vector Position;
					IntVector Node;

					CLOAKENGINE_API CLOAK_CALL WorldPosition();
					CLOAKENGINE_API CLOAK_CALL WorldPosition(In float X, In float Y, In float Z);
					CLOAKENGINE_API CLOAK_CALL WorldPosition(In float X, In float Y, In float Z, In int32_t nX, In int32_t nY, In int32_t nZ);
					CLOAKENGINE_API CLOAK_CALL WorldPosition(In const WorldPosition& v);
					CLOAKENGINE_API CLOAK_CALL WorldPosition(In const Vector& v);
					CLOAKENGINE_API CLOAK_CALL WorldPosition(In const Point& p);
					CLOAKENGINE_API CLOAK_CALL WorldPosition(In const __m128& p, In const __m128i& n);
					CLOAKENGINE_API CLOAK_CALL WorldPosition(In const Vector& v, In const IntVector& n);
					CLOAKENGINE_API explicit CLOAK_CALL_MATH operator Vector() const;
					CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH operator+(In const WorldPosition& v) const;
					CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH operator-(In const WorldPosition& v) const;
					CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH operator+(In const Vector& v) const;
					CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH operator-(In const Vector& v) const;
					CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH operator*(In float v) const;
					CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH operator/(In float v) const;
					CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH operator-() const;
					CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH operator=(In const WorldPosition& v);
					CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH operator=(In const Vector& v);
					CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH operator=(In const Point& v);
					CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH operator+=(In const WorldPosition& v);
					CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH operator-=(In const WorldPosition& v);
					CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH operator+=(In const Vector& v);
					CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH operator-=(In const Vector& v);
					CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH operator*=(In float v);
					CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH operator/=(In float v);

					CLOAKENGINE_API bool CLOAK_CALL_MATH operator==(In const WorldPosition& v) const;
					CLOAKENGINE_API bool CLOAK_CALL_MATH operator!=(In const WorldPosition& v) const;

					CLOAKENGINE_API float CLOAK_CALL_MATH Length() const;
					CLOAKENGINE_API float CLOAK_CALL_MATH Distance(In const WorldPosition& v) const;

					CLOAKENGINE_API static float CLOAK_CALL_MATH Length(In const WorldPosition& a);
					CLOAKENGINE_API static float CLOAK_CALL_MATH Distance(In const WorldPosition& a, In const WorldPosition& b);

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);
				};
				struct CLOAK_ALIGN AABB {
					union {
						struct { float Left, Right, Top, Bottom, MinDepth, MaxDepth; };
						float Data[6];
					};
					CLOAKENGINE_API CLOAK_CALL AABB();
					CLOAKENGINE_API CLOAK_CALL AABB(In float left, In float right, In float top, In float bottom, In_opt float depth = 0);
					CLOAKENGINE_API CLOAK_CALL AABB(In float left, In float right, In float top, In float bottom, In float minDepth, In float maxDepth);
					CLOAKENGINE_API CLOAK_CALL AABB(In size_t size, In_reads(size) Vector* points);
					CLOAKENGINE_API CLOAK_CALL AABB(In size_t size, In_reads(size) const Vector* points);
					CLOAKENGINE_API CLOAK_CALL AABB(In size_t size, In_reads(size) Point* points);
					CLOAKENGINE_API CLOAK_CALL AABB(In size_t size, In_reads(size) const Point* points);

					CLOAKENGINE_API float CLOAK_CALL Volume() const;
				};
				//Matrix is row-major!
				struct CLOAK_ALIGN Matrix {
					__m128 Row[4];
					CLOAKENGINE_API CLOAK_CALL Matrix();
					CLOAKENGINE_API CLOAK_CALL Matrix(In float d[4][4]);
					CLOAKENGINE_API CLOAK_CALL Matrix(In float r00, In float r01, In float r02, In float r03, In float r10, In float r11, In float r12, In float r13, In float r20, In float r21, In float r22, In float r23, In float r30, In float r31, In float r32, In float r33);
					CLOAKENGINE_API CLOAK_CALL Matrix(In const Matrix& m);
					CLOAKENGINE_API CLOAK_CALL Matrix(In const __m128& r0, In const __m128& r1, In const __m128& r2, In const __m128& r3);
					CLOAKENGINE_API const Matrix CLOAK_CALL_MATH operator*(In const Matrix& m) const;
					CLOAKENGINE_API Matrix& CLOAK_CALL_MATH operator*=(In const Matrix& m);

					CLOAKENGINE_API Matrix CLOAK_CALL_MATH Inverse() const;
					CLOAKENGINE_API Matrix CLOAK_CALL_MATH Transpose() const;
					CLOAKENGINE_API Matrix CLOAK_CALL_MATH RemoveTranslation() const;
					CLOAKENGINE_API Vector CLOAK_CALL_MATH GetTranslation() const;

					//CLOAKENGINE_API static Matrix CLOAK_CALL_MATH Identity();
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH Inverse(In const Matrix& m);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH PerspectiveProjection(In float fov, In unsigned int screenWidth, In unsigned int screenHeight, In float nearPlane, In float farPlane);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH OrthographicProjection(In unsigned int screenWidth, In unsigned int screenHeight, In float nearPlane, In float farPlane);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH OrthographicProjection(In float left, In float right, In float top, In float bottom, In float nearPlane, In float farPlane);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH LookAt(In const Vector& Position, In const Vector& Target, In const Vector& Up);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH LookTo(In const Vector& Position, In const Vector& Direction, In const Vector& Up);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH Translate(In float X, In float Y, In float Z);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH Translate(In const Vector& v);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH RotationX(In float angle);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH RotationY(In float angle);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH RotationZ(In float angle);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH Rotation(In float angleX, In float angleY, In float angleZ, In RotationOrder order);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH Rotation(In const EulerAngles& e, In RotationOrder order);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH Rotation(In const Vector& axis, In float angle);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH Scale(In float x, In float y, In float z);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH Scale(In const Vector& v);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH Transpose(In const Matrix& m);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH RemoveTranslation(In const Matrix& m);
					CLOAKENGINE_API static Vector CLOAK_CALL_MATH GetTranslation(In const Matrix& m);

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);

					CLOAKENGINE_API static const Matrix Identity;
				};
				struct CLOAK_ALIGN Quaternion {
					__m128 Data;
					CLOAKENGINE_API CLOAK_CALL Quaternion();
					CLOAKENGINE_API CLOAK_CALL Quaternion(In float I, In float J, In float K, In float R);
					CLOAKENGINE_API CLOAK_CALL Quaternion(In const Vector& axis, In float angle);
					CLOAKENGINE_API CLOAK_CALL Quaternion(In const Quaternion& q);
					CLOAKENGINE_API CLOAK_CALL Quaternion(In const Point& p);
					CLOAKENGINE_API CLOAK_CALL Quaternion(In const __m128& d);
					CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH operator*(In const Quaternion& q) const;
					CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH operator*=(In const Quaternion& q);
					CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH operator*(In const Vector& q) const;
					CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH operator*=(In const Vector& q);
					CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH operator*(In float a) const;
					CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH operator*=(In float a);
					CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH operator+(In const Quaternion& q) const;
					CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH operator+=(In const Quaternion& q);
					CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH operator-(In const Quaternion& q) const;
					CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH operator-=(In const Quaternion& q);
					CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH operator-() const;
					CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH operator*(In const DualQuaternion& q) const;
					CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH operator*=(In const DualQuaternion& q);
					CLOAKENGINE_API bool CLOAK_CALL_MATH operator==(In const Quaternion& q) const;
					CLOAKENGINE_API bool CLOAK_CALL_MATH operator!=(In const Quaternion& q) const;
					CLOAKENGINE_API explicit CLOAK_CALL_MATH operator Point() const;
					CLOAKENGINE_API explicit CLOAK_CALL_MATH operator Matrix() const;
					CLOAKENGINE_API explicit CLOAK_CALL_MATH operator Vector() const;

					CLOAKENGINE_API float CLOAK_CALL_MATH Length() const;
					CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Normalize() const;
					CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Inverse() const;
					CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Conjugate() const;
					CLOAKENGINE_API Vector CLOAK_CALL_MATH ToAngularVelocity() const;
					CLOAKENGINE_API Vector CLOAK_CALL_MATH Rotate(In const Vector& v) const;
					CLOAKENGINE_API EulerAngles CLOAK_CALL_MATH ToEulerAngles(In RotationOrder order) const;
					
					CLOAKENGINE_API static float CLOAK_CALL_MATH Length(In const Quaternion& q);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH Normalize(In const Quaternion& q);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH Inverse(In const Quaternion& q);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH Conjugate(In const Quaternion& q);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH RotationX(In float angle);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH RotationY(In float angle);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH RotationZ(In float angle);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH Rotation(In float angleX, In float angleY, In float angleZ, In RotationOrder order);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH Rotation(In const Vector& axis, In float angle);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH Rotation(In const EulerAngles& e, In RotationOrder order);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH Rotation(In const Vector& begin, In const Vector& end);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH NLerp(In const Quaternion& a, In const Quaternion& b, In float alpha);
					CLOAKENGINE_API static Quaternion CLOAK_CALL_MATH SLerp(In const Quaternion& a, In const Quaternion& b, In float alpha);
					CLOAKENGINE_API static Matrix CLOAK_CALL_MATH ToMatrix(In const Quaternion& q);
					CLOAKENGINE_API static EulerAngles CLOAK_CALL_MATH ToEulerAngles(In const Quaternion& q, In RotationOrder order);

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);
				};
				struct CLOAK_ALIGN DualQuaternion {
					Quaternion Real;
					Quaternion Dual;
					CLOAKENGINE_API CLOAK_CALL DualQuaternion(In const Quaternion& r, In const Quaternion& d);
					CLOAKENGINE_API CLOAK_CALL DualQuaternion(In const Quaternion& r);
					CLOAKENGINE_API CLOAK_CALL DualQuaternion();
					CLOAKENGINE_API CLOAK_CALL DualQuaternion(In const DualQuaternion& q);

					CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH operator*(In float s) const;
					CLOAKENGINE_API DualQuaternion& CLOAK_CALL_MATH operator*=(In float s);
					CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH operator*(In const DualQuaternion& q) const;
					CLOAKENGINE_API DualQuaternion& CLOAK_CALL_MATH operator*=(In const DualQuaternion& q);
					CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH operator+(In const DualQuaternion& q) const;
					CLOAKENGINE_API DualQuaternion& CLOAK_CALL_MATH operator+=(In const DualQuaternion& q);
					CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH operator-(In const DualQuaternion& q) const;
					CLOAKENGINE_API DualQuaternion& CLOAK_CALL_MATH operator-=(In const DualQuaternion& q);
					CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH operator-() const;
					CLOAKENGINE_API CLOAK_CALL_MATH operator Matrix() const;
					CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH operator*(In const Quaternion& q) const;

					CLOAKENGINE_API float CLOAK_CALL_MATH Length() const;
					CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH Normalize() const;
					CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH Inverse() const;
					CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH Conjugate() const;

					CLOAKENGINE_API Quaternion CLOAK_CALL_MATH GetRotationPart() const;
					CLOAKENGINE_API Vector CLOAK_CALL_MATH GetTranslationPart() const;

					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH Inverse(In const DualQuaternion& q);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH Normalize(In const DualQuaternion& q);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH Conjugate(In const DualQuaternion& q);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH Translate(In float X, In float Y, In float Z);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH Translate(In const Vector& v);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH RotationX(In float angle);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH RotationY(In float angle);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH RotationZ(In float angle);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH Rotation(In float angleX, In float angleY, In float angleZ, In RotationOrder order);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH Rotation(In const Vector& axis, In float angle);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH Rotation(In const EulerAngles& e, In RotationOrder order);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH Rotation(In const Vector& begin, In const Vector& end);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH NLerp(In const DualQuaternion& a, In const DualQuaternion& b, In float alpha);
					CLOAKENGINE_API static DualQuaternion CLOAK_CALL_MATH SLerp(In const DualQuaternion& a, In const DualQuaternion& b, In float alpha);

					CLOAKENGINE_API void* CLOAK_CALL operator new(In size_t s);
					CLOAKENGINE_API void* CLOAK_CALL operator new[](In size_t s);
					CLOAKENGINE_API void CLOAK_CALL operator delete(In void* ptr);
					CLOAKENGINE_API void CLOAK_CALL operator delete[](In void* ptr);
				};

				inline const Vector CLOAK_CALL operator*(In float a, In const Vector& b) { return b * a; }
				inline const IntVector CLOAK_CALL operator*(In int32_t a, In const IntVector& b) { return b * a; }
				inline const Vector CLOAK_CALL operator*(In float a, In const IntVector& b) { return b * a; }
				inline const Point CLOAK_CALL operator*(In float a, In const Point& b) { return b * a; }
				inline const IntPoint CLOAK_CALL operator*(In int32_t a, In const IntPoint& b) { return b * a; }
				inline const Point CLOAK_CALL operator*(In float a, In const IntPoint& b) { return b * a; }
				inline const Quaternion CLOAK_CALL operator*(In float a, In const Quaternion& b) { return b * a; }
				inline const WorldPosition CLOAK_CALL operator*(In float a, In const WorldPosition& b) { return b * a; }

				namespace Space2D {
					struct Point;
					struct Vector;

					struct Point {
						union {
							float Data[2];
							struct {
								float X;
								float Y;
							};
						};
						CLOAKENGINE_API CLOAK_CALL Point();
						CLOAKENGINE_API CLOAK_CALL Point(In float x, In float y);
						CLOAKENGINE_API CLOAK_CALL_MATH operator Vector() const;
						CLOAKENGINE_API inline const Space2D::Point CLOAK_CALL_MATH operator+(In const Space2D::Vector& b) const;
						CLOAKENGINE_API inline const Space2D::Point CLOAK_CALL_MATH operator-(In const Space2D::Vector& b) const;
					};
					struct Vector {
						union {
							float Data[3];
							struct {
								float X;
								float Y;
								float A;
							};
						};
						CLOAKENGINE_API CLOAK_CALL Vector();
						CLOAKENGINE_API CLOAK_CALL Vector(In float x, In float y);
						CLOAKENGINE_API CLOAK_CALL Vector(In float x, In float y, In float a);
						CLOAKENGINE_API explicit CLOAK_CALL_MATH operator Point() const;
						CLOAKENGINE_API inline float CLOAK_CALL_MATH operator*(In const Space2D::Vector& b) const;
						CLOAKENGINE_API inline const Space2D::Vector CLOAK_CALL_MATH operator*(In const float& b) const;
						CLOAKENGINE_API inline const Space2D::Vector CLOAK_CALL_MATH operator+(In const Space2D::Vector& b) const;
						CLOAKENGINE_API inline const Space2D::Vector CLOAK_CALL_MATH operator-(In const Space2D::Vector& b) const;
						CLOAKENGINE_API inline float CLOAK_CALL_MATH Length() const;
						CLOAKENGINE_API inline const Space2D::Vector CLOAK_CALL_MATH Normalize() const;
					};
					CLOAKENGINE_API float CLOAK_CALL DotProduct(const Vector& a, const Vector& b);
				}
				CLOAKENGINE_API inline const Space2D::Vector CLOAK_CALL operator*(In const float& a, In const Space2D::Vector& b);

				namespace SectionFunctions {
					template<typename W, typename D = W> struct Point {
						W X;
						D Y;
					};
					template<typename W, typename D = W> class LinearFunction {
					public:
						typedef Point<W, D> Point;

						CLOAK_CALL LinearFunction()
						{
							m_size = 0;
							m_points = nullptr;
						}
						CLOAK_CALL LinearFunction(In_reads(size) Point* points, In size_t size) : LinearFunction()
						{
							Reset(points, size);
						}
						CLOAK_CALL ~LinearFunction()
						{
							DeleteArray(m_points);
						}
						D CLOAK_CALL_THIS Calculate(In const W& v) const
						{
							if (m_size > 0)
							{
								if (v <= m_points[0].X) { return m_points[0].Y; }
								size_t b = 1;
								while (b < m_size && v > m_points[b].X) { b++; }
								if (b >= m_size) { return m_points[m_size - 1].Y; }
								const Point& l = m_points[b - 1];
								const Point& r = m_points[b];
								const S my = static_cast<S>(r.Y) - static_cast<S>(l.Y);
								const S dx = static_cast<S>(v - l.X);
								const S mx = static_cast<S>(r.X - l.X);
								return static_cast<D>(static_cast<S>(l.Y) + ((dx*my) / mx));
							}
							return D(0);
						}
						void CLOAK_CALL_THIS Reset(In_reads(size) Point* points, In size_t size)
						{
							DeleteArray(m_points);
							m_points = size > 0 ? NewArray(Point, size) : nullptr;
							m_size = size;
							if (size > 0)
							{
								size_t* pos = NewArray(size_t, size);
								for (size_t a = 0; a < size; a++)
								{
									size_t b = size - (a + 1);
									pos[b] = a;
									bool f = false;
									while (b + 1 < size && f == false)
									{
										if (points[pos[b]].X > points[pos[b + 1]].X)
										{
											size_t t = pos[b];
											pos[b] = pos[b + 1];
											pos[b + 1] = t;
											b++;
										}
										else { f = true; }
									}
								}
								for (size_t a = 0, o = 0; a < size; a++)
								{
									if (a > 0 && points[pos[a]].X == points[pos[a - 1]].X)
									{
										o++;
										m_size--;
									}
									m_points[a - o] = points[pos[a]];
								}
								DeleteArray(pos);
							}
						}
						const Point& CLOAK_CALL_THIS GetMax() const { return m_points[m_size - 1]; }
					private:
						typedef typename std::make_signed<typename std::conditional<(sizeof(D) <= sizeof(W)), W, D>::type>::type S;
						Point* m_points;
						size_t m_size;
					};
				}

				template<typename T = uint64_t, typename Allocator = API::Global::Memory::MemoryHeap, std::enable_if_t<std::is_integral_v<T>>* = nullptr> class BigInteger {
					public:
						typedef typename std::make_unsigned<T>::type type;
					private:
						static constexpr size_t BIT_SIZE = sizeof(type) * 8;
						type* m_data;
						size_t m_cap;
						size_t m_len;
						bool m_neg;
						bool m_nan;

						template<typename A, typename B, std::enable_if_t<std::is_integral_v<A> && std::is_integral_v<B> && std::is_unsigned_v<A> && std::is_unsigned_v<B>>* = nullptr> static size_t CLOAK_CALL to_big(In A x, Out B* res)
						{
							if (x == 0) { return 0; }
							if constexpr (sizeof(A) <= sizeof(B))
							{
								res[0] = static_cast<B>(x);
								return 1;
							}
							else
							{
								const size_t os = (sizeof(A) + sizeof(B) - 1) / sizeof(B);
								const size_t bs = sizeof(B) << 3;
								const A mask = static_cast<A>(~static_cast<B>(0));
								for (size_t a = 0; a < os; a++)
								{
									res[a] = x & mask;
									x >>= bs;
									if (x == 0) { return static_cast<size_t>(a + 1); }
								}
								return os;
							}
						}
						template<typename T, typename I, std::enable_if_t<std::is_integral_v<I> && std::is_integral_v<T> && std::is_unsigned_v<I> && std::is_unsigned_v<T>>* = nullptr> static size_t CLOAK_CALL to_big(In size_t count, In_reads(count) const I* x, Out T* res)
						{
							size_t o = 0, bo = 0;
							for (size_t i = 0, bi = 0; i < count;)
							{
								CLOAK_ASSUME(bi < sizeof(I));
								CLOAK_ASSUME(bo < sizeof(type));
								const size_t is = sizeof(I) - bi;
								const size_t os = sizeof(type) - bo;
								const size_t bc = min(is, os);
								const I im = static_cast<I>(((1Ui64 << (bc << 3)) - 1) << (bi << 3));
								const I v = x[i] & im;

								if (bi < bo) { res[o] |= static_cast<type>(v) << ((bo - bi) << 3); }
								else { res[o] |= static_cast<type>(v >> ((bi - bo) << 3)); }

								bi += bc;
								bo += bc;
								i += bi / sizeof(I);
								o += bo / sizeof(type);
								bi %= sizeof(I);
								bo %= sizeof(type);
							}
							if (bo != 0) { o++; }
							while (o > 0 && res[o - 1] == 0) { o--; }
							return o;
						}
						template<typename A> type CLOAK_CALL_THIS shiftedBlock(In const BigInteger<T, A>& n, In size_t x, In size_t y)
						{
							CLOAK_ASSUME(y <= BIT_SIZE);
							const type p1 = (x == 0 || y == 0) ? 0 : (n[x - 1] >> (BIT_SIZE - y));
							const type p2 = (x == n.size()) ? 0 : (n[x] << y);
							return p1 | p2;
						}
						void CLOAK_CALL_THIS allocate(In size_t size)
						{
							if (size > m_cap)
							{
								type* n = reinterpret_cast<type*>(Allocator::Allocate(size * sizeof(type)));
								if (m_cap > 0) { memcpy(n, m_data, m_cap * sizeof(type)); }
								memset(&n[m_cap], 0, (size - m_cap) * sizeof(type));
								type* o = m_data;
								m_data = n;
								Allocator::Free(o);
								m_cap = size;
							}
						}
						void CLOAK_CALL_THIS copy(In size_t size, In_reads_bytes(size) const void* data)
						{
							if (reinterpret_cast<uintptr_t>(data) == reinterpret_cast<uintptr_t>(m_data)) { return; }
							const size_t nc = (size + sizeof(type) - 1) / sizeof(type);
							if (nc > m_cap)
							{
								Allocator::Free(m_data);
								m_data = reinterpret_cast<type*>(Allocator::Allocate(nc * sizeof(type)));
								m_cap = nc;
							}
							if (size > 0) { memcpy(m_data, data, size); }
							if (m_cap > nc) { memset(&m_data[nc], 0, (m_cap - nc) * sizeof(type)); }
						}
						int CLOAK_CALL_THIS cmp(In_reads(m_len) const type* data) const
						{
							for (size_t a = m_len; a > 0; a--)
							{
								if (m_data[a - 1] != data[a - 1])
								{
									return (m_data[a - 1] > data[a - 1] ? 1 : -1) * (m_neg ? -1 : 1);
								}
							}
							return 0;
						}
						template<typename A> int CLOAK_CALL_THIS cmp(In const BigInteger<T, A>& x) const
						{
							if (m_neg != x.negative()) { return m_neg ? -1 : 1; }
							if (m_len == x.size()) { return cmp(x.data()); }
							return (m_len > x.size() ? 1 : -1) * (m_neg ? -1 : 1);
						}
						template<typename I, std::enable_if_t<std::is_integral_v<I>>* = nullptr> int CLOAK_CALL_THIS cmp(In I x) const
						{
							if ((x < 0) != m_neg) { return m_neg ? -1 : 1; }
							type tx[(sizeof(I) + sizeof(type) - 1) / sizeof(type)];
							const size_t lb = to_big(x, tx);
							CLOAK_ASSUME(lb <= ARRAYSIZE(tx));
							if (m_len == lb) { return cmp(tx); }
							return (m_len > lb ? 1 : -1) * (m_neg ? -1 : 1);
						}
						template<typename A> void CLOAK_CALL_THIS add(In const BigInteger<T, A>& x)
						{
							size_t ns = m_len;
							if (x.size() > ns) { ns = x.size(); }
							allocate(ns + 1);

							bool ci = false, co = false;
							size_t a = 0;
							for (; a < x.size(); a++)
							{
								type t = m_data[a] + x[a];
								co = (t < m_data[a]);
								if (ci == true)
								{
									t++;
									co |= (t == 0);
								}
								ci = co;
								m_data[a] = t;
							}
							for (; ci == true; a++)
							{
								m_data[a] = m_data[a] + 1;
								ci = (m_data[a] == 0);
							}
							if (ci == true) { m_data[a] = 1; m_len = a + 1; }
							else { m_len = a; }
						}
						template<typename A> void CLOAK_CALL_THIS sub(In const BigInteger<T, A>& x)
						{
							if (x.size() > m_len || (x.size() == m_len && cmp(x.data()) == (m_neg ? 1 : -1)))
							{
								allocate(x.size());
								bool ci = false, co = false;
								size_t a = 0;
								for (; a < x.size(); a++)
								{
									type t = x[a] - m_data[a];
									co = (t > x[a]);
									if (ci == true)
									{
										co |= (t == 0);
										t--;
									}
									m_data[a] = t;
									ci = co;
								}
								for (; a < x.size() && ci; a++)
								{
									ci = x[a] == 0;
									m_data[a] = x[a] - 1;
								}
								m_neg = !m_neg;
							}
							else
							{
								bool ci = false, co = false;
								size_t a = 0;
								for (; a < x.size(); a++)
								{
									type t = m_data[a] - x[a];
									co = (t > m_data[a]);
									if (ci == true)
									{
										co |= (t == 0);
										t--;
									}
									m_data[a] = t;
									ci = co;
								}
								for (; a < m_len && ci; a++)
								{
									ci = m_data[a] == 0;
									m_data[a]--;
								}
							}
							while (m_len > 0 && m_data[m_len - 1] == 0) { m_len--; }
						}
						//x / y = remainder(x).divide(y, &result)
						template<typename A> void CLOAK_CALL_THIS divide(In const BigInteger<T, A>& b, Out BigInteger<T, Allocator>* q)
						{
							CLOAK_ASSUME(q != this);
							CLOAK_ASSUME(b.size() > 0);
							if constexpr (std::is_same<A, Allocator>::value)
							{
								CLOAK_ASSUME(q != &b);
								if (this == &b)
								{
									BigInteger<T, A> t = b;
									divide(t, q);
									return;
								}
							}
							if (m_len < b.size()) 
							{ 
								if (q != nullptr) { q->m_len = 0; }
								return; 
							}
							allocate(++m_len);
							m_data[m_len - 1] = 0;
							type* tmp = reinterpret_cast<type*>(Allocator::Allocate(sizeof(type) * m_len));
							const size_t ql = m_len - b.size();
							if (q != nullptr)
							{
								q->m_len = ql;
								q->allocate(ql);
							}
							bool ci = false, co = false;
							for (size_t a = ql; a > 0; a--)
							{
								for (size_t c = BIT_SIZE; c > 0; c--)
								{
									size_t e = a - 1;
									ci = false;
									for (size_t d = 0; d <= b.size(); d++, e++)
									{
										type t = m_data[e] - shiftedBlock(b, d, c - 1);
										co = (t > m_data[e]);
										if (ci)
										{
											co |= (t == 0);
											t--;
										}
										tmp[e] = t;
										ci = co;
									}
									for (; e < m_len - 1 && ci; e++)
									{
										ci = (m_data[e] == 0);
										tmp[e] = m_data[e] - 1;
									}
									if (ci == false)
									{
										if (q != nullptr) { q->m_data[a - 1] |= (type(1) << (c - 1)); }
										while (e > a - 1)
										{
											e--;
											m_data[e] = tmp[e];
										}
									}
								}
							}
							while (m_len > 0 && m_data[m_len - 1] == 0) { m_len--; }
							if (q != nullptr) 
							{
								while (q->m_len > 0 && q->m_data[q->m_len - 1] == 0) { q->m_len--; }
							}
							Allocator::Free(tmp);
						}
					public:
						CLOAK_CALL BigInteger()
						{
							m_data = nullptr;
							m_cap = 0;
							m_len = 0;
							m_neg = false;
							m_nan = false;
						}
						CLOAK_CALL BigInteger(In BigInteger<T, Allocator>&& x) : BigInteger()
						{
							m_data = x.m_data;
							m_cap = x.m_cap;
							m_len = x.m_len;
							m_neg = x.m_neg;
							m_nan = x.m_nan;

							x.m_data = nullptr;
							x.m_len = 0;
							x.m_cap = 0;
							x.m_neg = false;
							x.m_nan = false;
						}
						CLOAK_CALL BigInteger(In const BigInteger<T, Allocator>& x) : BigInteger()
						{
							m_nan = x.m_nan;
							m_neg = x.m_neg;
							if (m_nan == false)
							{
								copy(x.m_cap * sizeof(BigInteger<T, Allocator>::type), x.m_data);
								m_len = x.m_len;
							}
						}
						template<typename A> CLOAK_CALL BigInteger(In const BigInteger<T, A>& x) : BigInteger()
						{
							m_nan = x.NaN();
							m_neg = x.negative();
							if (m_nan == false)
							{
								copy(x.capacity() * sizeof(BigInteger<T, A>::type), x.data());
								m_len = ((x.size() * sizeof(BigInteger<T, A>::type)) + sizeof(type) - 1) / sizeof(type);
							}
						}
						template<typename I, std::enable_if_t<std::is_integral_v<I>>* = nullptr> CLOAK_CALL BigInteger(In I x) : BigInteger()
						{
							const std::make_unsigned_t<I> y = API::Global::Math::abs(x);
							allocate((sizeof(I) + sizeof(type) - 1) / sizeof(type));
							m_len = to_big(y, m_data);
							m_neg = x < 0;
						}
						template<typename I, std::enable_if_t<std::is_integral_v<I> && std::is_unsigned_v<I>>* = nullptr> CLOAK_CALL BigInteger(In size_t count, In_reads(count) const I* x) : BigInteger()
						{
							allocate(((sizeof(I) * count) + sizeof(type) - 1) / sizeof(type));
							m_len = to_big(count, x, m_data);
						}
						template<typename I, size_t N, std::enable_if_t<std::is_integral_v<I> && std::is_unsigned_v<I>>* = nullptr> CLOAK_CALL BigInteger(In const I (&x)[N]) : BigInteger(N, x) {}

						CLOAK_CALL ~BigInteger() { Allocator::Free(m_data); }

						size_t CLOAK_CALL_THIS capacity() const { return m_cap; }
						size_t CLOAK_CALL_THIS size() const { return m_len; }
						size_t CLOAK_CALL_THIS popcount() const
						{
							if (m_len == 0) { return 0; }
							return ((m_len - 1) * BIT_SIZE) + API::Global::Math::popcount(m_data[m_len - 1]);
						}
						type* CLOAK_CALL_THIS data() { return m_data; }
						const type* CLOAK_CALL_THIS data() const { return m_data; }
						bool CLOAK_CALL_THIS negative() const { return m_neg; }
						bool CLOAK_CALL_THIS NaN() const { return m_nan; }
						void CLOAK_CALL_THIS reserve(In size_t capacity) { allocate(capacity); }
						void CLOAK_CALL_THIS reserveBytes(In size_t capacity) { allocate((capacity + sizeof(type) - 1) / sizeof(type)); }

						//Acces operators:
						type& CLOAK_CALL_THIS operator[](In size_t x) { return m_data[x]; }
						const type& CLOAK_CALL_THIS operator[](In size_t x) const { return m_data[x]; }
						type& CLOAK_CALL_THIS at(In size_t x) { return m_data[x]; }
						const type& CLOAK_CALL_THIS at(In size_t x) const { return m_data[x]; }

						//Set operators:
						BigInteger<T, Allocator>& CLOAK_CALL_THIS operator=(In BigInteger<T, Allocator>&& x)
						{
							m_nan = x.m_nan;
							m_neg = x.m_neg;
							m_data = x.m_data;
							m_cap = x.m_cap;
							m_len = x.m_len;

							x.m_data = nullptr;
							x.m_len = 0;
							x.m_cap = 0;
							x.m_neg = false;
							x.m_nan = false;

							return *this;
						}
						template<typename A> BigInteger<T, Allocator>& CLOAK_CALL_THIS operator=(In const BigInteger<T, A>& x)
						{
							m_nan = x.NaN();
							m_neg = x.negative();
							if (m_nan == false)
							{
								copy(x.capacity() * sizeof(BigInteger<T, A>::type), x.data());
								m_len = ((x.size() * sizeof(BigInteger<T, A>::type)) + sizeof(type) - 1) / sizeof(type);
							}
							return *this;
						}
						template<typename I, std::enable_if_t<std::is_integral_v<I>>* = nullptr> BigInteger<T, Allocator>& CLOAK_CALL_THIS operator=(In I x)
						{
							copy(sizeof(I), &x);
							for (m_len = m_cap; m_len > 0; m_len--)
							{
								if (m_data[m_len - 1] != 0) { break; }
							}
							m_neg = x < 0;
							m_nan = false;
							return *this;
						}
						template<typename I, size_t N, std::enable_if_t<std::is_integral_v<I> && std::is_unsigned_v<I>>* = nullptr> BigInteger<T, Allocator>& CLOAK_CALL_THIS operator=(In const I(&x)[N])
						{
							allocate(((sizeof(I) * count) + sizeof(type) - 1) / sizeof(type));
							m_len = to_big(count, x, m_data);
							m_neg = false;
							m_nan = false;
							return *this;
						}

						//Compare operators:
						template<typename A> bool CLOAK_CALL_THIS operator==(In const BigInteger<T, A>& x) const
						{
							if (m_nan || x.NaN()) { return false; }
							const size_t lb = m_len * sizeof(type);
							if (lb == x.size() * sizeof(BigInteger<T, A>::type))
							{
								if (lb == 0) { return true; }
								return memcmp(m_data, x.data(), lb) == 0;
							}
							return false;
						}
						template<typename I, std::enable_if_t<std::is_integral_v<I>>* = nullptr> bool CLOAK_CALL_THIS operator==(In I x) const
						{
							if (m_nan) { return false; }
							if (x == 0) { return m_len == 0; }
							const size_t lb = m_len * sizeof(type);
							if (lb <= sizeof(I))
							{
								uint8_t* a = reinterpret_cast<uint8_t*>(m_data);
								uint8_t* b = reinterpret_cast<uint8_t*>(&x);
								for (size_t c = 0; c < sizeof(I); c++)
								{
									if ((b[c] != 0 && c >= lb) || a[c] != b[c]) { return false; }
								}
								return true;
							}
							return false;
						}
						template<typename A> bool CLOAK_CALL_THIS operator!=(In const BigInteger<T, A>& x) const
						{
							return !operator==(x);
						}
						template<typename I, std::enable_if_t<std::is_integral_v<I>>* = nullptr> bool CLOAK_CALL_THIS operator!=(In I x) const
						{
							return !operator==(x);
						}
						template<typename A> bool CLOAK_CALL_THIS operator>(In const BigInteger<T, A>& x) const
						{
							if (m_nan || x.NaN()) { return false; }
							return cmp(x) > 0;
						}
						template<typename I, std::enable_if_t<std::is_integral_v<I>>* = nullptr> bool CLOAK_CALL_THIS operator>(In I x) const
						{
							if (m_nan || x.NaN()) { return false; }
							return cmp(x) > 0;
						}
						template<typename A> bool CLOAK_CALL_THIS operator>=(In const BigInteger<T, A>& x) const
						{
							if (m_nan || x.NaN()) { return false; }
							return cmp(x) >= 0;
						}
						template<typename I, std::enable_if_t<std::is_integral_v<I>>* = nullptr> bool CLOAK_CALL_THIS operator>=(In I x) const
						{
							if (m_nan || x.NaN()) { return false; }
							return cmp(x) >= 0;
						}
						template<typename A> bool CLOAK_CALL_THIS operator<(In const BigInteger<T, A>& x) const
						{
							if (m_nan || x.NaN()) { return false; }
							return cmp(x) < 0;
						}
						template<typename I, std::enable_if_t<std::is_integral_v<I>>* = nullptr> bool CLOAK_CALL_THIS operator<(In I x) const
						{
							if (m_nan || x.NaN()) { return false; }
							return cmp(x) < 0;
						}
						template<typename A> bool CLOAK_CALL_THIS operator<=(In const BigInteger<T, A>& x) const
						{
							if (m_nan || x.NaN()) { return false; }
							return cmp(x) <= 0;
						}
						template<typename I, std::enable_if_t<std::is_integral_v<I>>* = nullptr> bool CLOAK_CALL_THIS operator<=(In I x) const
						{
							if (m_nan || x.NaN()) { return false; }
							return cmp(x) <= 0;
						}

						//Arithmetic operators:
						BigInteger<T, Allocator> CLOAK_CALL_THIS operator-() const
						{
							BigInteger<T, Allocator> r = *this;
							r.m_neg = !m_neg;
							return r;
						}
						BigInteger<T, Allocator>& CLOAK_CALL_THIS operator++()
						{
							if (m_nan) { return *this; }
							if (m_neg == true)
							{
								CLOAK_ASSUME(m_len > 0);
								bool c = true;
								for (size_t a = 0; c; a++)
								{
									c = (m_data[a] == 0);
									m_data[a]--;
								}
								if (m_data[m_len - 1] == 0)
								{
									m_len--;
									if (m_len == 0) { m_neg = false; }
								}
								return *this;
							}
							for (size_t a = 0; a < m_len; a++)
							{
								m_data[a]++;
								if (m_data[a] != 0) { return *this; }
							}
							allocate(m_len + 1);
							m_data[m_len++] = 1;
							return *this;
						}
						BigInteger<T, Allocator>& CLOAK_CALL_THIS operator--()
						{
							if (m_nan) { return *this; }
							if (m_neg == false)
							{
								if (m_len > 0)
								{
									bool c = true;
									for (size_t a = 0; c; a++)
									{
										c = (m_data[a] == 0);
										m_data[a]--;
									}
									if (m_data[m_len - 1] == 0) { m_len--; }
									return *this;
								}
								allocate(1);
								m_data[0] = 1;
								m_len = 1;
								m_neg = true;
								return *this;
							}
							for (size_t a = 0; a < m_len; a++)
							{
								m_data[a]++;
								if (m_data[a] != 0) { return *this; }
							}
							allocate(m_len + 1);
							m_data[m_len++] = 1;
							return *this;
						}
						BigInteger<T, Allocator> CLOAK_CALL_THIS operator++(int)
						{
							BigInteger<T, Allocator> r = *this;
							operator++();
							return r;
						}
						BigInteger<T, Allocator> CLOAK_CALL_THIS operator--(int)
						{
							BigInteger<T, Allocator> r = *this;
							operator--();
							return r;
						}
						template<typename A> BigInteger<T, Allocator> CLOAK_CALL_THIS operator+(In const BigInteger<T, A>& x) const
						{
							BigInteger<T, Allocator> r = *this;
							r += x;
							return r;
						}
						template<typename A> BigInteger<T, Allocator> CLOAK_CALL_THIS operator-(In const BigInteger<T, A>& x) const
						{
							BigInteger<T, Allocator> r = *this;
							r -= x;
							return r;
						}
						template<typename A> BigInteger<T, Allocator> CLOAK_CALL_THIS operator*(In const BigInteger<T, A>& x) const
						{
							BigInteger<T, Allocator> r = *this;
							r *= x;
							return r;
						}
						template<typename A> BigInteger<T, Allocator> CLOAK_CALL_THIS operator/(In const BigInteger<T, A>& x) const
						{
							BigInteger<T, Allocator> r = *this;
							r /= x;
							return r;
						}
						template<typename A> BigInteger<T, Allocator> CLOAK_CALL_THIS operator%(In const BigInteger<T, A>& x) const
						{
							BigInteger<T, Allocator> r = *this;
							r %= x;
							return r;
						}
						template<typename A> BigInteger<T, Allocator>& CLOAK_CALL_THIS operator+=(In const BigInteger<T, A>& x)
						{
							if (m_nan || x.NaN()) { m_nan = true; return *this; }
							if (m_len == 0) { *this = x; return *this; }
							if (x.size() == 0) { return *this; }
							if (x.negative() != m_neg) { sub(x); }
							else { add(x); }
							return *this;
						}
						template<typename A> BigInteger<T, Allocator>& CLOAK_CALL_THIS operator-=(In const BigInteger<T, A>& x)
						{
							if (m_nan || x.NaN()) { m_nan = true; return *this; }
							if (m_len == 0) { *this = x; return *this; }
							if (x.size() == 0) { return *this; }
							if (x.negative() != m_neg) { add(x); }
							else { sub(x); }
							return *this;
						}
						template<typename A> BigInteger<T, Allocator>& CLOAK_CALL_THIS operator*=(In const BigInteger<T, A>& x)
						{
							if (m_nan || x.NaN()) { m_nan = true; return *this; }
							if (m_len == 0 || x.size() == 0) { m_len = 0; return *this; }
							m_neg = (m_neg != x.negative());
							allocate(m_len + x.size());
							bool ci = false, co = false;
							for (size_t a = m_len; a > 0; a--)
							{
								for (size_t b = BIT_SIZE; b > 0; b--)
								{
									if ((m_data[a - 1] & (type(1) << (b - 1))) != 0)
									{
										ci = false;
										size_t d = a - 1;
										for (size_t c = 0; c <= x.size(); c++, d++)
										{
											type t = m_data[d] + shiftedBlock(x, c, b - 1);
											co = (t < m_data[d]);
											if (ci == true)
											{
												t++;
												co |= (t == 0);
											}
											CLOAK_ASSUME(d >= a || (m_data[d] & ((type(1) << (b - 1)) - 1)) == (t & ((type(1) << (b - 1)) - 1)));
											m_data[d] = t;
											ci = co;
										}
									}
								}
							}
							m_len = m_cap;
							while (m_len > 0 && m_data[m_len - 1] == 0) { m_len--; }
							return *this;
						}
						template<typename A> BigInteger<T, Allocator>& CLOAK_CALL_THIS operator/=(In const BigInteger<T, A>& x)
						{
							if (m_nan || x.NaN() || x.size() == 0) { m_nan = true; return *this; }
							if (m_len == 0) { return *this; }
							m_neg = (m_neg != x.negative());
							BigInteger<T, Allocator> r = *this;
							r.divide(x, this);
							return *this;
						}
						template<typename A> BigInteger<T, Allocator>& CLOAK_CALL_THIS operator%=(In const BigInteger<T, A>& x)
						{
							if (m_nan || x.NaN() || x.size() == 0) { m_nan = true; return *this; }
							if (m_len == 0) { return *this; }
							m_neg = (m_neg != x.negative());
							divide(x, nullptr);
							return *this;
						}

						//Binary operators
						template<typename A> BigInteger<T, Allocator> CLOAK_CALL_THIS operator&(In const BigInteger<T, A>& x) const
						{
							BigInteger<T, Allocator> r = *this;
							r &= x;
							return r;
						}
						template<typename A> BigInteger<T, Allocator> CLOAK_CALL_THIS operator|(In const BigInteger<T, A>& x) const
						{
							BigInteger<T, Allocator> r = *this;
							r |= x;
							return r;
						}
						template<typename A> BigInteger<T, Allocator> CLOAK_CALL_THIS operator^(In const BigInteger<T, A>& x) const
						{
							BigInteger<T, Allocator> r = *this;
							r ^= x;
							return r;
						}
						BigInteger<T, Allocator> CLOAK_CALL_THIS operator<<(In int x) const
						{
							BigInteger<T, Allocator> r = *this;
							r <<= x;
							return r;
						}
						BigInteger<T, Allocator> CLOAK_CALL_THIS operator>>(In int x) const
						{
							BigInteger<T, Allocator> r = *this;
							r >>= x;
							return r;
						}
						template<typename A> BigInteger<T, Allocator>& CLOAK_CALL_THIS operator&=(In const BigInteger<T, A>& x)
						{
							if (m_nan || x.NaN()) { m_nan = true; return *this; }
							m_neg &= x.negative();
							if (m_len > x.size()) { m_len = x.size(); }
							for (size_t a = 0; a < m_len; a++) { m_data[a] &= x[a]; }
							while (m_len > 0 && m_data[m_len - 1] == 0) { m_len--; }
							return *this;
						}
						template<typename A> BigInteger<T, Allocator>& CLOAK_CALL_THIS operator|=(In const BigInteger<T, A>& x)
						{
							if (m_nan || x.NaN()) { m_nan = true; return *this; }
							m_neg |= x.negative();
							if (m_len < x.size()) { m_len = x.size(); }
							allocate(m_len);
							for (size_t a = 0; a < x.size(); a++) { m_data[a] |= x[a]; }
							return *this;
						}
						template<typename A> BigInteger<T, Allocator>& CLOAK_CALL_THIS operator^=(In const BigInteger<T, A>& x)
						{
							if (m_nan || x.NaN()) { m_nan = true; return *this; }
							m_neg ^= x.negative();
							if (m_len < x.size()) { m_len = x.size(); }
							allocate(m_len);
							for (size_t a = 0; a < x.size(); a++) { m_data[a] ^= x[a]; }
							while (m_len > 0 && m_data[m_len - 1] == 0) { m_len--; }
							return *this;
						}
						BigInteger<T, Allocator>& CLOAK_CALL_THIS operator<<=(In int x)
						{
							if (m_nan) { return *this; }
							if (x < 0) { return operator>>=(-x); }
							else if (x == 0) { return *this; }
							const size_t sBl = x / BIT_SIZE;
							const size_t sBi = x % BIT_SIZE;
							allocate(m_len + sBl + 1);
							for (size_t a = m_len + 1; a > 0; a--) { m_data[a + sBl - 1] = shiftedBlock(*this, a - 1, sBi); }
							for (size_t a = 0; a < sBl; a++) { m_data[a] = 0; }
							m_len += sBl + 1;
							if (m_data[m_len - 1] == 0) { m_len--; }
							return *this;
						}
						BigInteger<T, Allocator>& CLOAK_CALL_THIS operator>>=(In int x)
						{
							if (m_nan) { return *this; }
							if (x < 0) { return operator<<=(-x); }
							else if (x == 0) { return *this; }
							const size_t sBl = (x + BIT_SIZE - 1) / BIT_SIZE;
							const size_t sBi = (BIT_SIZE * sBl) - x;
							if (sBl >= m_len + 1) { m_len = 0; return *this; }
							allocate(m_len + 1 - sBl);
							for (size_t a = sBl; a <= m_len; a++) { m_data[a - sBl] = shiftedBlock(*this, a, sBi); }
							m_len = m_len + 1 - sBl;
							if (m_data[m_len - 1] == 0) { m_len--; }
							return *this;
						}

						//Additional Functions:
						template<typename A, typename B> BigInteger<T, Allocator> CLOAK_CALL_THIS pow_mod(In const BigInteger<T, A>& exponent, In const BigInteger<T, B>& divider) const
						{
							BigInteger<T, Allocator> r = 1;
							BigInteger<T, Allocator> base = (*this) % divider;
							BigInteger<T, A> exp = exponent;

							while (exp != 0)
							{
								if ((exp[0] & 0x1) != 0)
								{
									exp--;
									r *= base;
									r %= divider;
									if (exp == 0) { break; }
								}
								exp >>= 1;
								base *= base;
								base %= divider;
							}
							return r;
						}

						//new/delete operators
						void* CLOAK_CALL operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
						void* CLOAK_CALL operator new[](In size_t s) { return API::Global::Memory::MemoryHeap::Allocate(s); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }
						void CLOAK_CALL operator delete[](In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }
				};
			}
		}
	}
}

typedef CloakEngine::API::Global::Math::BigInteger<> int_bit_t;

#undef HAS_CPUID
#undef HAS_POPCOUNT_BUILTIN
#undef HAS_POPCOUNT_ASM
#undef HAS_POPCNT
#ifdef POP_MACRO_MIN
#	pragma pop_macro("min")
#	undef POP_MACRO_MIN
#endif
#ifdef POP_MACRO_MAX
#	pragma pop_macro("max")
#	undef POP_MACRO_MAX
#endif

#endif