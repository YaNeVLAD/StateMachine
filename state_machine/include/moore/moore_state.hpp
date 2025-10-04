#ifndef MOORE_STATE_HPP
#define MOORE_STATE_HPP

#include <map>
#include <set>
#include <string>
#include <utility>

struct moore_state
{
	using state_id = std::string;
	using input = std::string;
	using output = std::string;

	using transitions_t = std::map<std::pair<state_id, input>, state_id>;

	std::map<state_id, output> outputs;
	std::set<state_id> state_ids;
	transitions_t transitions;
	state_id initial_state_id;
	state_id current_state_id;
};

#endif // MOORE_STATE_HPP
