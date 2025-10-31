#ifndef MEALY_STATE_MACHINE_TRAITS_HPP
#define MEALY_STATE_MACHINE_TRAITS_HPP

#include <traits/state_machine_traits.hpp>

#include "mealy_state.hpp"

namespace fsm
{
class mealy_machine;
}

template <>
struct fsm::state_machine_traits<fsm::mealy_machine>
{
	using state_type = mealy_state;
	using input_type = mealy_state::input;
	using output_type = mealy_state::output;
};

#endif // MEALY_STATE_MACHINE_TRAITS_HPP
