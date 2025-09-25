#ifndef STATE_MACHINE_CONCEPT_HPP
#define STATE_MACHINE_CONCEPT_HPP

#include "state_machine_traits.hpp"

namespace fsm::concepts
{
template <typename D>
concept state_machine = requires(D const& derived) {
	typename state_machine_traits<D>::state_type;
	typename state_machine_traits<D>::input_type;
	typename state_machine_traits<D>::output_type;
};
} // namespace fsm::concepts

#endif // STATE_MACHINE_CONCEPT_HPP
