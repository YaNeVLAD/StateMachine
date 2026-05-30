#ifndef FSM_DETAIL_CFG_RULE_HPP
#define FSM_DETAIL_CFG_RULE_HPP

#include <vector>

namespace fsm
{
namespace detail
{
template <typename T_Symbol>
struct basic_cfg_rule
{
	T_Symbol lhs;
	std::vector<T_Symbol> rhs;

	[[nodiscard]] bool is_epsilon() const
	{
		return rhs.empty();
	}

	template <typename T_OtherSymbol>
	bool operator<(const basic_cfg_rule<T_OtherSymbol>& other) const
	{
		if (lhs != other.lhs)
		{
			return lhs < other.lhs;
		}

		return rhs < other.rhs;
	}

	template <typename T_OtherSymbol>
	bool operator==(const basic_cfg_rule<T_OtherSymbol>& other) const
	{
		return lhs == other.lhs && rhs == other.rhs;
	}
};
} // namespace detail

template <typename T_Symbol>
using cfg_rule = detail::basic_cfg_rule<T_Symbol>;

} // namespace fsm

#endif // FSM_DETAIL_CFG_RULE_HPP
