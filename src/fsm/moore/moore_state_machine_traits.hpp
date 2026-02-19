#ifndef MOORE_STATE_MACHINE_TRAITS_HPP
#define MOORE_STATE_MACHINE_TRAITS_HPP

#include "../traits/state_machine_traits.hpp"
#include "moore_state.hpp"

namespace fsm
{
class moore_machine;
}

template <>
struct fsm::state_machine_traits<fsm::moore_machine>
{
	using state_type = moore_state;
	using input_type = moore_state::input;
	using output_type = moore_state::output;
};

#endif // MOORE_STATE_MACHINE_TRAITS_HPP
