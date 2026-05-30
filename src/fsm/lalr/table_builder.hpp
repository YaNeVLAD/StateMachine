#ifndef FSM_LALR_TABLE_BUILDER_HPP
#define FSM_LALR_TABLE_BUILDER_HPP

#include "../cfg/cfg_algorithms.hpp"
#include "../lr/basic_table_builder.hpp"
#include "../lr/lr0_item.hpp"
#include "../lr/table.hpp"

#include <expected>
#include <queue>
#include <set>

namespace fsm::lalr
{
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
class table_builder : public lr::basic_table_builder<T_Symbol, T_Compare>
{
	using base_t = lr::basic_table_builder<T_Symbol, T_Compare>;

	using lr0_item_t = lr::lr0_item<T_Symbol>;

	using lr1_state_t = std::map<lr0_item_t, std::set<T_Symbol, T_Compare>>;
	using first_sets_t = std::map<T_Symbol, std::set<T_Symbol, T_Compare>, T_Compare>;

public:
	using base_t::base_table_builder;

	template <typename T_CollisionPolicy = lr::detail::strict_t>
	std::expected<typename base_t::table_type, std::vector<lr::conflict_error<T_Symbol>>>
	build(T_CollisionPolicy policy = lr::collision_policy::strict) const
	{
		using core_set_t = std::set<lr0_item_t>;

		typename base_t::table_type result;

		result.set_end_marker(this->m_eof);
		std::vector<lr::conflict_error<T_Symbol>> errors;

		auto first_sets = algorithms::compute_first(this->m_grammar, this->m_epsilon);

		lr0_item_t start_item{ { this->m_aug_start, { this->m_grammar.start_symbol() } }, 0 };
		lr1_state_t initial_state;
		initial_state[start_item].insert(this->m_eof);
		initial_state = compute_closure(initial_state, first_sets);

		std::vector<lr1_state_t> lr1_states;
		std::map<lr1_state_t, std::size_t> state_to_id;
		std::map<std::pair<std::size_t, T_Symbol>, std::size_t> lr1_transitions;

		lr1_states.push_back(initial_state);
		state_to_id[initial_state] = 0;

		std::queue<std::size_t> q;
		q.push(0);

		while (!q.empty())
		{
			std::size_t current_id = q.front();
			q.pop();
			const lr1_state_t current_state = lr1_states[current_id];

			std::set<T_Symbol, T_Compare> next_symbols;
			for (const auto& [item, _] : current_state)
			{
				if (!item.is_complete() && item.next_symbol() != this->m_epsilon)
				{
					next_symbols.insert(item.next_symbol());
				}
			}

			for (const auto& X : next_symbols)
			{
				lr1_state_t next_state = compute_goto(current_state, X, first_sets);
				if (next_state.empty())
					continue;

				if (!state_to_id.contains(next_state))
				{
					std::size_t new_id = lr1_states.size();
					lr1_states.push_back(next_state);
					state_to_id[next_state] = new_id;
					q.push(new_id);
				}

				lr1_transitions[{ current_id, X }] = state_to_id[next_state];
			}
		}

		std::vector<lr1_state_t> lalr_states;
		std::map<core_set_t, std::size_t> core_to_lalr_index;
		std::map<std::size_t, std::size_t> lr1_to_lalr;

		for (std::size_t i = 0; i < lr1_states.size(); ++i)
		{
			core_set_t core;
			for (const auto& [item, _] : lr1_states[i])
			{
				core.insert(item);
			}

			if (!core_to_lalr_index.contains(core))
			{
				core_to_lalr_index[core] = lalr_states.size();
				lalr_states.push_back(lr1_states[i]);
				lr1_to_lalr[i] = core_to_lalr_index[core];
			}
			else
			{
				std::size_t lalr_idx = core_to_lalr_index[core];
				lr1_to_lalr[i] = lalr_idx;

				for (const auto& [item, lookaheads] : lr1_states[i])
				{
					lalr_states[lalr_idx][item].insert(lookaheads.begin(), lookaheads.end());
				}
			}
		}

		std::map<std::pair<std::size_t, T_Symbol>, std::size_t> lalr_transitions;
		for (const auto& [key, target_lr1] : lr1_transitions)
		{
			const auto& [source_lr1, sym] = key;
			lalr_transitions[{ lr1_to_lalr[source_lr1], sym }] = lr1_to_lalr[target_lr1];
		}

		for (std::size_t i = 0; i < lalr_states.size(); ++i)
		{
			for (const auto& [item, lookaheads] : lalr_states[i])
			{
				if (!item.is_complete())
				{ // SHIFT or GOTO
					if (T_Symbol a = item.next_symbol(); this->m_grammar.is_terminal(a) && lalr_transitions.contains({ i, a }))
					{
						this->try_add_action(result, i, a, lr::action_shift<std::size_t>{ lalr_transitions.at({ i, a }) }, policy, errors);
					}
					else if (this->m_grammar.is_non_terminal(a) && lalr_transitions.contains({ i, a }))
					{
						result.add_goto(i, a, lalr_transitions.at({ i, a }));
					}
				}
				else if (item.rule.lhs == this->m_aug_start)
				{ // ACCEPT
					this->try_add_action(result, i, this->m_eof, lr::action_accept{}, policy, errors);
				}
				else
				{ // REDUCE
					for (const auto& a : lookaheads)
					{
						this->try_add_action(result, i, a, lr::action_reduce<T_Symbol>{ item.rule }, policy, errors);
					}
				}
			}
		}

		if constexpr (std::is_same_v<T_CollisionPolicy, lr::detail::strict_t>)
		{
			if (!errors.empty())
			{
				return std::unexpected(std::move(errors));
			}
		}

		return result;
	}

private:
	base_t::grammar_t m_grammar;
	T_Symbol m_epsilon;
	T_Symbol m_eof;
	T_Symbol m_aug_start;

	struct closure_result
	{
		std::set<lr0_item_t> items;
		std::map<lr0_item_t, std::set<T_Symbol, T_Compare>> lookaheads;
	};

	lr1_state_t compute_closure(lr1_state_t I, const first_sets_t& first_sets) const
	{
		bool changed = true;
		while (changed)
		{
			changed = false;
			for (auto current_I = I; const auto& [item, lookaheads] : current_I)
			{
				if (item.is_complete() || this->m_grammar.is_terminal(item.next_symbol()))
				{
					continue;
				}

				T_Symbol B = item.next_symbol();
				std::vector<T_Symbol> beta(item.rule.rhs.begin() + item.dot + 1, item.rule.rhs.end());

				for (const auto& rule : this->m_grammar.rules())
				{
					if (rule.lhs == B)
					{
						std::size_t initial_dot = (rule.rhs.size() == 1 && rule.rhs[0] == this->m_epsilon) ? 1 : 0;
						lr0_item_t new_core{ rule, initial_dot };

						// FIRST(beta + a) for 'a'
						for (const auto& la : lookaheads)
						{
							for (auto first_beta_la = compute_first_of_sequence_with_la(beta, la, first_sets);
								const auto& b : first_beta_la)
							{
								if (I[new_core].insert(b).second)
								{
									changed = true;
								}
							}
						}
					}
				}
			}
		}

		return I;
	}

	lr1_state_t compute_goto(const lr1_state_t& I, const T_Symbol& X, const first_sets_t& first_sets) const
	{
		lr1_state_t J;
		for (const auto& [item, lookaheads] : I)
		{
			if (!item.is_complete() && item.next_symbol() == X)
			{
				lr0_item_t next_core = item;
				++next_core.dot;
				J[next_core] = lookaheads;
			}
		}
		return compute_closure(J, first_sets);
	}

	std::set<T_Symbol, T_Compare> compute_first_of_sequence_with_la(
		const std::vector<T_Symbol>& beta,
		const T_Symbol& la,
		const first_sets_t& first_sets) const
	{
		auto res = algorithms::impl::ComputeFirstOf_fn{}(beta, first_sets, this->m_epsilon);

		if (res.contains(this->m_epsilon))
		{
			res.erase(this->m_epsilon);
			res.insert(la);
		}
		return res;
	}
};

} // namespace fsm::lalr

#endif // FSM_LALR_TABLE_BUILDER_HPP
