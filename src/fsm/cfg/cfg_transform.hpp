#ifndef FSM_CFG_TRANSFORM_HPP
#define FSM_CFG_TRANSFORM_HPP

#include "basic_cfg.hpp"
#include "cfg_algorithms.hpp"

namespace fsm::transforms
{
struct isolate_start_symbol_t
{
};

struct remove_epsilon_rules_t
{
};

struct remove_unit_rules_t
{
};

struct remove_useless_symbols_t
{
};

struct to_chomsky_normal_form_t
{
};

struct merge_equivalent_symbols_t
{
};

inline constexpr isolate_start_symbol_t isolate_start_symbol{};
inline constexpr remove_epsilon_rules_t remove_epsilon_rules{};
inline constexpr remove_unit_rules_t remove_unit_rules{};
inline constexpr remove_useless_symbols_t remove_useless_symbols{};
inline constexpr to_chomsky_normal_form_t to_chomsky_normal_form{};
inline constexpr merge_equivalent_symbols_t merge_equivalent_symbols{};

template <typename T_Symbol, typename T_Comp>
basic_cfg<T_Symbol, T_Comp> operator|(const basic_cfg<T_Symbol, T_Comp>& grammar, isolate_start_symbol_t)
{
	return algorithms::isolate_start_symbol(grammar);
}

template <typename T_Symbol, typename T_Comp>
basic_cfg<T_Symbol, T_Comp> operator|(const basic_cfg<T_Symbol, T_Comp>& grammar, remove_epsilon_rules_t)
{
	return algorithms::remove_epsilon_rules(grammar);
}

template <typename T_Symbol, typename T_Comp>
basic_cfg<T_Symbol, T_Comp> operator|(const basic_cfg<T_Symbol, T_Comp>& grammar, remove_unit_rules_t)
{
	return algorithms::remove_unit_rules(grammar);
}

template <typename T_Symbol, typename T_Comp>
basic_cfg<T_Symbol, T_Comp> operator|(const basic_cfg<T_Symbol, T_Comp>& grammar, remove_useless_symbols_t)
{
	return algorithms::remove_useless_symbols(grammar);
}

template <typename T_Symbol, typename T_Comp>
basic_cfg<T_Symbol, T_Comp> operator|(const basic_cfg<T_Symbol, T_Comp>& grammar, to_chomsky_normal_form_t)
{
	return algorithms::to_chomsky_normal_form(grammar);
}

template <typename T_Symbol, typename T_Comp>
basic_cfg<T_Symbol, T_Comp> operator|(const basic_cfg<T_Symbol, T_Comp>& grammar, merge_equivalent_symbols_t)
{
	return algorithms::merge_equivalent_symbols(grammar);
}
} // namespace fsm::transforms

#endif // FSM_CFG_TRANSFORM_HPP
