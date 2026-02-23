#ifndef FSM_DEFAULT_SYMBOL_GENERATOR_HPP
#define FSM_DEFAULT_SYMBOL_GENERATOR_HPP

namespace fsm
{
template <typename T_Symbol, typename = void>
struct default_symbol_generator
{
	static_assert(sizeof(T_Symbol) == 0,
		"To use algorithms that generate new symbols, provide a default_symbol_generator specialization for your type.");
};
} // namespace fsm

#endif // FSM_DEFAULT_SYMBOL_GENERATOR_HPP
