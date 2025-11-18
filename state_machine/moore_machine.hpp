#ifndef MOORE_MACHINE_HPP
#define MOORE_MACHINE_HPP

#include <format>
#include <map>
#include <ranges>
#include <string>
#include <utility>

#include <base_state_machine.hpp>
#include <default_translator.hpp>
#include <dot.hpp>
#include <labeled.hpp>

#include "moore/moore_state.hpp"
#include "moore/moore_state_machine_traits.hpp"
#include "moore/moore_translation_traits.hpp"

namespace fsm
{
/**
 * @brief An implementation of a Moore finite state machine.
 *
 * This class models a Moore machine, where the output is determined solely by
 * the current state. It inherits its core FSM loop from `base_state_machine`
 * and its transition lookup logic from `default_translator`. The machine's
 * entire state is encapsulated within the `fsm::moore_state` struct.
 * This class is marked as `final` as it is not designed for further user extension.
 *
 * @see fsm::moore_state
 */
class moore_machine final
	: public base_state_machine<moore_machine>
	, public default_translator<moore_machine>
{
	using base = base_state_machine;
	using output = moore_state::output;
	using translation_result = moore_state::state_id;

	friend class fsm::base_state_machine<moore_machine>;
	friend class fsm::default_translator<moore_machine>;

public:
	using state_type = moore_state;

	/**
	 * @brief Constructs a moore_machine from a given state object.
	 * @param initial_state The complete initial state of the machine (by copy).
	 */
	explicit moore_machine(moore_state const& initial_state)
		: base(initial_state)
		, default_translator()
	{
	}

	/**
	 * @brief Constructs a moore_machine from a given state object.
	 * @param initial_state The complete initial state of the machine (by move).
	 */
	explicit moore_machine(moore_state&& initial_state)
		: base(std::move(initial_state))
		, default_translator()
	{
	}

private:
	[[nodiscard]] output output_from(translation_result const& result) const
	{
		const auto& outputs = state().outputs;
		const auto it = outputs.find(result);

		if (it == outputs.end())
		{
			throw std::runtime_error("Output for state '" + result + "' is not defined");
		}

		return it->second;
	}

	moore_state next_state_from(translation_result const& result)
	{
		current_state().current_state_id = result;
		return current_state();
	}
};

template <>
inline void dot(std::ostream& os, moore_machine const& machine)
{
	os << "digraph MooreMachine {\n";
	os << "    rankdir = LR;\n\n";

	for (auto const& state_id : machine.state().state_ids)
	{
		auto it = machine.state().outputs.find(state_id);
		if (it == machine.state().outputs.end())
		{
			throw std::runtime_error("Inconsistent Moore machine: No output for state " + state_id);
		}
		auto const& output = it->second;
		auto label = details::quote(std::format("{} / {}", state_id, output));

		details::print_node(os,
			details::quote(state_id),
			make_labeled<"label">(label));
	}
	os << "\n";

	for (auto const& [state_input, to_state] : machine.state().transitions)
	{
		auto const& [from_state, input] = state_input;

		details::print_edge(os,
			details::quote(from_state),
			details::quote(to_state),
			std::optional{ details::quote(input) });
	}

	os << "}" << std::endl;
}
} // namespace fsm

#endif // MOORE_MACHINE_HPP