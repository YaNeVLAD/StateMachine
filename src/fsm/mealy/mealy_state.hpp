#ifndef MEALY_STATE_HPP
#define MEALY_STATE_HPP

#include <map>
#include <set>
#include <string>
#include <utility>

/**
 * @brief A plain data structure that holds the entire state of a `mealy_machine`.
 *
 * This struct contains all the necessary components to define a Mealy machine:
 * the set of all possible states, the initial and current states, and the
 * transition table. The `transitions` map is the core of the state, defining
 * the transition function `(State, Input) -> (Next State, Output)`.
 *
 * @see fsm::mealy_machine
 */
template <typename T_StateID, typename T_Input, typename T_Output>
struct basic_mealy_state
{
	using state_id = T_StateID;
	using input = T_Input;
	using output = T_Output;

	using transitions_t = std::map<std::pair<state_id, input>, std::pair<state_id, output>>;

	/// @brief The set of all unique state identifiers in the machine.
	std::set<state_id> state_ids;

	/// @brief The identifier of the machine's starting state.
	state_id initial_state_id;

	/// @brief The identifier of the machine's current state.
	state_id current_state_id;

	/// @brief The transition table for the machine.
	transitions_t transitions;
};

using mealy_state = basic_mealy_state<std::string, std::string, std::string>;

#endif // MEALY_STATE_HPP
