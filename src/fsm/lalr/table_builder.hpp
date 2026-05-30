#ifndef FSM_LALR_TABLE_BUILDER_HPP
#define FSM_LALR_TABLE_BUILDER_HPP

#include "../cfg/basic_cfg.hpp"
#include "../lr/lr0_item.hpp"

#include <set>

namespace fsm::lalr
{
template <typename T_Symbol, typename T_Compare = std::less<T_Symbol>>
struct lalr_item
{
	lr::lr0_item<T_Symbol> core;
	std::set<T_Symbol, T_Compare> lookaheads;

	bool operator==(lalr_item const& other) const
	{
		return core == other.core;
	}

	auto operator<=>(lalr_item const& other) const
	{
		return core <=> other.core;
	}
};

namespace detail
{
struct propagation_edge
{
	std::size_t from_state;
	std::size_t from_item_index;

	std::size_t to_state;
	std::size_t to_item_index;
};
} // namespace detail

template <typename T_Symbol, typename T_Compare = std::less<T_Symbol>>
class table_builder
{
	using grammar_t = basic_cfg<T_Symbol, T_Compare>;
	using lr0_item_t = lr::lr0_item<T_Symbol>;
	using lalr_item_t = lalr_item<T_Symbol, T_Compare>;
};

} // namespace fsm::lalr

#endif // FSM_LALR_TABLE_BUILDER_HPP
