#ifndef FSM_SLR_PARSER_HPP
#define FSM_SLR_PARSER_HPP

#include "../utility.hpp"
#include "table.hpp"

#include <span>
#include <variant>

namespace fsm::slr
{

template <typename T_Symbol>
struct event_shift
{
	T_Symbol token;
};

template <typename T_Symbol>
struct event_reduce
{
	cfg_rule<T_Symbol> rule;
};

struct event_accept
{
};

template <typename T_Symbol>
struct event_error
{
	T_Symbol unexpected_token;
};

template <typename T_Symbol>
using parse_event = std::variant<
	event_shift<T_Symbol>,
	event_reduce<T_Symbol>,
	event_accept,
	event_error<T_Symbol>>;

namespace events
{
template <typename T_Symbol>
bool is_error(const parse_event<T_Symbol>& act) { return std::holds_alternative<event_error<T_Symbol>>(act); }

template <typename T_Symbol>
bool is_accept(const parse_event<T_Symbol>& act) { return std::holds_alternative<event_accept>(act); }

template <typename T_Symbol>
bool is_shift(const parse_event<T_Symbol>& act) { return std::holds_alternative<event_shift<T_Symbol>>(act); }

template <typename T_Symbol>
bool is_reduce(const parse_event<T_Symbol>& act) { return std::holds_alternative<event_reduce<T_Symbol>>(act); }

template <typename T_Symbol>
event_error<T_Symbol> as_error(const parse_event<T_Symbol>& act) { return std::get<event_error<T_Symbol>>(act); }

template <typename T_Symbol>
event_accept as_accept(const parse_event<T_Symbol>& act) { return std::get<event_accept>(act); }

template <typename T_Symbol>
event_shift<T_Symbol> as_shift(const parse_event<T_Symbol>& act) { return std::get<event_shift<T_Symbol>>(act); }

template <typename T_Symbol>
event_reduce<T_Symbol> as_reduce(const parse_event<T_Symbol>& act) { return std::get<event_reduce<T_Symbol>>(act); }
} // namespace events

template <typename T_Symbol, typename T_Compare = std::less<T_Symbol>>
class parser
{
	using event_t = parse_event<T_Symbol>;
	using opt_event_t = std::optional<event_t>;

	class iterator
	{
	public:
		iterator() = default;

		explicit iterator(parser* ptr)
			: m_ptr(ptr)
		{
			advance();
		}

		const event_t& operator*() const
		{
			return *m_current;
		}

		const event_t* operator->() const
		{
			return m_current;
		}

		iterator& operator++()
		{
			advance();

			return *this;
		}

		bool operator!=(const iterator& rhs) const
		{
			return m_ptr != rhs.m_ptr;
		}

	private:
		void advance()
		{
			if (m_ptr)
			{
				m_current = m_ptr->next();
				if (!m_current)
				{
					m_ptr = nullptr;
				}
			}
		}

		parser* m_ptr = nullptr;
		opt_event_t m_current = std::nullopt;
	};

	class iter_range
	{
	public:
		explicit iter_range(parser& p)
			: m_parser(p)
		{
		}

		iterator begin() const
		{
			return iterator{ &m_parser };
		}

		iterator end() const
		{
			return iterator{ nullptr };
		}

	private:
		parser& m_parser;
	};

public:
	using table_type = table<T_Symbol, T_Compare>;
	using state_type = typename table_type::state_type;
	using event_type = event_t;
	using optional_event_type = opt_event_t;

	using iterator = iterator;

	parser(const table_type& tbl, std::type_identity_t<T_Symbol> epsilon_symbol)
		: m_table{ tbl }
		, m_epsilon{ std::move(epsilon_symbol) }
	{
	}

	iter_range parse(std::span<const T_Symbol> input)
	{
		m_input = input;
		m_input_ptr = 0;

		m_state_stack.clear();
		m_state_stack.push_back(0);

		m_is_finished = false;

		return iter_range{ *this };
	}

	[[nodiscard]] bool is_finished() const
	{
		return m_is_finished;
	}

	[[nodiscard]] optional_event_type next()
	{
		if (m_is_finished)
		{
			return std::nullopt;
		}

		state_type current_state = m_state_stack.back();
		T_Symbol current_token = (m_input_ptr < m_input.size())
			? m_input[m_input_ptr]
			: m_table.end_marker();

		auto act = m_table.get_action(current_state, current_token);

		return utility::overloaded_visitor(
			act,
			[&](const action_shift<state_type>& arg) -> std::optional<event_type> {
				m_state_stack.emplace_back(arg.target_state);
				if (m_input_ptr < m_input.size())
				{
					m_input_ptr++;
				}

				return event_shift<T_Symbol>{ current_token };
			},
			[&](const action_reduce<T_Symbol>& arg) -> std::optional<event_type> {
				using opt_state = typename table_type::optional_state;

				const auto& rule = arg.rule;
				std::size_t pop_count = rule.rhs.size();

				if (pop_count == 1 && rule.rhs[0] == m_epsilon)
				{
					pop_count = 0;
				}

				for (std::size_t i = 0; i < pop_count; ++i)
				{
					m_state_stack.pop_back();
				}

				state_type state_after_pop = m_state_stack.back();
				opt_state next_state = m_table.get_goto(state_after_pop, rule.lhs);

				if (!next_state.has_value())
				{
					m_is_finished = true;

					return event_error<T_Symbol>{ current_token };
				}

				m_state_stack.emplace_back(next_state.value());

				return event_reduce<T_Symbol>{ rule };
			},
			[&](const action_accept&) -> std::optional<event_type> {
				m_is_finished = true;

				return event_accept{};
			},
			[&](const action_error&) -> std::optional<event_type> {
				m_is_finished = true;

				return event_error<T_Symbol>{ current_token };
			});
	}

private:
	const table_type& m_table;
	T_Symbol m_epsilon;

	std::span<const T_Symbol> m_input;
	std::size_t m_input_ptr = 0;

	std::vector<state_type> m_state_stack;
	bool m_is_finished = true;
};

} // namespace fsm::slr

#endif // FSM_SLR_PARSER_HPP
