#ifndef FSM_DETAIL_LR0_ITEM_HPP
#define FSM_DETAIL_LR0_ITEM_HPP

#include "../detail/cfg_rule.hpp"

namespace fsm::lr
{
namespace detail
{
template <typename T_Symbol>
struct basic_lr0_item
{
	cfg_rule<T_Symbol> rule;
	std::size_t dot = 0;

	[[nodiscard]] bool is_complete() const
	{
		return dot >= rule.rhs.size();
	}

	T_Symbol next_symbol() const
	{
		return rule.rhs[dot];
	}

	bool operator<(basic_lr0_item const& other) const
	{
		if (dot != other.dot)
		{
			return dot < other.dot;
		}
		if (rule.lhs != other.rule.lhs)
		{
			return rule.lhs < other.rule.lhs;
		}

		return rule.rhs < other.rule.rhs;
	}

	bool operator==(basic_lr0_item const& other) const
	{
		return dot == other.dot && rule.lhs == other.rule.lhs && rule.rhs == other.rule.rhs;
	}
};
} // namespace detail

template <typename T_Symbol>
using lr0_item = detail::basic_lr0_item<T_Symbol>;

} // namespace fsm

#endif // FSM_DETAIL_LR0_ITEM_HPP
