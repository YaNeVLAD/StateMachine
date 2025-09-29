#ifndef STATE_MACHINE_TRAITS_HPP
#define STATE_MACHINE_TRAITS_HPP

namespace fsm
{
template <typename T_StateMachine>
struct state_machine_traits
{
	using state_type = typename T_StateMachine::state_type;
	using input_type = typename T_StateMachine::input_type;
	using output_type = typename T_StateMachine::output_type;
};
} // namespace fsm

#endif // STATE_MACHINE_TRAITS_HPP
