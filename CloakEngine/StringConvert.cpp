#include "stdafx.h"
#include "CloakEngine/Helper/StringConvert.h"
#include "CloakEngine/Global/Log.h"

#include <sstream>

namespace CloakEngine {
	namespace API {
		namespace Helper {
			namespace StringConvert {
				constexpr size_t ONE_MASK = static_cast<size_t>(-1) / 0xFF;

				template<typename T> struct utf_run {};
				template<> struct utf_run<char> {
					static inline char32_t CLOAK_CALL process(In const std::basic_string<char>& data, Inout size_t* pos)
					{
						CLOAK_ASSUME((*pos) < data.length());
						char32_t c1 = data[(*pos)++];
						if (c1 >= 0xF0)
						{
							CLOAK_ASSUME((*pos) < data.length());
							char32_t c2 = data[(*pos)++];
							CLOAK_ASSUME((*pos) < data.length());
							char32_t c3 = data[(*pos)++];
							CLOAK_ASSUME((*pos) < data.length());
							char32_t c4 = data[(*pos)++];
							return ((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | ((c4 & 0x3F) << 0);
						}
						else if (c1 >= 0xE0)
						{
							CLOAK_ASSUME((*pos) < data.length());
							char32_t c2 = data[(*pos)++];
							CLOAK_ASSUME((*pos) < data.length());
							char32_t c3 = data[(*pos)++];
							return ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | ((c3 & 0x3F) << 0);
						}
						else if (c1 >= 0xC0)
						{
							CLOAK_ASSUME((*pos) < data.length());
							char32_t c2 = data[(*pos)++];
							return ((c1 & 0x1F) << 6) | ((c2 & 0x3F) << 0);
						}
						return c1 & 0x7F;
					}
					static inline void CLOAK_CALL write(Inout std::basic_stringstream<char>& s, In char32_t c)
					{
						CLOAK_ASSUME(c <= 0x10FFFF);
						if (c >= 0x10000)
						{
							s << static_cast<char>(0xF0 | ((c >> 18) & 0x07)) << static_cast<char>(0x80 | ((c >> 12) & 0x3F)) << static_cast<char>(0x80 | ((c >> 6) & 0x3F)) << static_cast<char>(0x80 | ((c >> 0) & 0x3F));
						}
						else if (c >= 0x00800)
						{
							s << static_cast<char>(0xE0 | ((c >> 12) & 0x0F)) << static_cast<char>(0x80 | ((c >> 6) & 0x3F)) << static_cast<char>(0x80 | ((c >> 0) & 0x3F));
						}
						else if (c >= 0x00080)
						{
							s << static_cast<char>(0xC0 | ((c >> 6) & 0x1F)) << static_cast<char>(0x80 | ((c >> 0) & 0x3F));
						}
						else
						{
							s << static_cast<char>(c);
						}
					}
				};
				template<> struct utf_run<char16_t> {
					static inline char32_t CLOAK_CALL process(In const std::basic_string<char16_t>& data, Inout size_t* pos)
					{
						CLOAK_ASSUME((*pos) < data.length());
						const char32_t c1 = data[(*pos)++];
						if ((c1 & 0xFC00) == 0xD800)
						{
							CLOAK_ASSUME((*pos) < data.length());
							const char32_t c2 = data[(*pos)++];
							return (((c1 & 0x3FF) << 10) | ((c2 & 0x3FF) << 0)) + 0x10000;
						}
						return c1;
					}
					static inline void CLOAK_CALL write(Inout std::basic_stringstream<char16_t>& s, In char32_t c)
					{
						CLOAK_ASSUME(c < 0xD800 || c>0xDFFF);
						if (c > 0xFFFF)
						{
							c -= 0x10000;
							s << static_cast<char16_t>(0xD800 | ((c & 0xFFC0) >> 10)) << static_cast<char16_t>(0xDC00 | ((c & 0x03FF) >> 0));
						}
						else
						{
							s << static_cast<char16_t>(c);
						}
					}
				};
				template<> struct utf_run<char32_t> {
					static inline char32_t CLOAK_CALL process(In const std::basic_string<char32_t>& data, Inout size_t* pos)
					{ 
						CLOAK_ASSUME((*pos) < data.length());
						return data[(*pos)++]; 
					}
					static inline void CLOAK_CALL write(Inout std::basic_stringstream<char32_t>& s, In char32_t c)
					{
						s << c;
					}
				};

				template<typename A, typename B> struct utf_converter {
					private:
						typedef utf_run<A> run_in;
						typedef utf_run<B> run_out;
						typedef std::basic_stringstream<B> out_stream;
					public:
						typedef std::basic_string<A> in_type;
						typedef std::basic_string<B> out_type;

						static inline out_type CLOAK_CALL convert(In const in_type& a)
						{
							out_stream s;
							for (size_t p = 0; p < a.length();) { run_out::write(s, run_in::process(a, &p)); }
							return s.str();
						}
				};
				template<typename A> struct utf_converter<A, A> {
					typedef std::basic_string<A> in_type;
					typedef std::basic_string<A> out_type;

					static inline const out_type& CLOAK_CALL convert(In const in_type& a) { return a; }
				};
				template<typename A> struct utf_converter<A, wchar_t> {
					private:
						typedef utf_converter<A, char> base_converter;
						typedef std::basic_string<char> mid_type;
					public:
						typedef std::basic_string<A> in_type;
						typedef std::basic_string<wchar_t> out_type;

						static inline out_type CLOAK_CALL convert(In const in_type& a)
						{
							mid_type mid = base_converter::convert(a);
							if (mid.length() == 0) { return out_type(); }
							const int sizeReq = MultiByteToWideChar(CP_UTF8, 0, &mid[0], static_cast<int>(mid.size())/sizeof(char), nullptr, 0);
							out_type res(sizeReq, 0);
							MultiByteToWideChar(CP_UTF8, 0, &mid[0], static_cast<int>(mid.size())/sizeof(char), &res[0], sizeReq);
							return res;
						}
				};
				template<typename A> struct utf_converter<wchar_t, A> {
					private:
						typedef utf_converter<char, A> base_converter;
						typedef std::basic_string<char> mid_type;
					public:
						typedef std::basic_string<wchar_t> in_type;
						typedef std::basic_string<A> out_type;

						static inline out_type CLOAK_CALL convert(In const in_type& a)
						{
							mid_type mid = mid_type();
							if (a.length() > 0)
							{
								const int sizeReq = WideCharToMultiByte(CP_UTF8, 0, &a[0], static_cast<int>(a.size()), nullptr, 0, nullptr, nullptr);
								mid = mid_type(sizeReq, 0);
								WideCharToMultiByte(CP_UTF8, 0, &a[0], static_cast<int>(a.size()), &mid[0], sizeReq, nullptr, nullptr);
							}
							return base_converter::convert(mid);
						}
				};
				template<> struct utf_converter<wchar_t, wchar_t> {
					typedef std::basic_string<wchar_t> in_type;
					typedef std::basic_string<wchar_t> out_type;

					static inline const out_type& CLOAK_CALL convert(In const in_type& a) { return a; }
				};

#ifdef UNICODE
				typedef wchar_t tchar_t;
#else
				typedef char tchar_t;
#endif

				typedef utf_converter<char, char>			utf8_to_utf8;
				typedef utf_converter<char, char16_t>		utf8_to_utf16;
				typedef utf_converter<char, char32_t>		utf8_to_utf32;
				typedef utf_converter<char, wchar_t>		utf8_to_wchar;
				typedef utf_converter<char, tchar_t>		utf8_to_tchar;

				typedef utf_converter<char16_t, char>		utf16_to_utf8;
				typedef utf_converter<char16_t, char16_t>	utf16_to_utf16;
				typedef utf_converter<char16_t, char32_t>	utf16_to_utf32;
				typedef utf_converter<char16_t, wchar_t>	utf16_to_wchar;
				typedef utf_converter<char16_t, tchar_t>	utf16_to_tchar;

				typedef utf_converter<char32_t, char>		utf32_to_utf8;
				typedef utf_converter<char32_t, char16_t>	utf32_to_utf16;
				typedef utf_converter<char32_t, char32_t>	utf32_to_utf32;
				typedef utf_converter<char32_t, wchar_t>	utf32_to_wchar;
				typedef utf_converter<char32_t, tchar_t>	utf32_to_tchar;

				typedef utf_converter<wchar_t, char>		wchar_to_utf8;
				typedef utf_converter<wchar_t, char16_t>	wchar_to_utf16;
				typedef utf_converter<wchar_t, char32_t>	wchar_to_utf32;
				typedef utf_converter<wchar_t, wchar_t>		wchar_to_wchar;
				typedef utf_converter<wchar_t, tchar_t>		wchar_to_tchar;

				typedef utf_converter<tchar_t, char>		tchar_to_utf8;
				typedef utf_converter<tchar_t, char16_t>	tchar_to_utf16;
				typedef utf_converter<tchar_t, char32_t>	tchar_to_utf32;
				typedef utf_converter<tchar_t, wchar_t>		tchar_to_wchar;
				typedef utf_converter<tchar_t, tchar_t>		tchar_to_tchar;

				CLOAKENGINE_API std::string CLOAK_CALL ConvertToU8(In const std::u32string& str)
				{
					return utf32_to_utf8::convert(str);
				}
				CLOAKENGINE_API std::string CLOAK_CALL ConvertToU8(In const std::u16string& str)
				{
					return utf16_to_utf8::convert(str);
				}
				CLOAKENGINE_API std::string CLOAK_CALL ConvertToU8(In const std::string& str)
				{
					return utf8_to_utf8::convert(str);
				}
				CLOAKENGINE_API std::string CLOAK_CALL ConvertToU8(In const std::wstring& str)
				{
					return wchar_to_utf8::convert(str);
				}
				CLOAKENGINE_API std::u16string CLOAK_CALL ConvertToU16(In const std::string& str)
				{
					return utf8_to_utf16::convert(str);
				}
				CLOAKENGINE_API std::u16string CLOAK_CALL ConvertToU16(In const std::wstring& str)
				{
					return wchar_to_utf16::convert(str);
				}
				CLOAKENGINE_API std::u16string CLOAK_CALL ConvertToU16(In const std::u16string& str)
				{
					return utf16_to_utf16::convert(str);
				}
				CLOAKENGINE_API std::u16string CLOAK_CALL ConvertToU16(In const std::u32string& str)
				{
					return utf32_to_utf16::convert(str);
				}
				CLOAKENGINE_API std::wstring CLOAK_CALL ConvertToW(In const std::string& str)
				{
					return utf8_to_wchar::convert(str);
				}
				CLOAKENGINE_API std::wstring CLOAK_CALL ConvertToW(In const std::wstring& str)
				{
					return wchar_to_wchar::convert(str);
				}
				CLOAKENGINE_API std::wstring CLOAK_CALL ConvertToW(In const std::u16string& str)
				{
					return utf16_to_wchar::convert(str);
				}
				CLOAKENGINE_API std::wstring CLOAK_CALL ConvertToW(In const std::u32string& str) 
				{
					return utf32_to_wchar::convert(str);
				}
				CLOAKENGINE_API std::u32string CLOAK_CALL ConvertToU32(In const std::string& str)
				{
					return utf8_to_utf32::convert(str);
				}
				CLOAKENGINE_API std::u32string CLOAK_CALL ConvertToU32(In const std::wstring& str) 
				{
					return wchar_to_utf32::convert(str);
				}
				CLOAKENGINE_API std::u32string CLOAK_CALL ConvertToU32(In const std::u16string& str)
				{
					return utf16_to_utf32::convert(str);
				}
				CLOAKENGINE_API std::u32string CLOAK_CALL ConvertToU32(In const std::u32string& str)
				{
					return utf32_to_utf32::convert(str);
				}
				CLOAKENGINE_API std::tstring CLOAK_CALL ConvertToT(In const std::string& str) 
				{
					return utf8_to_tchar::convert(str);
				}
				CLOAKENGINE_API std::tstring CLOAK_CALL ConvertToT(In const std::wstring& str) 
				{
					return wchar_to_tchar::convert(str);
				}
				CLOAKENGINE_API std::tstring CLOAK_CALL ConvertToT(In const std::u16string& str) 
				{
					return utf16_to_tchar::convert(str);
				}
				CLOAKENGINE_API std::tstring CLOAK_CALL ConvertToT(In const std::u32string& str) 
				{
					return utf32_to_tchar::convert(str);
				}
				CLOAKENGINE_API char32_t CLOAK_CALL ConvertToU32(In const char16_t& str)
				{
					std::u16string s = u"";
					s += str;
					return ConvertToU32(s)[0];
				}
				CLOAKENGINE_API char32_t CLOAK_CALL ConvertToU32(In const char32_t& str)
				{
					return str;
				}
				CLOAKENGINE_API char32_t CLOAK_CALL ConvertToU32(In const wchar_t& str)
				{
					std::wstring s = L"";
					s += str;
					return ConvertToU32(s)[0];
				}
				CLOAKENGINE_API char16_t CLOAK_CALL ConvertToU16(In const char16_t& c)
				{
					return c;
				}
				CLOAKENGINE_API char16_t CLOAK_CALL ConvertToU16(In const char32_t& c)
				{
					std::u32string s = U"";
					s += c;
					return ConvertToU16(s)[0];
				}
				CLOAKENGINE_API char16_t CLOAK_CALL ConvertToU16(In const wchar_t& c)
				{
					std::wstring s = L"";
					s += c;
					return ConvertToU16(s)[0];
				}
				//Calculate actual length of utf8 string by using parallel computing and binary operators
				CLOAKENGINE_API size_t CLOAK_CALL UTF8Length(In const std::string& str)
				{
					uintptr_t start = reinterpret_cast<uintptr_t>(str.c_str());
					uintptr_t cur = start;
					size_t count = 0;
					size_t u = 0;
					uint8_t c;
					//Handle misaligned bytes
					for (; (cur & (sizeof(size_t) - 1)) != 0; cur++)
					{
						c = *reinterpret_cast<uint8_t*>(cur);
						if (c == 0) { return (cur - start) - count; }
						//Check if byte is not begin of character (so it starts with 10xxxxxx)
						count += (c >> 7) & ((~c) >> 6);
					}
					//Handle aligned byte blocks
					for (;; cur += sizeof(size_t))
					{
						u = *reinterpret_cast<const size_t*>(cur);
						//Break if any zero bytes:
						if (((u - ONE_MASK) & (~u) & (ONE_MASK << 7)) != 0) { break; }

						u = ((u & (ONE_MASK << 7)) >> 7) & ((~u) >> 6);
						count += (u*ONE_MASK) >> ((sizeof(size_t) - 1) << 3);
					}
					//Handle left over bytes
					for (;; cur++)
					{
						c = *reinterpret_cast<uint8_t*>(cur);
						if (c == 0) { break; }
						//Check if byte is not begin of character (so it starts with 10xxxxxx)
						count += (c >> 7) & ((~c) >> 6);
					}
					return (cur - start) - count;
				}
			}
		}
	}
}