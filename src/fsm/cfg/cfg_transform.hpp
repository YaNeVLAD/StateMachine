#ifndef FSM_CFG_TRANSFORM_HPP
#define FSM_CFG_TRANSFORM_HPP

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

inline constexpr isolate_start_symbol_t isolate_start_symbol{};
inline constexpr remove_epsilon_rules_t remove_epsilon_rules{};
inline constexpr remove_unit_rules_t remove_unit_rules{};
inline constexpr remove_useless_symbols_t remove_useless_symbols{};
inline constexpr to_chomsky_normal_form_t to_chomsky_normal_form{};

template <typename T_Grammar>
T_Grammar operator|(const T_Grammar& grammar, isolate_start_symbol_t)
{
	return algorithms::isolate_start_symbol(grammar);
}

template <typename T_Grammar>
T_Grammar operator|(const T_Grammar& grammar, remove_epsilon_rules_t)
{
	return algorithms::remove_epsilon_rules(grammar);
}

template <typename T_Grammar>
T_Grammar operator|(const T_Grammar& grammar, remove_unit_rules_t)
{
	return algorithms::remove_unit_rules(grammar);
}

template <typename T_Grammar>
T_Grammar operator|(const T_Grammar& grammar, remove_useless_symbols_t)
{
	return algorithms::remove_useless_symbols(grammar);
}

template <typename T_Grammar>
T_Grammar operator|(const T_Grammar& grammar, to_chomsky_normal_form_t)
{
	return algorithms::to_chomsky_normal_form(grammar);
}
} // namespace fsm::transforms

#endif // FSM_CFG_TRANSFORM_HPP
