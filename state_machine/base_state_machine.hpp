#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <concepts.hpp>
#include <traits/state_machine_traits.hpp>

#include <utility>

namespace fsm
{
/**
 * @brief A base class for implementing a finite state machine.
 *
 * This class provides the core logic for input handling and state transitions.
 * The specific implementation of transition logic, output generation, and
 * next state determination is delegated to the derived class `T_Derived`.
 *
 * @tparam T_Derived The derived class type that implements the concrete state
 * machine. It must satisfy the `concepts::state_machine` concept.
 * @tparam T_Traits The traits class that defines associated types, such as
 * `state_type`, `input_type`, and `output_type`. Defaults to
 * `state_machine_traits<T_Derived>`.
 */
template <
	concepts::state_machine T_Derived,
	typename T_Traits = state_machine_traits<T_Derived>>
class base_state_machine
{
	using traits = T_Traits;

public:
	using state_type = typename traits::state_type;
	using input_type = typename traits::input_type;
	using output_type = typename traits::output_type;

	/**
	 * @brief Processes the given input, performs a state transition, and returns an output.
	 *
	 * This is the main method of the state machine. It delegates the transition
	 * logic to the derived class by calling `translate()`, `output_from()`,
	 * and `next_state_from()` to determine the output and the new state.
	 *
	 * @param input The input to be processed.
	 * @return The resulting output of type `output_type`.
	 * @throw May throw exceptions if they are thrown by the derived class's methods.
	 */
	output_type handle_input(input_type const& input) noexcept(false)
	{
		auto transition_result = derived().translate(input, m_current_state);

		output_type output = derived().output_from(transition_result);
		m_current_state = derived().next_state_from(transition_result);

		return output;
	}

	/**
	 * @brief Returns a constant reference to the current state of the machine.
	 */
	state_type const& state() const noexcept { return m_current_state; }

protected:
	/**
	 * @brief Initializes the machine with an initial state (by copying).
	 * @param initial_state The initial state for the machine.
	 */
	explicit base_state_machine(state_type const& initial_state) noexcept(
		std::is_nothrow_copy_constructible_v<state_type>)
		: m_current_state(initial_state)
	{
	}

	/**
	 * @brief Initializes the machine with an initial state (by moving).
	 * @param initial_state The initial state for the machine.
	 */
	explicit base_state_machine(state_type&& initial_state) noexcept(
		std::is_nothrow_move_constructible_v<state_type>)
		: m_current_state(std::move(initial_state))
	{
	}

	/**
	 * @brief Protected destructor. This class is not intended for polymorphic
	 * deletion via a base pointer.
	 */
	~base_state_machine() = default;

	state_type& current_state() noexcept { return m_current_state; }

private:
	state_type m_current_state;

	T_Derived& derived() noexcept { return static_cast<T_Derived&>(*this); }

	T_Derived const& derived() const noexcept
	{
		return static_cast<T_Derived const&>(*this);
	}
};
} // namespace fsm

#endif // STATE_MACHINE_HPP