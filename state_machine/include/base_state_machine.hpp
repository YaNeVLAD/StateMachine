#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include "state_machine_concept.hpp"
#include "state_machine_traits.hpp"

/// Что общего у конечных автоматов?
/// X - множество входных значений,
/// Y - множество выходных значений,
/// Q - множество состояний,
/// q0 - начальное состояние,
/// F - функция переходов.

/// Отличия автоматов Мили и Мура \n
/// Мили: F(X * Q) -> Q * Y \n
/// Мур: F(x * Q) -> Q, R(Q) -> Y \n
/// R - функция получения выходного сигнала из состояния


namespace fsm
{
template <concepts::state_machine T_Derived>
class base_state_machine
{
	using traits = state_machine_traits<T_Derived>;

public:
	using state_type = typename traits::state_type;
	using input_type = typename traits::input_type;
	using output_type = typename traits::output_type;

	output_type handle_input(input_type const& input)
	{
		auto transition_result = derived().translate(input, m_current_state);

		output_type output = derived().output_from(transition_result);
		m_current_state = derived().next_state_from(transition_result);

		return output;
	}

	state_type const& state() const { return m_current_state; }

protected:
	explicit base_state_machine(state_type const& initial_state)
		: m_current_state(initial_state)
	{
	}

	~base_state_machine() = default;

	state_type& current_state() { return m_current_state; }

private:
	state_type m_current_state;

	T_Derived& derived() { return static_cast<T_Derived&>(*this); }

	T_Derived const& derived() const
	{
		return static_cast<T_Derived const&>(*this);
	}
};
} // namespace fsm

#endif // STATE_MACHINE_HPP
