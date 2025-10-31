#ifndef STATE_MACHINE_CONCEPT_HPP
#define STATE_MACHINE_CONCEPT_HPP

#include <traits/state_machine_traits.hpp>
#include <traits/translation_traits.hpp>

#include <concepts>

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

} // namespace fsm::concepts

#endif // STATE_MACHINE_CONCEPT_HPP
