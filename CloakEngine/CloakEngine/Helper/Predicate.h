#pragma once
#ifndef CE_API_HELPER_PREDICATE_H
#define CE_API_HELPER_PREDICATE_H

//This file implements the idea of STEVE DEWHURST for PAST (Predicate AST)
//Example Usage:
//	constexpr auto my_needs = (is_class && is_constructible) || is_pod
//	static_assert(satisfies<T>(my_needs),"Type T does not satisfy my needs");
//	template<typename T, typename = enable_if_satisfied<my_needs, T>> class MyClass {}; //using SFINAE to enable only if satisfied

#include <type_traits>
#include "CloakEngine/Helper/Types.h"

namespace CloakEngine {
	namespace Predicate {
		struct Node {};

#ifdef ALLOW_FOLD_EXPRESSIONS
		//Visual studio currently don't support fold expressions in a constexpr context
		template<bool... b> struct allTrue_t {
			static constexpr bool value = ... && b;
		};
#else
		template<bool... b> struct allTrue_t;
		template<bool a, bool... b> struct allTrue_t<a, b...> {
			static constexpr bool value = a && allTrue_t<b...>::value;
		};
		template<> struct allTrue_t<> {
			static constexpr bool value = true;
		};
#endif

		template<typename B, typename... T> struct derivedFrom_t;
		template<typename B, typename F, typename... T> struct derivedFrom_t<B, F, T...> {
			static constexpr bool value = std::is_convertible_v<F*, const volatile B*> && derivedFrom_t<B, T...>::value;
		};
		template<typename B> struct derivedFrom_t<B> {
			static constexpr bool value = true;
		};
		template<typename... T> using is_node = derivedFrom_t<Node, T...>;
		//Parameter Binding types:
		template<template<typename...> class P> struct id_t : public Node {
			template<typename T> static constexpr bool CLOAK_CALL value() { return P<T>::value; }
		};
		template<template<typename...> class P> constexpr auto ID() { return id_t<P>(); }
		template<template<typename, typename> class BP, typename First> struct bind1t_t : public Node {
			template<typename Second> static constexpr bool CLOAK_CALL value() { return BP<First, Second>::value; }
		};
		template<template<typename, typename> class BP, typename First> constexpr auto Bind1T() { return bind1t_t<BP, First>(); }
		template<template<typename, typename> class BP, typename Second> struct bind2t_t : public Node {
			template<typename First> static constexpr bool CLOAK_CALL value() { return BP<First, Second>::value; }
		};
		template<template<typename, typename> class BP, typename Second> constexpr auto Bind2T() { return bind2t_t<BP, Second>(); }
		template<template<typename, typename...> class BP, typename... Second> struct bind2nt_t : public Node {
			template<typename First> static constexpr bool CLOAK_CALL value() { return BP<First, Second...>::value; }
		};
		template<template<typename, typename...> class BP, typename... Second> constexpr auto Bind2NT() { return bind2nt_t<BP, Second...>(); }
		template<template<size_t, typename> class BP, size_t First> struct bind1i_t : public Node {
			template<typename Second> static constexpr bool CLOAK_CALL value() { return BP<First, Second>::value; }
		};
		template<template<size_t, typename> class BP, size_t First> constexpr auto Bind1I() { return bind1i_t<BP, First>(); }
		template<template<typename, size_t> class BP, size_t Second> struct bind2i_t : public Node {
			template<typename First> static constexpr bool CLOAK_CALL value() { return BP<First, Second>::value; }
		};
		template<template<typename, size_t> class BP, size_t Second> constexpr auto Bind2I() { return bind2i_t<BP, Second>(); }

		//Operators types:
		template<typename P1, typename P2> struct and_t : public Node {
			template<typename T> static constexpr bool CLOAK_CALL value() { return P1::template value<T>() && P2::template value<T>(); }
		};
		template<typename P1, typename P2> struct or_t : public Node {
			template<typename T> static constexpr bool CLOAK_CALL value() { return P1::template value<T>() || P2::template value<T>(); }
		};
		template<typename P1, typename P2> struct xor_t : public Node {
			template<typename T> static constexpr bool CLOAK_CALL value() { return P1::template value<T>() != P2::template value<T>(); }
		};
		template<typename P> struct not_t : public Node {
			template<typename T> static constexpr bool CLOAK_CALL value() { return !P::template value<T>(); }
		};

		//Special Predicate types
		template<typename A, typename B> struct is_similar : public std::is_same<std::decay<A>, std::decay<B>> {};
		template<size_t A, typename B> struct is_of_size : public std::bool_constant<A == sizeof(B)> {};
		template<typename A> struct is_pointer_impl : public std::false_type {};
		template<typename A> struct is_pointer_impl<A*> : public std::true_type {};
		template<typename A> struct is_pointer_impl<A* __restrict> : public std::true_type {};
		template<typename A> struct is_pointer : is_pointer_impl<typename std::remove_cv<A>::type> {};

		template<typename A, typename B> struct has_operator_equals {
			private:
				template<typename C, typename D> static auto test(int) -> decltype(std::declval<C>().operator==(std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D> static auto test(int) -> decltype(operator==(std::declval<const C&>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<C>::value || std::is_floating_point<C>::value>::type || std::is_pointer<C>::value> static auto test(int) -> decltype(operator==(std::declval<C>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<D>::value || std::is_floating_point<D>::value>::type || std::is_pointer<D>::value> static auto test(int) -> decltype(operator==(std::declval<const C&>(), std::declval<D>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<(std::is_integral<C>::value || std::is_floating_point<C>::value || std::is_pointer<C>::value) && (std::is_integral<D>::value || std::is_floating_point<D>::value || std::is_pointer<D>::value)>::type> static auto test(int) -> std::true_type {}
				template<typename, typename> static auto test(...) -> std::false_type {}
			public:
				static constexpr bool value = decltype(test<A, B>(0))::value;
		};
		template<typename A> struct has_default_operator_equals : public has_operator_equals<A, A> {};
		template<typename A, typename B> struct has_operator_not_equals {
			private:
				template<typename C, typename D> static auto test(int) -> decltype(std::declval<C>().operator!=(std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D> static auto test(int) -> decltype(operator!=(std::declval<const C&>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<C>::value || std::is_floating_point<C>::value>::type || std::is_pointer<C>::value> static auto test(int) -> decltype(operator!=(std::declval<C>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<D>::value || std::is_floating_point<D>::value>::type || std::is_pointer<D>::value> static auto test(int) -> decltype(operator!=(std::declval<const C&>(), std::declval<D>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<(std::is_integral<C>::value || std::is_floating_point<C>::value || std::is_pointer<C>::value) && (std::is_integral<D>::value || std::is_floating_point<D>::value || std::is_pointer<D>::value)>::type> static auto test(int) -> std::true_type {}
				template<typename, typename> static auto test(...) -> std::false_type {}
			public:
				static constexpr bool value = decltype(test<A, B>(0))::value;
		};
		template<typename A> struct has_default_operator_not_equals : public has_operator_not_equals<A, A> {};
		template<typename A, typename B> struct has_operator_less_or_equals {
			private:
				template<typename C, typename D> static auto test(int) -> decltype(std::declval<C>().operator<=(std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D> static auto test(int) -> decltype(operator<=(std::declval<const C&>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<C>::value || std::is_floating_point<C>::value>::type> static auto test(int) -> decltype(operator<=(std::declval<C>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<D>::value || std::is_floating_point<D>::value>::type> static auto test(int) -> decltype(operator<=(std::declval<const C&>(), std::declval<D>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<(std::is_integral<C>::value || std::is_floating_point<C>::value) && (std::is_integral<D>::value || std::is_floating_point<D>::value)>::type> static auto test(int) -> std::true_type {}
				template<typename, typename> static auto test(...) -> std::false_type {}
			public:
				static constexpr bool value = decltype(test<A, B>(0))::value;
		};
		template<typename A> struct has_default_operator_less_or_equals : public has_operator_less_or_equals<A, A> {};
		template<typename A, typename B> struct has_operator_greater_or_equals {
			private:
				template<typename C, typename D> static auto test(int) -> decltype(std::declval<C>().operator>=(std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D> static auto test(int) -> decltype(operator>=(std::declval<const C&>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<C>::value || std::is_floating_point<C>::value>::type> static auto test(int) -> decltype(operator>=(std::declval<C>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<D>::value || std::is_floating_point<D>::value>::type> static auto test(int) -> decltype(operator>=(std::declval<const C&>(), std::declval<D>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<(std::is_integral<C>::value || std::is_floating_point<C>::value) && (std::is_integral<D>::value || std::is_floating_point<D>::value)>::type> static auto test(int) -> std::true_type {}
				template<typename, typename> static auto test(...) -> std::false_type {}
			public:
				static constexpr bool value = decltype(test<A, B>(0))::value;
		};
		template<typename A> struct has_default_operator_greater_or_equals : public has_operator_greater_or_equals<A, A> {};
		template<typename A, typename B> struct has_operator_less {
			private:
				template<typename C, typename D> static auto test(int) -> decltype(std::declval<C>().operator<(std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D> static auto test(int) -> decltype(operator<(std::declval<const C&>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<C>::value || std::is_floating_point<C>::value>::type> static auto test(int) -> decltype(operator<(std::declval<C>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<D>::value || std::is_floating_point<D>::value>::type> static auto test(int) -> decltype(operator<(std::declval<const C&>(), std::declval<D>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<(std::is_integral<C>::value || std::is_floating_point<C>::value) && (std::is_integral<D>::value || std::is_floating_point<D>::value)>::type> static auto test(int) -> std::true_type {}
				template<typename, typename> static auto test(...) -> std::false_type {}
			public:
				static constexpr bool value = decltype(test<A, B>(0))::value;
		};
		template<typename A> struct has_default_operator_less : public has_operator_less<A, A> {};
		template<typename A, typename B> struct has_operator_greater {
			private:
				template<typename C, typename D> static auto test(int) -> decltype(std::declval<C>().operator>(std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D> static auto test(int) -> decltype(operator>(std::declval<const C&>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<C>::value || std::is_floating_point<C>::value>::type> static auto test(int) -> decltype(operator>(std::declval<C>(), std::declval<const D&>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<std::is_integral<D>::value || std::is_floating_point<D>::value>::type> static auto test(int) -> decltype(operator>(std::declval<const C&>(), std::declval<D>()), std::true_type()) {}
				template<typename C, typename D, typename = std::enable_if<(std::is_integral<C>::value || std::is_floating_point<C>::value) && (std::is_integral<D>::value || std::is_floating_point<D>::value)>::type> static auto test(int) -> std::true_type {}
				template<typename, typename> static auto test(...) -> std::false_type {}
			public:
				static constexpr bool value = decltype(test<A, B>(0))::value;
		};
		template<typename A> struct has_default_operator_greater : public has_operator_greater<A, A> {};

		template<typename A, typename B> struct is_unidirectional_comparable_equals : public std::bool_constant<has_operator_equals<A, B>::value && has_operator_not_equals<A, B>::value> {};
		template<typename A> struct is_unidirectional_default_comparable_equals : public is_unidirectional_comparable_equals<A, A> {};
		template<typename A, typename B> struct is_unidirectional_comparable : public std::bool_constant<is_unidirectional_comparable_equals<A, B>::value && has_operator_less_or_equals<A, B>::value && has_operator_greater_or_equals<A, B>::value && has_operator_less<A, B>::value && has_operator_greater<A, B>::value> {};
		template<typename A> struct is_unidirectional_default_comparable : public is_unidirectional_comparable<A, A> {};

		template<typename A, typename B> struct is_comparable_equals : public std::bool_constant<is_unidirectional_comparable_equals<A, B>::value && is_unidirectional_comparable_equals<B, A>::value> {};
		template<typename A> struct is_default_comparable_equals : public is_unidirectional_comparable_equals<A, A> {};
		template<typename A, typename B> struct is_comparable : public std::bool_constant<is_unidirectional_comparable<A, B>::value && is_unidirectional_comparable<B, A>::value> {};
		template<typename A> struct is_default_comparable : public is_unidirectional_default_comparable<A> {};

		template<typename... List> struct unique_list {
			private:
				template<typename A, typename... B> struct can_find {
					static constexpr bool value = index_of_type<A, B...>::value < number_of_types<B...>::value;
				};
				template<typename... A> struct is_unique : public std::true_type {};
				template<typename A> struct is_unique<A> : public std::true_type {};
				template<typename A, typename B, typename... C> struct is_unique<A, B, C...> {
					static constexpr bool value = (can_find<A, B, C...>::value == false) && is_unique<B, C...>::value;
				};
			public:
				static constexpr bool value = is_unique<List...>::value;
		};
		template<typename A, typename... B> struct all_base_of {
			private:
				template<typename C, typename... D> struct base_helper : public std::false_type {};
				template<typename C, typename D> struct base_helper<C, D> {
					static constexpr bool value = std::is_convertible_v<D*, const volatile C*>;
				};
				template<typename C, typename D> struct base_helper<C, D*> : public base_helper<C, D> {};
				template<typename C, typename D, typename... E> struct base_helper<C, D, E...> {
					static constexpr bool value = base_helper<C, D>::value && base_helper<C, E...>::value;
				};
				template<typename C, typename D, typename... E> struct base_helper<C*, D, E...> : public base_helper<C, D, E...> {};
			public:
				static constexpr bool value = base_helper<A, B...>::value;
		};
		template<typename From, typename To, typename = void> struct is_safely_castable : std::false_type {};
		template<typename From, typename To> struct is_safely_castable<From, To, std::void_t<decltype(static_cast<To>(std::declval<From>()))>> : std::true_type {};
		template<typename From, typename To> constexpr bool is_safely_castable_v = is_safely_castable<From, To>::value;
	}
}

//Operators:
template<typename P1, typename P2, typename = std::enable_if_t<CloakEngine::Predicate::is_node<P1, P2>::value>> inline constexpr auto CLOAK_CALL operator&&(In const P1&, In const P2&) { return CloakEngine::Predicate::and_t<P1, P2>(); }
template<typename P1, typename P2, typename = std::enable_if_t<CloakEngine::Predicate::is_node<P1, P2>::value>> inline constexpr auto CLOAK_CALL operator&(In const P1&, In const P2&) { return CloakEngine::Predicate::and_t<P1, P2>(); }
template<typename P1, typename P2, typename = std::enable_if_t<CloakEngine::Predicate::is_node<P1, P2>::value>> inline constexpr auto CLOAK_CALL operator||(In const P1&, In const P2&) { return CloakEngine::Predicate::or_t<P1, P2>(); }
template<typename P1, typename P2, typename = std::enable_if_t<CloakEngine::Predicate::is_node<P1, P2>::value>> inline constexpr auto CLOAK_CALL operator|(In const P1&, In const P2&) { return CloakEngine::Predicate::or_t<P1, P2>(); }
template<typename P1, typename P2, typename = std::enable_if_t<CloakEngine::Predicate::is_node<P1, P2>::value>> inline constexpr auto CLOAK_CALL operator^(In const P1&, In const P2&) { return CloakEngine::Predicate::xor_t<P1, P2>(); }
template<typename P, typename = std::enable_if_t<CloakEngine::Predicate::is_node<P>::value>> inline constexpr auto CLOAK_CALL operator!(In const P&) { return CloakEngine::Predicate::not_t<P>(); }
template<typename P, typename = std::enable_if_t<CloakEngine::Predicate::is_node<P>::value>> inline constexpr auto CLOAK_CALL operator~(In const P&) { return CloakEngine::Predicate::not_t<P>(); }
template<typename P1, typename P2, typename = std::enable_if_t<CloakEngine::Predicate::is_node<P1, P2>::value>> inline constexpr bool CLOAK_CALL operator==(In const P1&, In const P2&) { return false; }
template<typename P1, typename = std::enable_if_t<CloakEngine::Predicate::is_node<P1>::value>> inline constexpr bool CLOAK_CALL operator==(In const P1&, In const P1&) { return true; }
template<typename P1, typename P2, typename = std::enable_if_t<CloakEngine::Predicate::is_node<P1, P2>::value>> inline constexpr bool CLOAK_CALL operator!=(In const P1&, In const P2&) { return true; }
template<typename P1, typename = std::enable_if_t<CloakEngine::Predicate::is_node<P1>::value>> inline constexpr bool CLOAK_CALL operator!=(In const P1&, In const P1&) { return false; }

//Check alternatives:
template<typename... T, typename AST, typename = CloakEngine::Predicate::is_node<AST>> constexpr bool CLOAK_CALL satisfies(AST) { return CloakEngine::Predicate::allTrue_t<AST().template value<T>()...>::value; }
template<typename AST, typename... T> using enable_if_satisfied = typename std::enable_if<satisfies<T...>(AST())>::type;
template<typename C, typename AST, typename... T> using type_if_satisfied = typename std::enable_if<satisfies<T...>(AST()), C>::type;

//All std predicates in type_traits:
#define CE_PRED_CREATOR(name) constexpr auto name = CloakEngine::Predicate::ID<CloakEngine::Predicate::name>(); using name##_t = decltype(name);
#define CE_BIND1T_CREATOR(name) template<typename T> constexpr auto name = CloakEngine::Predicate::Bind1T<CloakEngine::Predicate::name, T>(); template<typename T> using name##_t = decltype(name<T>)
#define CE_BIND1I_CREATOR(name) template<size_t T> constexpr auto name = CloakEngine::Predicate::Bind1I<CloakEngine::Predicate::name, T>(); template<size_t T> using name##_t = decltype(name<T>)
#define CE_PRED_STD_CREATOR(name) constexpr auto name = CloakEngine::Predicate::ID<std::name>(); using name##_t = decltype(name)
#define CE_BIND1T_STD_CREATOR(name) template<typename T> constexpr auto name = CloakEngine::Predicate::Bind1T<std::name, T>(); template<typename T> using name##_t = decltype(name<T>)
#define CE_BIND2NT_STD_CREATOR(name) template<typename... T> constexpr auto name = CloakEngine::Predicate::Bind2NT<std::name, T...>(); template<typename... T> using name##_t = decltype(name<T...>)

CE_PRED_STD_CREATOR(is_void);
CE_PRED_STD_CREATOR(is_null_pointer);
CE_PRED_STD_CREATOR(is_integral);
//CE_PRED_STD_CREATOR(is_floating_point);
constexpr auto is_floating_point = !is_integral && CloakEngine::Predicate::ID<std::is_floating_point>(); using is_floating_point_t = decltype(is_floating_point);
CE_PRED_STD_CREATOR(is_array);
CE_PRED_STD_CREATOR(is_enum);
CE_PRED_STD_CREATOR(is_union);
CE_PRED_STD_CREATOR(is_class);
CE_PRED_STD_CREATOR(is_function);
CE_PRED_CREATOR(is_pointer);
CE_PRED_STD_CREATOR(is_lvalue_reference);
CE_PRED_STD_CREATOR(is_rvalue_reference);
CE_PRED_STD_CREATOR(is_member_object_pointer);
CE_PRED_STD_CREATOR(is_member_function_pointer);

CE_PRED_STD_CREATOR(is_fundamental);
CE_PRED_STD_CREATOR(is_arithmetic);
CE_PRED_STD_CREATOR(is_scalar);
CE_PRED_STD_CREATOR(is_object);
CE_PRED_STD_CREATOR(is_compound);
CE_PRED_STD_CREATOR(is_reference);
CE_PRED_STD_CREATOR(is_member_pointer);

CE_PRED_STD_CREATOR(is_const);
CE_PRED_STD_CREATOR(is_volatile);
CE_PRED_STD_CREATOR(is_trivial);
CE_PRED_STD_CREATOR(is_trivially_copyable);
CE_PRED_STD_CREATOR(is_standard_layout);
CE_PRED_STD_CREATOR(is_pod);
CE_PRED_STD_CREATOR(is_empty);
CE_PRED_STD_CREATOR(is_polymorphic);
CE_PRED_STD_CREATOR(is_abstract);
CE_PRED_STD_CREATOR(is_final);
CE_PRED_STD_CREATOR(is_signed);
CE_PRED_STD_CREATOR(is_unsigned);

CE_BIND2NT_STD_CREATOR(is_constructible);
CE_PRED_STD_CREATOR(is_trivially_constructible);
CE_PRED_STD_CREATOR(is_nothrow_constructible);
CE_PRED_STD_CREATOR(is_default_constructible);
CE_PRED_STD_CREATOR(is_trivially_default_constructible);
CE_PRED_STD_CREATOR(is_nothrow_default_constructible);
CE_PRED_STD_CREATOR(is_copy_constructible);
CE_PRED_STD_CREATOR(is_trivially_copy_constructible);
CE_PRED_STD_CREATOR(is_nothrow_copy_constructible);
CE_PRED_STD_CREATOR(is_move_constructible);
CE_PRED_STD_CREATOR(is_trivially_move_constructible);
CE_PRED_STD_CREATOR(is_nothrow_move_constructible);
CE_PRED_STD_CREATOR(is_assignable);
CE_PRED_STD_CREATOR(is_trivially_assignable);
CE_PRED_STD_CREATOR(is_nothrow_assignable);
CE_PRED_STD_CREATOR(is_copy_assignable);
CE_PRED_STD_CREATOR(is_trivially_copy_assignable);
CE_PRED_STD_CREATOR(is_nothrow_copy_assignable);
CE_PRED_STD_CREATOR(is_move_assignable);
CE_PRED_STD_CREATOR(is_trivially_move_assignable);
CE_PRED_STD_CREATOR(is_nothrow_move_assignable);
CE_PRED_STD_CREATOR(is_destructible);
CE_PRED_STD_CREATOR(is_trivially_destructible);
CE_PRED_STD_CREATOR(is_nothrow_destructible);

CE_BIND1T_STD_CREATOR(is_same);
CE_BIND1T_STD_CREATOR(is_base_of);
CE_BIND1T_STD_CREATOR(is_convertible);

CE_PRED_CREATOR(is_default_comparable);
CE_PRED_CREATOR(is_default_comparable_equals);
CE_PRED_CREATOR(is_unidirectional_default_comparable);
CE_PRED_CREATOR(is_unidirectional_default_comparable_equals);
CE_PRED_CREATOR(has_default_operator_equals);
CE_PRED_CREATOR(has_default_operator_not_equals);
CE_PRED_CREATOR(has_default_operator_less_or_equals);
CE_PRED_CREATOR(has_default_operator_greater_or_equals);
CE_PRED_CREATOR(has_default_operator_less);
CE_PRED_CREATOR(has_default_operator_greater);

CE_BIND1T_CREATOR(is_similar);
CE_BIND1T_CREATOR(is_comparable);
CE_BIND1T_CREATOR(is_comparable_equals);
CE_BIND1T_CREATOR(is_unidirectional_comparable);
CE_BIND1T_CREATOR(is_unidirectional_comparable_equals);
CE_BIND1T_CREATOR(has_operator_equals);
CE_BIND1T_CREATOR(has_operator_not_equals);
CE_BIND1T_CREATOR(has_operator_less_or_equals);
CE_BIND1T_CREATOR(has_operator_greater_or_equals);
CE_BIND1T_CREATOR(has_operator_less);
CE_BIND1T_CREATOR(has_operator_greater);

CE_BIND1I_CREATOR(is_of_size);

#undef CE_BIND2NT_STD_CREATOR
#undef CE_PRED_STD_CREATOR
#undef CE_BIND1T_STD_CREATOR
#undef CE_PRED_CREATOR
#undef CE_BIND1T_CREATOR
#undef CE_BIND1I_CREATOR

#endif