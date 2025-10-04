#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include "concepts.hpp"
#include "traits/state_machine_traits.hpp"

namespace fsm
{
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

	output_type handle_input(input_type const& input) noexcept(false)
	{
		auto transition_result = derived().translate(input, m_current_state);

		output_type output = derived().output_from(transition_result);
		m_current_state = derived().next_state_from(transition_result);

		return output;
	}

	state_type const& state() const noexcept { return m_current_state; }

protected:
	explicit base_state_machine(state_type const& initial_state) noexcept(
		std::is_nothrow_copy_constructible_v<state_type>)
		: m_current_state(initial_state)
	{
	}

	explicit base_state_machine(state_type&& initial_state) noexcept(
		std::is_nothrow_move_constructible_v<state_type>)
		: m_current_state(std::move(initial_state))
	{
	}

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
