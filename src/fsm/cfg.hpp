#ifndef FSM_CFH_HPP
#define FSM_CFH_HPP

#include "cfg/basic_cfg.hpp"
#include "cfg/cfg_algorithms.hpp"
#include "cfg/cfg_transform.hpp"
#include "concepts.hpp"

#include <string>

namespace fsm
{
using cfg = basic_cfg<std::string>;

namespace algorithms
{
template <concepts::is_string_like T_String>
bool cyk(const basic_cfg<T_String>& cnf_grammar, const std::type_identity_t<T_String>& word)
{
	auto distance = std::distance(word.begin(), word.end());

	std::vector<T_String> vec;
	vec.reserve(static_cast<std::size_t>(distance));
	for (const auto c : word)
	{
		vec.push_back(T_String{ c });
	}
	return cyk(cnf_grammar, vec);
}
} // namespace algorithms
} // namespace fsm

#endif // FSM_CFH_HPP
