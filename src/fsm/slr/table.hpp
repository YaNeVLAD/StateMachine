#ifndef FSM_SLR_TABLE_HPP
#define FSM_SLR_TABLE_HPP

#include "../cfg/basic_cfg.hpp"

#include <map>
#include <variant>

namespace fsm::slr
{
struct action_error
{
};

struct action_accept
{
};

template <typename T_State>
struct action_shift
{
	T_State target_state;
};

template <typename T_Symbol>
struct action_reduce
{
	cfg_rule<T_Symbol> rule;
};

template <typename T_Symbol, typename T_State>
using action = std::variant<
	action_error,
	action_accept,
	action_shift<T_State>,
	action_reduce<T_Symbol>>;

namespace actions
{
template <typename T_Symbol, typename T_State>
bool is_error(const action<T_Symbol, T_State>& act) { return std::holds_alternative<action_error>(act); }

template <typename T_Symbol, typename T_State>
bool is_accept(const action<T_Symbol, T_State>& act) { return std::holds_alternative<action_accept>(act); }

template <typename T_Symbol, typename T_State>
bool is_shift(const action<T_Symbol, T_State>& act) { return std::holds_alternative<action_shift<T_State>>(act); }

template <typename T_Symbol, typename T_State>
bool is_reduce(const action<T_Symbol, T_State>& act) { return std::holds_alternative<action_reduce<T_Symbol>>(act); }

template <typename T_Symbol, typename T_State>
action_error as_error(const action<T_Symbol, T_State>& act) { return std::get<action_error>(act); }

template <typename T_Symbol, typename T_State>
action_accept as_accept(const action<T_Symbol, T_State>& act) { return std::get<action_accept>(act); }

template <typename T_Symbol, typename T_State>
action_shift<T_State> as_shift(const action<T_Symbol, T_State>& act) { return std::get<action_shift<T_State>>(act); }

template <typename T_Symbol, typename T_State>
action_reduce<T_Symbol> as_reduce(const action<T_Symbol, T_State>& act) { return std::get<action_reduce<T_Symbol>>(act); }
} // namespace actions

template <typename T_Symbol, typename T_Compare = std::less<T_Symbol>>
class table
{
	using state_t = std::size_t;
	static_assert(std::is_integral_v<state_t> && std::is_convertible_v<state_t, std::size_t>,
		"state_t must be integral and convertable to std::size_t");

	using action_t = action<T_Symbol, state_t>;

	using action_map_t = std::map<T_Symbol, action_t, T_Compare>;
	using goto_map_t = std::map<T_Symbol, state_t, T_Compare>;

public:
	using action_type = action_t;
	using symbol_type = T_Symbol;
	using state_type = state_t;
	using optional_state = std::optional<state_type>;

	using action_table_type = std::map<state_type, action_map_t>;
	using goto_table_type = std::map<state_type, goto_map_t>;

	static constexpr state_type invalid_state = static_cast<state_type>(-1);

	void add_action(const state_type state, const T_Symbol& terminal, const action_type& act)
	{
		m_action_table[state][terminal] = act;
	}

	void add_goto(const state_type state, const T_Symbol& non_terminal, state_type target_state)
	{
		m_goto_table[state][non_terminal] = target_state;
	}

	const action_type& get_action(const state_type state, const T_Symbol& terminal) const
	{
		static const action_type default_error = action_error{};

		return find_action(state, terminal, default_error);
	}

	const action_type& get_action(const optional_state state, const T_Symbol& terminal) const
	{
		if (!state.has_value())
		{
			return get_action(invalid_state, terminal);
		}

		return get_action(*state, terminal);
	}

	optional_state get_goto(const state_type state, const T_Symbol& non_terminal) const
	{
		return find_goto(state, non_terminal);
	}

	table& set_end_marker(T_Symbol eof)
	{
		m_eof = std::move(eof);

		return *this;
	}

	const T_Symbol& end_marker() const
	{
		return m_eof;
	}

	const action_table_type& action_table() const
	{
		return m_action_table;
	}

	const goto_table_type& goto_table() const
	{
		return m_goto_table;
	}

private:
	action_table_type m_action_table;
	goto_table_type m_goto_table;
	T_Symbol m_eof;

	const action_type& find_action(state_type state, const T_Symbol& symbol, const action_type& default_val) const
	{
		if (auto state_it = m_action_table.find(state); state_it != m_action_table.end())
		{
			if (auto action_it = state_it->second.find(symbol); action_it != state_it->second.end())
			{
				return action_it->second;
			}
		}

		return default_val;
	}

	optional_state find_goto(state_type state, const T_Symbol& symbol) const
	{
		if (auto state_it = m_goto_table.find(state); state_it != m_goto_table.end())
		{
			if (auto action_it = state_it->second.find(symbol); action_it != state_it->second.end())
			{
				return action_it->second;
			}
		}

		return std::nullopt;
	}
};
} // namespace fsm::slr

#endif // FSM_SLR_TABLE_HPP
