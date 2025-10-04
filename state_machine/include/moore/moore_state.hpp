#ifndef MOORE_STATE_HPP
#define MOORE_STATE_HPP

#include <map>
#include <set>
#include <string>
#include <utility>

/**
 * @brief A plain data structure that holds the entire state of a `moore_machine`.
 *
 * This struct defines a Moore machine's components. Unlike a Mealy machine,
 * the output is associated directly with a state and is stored in the `outputs`
 * map. The `transitions` map defines the transition function
 * `(State, Input) -> Next State`.
 *
 * @see fsm::moore_machine
 */
struct moore_state
{
	using state_id = std::string;
	using input = std::string;
	using output = std::string;

	using transitions_t = std::map<std::pair<state_id, input>, state_id>;

	/// @brief A map associating each state identifier with its corresponding output.
	std::map<state_id, output> outputs;
	/// @brief The set of all unique state identifiers in the machine.
	std::set<state_id> state_ids;
	/// @brief The transition table for the machine.
	transitions_t transitions;
	/// @brief The identifier of the machine's starting state.
	state_id initial_state_id;
	/// @brief The identifier of the machine's current state.
	state_id current_state_id;
};

#endif // MOORE_STATE_HPP