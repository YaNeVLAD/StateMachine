#ifndef STATE_MACHINE_CONCEPT_HPP
#define STATE_MACHINE_CONCEPT_HPP

#include "traits/state_machine_traits.hpp"
#include "traits/translation_traits.hpp"

#include <concepts>
#include <ranges>

namespace fsm::concepts
{
template <typename D>
concept state_machine = requires(D const& derived) {
	typename state_machine_traits<D>::state_type;
	typename state_machine_traits<D>::input_type;
	typename state_machine_traits<D>::output_type;
};

template <typename T>
concept translatable = requires(
	const typename state_machine_traits<T>::state_type state,
	const typename state_machine_traits<T>::input_type input,
	const typename translation_traits<T>::find_result_type find_result) {
	typename translation_traits<T>::find_result_type;
	typename translation_traits<T>::result_type;

	{ translation_traits<T>::find(state, input) }
	-> std::same_as<typename translation_traits<T>::find_result_type>;

	{ translation_traits<T>::is_valid(find_result, state) }
	-> std::convertible_to<bool>;

	{ translation_traits<T>::result(find_result) }
	-> std::same_as<typename translation_traits<T>::result_type>;
};

template <typename T>
concept container = requires(T container) {
	container.begin();
	container.end();
};

template <typename T>
concept has_value_type = requires { typename T::value_type; };

template <typename T>
concept is_string_like = std::ranges::common_range<T>
	&& has_value_type<T>
	&& requires(
		T str,
		const typename T::value_type* raw_ptr,
		const typename T::value_type single_char,
		std::size_t size) {
		   { T(raw_ptr) };
		   { T(raw_ptr, size) };
		   { str += raw_ptr };

		   { str + raw_ptr } -> std::same_as<T>;
		   { str + str } -> std::same_as<T>;

		   { str += single_char };
		   { str + single_char } -> std::same_as<T>;
	   };

template <typename T>
concept is_std_string_constructible = std::is_default_constructible_v<T>
	&& std::constructible_from<T, std::string>
	&& std::regular<T>;

template <typename T>
concept streamable = requires(std::ostream& os, const std::remove_reference_t<T>& a) {
	{ os << a } -> std::same_as<std::ostream&>;
};
} // namespace fsm::concepts

#endif // STATE_MACHINE_CONCEPT_HPP
