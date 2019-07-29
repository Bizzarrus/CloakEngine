#pragma once
#ifndef CE_API_HELPER_REFLECTION_H
#define CE_API_HELPER_REFLECTION_H

//type id / type name based on CTTI ( https://github.com/Manu343726/ctti )

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/Types.h"
#include "CloakEngine/Helper/StringConvert.h"
#include "CloakEngine/OS.h"

#include <string>
#include <string_view>

#if CHECK_COMPILER(Clang)
#	define CE_PRETTY_FUNCTION __PRETTY_FUNCTION__
#	define CE_PRETTY_FUNCTION_PREFIX "static std::string_view CloakEngine::Reflection::PrettyName() [T = "
#	define CE_PRETTY_FUNCTION_SUFFIX "]"
#elif CHECK_COMPILER(GCC)
#	define CE_PRETTY_FUNCTION __PRETTY_FUNCTION__
#	define CE_PRETTY_FUNCTION_PREFIX "static constexpr std::string_view CloakEngine::Reflection::PrettyName() [with T = "
#	define CE_PRETTY_FUNCTION_SUFFIX "; std::string_view = std::basic_string_view<char>]"
#elif CHECK_COMPILER(MicrosoftVisualCpp)
#	define CE_PRETTY_FUNCTION __FUNCSIG__
#	ifdef __INTELLISENSE__ 
#		define CE_PRETTY_FUNCTION_PREFIX "std::basic_string_view<char, std::char_traits<char>> CloakEngine::Reflection::PrettyName<"
#		define CE_PRETTY_FUNCTION_SUFFIX ">()"
#	else
#		define CE_PRETTY_FUNCTION_PREFIX "class std::basic_string_view<char,struct std::char_traits<char> > __cdecl CloakEngine::Reflection::PrettyName<"
#		define CE_PRETTY_FUNCTION_SUFFIX ">(void)"
#	endif
#else
#	warning "Reflections is not yet implemented for this compiler, using fallback"
#	define CE_SKIP_REFLECTION
#endif

namespace CloakEngine {
	struct Reflection {
		private:
			constexpr CLOAK_CALL Reflection() {}

			static constexpr uint64_t FNV_BASIS = 14695981039346656037Ui64;
			static constexpr uint64_t FNV_PRIME = 1099511628211Ui64;

			static constexpr uint64_t CLOAK_CALL loW(In uint64_t x) { return x & 0xFFFFFFFF; }
			static constexpr uint64_t CLOAK_CALL hiW(In uint64_t x) { return x >> 32; }
			static constexpr uint64_t CLOAK_CALL mul(In uint64_t a, In uint64_t b)
			{
				const uint64_t c = loW(a)*loW(b);
				return (c & 0xFFFFFFFF) + ((hiW(c) + ((loW(a) * hiW(b)) & 0xFFFFFFFF) + ((hiW(a) * loW(b)) & 0xFFFFFFFF)) << 32);
			}
			template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>> static constexpr uint64_t CLOAK_CALL CalcHash(In T x, In_opt uint64_t h = FNV_BASIS)
			{
				for (size_t a = 0; a < sizeof(T); a++)
				{
					h = mul(h ^static_cast<uint8_t>((x >> (a << 3)) & 0xFF), FNV_PRIME);
				}
				return h;
			}
			template<typename T, size_t N> static constexpr uint64_t CLOAK_CALL CalcHash(In T(&x)[N], In_opt uint64_t h = FNV_BASIS)
			{
				for (size_t a = 0; a < N; a++) { h = CalcHash(x[a], h); }
				return h;
			}
			static constexpr uint64_t CLOAK_CALL CalcHash(In const GUID& s, In_opt uint64_t h = FNV_BASIS)
			{
				h = CalcHash(s.Data1, h);
				h = CalcHash(s.Data2, h);
				h = CalcHash(s.Data3, h);
				h = CalcHash(s.Data4, h);
				return h;
			}
			static constexpr uint64_t CLOAK_CALL CalcHash(In const std::string_view& s, In_opt uint64_t h = FNV_BASIS)
			{
				for (size_t a = 0; a < s.length(); a++) { h = mul(h ^ s[a], FNV_PRIME); }
				return h;
			}

#ifdef CE_SKIP_REFLECTION
		public:
			struct type_name_t {
				friend class Reflections;
				private:
					GUID m_data;
					
					constexpr CLOAK_CALL type_name_t(In const GUID& g) : m_data(g) {}
				public:
					constexpr CLOAK_CALL type_name_t() : type_name_t({ 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } }) {}

					constexpr uint64_t CLOAK_CALL_THIS hash() const { return calc_hash(m_data); }

					constexpr type_name_t& CLOAK_CALL_THIS operator=(In const type_name_t&) = default;
					constexpr bool CLOAK_CALL_THIS operator==(In const type_name_t& o) const { return hash() == o.hash(); }
					constexpr bool CLOAK_CALL_THIS operator!=(In const type_name_t& o) const { return hash() != o.hash(); }
					constexpr bool CLOAK_CALL_THIS operator<(In const type_name_t& o) const { return hash() < o.hash(); }
					constexpr bool CLOAK_CALL_THIS operator<=(In const type_name_t& o) const { return hash() <= o.hash(); }
					constexpr bool CLOAK_CALL_THIS operator>(In const type_name_t& o) const { return hash() > o.hash(); }
					constexpr bool CLOAK_CALL_THIS operator>=(In const type_name_t& o) const { return hash() >= o.hash(); }

					std::string CLOAK_CALL_THIS str() const 
					{
						OLECHAR* s;
						StringFromCLSID(m_data, &s);
						std::string res = API::Helper::StringConvert::ConvertToU8(s);
						CoTaskMemFree(s);
						return res;
					}
			};
#else
			static constexpr size_t PRETTY_FUNCTION_LEFT = ARRAYSIZE(CE_PRETTY_FUNCTION_PREFIX) - 1;
			static constexpr size_t PRETTY_FUNCTION_RIGHT = ARRAYSIZE(CE_PRETTY_FUNCTION_SUFFIX) - 1;

			static constexpr std::string_view CLOAK_CALL FilterPrefix(In const std::string_view& s, In const std::string_view& p)
			{
				if (s.length() < p.length()) { return s; }
				if (s.substr(0, p.length()) == p) { return s.substr(p.length()); }
				return s;
			}
			static constexpr std::string_view CLOAK_CALL FilterPading(In const std::string_view& s)
			{
				size_t p = 0;
				while (p < s.length() && s[p] == ' ') { p++; }
				return s.substr(p);
			}
			static constexpr std::string_view CLOAK_CALL FilterTypename(In const std::string_view& s)
			{
				std::string_view p = FilterPading(s);
				p = FilterPading(FilterPrefix(p, "class"));
				p = FilterPading(FilterPrefix(p, "struct"));
				return p;
			}
			//No calling convention for "PrettyName", so we don't have to worry about defining it in the prefix string
			template<typename T> static constexpr std::string_view PrettyName() { return std::string_view(CE_PRETTY_FUNCTION); }

			template<typename T, typename = void> struct name_of_t {
				private:
					static constexpr std::string_view CLOAK_CALL Calc() 
					{
						std::string_view t = PrettyName<T>();
						t = t.substr(PRETTY_FUNCTION_LEFT, t.length() - (PRETTY_FUNCTION_RIGHT + PRETTY_FUNCTION_LEFT));
						return FilterTypename(t);
					}
				public:
					static constexpr std::string_view value = Calc();
			};
			template<typename T> struct name_of_t<T, typename type_if<false, decltype(T::name_of)>::type> {
				static constexpr std::string_view value = T::name_of;
			};
			template<typename T> struct name_of_t<API::RefPointer<T>> {
				static constexpr std::string_view value = name_of_t<T>::value;
			};
		public:
			struct type_name_t {
				friend struct Reflection;
				private:
					std::string_view m_name;
					uint64_t m_hash;

					constexpr CLOAK_CALL type_name_t(In const std::string_view& c) : m_name(c), m_hash(CalcHash(c)) {}
				public:
					typedef std::string_view::iterator iterator;
					typedef std::string_view::reverse_iterator reverse_iterator;
					typedef std::string_view::const_iterator const_iterator;
					typedef std::string_view::const_reverse_iterator const_reverse_iterator;
					typedef std::string_view::pointer pointer;
					typedef std::string_view::const_pointer const_pointer;
					typedef std::string_view::reference reference;
					typedef std::string_view::const_reference const_reference;
					
					constexpr CLOAK_CALL type_name_t() : type_name_t(std::string_view("void")) {}
					constexpr CLOAK_CALL type_name_t(In const type_name_t&) = default;

					constexpr iterator CLOAK_CALL_THIS begin() { return m_name.begin(); }
					constexpr const_iterator CLOAK_CALL_THIS begin() const { return m_name.begin(); }
					constexpr const_iterator CLOAK_CALL_THIS cbegin() const { return m_name.cbegin(); }
					constexpr reverse_iterator CLOAK_CALL_THIS rbegin() { return m_name.rbegin(); }
					constexpr const_reverse_iterator CLOAK_CALL_THIS rbegin() const { return m_name.rbegin(); }
					constexpr const_reverse_iterator CLOAK_CALL_THIS crbegin() const { return m_name.crbegin(); }

					constexpr iterator CLOAK_CALL_THIS end() { return m_name.end(); }
					constexpr const_iterator CLOAK_CALL_THIS end() const { return m_name.end(); }
					constexpr const_iterator CLOAK_CALL_THIS cend() const { return m_name.cend(); }
					constexpr reverse_iterator CLOAK_CALL_THIS rend() { return m_name.rend(); }
					constexpr const_reverse_iterator CLOAK_CALL_THIS rend() const { return m_name.rend(); }
					constexpr const_reverse_iterator CLOAK_CALL_THIS crend() const { return m_name.crend(); }

					constexpr uint64_t CLOAK_CALL_THIS hash() const { return m_hash; }
					constexpr type_name_t& CLOAK_CALL_THIS operator=(In const type_name_t&) = default;
					constexpr bool CLOAK_CALL_THIS operator==(In const type_name_t& o) const { return m_hash == o.m_hash && m_name == o.m_name; }
					constexpr bool CLOAK_CALL_THIS operator!=(In const type_name_t& o) const { return m_hash != o.m_hash || m_name != o.m_name; }
					constexpr bool CLOAK_CALL_THIS operator<(In const type_name_t& o) const { return m_name < o.m_name; }
					constexpr bool CLOAK_CALL_THIS operator<=(In const type_name_t& o) const { return m_name <= o.m_name; }
					constexpr bool CLOAK_CALL_THIS operator>(In const type_name_t& o) const { return m_name > o.m_name; }
					constexpr bool CLOAK_CALL_THIS operator>=(In const type_name_t& o) const { return m_name >= o.m_name; }

					std::string CLOAK_CALL_THIS str() const { return static_cast<std::string>(m_name); }
			};
#endif
			struct type_id_t {
				friend struct Reflection;
				private:
					uint64_t m_hash;

					constexpr CLOAK_CALL type_id_t(In const type_name_t& o) : m_hash(o.hash()) {}
					constexpr CLOAK_CALL type_id_t(In uint64_t h) : m_hash(h) {}
				public:
					constexpr CLOAK_CALL type_id_t() : m_hash(type_name_t().hash()) {}
					constexpr CLOAK_CALL type_id_t(In const type_id_t&) = default;

					constexpr uint64_t CLOAK_CALL_THIS hash() const { return m_hash; }

					constexpr type_id_t& CLOAK_CALL_THIS operator=(In const type_id_t&) = default;
					constexpr bool CLOAK_CALL_THIS operator==(In const type_id_t& o) const { return m_hash == o.m_hash; }
					constexpr bool CLOAK_CALL_THIS operator!=(In const type_id_t& o) const { return m_hash != o.m_hash; }
					constexpr bool CLOAK_CALL_THIS operator<(In const type_id_t& o) const { return m_hash < o.m_hash; }
					constexpr bool CLOAK_CALL_THIS operator<=(In const type_id_t& o) const { return m_hash <= o.m_hash; }
					constexpr bool CLOAK_CALL_THIS operator>(In const type_id_t& o) const { return m_hash > o.m_hash; }
					constexpr bool CLOAK_CALL_THIS operator>=(In const type_id_t& o) const { return m_hash >= o.m_hash; }
			};

#ifdef CE_SKIP_REFLECTION
			template<typename T> static constexpr type_name_t CLOAK_CALL name_of_type() { return type_name_t(__uuidof(T)); }
#else
			template<typename T> static constexpr type_name_t CLOAK_CALL name_of_type() { return type_name_t(name_of_t<T>::value); }
#endif
			template<typename T> static constexpr type_id_t CLOAK_CALL id_of_type() { return type_id_t(name_of_type<T>()); }
	};

	typedef Reflection::type_name_t type_name;
	typedef Reflection::type_id_t type_id;
	template<typename T> constexpr type_name CLOAK_CALL name_of_type() { return Reflection::name_of_type<typename std::remove_cv<T>::type>(); }
	template<typename T> constexpr type_id CLOAK_CALL id_of_type() { return Reflection::id_of_type<typename std::remove_cv<T>::type>(); }
}

#ifdef CE_SKIP_REFLECTION
#	undef CE_SKIP_REFLECTION
#endif
#ifdef CE_PRETTY_FUNCTION
#	undef CE_PRETTY_FUNCTION
#	undef CE_PRETTY_FUNCTION_PREFIX
#	undef CE_PRETTY_FUNCTION_SUFFIX
#endif

#endif