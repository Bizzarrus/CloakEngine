#pragma once
#ifndef CE_API_HELPER_TYPES_H
#define CE_API_HELPER_TYPES_H

template<bool V, typename A, typename B = void> struct type_if { typedef B type; };
template<typename A, typename B> struct type_if<true, A, B> { typedef A type; };

template<unsigned int I, typename... A> struct type_by_size {
	private:
		template<unsigned int I, typename F, typename... X> struct type_find1 { typedef F type; };
		template<unsigned int I, typename F, typename A, typename... X> struct type_find1<I, F, A, X...> {
			typedef typename type_if<sizeof(A) >= I && sizeof(A) < sizeof(F), typename type_find1<I, A, X...>::type, typename type_find1<I, F, X...>::type>::type type;
		};

		template<unsigned int I, typename... X> struct type_find2 { typedef void type; };
		template<unsigned int I, typename F, typename... X> struct type_find2<I, F, X...> {
			typedef typename type_if<sizeof(F) >= I, typename type_find1<I, F, X...>::type, typename type_find2<I, X...>::type>::type type;
		};

		template<typename... X> struct check_types { static constexpr bool value = true; };
		template<typename A, typename B, typename... X> struct check_types<A, B, X...> {
			static constexpr bool value = sizeof(A) <= sizeof(B) && check_types<B, X...>::value;
		};
		static_assert(check_types<A...>::value, "Passed types must be ordered from lowest to highest");
	public:
		typedef typename type_find2<I, A...>::type type;
};

template<unsigned int I> struct int_by_size {
	typedef typename type_by_size<I, signed char, signed short, signed int, signed long int, signed long long>::type signed_t;
	typedef typename type_by_size<I, unsigned char, unsigned short, unsigned int, unsigned long int, unsigned long long>::type unsigned_t;
};

template<unsigned int I> struct float_by_size {
	typedef typename type_by_size<I, float, double, long double>::type type;
};

template<typename... A> struct largest_type {};
template<typename A> struct largest_type<A> { typedef A type; };
template<typename A, typename B> struct largest_type<A, B> {
	typedef typename type_if<sizeof(A) >= sizeof(B), A, B>::type type;
};
template<typename A, typename B, typename... C> struct largest_type<A, B, C...> {
	typedef typename largest_type<typename type_if<sizeof(A) >= sizeof(B), A, B>::type, C...>::type type;
};

template<typename... A> struct smallest_type {};
template<typename A> struct smallest_type<A> { typedef A type; };
template<typename A, typename B> struct smallest_type<A, B> {
	typedef typename type_if<sizeof(A) <= sizeof(B), A, B>::type type;
};
template<typename A, typename B, typename... C> struct smallest_type<A, B, C...> {
	typedef typename smallest_type<typename type_if<sizeof(A) <= sizeof(B), A, B>::type, C...>::type type;
};

typedef typename int_by_size<1>::signed_t	int8_t;
typedef typename int_by_size<2>::signed_t	int16_t;
typedef typename int_by_size<4>::signed_t	int32_t;
typedef typename int_by_size<8>::signed_t	int64_t;
typedef typename int_by_size<1>::unsigned_t uint8_t;
typedef typename int_by_size<2>::unsigned_t uint16_t;
typedef typename int_by_size<4>::unsigned_t uint32_t;
typedef typename int_by_size<8>::unsigned_t uint64_t;

typedef typename int_by_size<sizeof(void*)>::signed_t intptr_t;
typedef typename int_by_size<sizeof(void*)>::unsigned_t uintptr_t;
typedef intptr_t ptrdiff_t;

typedef typename int_by_size<sizeof(decltype(sizeof(void*)))>::unsigned_t size_t;

template<typename... A> struct number_of_types {};
template<> struct number_of_types<> { static constexpr size_t value = 0; };
template<typename A> struct number_of_types<A> { static constexpr size_t value = 1; };
template<typename A, typename... B> struct number_of_types<A, B...> { static constexpr size_t value = 1 + number_of_types<B...>::value; };

template<size_t N, typename... A> struct nth_type {};
template<size_t N> struct nth_type<N> { typedef void type; };
template<typename A> struct nth_type<0, A> { typedef A type; };
template<size_t N, typename A> struct nth_type<N, A> { typedef void type; };
template<typename A, typename... B> struct nth_type<0, A, B...> { typedef A type; };
template<size_t N, typename A, typename... B> struct nth_type<N, A, B...> { typedef typename nth_type<N - 1, B...>::type type; };

template<typename Type, typename... List> struct index_of_type {
	private:
		template<size_t N, typename A, typename B, typename... C> struct index_of_type_helper {
			static constexpr size_t value = index_of_type_helper<N + 1, A, C...>::value;
		};
		template<size_t N, typename A> struct index_of_type_helper<N, A, A> {
			static constexpr size_t value = N;
		};
		template<size_t N, typename A, typename... B> struct index_of_type_helper<N, A, A, B...> {
			static constexpr size_t value = N;
		};
		template<size_t N, typename A, typename B, typename... C> struct index_of_type_helper<N, A, const B, C...> {
			static constexpr size_t value = index_of_type_helper<N, A, B, C...>::value;
		};
		template<size_t N, typename A, typename B, typename... C> struct index_of_type_helper<N, A, volatile B, C...> {
			static constexpr size_t value = index_of_type_helper<N, A, B, C...>::value;
		};
		template<size_t N, typename A, typename B, typename... C> struct index_of_type_helper<N, A, B*, C...> {
			static constexpr size_t value = index_of_type_helper<N, A, B, C...>::value;
		};
		template<size_t N, typename A, typename B, typename... C> struct index_of_type_helper<N, A, B&, C...> {
			static constexpr size_t value = index_of_type_helper<N, A, B, C...>::value;
		};
		template<size_t N, typename A, typename B> struct index_of_type_helper<N, A, B> {
			static constexpr size_t value = N + 1;
		};

		template<typename A> struct decay {
			typedef A type;
		};
		template<typename A> struct decay<A&> {
			typedef typename decay<A>::type type;
		};
		template<typename A> struct decay<A*> {
			typedef typename decay<A>::type type;
		};
		template<typename A> struct decay<const A> {
			typedef typename decay<A>::type type;
		};
		template<typename A> struct decay<volatile A> {
			typedef typename decay<A>::type type;
		};
	public:
		static constexpr size_t value = index_of_type_helper<0, typename decay<Type>::type, List...>::value;
};

template<typename... X> struct type_tuple {
	static constexpr size_t Count = number_of_types<X...>::value;
	template<size_t N> using Get = typename nth_type<N, X...>::type;
};

template<typename T> struct remove_ref { typedef T type; };
template<typename T> struct remove_ref<T&> { typedef typename remove_ref<T>::type type; };

template<typename T> struct remove_pointer { typedef T type; };
template<typename T> struct remove_pointer<T*> { typedef typename remove_pointer<T>::type type; };

template<typename T> struct remove_rp { typedef typename remove_ref<typename remove_pointer<T>::type>::type type; };

//Calculates to byte offset of a given member to it's parent beginning
template<typename A, typename B> constexpr size_t offset_of(B A::*member) { return static_cast<size_t>(reinterpret_cast<uintptr_t>(&(static_cast<A*>(nullptr)->*member))); }
//Given a pointer to a member, returns the pointer to it's parent
template<typename A, typename B> A* get_from_member(B A::*member, const B* b)
{
	if (b == nullptr) { return nullptr; }
	return reinterpret_cast<A*>(reinterpret_cast<uintptr_t>(b) - offset_of(member)); 
}

template<typename T, size_t N> constexpr size_t array_size(T(&)[N]) { return N; }
template<typename T> constexpr inline bool IsFlagSet(T v, T mask, T option) { return (v & mask) == option; }
template<typename T> constexpr inline bool IsFlagSet(T v, T mask) { return IsFlagSet(v, mask, mask); }

template<typename T> constexpr void is_constexpr_helper(T&&) {}

#define is_constexpr(...) (!!(noexcept(is_constexpr_helper((__VA_ARGS__,0)))))

//Notice: Deteciting whether we are inside a constexpr context is currently not possible. In case this becomes possible in the future,
//we already include a macro to easily use this.
#define in_constexpr() (false)

#if (defined(in_constexpr) && in_constexpr() == true)
#	define opt_in_constexpr constexpr
#else
#	define opt_in_constexpr
#endif

#endif