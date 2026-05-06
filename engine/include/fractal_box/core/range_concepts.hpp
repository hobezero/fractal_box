#ifndef FRACTAL_BOX_CORE_RANGE_CONCEPTS_HPP
#define FRACTAL_BOX_CORE_RANGE_CONCEPTS_HPP

#include <iterator>
#include <ranges>

#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/meta/meta.hpp"

namespace fr {

// Helpers
// -------

/// @brief Same as `std::ranges::range_value_t`
/// @todo TODO: Remove this, it's the same as `std::ranges::range_value_t`
template<class T>
using RangeValue = std::iter_value_t<decltype(std::ranges::begin(std::declval<T&>()))>;

template<class R>
concept c_constexpr_sized_range
	= std::ranges::sized_range<R>
	&& requires(const R& r) {
		typename ValueC<std::ranges::size(r)>;
	};

/// @see [Missing (and Future?) C++ Range Concepts - Jonathan Müller - C++Now 2025](
///   https://www.youtube.com/watch?v=RemzByMHWjI)
template<c_constexpr_sized_range R>
inline constexpr auto constexpr_size = decltype([](R& r) {
	return value_c<std::ranges::size(r)>;
}(std::declval<R&>()))::value;

template<class InIter, class OutIter, class Cmp = std::ranges::less, class Proj = std::identity>
concept c_self_mergeable = std::mergeable<InIter, InIter, OutIter, Cmp, Proj, Proj>;

// Detection of container-like classes
// -----------------------------------

template<class O>
concept c_optional_like
	= std::default_initializable<O>
	&& requires(O& mut_opt, const O& const_opt, typename O::value_type v) {
		typename O::value_type;

		{ const_opt.operator->() } -> std::same_as<const typename O::value_type*>;
		{ mut_opt.operator->() } -> std::same_as<typename O::value_type*>;

		{ const_opt.operator*() } -> std::same_as<const typename O::value_type&>;
		{ mut_opt.operator*() } -> std::same_as<typename O::value_type&>;

		{ const_opt.operator bool() } -> std::same_as<bool>;
		{ const_opt.has_value() } -> std::same_as<bool>;

		{ const_opt.value() } -> std::same_as<const typename O::value_type&>;
		{ mut_opt.value() } -> std::same_as<typename O::value_type&>;

		{ const_opt.value_or(v) } -> std::same_as<typename O::value_type>;

		{ mut_opt.reset() } -> std::same_as<void>;
	};

template<class V>
concept c_variant_like = requires(V& mut_var, const V& const_var) {
	{ const_var.index() } -> std::integral;
	{ const_var.valueless_by_exception() } -> std::same_as<bool>;
	visit([](auto&) { }, mut_var);
	visit([](const auto&) { }, const_var);
	{ get<0>(mut_var) } -> std::same_as<MpFirst<V>&>;
	{ get<0>(const_var) } -> std::same_as<const MpFirst<V>&>;
};

/// @brief Detects tuple-like classes: `std::tuple`, `std::pair`, `std::ranges::subrange`,
/// `std::array`, `std::complex`, etc.
template<class T>
concept c_tuple_like = requires {
	{ std::tuple_size<T>::value } -> std::convertible_to<size_t>;
};

template<class T>
concept c_record_like = c_aggregate<T> || c_tuple_like<T>;

template<class P>
concept c_pair_like
	= c_tuple_like<P>
	&& requires(P& pair) {
	typename P::first_type;
	typename P::second_type;

	{ pair.first } -> std::same_as<typename P::first_type&>;
	{ pair.second } -> std::same_as<typename P::second_type&>;

	requires (std::tuple_size<P>::value == 2zu);
	requires std::same_as<std::tuple_element_t<0zu, P>, typename P::first_type>;
	requires std::same_as<std::tuple_element_t<1zu, P>, typename P::second_type>;
};

template<class P, class T1, class T2>
concept c_pair_of
	= c_pair_like<P>
	&& std::same_as<typename P::first_type, T1>
	&& std::same_as<typename P::second_type, T2>;

// Detection of container types
// ----------------------------

/// @brief `Container` named requirements
/// @see https://en.cppreference.com/w/cpp/named_req/Container
template<class C>
concept c_container
	= std::destructible<C>
	&& std::swappable<C>
	// Don't detect view-like types (e.g. `std::span`)
	&& !std::ranges::borrowed_range<C>
	&& requires(C& mut_container, const C& const_container) {
		// Member types
		// ^^^^^^^^^^^^

		typename C::value_type;
		typename C::reference;
		typename C::const_reference;
		requires std::forward_iterator<typename C::iterator>;
		requires std::forward_iterator<typename C::const_iterator>;
		requires std::convertible_to<typename C::iterator, typename C::const_iterator>;
		requires std::signed_integral<typename C::difference_type>;
		/// @note The standard mandates unsigned integers, but let's give some leeway to the
		/// `ssize_t` gang
		requires std::integral<typename C::size_type>;

		// Member functions
		// ^^^^^^^^^^^^^^^^

		// requires std::default_initializable<C>;
		// requires std::copy_constructible<C>;
		// requires std::move_constructible<C>;
		// { mut_container = mut_container } -> std::same_as<C&>;
		// { mut_container = std::move(mut_container) } -> std::same_as<C&>;

		{ mut_container.begin() } -> std::same_as<typename C::iterator>;
		{ const_container.begin() } -> std::same_as<typename C::const_iterator>;
		{ const_container.cbegin() } -> std::same_as<typename C::const_iterator>;

		/// @note Standard says `typename C::iterator>
		{ mut_container.end() } -> std::sentinel_for<typename C::iterator>;
		{ const_container.end() } -> std::sentinel_for<typename C::const_iterator>;
		{ const_container.cend() } -> std::sentinel_for<typename C::const_iterator>;

		{ const_container.size() } -> std::same_as<typename C::size_type>;
		{ const_container.max_size() } -> std::same_as<typename C::size_type>;
		{ const_container.empty() } -> std::same_as<bool>;
	};

template<class C>
concept c_constexpr_sized_container = c_container<C> && c_constexpr_sized_range<C>;

template<class C>
concept c_bidirectional_container = c_container<C> && std::ranges::bidirectional_range<C>;

template<class C>
concept c_random_access_container = c_bidirectional_container<C>
	&& std::ranges::random_access_range<C>;

template<class C>
concept c_contiguous_container = c_random_access_container<C> && std::ranges::contiguous_range<C>;

template<class C>
concept c_std_array_like
	= c_constexpr_sized_container<C>
	&& requires(C& container) {
		{ container.data() } -> std::same_as<typename C::pointer>;
		// Check that all elements are stored in-place, or size is zero
		requires (constexpr_size<C> == sizeof(C) / sizeof(typename C::value_type))
			|| (constexpr_size<C> == 0 && sizeof(C) == sizeof(char)) // in libstdc++
			|| (constexpr_size<C> == 0 && sizeof(C) == sizeof(typename C::value_type)); // in libc++
	};

template<class C>
concept c_array_like = c_bounded_array<C> || c_std_array_like<C>;

template<class C>
concept c_list_like
	= c_container<C>
	&& std::ranges::bidirectional_range<C>
	&& !std::ranges::random_access_range<C>
	&& requires(C& container, typename C::value_type v) {
		container.push_back(v);
		container.push_front(v);
		container.pop_back();
		container.pop_front();
	}
	&& !requires(C& container, typename C::size_type i) {
		container[i];
		container.at(i);
	};

template<class C>
concept c_vector_like
	= c_contiguous_container<C>
	&& requires(
		C& mut_container,
		const C& const_container,
		typename C::value_type v,
		typename C::size_type i
	) {
		mut_container.clear();
		mut_container.push_back(v);
		mut_container.pop_back();

		mut_container.reserve(i);
		{ const_container.capacity() } -> std::same_as<typename C::size_type>;

		{ mut_container.data() } -> std::same_as<typename C::value_type*>;
		{ const_container.data() } -> std::same_as<const typename C::value_type*>;

		{ mut_container[i] } -> std::same_as<typename C::reference>;
		{ const_container[i] } -> std::same_as<typename C::const_reference>;

		// `std::string` is not a vector because of null-terminator
		requires !requires { const_container.c_str(); };
		requires !requires { const_container.substr(i); };
	};

template<class C>
concept c_string_like
	= c_contiguous_container<C>
	&& c_default_constructible<C>
	&& c_nothrow_movable<C>
	&& std::copyable<C>
	&& requires(C& mut_container,
		const C& const_container,
		typename C::size_type n,
		typename C::value_type ch
	) {
		typename C::traits_type;
		typename C::allocator_type;
		typename C::pointer;
		typename C::const_pointer;

		{ mut_container.data() } -> std::same_as<typename C::value_type*>;
		{ mut_container.length() } -> std::same_as<typename C::size_type>;

		mut_container.clear();
		mut_container.push_back(ch);
		mut_container.pop_back();

		{ const_container.c_str() } -> std::same_as<const typename C::value_type*>;

		{ mut_container.substr(n, n) } -> std::same_as<C>;
		{ mut_container.find(const_container, n) } -> std::same_as<typename C::size_type>;
	};

template<class S>
concept c_set_like
	= c_container<S>
	&& std::default_initializable<S>
	&& requires(
		S& mut_container,
		const S& const_container,
		typename S::value_type& v,
		const typename S::value_type& cv,
		typename S::const_iterator cit
	) {
		typename S::key_type;
		typename S::value_type;
		requires std::same_as<typename S::key_type, typename S::value_type>;
		typename S::key_compare;
		typename S::value_compare;
		typename S::pointer;
		typename S::const_pointer;
		requires std::bidirectional_iterator<typename S::iterator>;
		requires std::bidirectional_iterator<typename S::const_iterator>;

		mut_container.clear();
		{ mut_container.insert(std::move(v)) } -> c_pair_of<typename S::iterator, bool>;
		{ mut_container.insert(cit, std::move(v)) } -> std::same_as<typename S::iterator>;
		{ mut_container.emplace(std::move(v)) } -> c_pair_of<typename S::iterator, bool>;
		{ mut_container.erase(cit) } -> std::same_as<typename S::iterator>;
		{ mut_container.erase(cit, cit) } -> std::same_as<typename S::iterator>;
		{ mut_container.erase(v) } -> std::same_as<typename S::size_type>;

		mut_container.swap(mut_container);

		{ const_container.count(cv) } -> std::same_as<typename S::size_type>;
		{ mut_container.find(cv) } -> std::same_as<typename S::iterator>;
		{ const_container.find(cv) } -> std::same_as<typename S::const_iterator>;
		{ const_container.contains(cv) } -> std::same_as<bool>;

		{ const_container.key_comp() } -> std::same_as<typename S::key_compare>;
		{ const_container.value_comp() } -> std::same_as<typename S::value_compare>;
	};

template<class S>
concept c_multiset_like
	= c_container<S>
	&& std::default_initializable<S>
	&& requires(
		S& mut_container,
		const S& const_container,
		typename S::value_type& v,
		const typename S::value_type& cv,
		typename S::const_iterator cit
	) {
		typename S::key_type;
		typename S::value_type;
		requires std::same_as<typename S::key_type, typename S::value_type>;
		typename S::key_compare;
		typename S::value_compare;
		typename S::pointer;
		typename S::const_pointer;
		requires std::bidirectional_iterator<typename S::iterator>;
		requires std::bidirectional_iterator<typename S::const_iterator>;

		mut_container.clear();
		{ mut_container.insert(std::move(v)) } -> std::same_as<typename S::iterator>;
		{ mut_container.insert(cit, std::move(v)) } -> std::same_as<typename S::iterator>;
		{ mut_container.emplace(std::move(v)) } -> std::same_as<typename S::iterator>;
		{ mut_container.erase(cit) } -> std::same_as<typename S::iterator>;
		{ mut_container.erase(cit, cit) } -> std::same_as<typename S::iterator>;
		{ mut_container.erase(v) } -> std::same_as<typename S::size_type>;

		mut_container.swap(mut_container);

		{ const_container.count(cv) } -> std::same_as<typename S::size_type>;
		{ mut_container.find(cv) } -> std::same_as<typename S::iterator>;
		{ const_container.find(cv) } -> std::same_as<typename S::const_iterator>;
		{ const_container.contains(cv) } -> std::same_as<bool>;

		{ const_container.key_comp() } -> std::same_as<typename S::key_compare>;
		{ const_container.value_comp() } -> std::same_as<typename S::value_compare>;
	};

template<class M>
concept c_map_like
	= c_container<M>
	&& std::default_initializable<M>
	&& requires(
		M& mut_container,
		const M& const_container,
		typename M::key_type& k,
		const typename M::key_type& ck,
		typename M::mapped_type& m,
		const typename M::mapped_type& cm,
		typename M::value_type& v,
		typename M::const_iterator cit
	) {
		typename M::key_type;
		typename M::mapped_type;
		typename M::value_type; // a `pair<const key_type, mapped_type>` in std::map
		typename M::key_compare;
		typename M::pointer;
		typename M::const_pointer;
		requires std::bidirectional_iterator<typename M::iterator>;
		requires std::bidirectional_iterator<typename M::const_iterator>;

		{ mut_container[ck] } -> std::same_as<typename M::mapped_type&>;
		{ mut_container[std::move(k)] } -> std::same_as<typename M::mapped_type&>;

		mut_container.clear();
		{ mut_container.insert(std::move(v)) } -> c_pair_of<typename M::iterator, bool>;
		{ mut_container.insert(cit, std::move(v)) } -> std::same_as<typename M::iterator>;
		{ mut_container.insert_or_assign(std::move(k), std::move(m)) }
			-> c_pair_of<typename M::iterator, bool>;
		{ mut_container.emplace(std::move(v)) } -> c_pair_of<typename M::iterator, bool>;
		{ mut_container.try_emplace(std::move(k), std::move(m)) }
			-> c_pair_of<typename M::iterator, bool>;
		{ mut_container.erase(cit) } -> std::same_as<typename M::iterator>;
		{ mut_container.erase(cit, cit) } -> std::same_as<typename M::iterator>;
		{ mut_container.erase(ck) } -> std::same_as<typename M::size_type>;

		mut_container.swap(mut_container);

		{ const_container.count(ck) } -> std::same_as<typename M::size_type>;
		{ mut_container.find(ck) } -> std::same_as<typename M::iterator>;
		{ const_container.find(ck) } -> std::same_as<typename M::const_iterator>;
		{ const_container.contains(ck) } -> std::same_as<bool>;

		{ const_container.key_comp() } -> std::same_as<typename M::key_compare>;
		{ const_container.value_comp() } -> std::same_as<typename M::value_compare>;
	};

template<class M>
concept c_multimap_like
	= c_container<M>
	&& std::default_initializable<M>
	&& requires(
		M& mut_container,
		const M& const_container,
		typename M::key_type& k,
		const typename M::key_type& ck,
		typename M::mapped_type& m,
		const typename M::mapped_type& cm,
		typename M::value_type& v,
		typename M::const_iterator cit
	) {
		typename M::key_type;
		typename M::mapped_type;
		typename M::value_type; // a `pair<const key_type, mapped_type>` in std::map
		typename M::key_compare;
		typename M::pointer;
		typename M::const_pointer;
		requires std::bidirectional_iterator<typename M::iterator>;
		requires std::bidirectional_iterator<typename M::const_iterator>;

		mut_container.clear();
		{ mut_container.insert(std::move(v)) } -> std::same_as<typename M::iterator>;
		{ mut_container.insert(cit, std::move(v)) } -> std::same_as<typename M::iterator>;
		{ mut_container.emplace(std::move(v)) } -> std::same_as<typename M::iterator>;
		{ mut_container.erase(cit) } -> std::same_as<typename M::iterator>;
		{ mut_container.erase(cit, cit) } -> std::same_as<typename M::iterator>;
		{ mut_container.erase(ck) } -> std::same_as<typename M::size_type>;

		mut_container.swap(mut_container);

		{ const_container.count(ck) } -> std::same_as<typename M::size_type>;
		{ mut_container.find(ck) } -> std::same_as<typename M::iterator>;
		{ const_container.find(ck) } -> std::same_as<typename M::const_iterator>;
		{ const_container.contains(ck) } -> std::same_as<bool>;

		{ const_container.key_comp() } -> std::same_as<typename M::key_compare>;
		{ const_container.value_comp() } -> std::same_as<typename M::value_compare>;
	};


template<class S>
concept c_unordered_set_like
	= c_container<S>
	&& std::default_initializable<S>
	&& requires(
		S& mut_container,
		const S& const_container,
		typename S::value_type& v,
		const typename S::value_type& cv,
		typename S::iterator it,
		typename S::const_iterator cit
	) {
		typename S::key_type;
		typename S::value_type;
		requires std::same_as<typename S::key_type, typename S::value_type>;
		typename S::hasher;
		typename S::key_equal;
		requires std::forward_iterator<typename S::iterator>;
		requires std::forward_iterator<typename S::const_iterator>;

		mut_container.clear();
		{ mut_container.insert(std::move(v)) } -> c_pair_of<typename S::iterator, bool>;
		{ mut_container.insert(cit, std::move(v)) } -> std::same_as<typename S::iterator>;
		{ mut_container.emplace(std::move(v)) } -> c_pair_of<typename S::iterator, bool>;
		{ mut_container.erase(cit) } -> std::same_as<typename S::iterator>;
		{ mut_container.erase(cit, cit) } -> std::same_as<typename S::iterator>;
		{ mut_container.erase(v) } -> std::same_as<typename S::size_type>;

		mut_container.swap(mut_container);

		{ const_container.count(cv) } -> std::same_as<typename S::size_type>;
		{ mut_container.find(cv) } -> std::same_as<typename S::iterator>;
		{ const_container.find(cv) } -> std::same_as<typename S::const_iterator>;
		{ const_container.contains(cv) } -> std::same_as<bool>;

		{ const_container.hash_function() } -> std::same_as<typename S::hasher>;
		{ const_container.key_eq() } -> std::same_as<typename S::key_equal>;
	};

template<class S>
concept c_unordered_multiset_like
	= c_container<S>
	&& std::default_initializable<S>
	&& requires(
		S& mut_container,
		const S& const_container,
		typename S::value_type& v,
		const typename S::value_type& cv,
		typename S::iterator it,
		typename S::const_iterator cit
	) {
		typename S::key_type;
		typename S::value_type;
		requires std::same_as<typename S::key_type, typename S::value_type>;
		typename S::hasher;
		typename S::key_equal;
		requires std::forward_iterator<typename S::iterator>;
		requires std::forward_iterator<typename S::const_iterator>;

		mut_container.clear();
		{ mut_container.insert(std::move(v)) } -> std::same_as<typename S::iterator>;
		{ mut_container.insert(cit, std::move(v)) } -> std::same_as<typename S::iterator>;
		{ mut_container.emplace(std::move(v)) } -> std::same_as<typename S::iterator>;
		{ mut_container.erase(cit) } -> std::same_as<typename S::iterator>;
		{ mut_container.erase(cit, cit) } -> std::same_as<typename S::iterator>;
		{ mut_container.erase(v) } -> std::same_as<typename S::size_type>;

		mut_container.swap(mut_container);

		{ const_container.count(cv) } -> std::same_as<typename S::size_type>;
		{ mut_container.find(cv) } -> std::same_as<typename S::iterator>;
		{ const_container.find(cv) } -> std::same_as<typename S::const_iterator>;
		{ const_container.contains(cv) } -> std::same_as<bool>;

		{ const_container.hash_function() } -> std::same_as<typename S::hasher>;
		{ const_container.key_eq() } -> std::same_as<typename S::key_equal>;
	};

template<class M>
concept c_unordered_map_like
	= c_container<M>
	&& std::default_initializable<M>
	&& requires(
		M& mut_container,
		const M& const_container,
		typename M::key_type& k,
		const typename M::key_type& ck,
		typename M::mapped_type& m,
		const typename M::mapped_type& cm,
		typename M::value_type& v,
		typename M::const_iterator cit
	) {
		typename M::key_type;
		typename M::mapped_type;
		typename M::value_type; // a `pair<const key_type, mapped_type>` in std::map
		typename M::hasher;
		typename M::key_equal;
		typename M::pointer;
		typename M::const_pointer;
		requires std::forward_iterator<typename M::iterator>;
		requires std::forward_iterator<typename M::const_iterator>;

		mut_container.clear();
		{ mut_container.insert(std::move(v)) } -> c_pair_of<typename M::iterator, bool>;
		{ mut_container.insert(cit, std::move(v)) } -> std::same_as<typename M::iterator>;
		{ mut_container.insert_or_assign(std::move(k), std::move(m)) }
			-> c_pair_of<typename M::iterator, bool>;
		{ mut_container.emplace(std::move(v)) } -> c_pair_of<typename M::iterator, bool>;
		{ mut_container.try_emplace(std::move(k), std::move(m)) }
			-> c_pair_of<typename M::iterator, bool>;
		{ mut_container.erase(cit) } -> std::same_as<typename M::iterator>;
		{ mut_container.erase(cit, cit) } -> std::same_as<typename M::iterator>;
		{ mut_container.erase(ck) } -> std::same_as<typename M::size_type>;

		mut_container.swap(mut_container);

		{ mut_container[ck] } -> std::same_as<typename M::mapped_type&>;
		{ mut_container[std::move(k)] } -> std::same_as<typename M::mapped_type&>;

		{ const_container.count(ck) } -> std::same_as<typename M::size_type>;
		{ mut_container.find(ck) } -> std::same_as<typename M::iterator>;
		{ const_container.find(ck) } -> std::same_as<typename M::const_iterator>;
		{ const_container.contains(ck) } -> std::same_as<bool>;

		{ const_container.hash_function() } -> std::same_as<typename M::hasher>;
		{ const_container.key_eq() } -> std::same_as<typename M::key_equal>;
	};

template<class M>
concept c_unordered_multimap_like
	= c_container<M>
	&& std::default_initializable<M>
	&& requires(
		M& mut_container,
		const M& const_container,
		typename M::key_type& k,
		const typename M::key_type& ck,
		typename M::mapped_type& m,
		const typename M::mapped_type& cm,
		typename M::value_type& v,
		typename M::const_iterator cit
	) {
		typename M::key_type;
		typename M::mapped_type;
		typename M::value_type; // a `pair<const key_type, mapped_type>` in std::map
		typename M::hasher;
		typename M::key_equal;
		typename M::pointer;
		typename M::const_pointer;
		requires std::forward_iterator<typename M::iterator>;
		requires std::forward_iterator<typename M::const_iterator>;

		mut_container.clear();
		{ mut_container.insert(std::move(v)) } -> std::same_as<typename M::iterator>;
		{ mut_container.insert(cit, std::move(v)) } -> std::same_as<typename M::iterator>;
		{ mut_container.emplace(std::move(v)) } -> std::same_as<typename M::iterator>;
		{ mut_container.erase(cit) } -> std::same_as<typename M::iterator>;
		{ mut_container.erase(cit, cit) } -> std::same_as<typename M::iterator>;
		{ mut_container.erase(ck) } -> std::same_as<typename M::size_type>;

		mut_container.swap(mut_container);

		{ const_container.count(ck) } -> std::same_as<typename M::size_type>;
		{ mut_container.find(ck) } -> std::same_as<typename M::iterator>;
		{ const_container.find(ck) } -> std::same_as<typename M::const_iterator>;
		{ const_container.contains(ck) } -> std::same_as<bool>;

		{ const_container.hash_function() } -> std::same_as<typename M::hasher>;
		{ const_container.key_eq() } -> std::same_as<typename M::key_equal>;
	};

// TODO: c_flat_set_like, c_flat_map_like

// Detection of view types
// -----------------------

template<class S>
concept c_span_like
	= std::ranges::borrowed_range<S>
	&& std::ranges::random_access_range<S>
	&& std::ranges::contiguous_range<S>
	// NOTE: Const-c_std_array_like extent spans aren't default-constructible
	&& c_nothrow_movable<S>
	&& c_nothrow_copyable<S>
	&& requires(S& mut_span, const S& const_span) {
		// Member types (similar to `c_container` but with `element_type`)
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		typename S::element_type;
		typename S::value_type;
		typename S::pointer;
		typename S::const_pointer;
		typename S::reference;
		typename S::const_reference;
		requires std::contiguous_iterator<typename S::iterator>;
		// NOTE: Not available in Clang (libc++) yet
#if 0
		requires std::contiguous_iterator<typename S::const_iterator>;
		requires std::convertible_to<typename S::iterator, typename S::const_iterator>;
#endif
		requires std::signed_integral<typename S::difference_type>;
		requires std::integral<typename S::size_type>;
		requires std::integral<decltype(S::extent)>;

		// Member functions
		// ^^^^^^^^^^^^^^^^

		// NOTE: no const propagation
		{ mut_span.begin() } -> std::same_as<typename S::iterator>;
		{ const_span.begin() } -> std::same_as<typename S::iterator>;
#if 0
		{ const_span.cbegin() } -> std::same_as<typename S::const_iterator>;
#endif

		{ mut_span.end() } -> std::same_as<typename S::iterator>;
		{ const_span.end() } -> std::same_as<typename S::iterator>;
#if 0
		{ const_span.cend() } -> std::same_as<typename S::const_iterator>;
#endif

		{ mut_span.data() } -> std::same_as<typename S::pointer>;
		{ mut_span.size() } -> std::same_as<typename S::size_type>;
	};

template<class S>
concept c_const_extent_span_like = c_span_like<S> && c_constexpr_sized_range<S>;

template<class S>
concept c_dynamic_extent_span_like = c_span_like<S> && !c_constexpr_sized_range<S>;

template<class S>
concept c_string_view_like
	= std::ranges::sized_range<S>
	&& std::ranges::borrowed_range<S>
	&& std::ranges::random_access_range<S>
	&& std::ranges::contiguous_range<S>
	&& c_default_constructible<S>
	&& c_nothrow_movable<S>
	&& c_nothrow_copyable<S>
	&& requires(
		S& mut_sview,
		const S& const_sview,
		typename S::size_type n
	) {
		// Member types
		// ^^^^^^^^^^^^

		typename S::traits_type;
		typename S::value_type;
		!requires { typename S::allocator_type; };
		typename S::pointer;
		typename S::const_pointer;
		typename S::reference;
		typename S::const_reference;
		requires std::contiguous_iterator<typename S::const_iterator>;
		requires std::contiguous_iterator<typename S::iterator>;
		requires std::same_as<typename S::iterator, typename S::const_iterator>;
		requires std::signed_integral<typename S::difference_type>;
		requires std::integral<typename S::size_type>;

		// Member functions
		// ^^^^^^^^^^^^^^^^

		// NOTE: `const_iterator`'s everywhere
		{ mut_sview.begin() } -> std::same_as<typename S::const_iterator>;
		{ const_sview.begin() } -> std::same_as<typename S::const_iterator>;
		{ const_sview.cbegin() } -> std::same_as<typename S::const_iterator>;

		{ mut_sview.end() } -> std::same_as<typename S::const_iterator>;
		{ const_sview.end() } -> std::same_as<typename S::const_iterator>;
		{ const_sview.cend() } -> std::same_as<typename S::const_iterator>;

		{ mut_sview.data() } -> std::same_as<typename S::const_pointer>;
		{ mut_sview.size() } -> std::same_as<typename S::size_type>;
		{ mut_sview.length() } -> std::same_as<typename S::size_type>;

		{ mut_sview.remove_prefix(n) } -> std::same_as<void>;
		{ mut_sview.remove_suffix(n) } -> std::same_as<void>;
		{ mut_sview.substr(n, n) } -> std::same_as<S>;
		{ mut_sview.find(const_sview, n) } -> std::same_as<typename S::size_type>;
	};

} // namespace fr
#endif
