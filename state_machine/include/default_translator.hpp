#ifndef DEFAULT_TRANSLATOR_HPP
#define DEFAULT_TRANSLATOR_HPP

#include "concepts.hpp"
#include "traits/translation_traits.hpp"

#include <stdexcept>

namespace fsm
{
/**
 * @brief A mixin class that provides a generic implementation for state transition logic.
 *
 * This class implements a standard algorithm for transitioning between states. It relies on a
 * specialization of `translation_traits` to find, validate, and retrieve the result of a
 * transition for a given state and input. It is intended to be inherited by a concrete state
 * machine implementation (`T_Derived`).
 *
 * @tparam T_Derived The concrete state machine class that inherits from this mixin. It must
 * satisfy the `concepts::translatable` concept.
 * @tparam T_MachineTraits The traits class for the state machine, defining
 * `input_type` and `state_type` types.
 * @tparam T_Traits The traits class defining the transition logic, providing `find`, `is_valid`,
 * and `result` static functions.
 */
template <
	concepts::translatable T_Derived,
	typename T_MachineTraits = state_machine_traits<T_Derived>,
	typename T_Traits = translation_traits<T_Derived>>
class default_translator
{
	using machine_traits = T_MachineTraits;
	using translation_traits = T_Traits;

	using input_type = typename machine_traits::input_type;
	using state_type = typename machine_traits::state_type;

	using find_result_type = typename translation_traits::find_result_type;
	using result_type = typename translation_traits::result_type;

public:
	/**
	 * @brief Performs a state transition lookup based on the current state and input.
	 *
	 * This static function encapsulates the transition logic. It first calls `traits::find`
	 * to locate a potential transition. It then validates this transition using
	 * `traits::is_valid`. If valid, it returns the outcome via `traits::result`.
	 *
	 * @param input The input that triggers the transition.
	 * @param state The current state from which the transition should occur.
	 * @return The result of the transition, as defined by `translation_traits::result_type`.
	 * @throw std::runtime_error Thrown if a valid transition cannot be found.
	 */
	[[nodiscard]] static result_type
	translate(input_type const& input, state_type const& state) noexcept(false)
	{
		using traits = translation_traits;

		const find_result_type res = traits::find(state, input);

		if (!traits::is_valid(res, state))
		{
			throw std::runtime_error("Undefined transition for the given input");
		}

		return traits::result(res);
	}

protected:
	/**
	 * @brief Protected default constructor to ensure this class is only used as a mixin.
	 */
	default_translator() = default;

	/**
	 * @brief Protected default destructor.
	 */
	~default_translator() = default;
};
} // namespace fsm

#endif // DEFAULT_TRANSLATOR_HPP