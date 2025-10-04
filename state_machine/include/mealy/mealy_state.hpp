#ifndef MEALY_STATE_HPP
#define MEALY_STATE_HPP

#include <map>
#include <set>
#include <string>
#include <utility>

struct mealy_state
{
	using state_id = std::string;
	using input = std::string;
	using output = std::string;

	using transitions_t = std::map<std::pair<state_id, input>, std::pair<state_id, output>>;

	std::set<state_id> state_ids;
	state_id initial_state_id;
	state_id current_state_id;
	transitions_t transitions;
};

#endif // MEALY_STATE_HPP
