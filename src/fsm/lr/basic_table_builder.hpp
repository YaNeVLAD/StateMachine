#ifndef FSM_LR_BASIC_TABLE_BUILDER_HPP
#define FSM_LR_BASIC_TABLE_BUILDER_HPP

#include "../utility.hpp"
#include "lr0_item.hpp"
#include "table.hpp"

#include <format>
#include <functional>
#include <string>

namespace fsm::lr
{

template <typename T_Symbol>
struct conflict_error
{
	std::size_t state_id;
	T_Symbol terminal;
	bool is_shift_reduce;
	std::string description;

	[[nodiscard]] std::string to_string() const
	{
		return std::format("State {}: {} conflict on terminal '{}'\nDetails:\n{}",
			state_id,
			is_shift_reduce ? "Shift/Reduce" : "Reduce/Reduce",
			terminal,
			description);
	}
};

namespace detail
{
#define FSM_LR_EMPTY_TYPE(name) \
	struct name                 \
	{                           \
	}

FSM_LR_EMPTY_TYPE(strict_t);
FSM_LR_EMPTY_TYPE(prefer_shift_t);
FSM_LR_EMPTY_TYPE(keep_first_t);
FSM_LR_EMPTY_TYPE(keep_last_t);

#undef FSM_LR_EMPTY_TYPE
} // namespace detail

namespace collision_policy
{
inline constexpr detail::strict_t strict{};
inline constexpr detail::prefer_shift_t prefer_shift{};
inline constexpr detail::keep_first_t keep_first{};
inline constexpr detail::keep_last_t keep_last{};
} // namespace collision_policy

template <typename T_Symbol, typename T_Compare = std::less<T_Symbol>>
class basic_table_builder
{
	using table_t = table<T_Symbol, T_Compare>;

protected:
	using grammar_t = basic_cfg<T_Symbol, T_Compare>;
	using item_t = lr0_item<T_Symbol>;
	using state_t = std::set<item_t>;
	using identity_symbol = std::type_identity_t<T_Symbol>;

	using action_type = table_t::action_type;
	using state_id_t = table_t::state_type;

public:
	using grammar_type = grammar_t;
	using table_type = table_t;
	using warning_callback_type = std::function<void(const std::string&)>;

	basic_table_builder() = delete;

	basic_table_builder(grammar_t const& grammar, T_Symbol epsilon, T_Symbol end_marker, T_Symbol aug_start)
		: m_grammar(grammar)
		, m_epsilon(std::move(epsilon))
		, m_eof(std::move(end_marker))
		, m_aug_start(std::move(aug_start))
	{
	}

	auto& with_epsilon(T_Symbol epsilon)
	{
		m_epsilon = std::move(epsilon);

		return *this;
	}

	auto& with_end_marker(T_Symbol end_marker)
	{
		m_eof = std::move(end_marker);

		return *this;
	}

	auto& with_augmented_start(T_Symbol aug_start)
	{
		m_aug_start = std::move(aug_start);

		return *this;
	}

	auto& on_warning(warning_callback_type callback)
	{
		m_warning_callback = std::move(callback);

		return *this;
	}

protected:
	grammar_t m_grammar;
	T_Symbol m_epsilon;
	T_Symbol m_eof;
	T_Symbol m_aug_start;
	warning_callback_type m_warning_callback;

	void call_warning(const std::string& msg) const
	{
		if (m_warning_callback)
		{
			m_warning_callback(msg);
		}
	}

	template <typename T_CollisionPolicy>
	void try_add_action(
		table_type& out_table,
		state_id_t state_id,
		const T_Symbol& terminal,
		const action_type& new_action,
		T_CollisionPolicy policy,
		std::vector<conflict_error<T_Symbol>>& collected_errors) const
	{
		const auto& existing = out_table.get_action(state_id, terminal);

		if (actions::is_error(existing))
		{
			out_table.add_action(state_id, terminal, new_action);
			return;
		}

		const bool is_sr = (actions::is_shift(existing) && actions::is_reduce(new_action))
			|| (actions::is_reduce(existing) && actions::is_shift(new_action));
		const bool is_rr = actions::is_reduce(existing) && actions::is_reduce(new_action);

		const std::string conflict_type = is_sr ? "Shift/Reduce" : (is_rr ? "Reduce/Reduce" : "Unknown");
		const std::string msg = "LR " + conflict_type + " conflict at state " + std::to_string(state_id);

		conflict_error<T_Symbol> err{ state_id, terminal, is_sr, msg };

		utility::overloaded{
			[&](detail::strict_t) {
				collected_errors.push_back(std::move(err));
			},
			[&](detail::prefer_shift_t) {
				if (is_sr)
				{
					if (actions::is_shift(new_action))
						out_table.add_action(state_id, terminal, new_action);
					call_warning("[WARNING] " + msg + " -> Resolved: Prefer Shift.");
				}
				else if (is_rr)
				{
					call_warning("[WARNING] " + msg + " -> Resolved: Keep First (R/R).");
				}
			},
			[&](detail::keep_first_t) {
				call_warning("[WARNING] " + msg + " -> Resolved: Keep First.");
			},
			[&](detail::keep_last_t) {
				call_warning("[WARNING] " + msg + " -> Resolved: Keep Last.");
				out_table.add_action(state_id, terminal, new_action);
			}
		}(policy);
	}
};

} // namespace fsm::lr

#endif // FSM_LR_BASIC_TABLE_BUILDER_HPP
