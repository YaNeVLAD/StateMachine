#ifndef FSM_LR_TABLE_IO_HPP
#define FSM_LR_TABLE_IO_HPP

#include "../concepts.hpp"
#include "../utility.hpp"
#include "table.hpp"

#include <iostream>

namespace fsm::lr::io
{
namespace detail
{
template <typename T>
void write_bin(std::ostream& os, const T& val)
{
	if constexpr (concepts::contiguous_container<T>)
	{
		const size_t sz = val.size();
		os.write(reinterpret_cast<const char*>(&sz), sizeof(sz));

		if (sz > 0)
		{
			os.write(reinterpret_cast<const char*>(val.data()), sz * sizeof(*val.data()));
		}
	}
	else if constexpr (std::is_trivially_copyable_v<T>)
	{
		os.write(reinterpret_cast<const char*>(&val), sizeof(val));
	}
	else
	{
		static_assert(sizeof(T) == 0, "Type is not serializable. It must be TriviallyCopyable or a ContiguousContainer of TriviallyCopyable elements.");
	}
}

template <typename T>
void read_bin(std::istream& is, T& val)
{
	if constexpr (concepts::contiguous_container<T>)
	{
		size_t sz = 0;
		is.read(reinterpret_cast<char*>(&sz), sizeof(sz));
		val.resize(sz);

		if (sz > 0)
		{
			const auto const_byte_ptr = reinterpret_cast<const char*>(val.data());
			const auto mutable_byte_ptr = const_cast<char*>(const_byte_ptr);

			is.read(mutable_byte_ptr, sz * sizeof(*val.data()));
		}
	}
	else if constexpr (std::is_trivially_copyable_v<T>)
	{
		is.read(reinterpret_cast<char*>(&val), sizeof(val));
	}
	else
	{
		static_assert(sizeof(T) == 0, "Type is not deserializable.");
	}
}
} // namespace detail

template <typename T_Symbol, typename T_Compare>
void save_to_binary(const table<T_Symbol, T_Compare>& tbl, std::ostream& os)
{
	using state_t = typename table<T_Symbol, T_Compare>::state_type;

	detail::write_bin(os, tbl.end_marker());

	const auto& action_tbl = tbl.action_table();
	const std::size_t action_size = action_tbl.size();
	detail::write_bin(os, action_size);

	for (const auto& [state, row] : action_tbl)
	{
		detail::write_bin(os, state);
		const std::size_t row_size = row.size();
		detail::write_bin(os, row_size);

		for (const auto& [sym, act] : row)
		{
			detail::write_bin(os, sym);

			const std::size_t act_idx = act.index();
			detail::write_bin(os, act_idx);

			utility::overloaded_visitor(
				act,
				[&](const action_error&) {},
				[&](const action_accept&) {},
				[&](const action_shift<state_t>& s) {
					detail::write_bin(os, s.target_state);
				},
				[&](const action_reduce<T_Symbol>& r) {
					detail::write_bin(os, r.rule.lhs);
					const size_t rhs_size = r.rule.rhs.size();
					detail::write_bin(os, rhs_size);
					for (const auto& rs : r.rule.rhs)
					{
						detail::write_bin(os, rs);
					}
				});
		}
	}

	const auto& goto_tbl = tbl.goto_table();
	const std::size_t goto_size = goto_tbl.size();
	detail::write_bin(os, goto_size);

	for (const auto& [state, row] : goto_tbl)
	{
		detail::write_bin(os, state);
		const std::size_t row_size = row.size();
		detail::write_bin(os, row_size);

		for (const auto& [sym, target] : row)
		{
			detail::write_bin(os, sym);
			detail::write_bin(os, target);
		}
	}
}

template <typename T_Symbol, typename T_Compare = std::less<T_Symbol>>
table<T_Symbol, T_Compare> load_from_binary(std::istream& is)
{
	using state_t = typename table<T_Symbol, T_Compare>::state_type;
	table<T_Symbol, T_Compare> tbl;

	T_Symbol eof;
	detail::read_bin(is, eof);
	tbl.set_end_marker(eof);

	std::size_t action_tbl_size = 0;
	detail::read_bin(is, action_tbl_size);

	for (std::size_t i = 0; i < action_tbl_size; ++i)
	{
		state_t state;
		std::size_t row_size;
		detail::read_bin(is, state);
		detail::read_bin(is, row_size);

		for (std::size_t j = 0; j < row_size; ++j)
		{
			T_Symbol sym;
			std::size_t act_idx;
			detail::read_bin(is, sym);
			detail::read_bin(is, act_idx);

			if (act_idx == 0)
			{
				tbl.add_action(state, sym, action_error{});
			}
			else if (act_idx == 1)
			{
				tbl.add_action(state, sym, action_accept{});
			}
			else if (act_idx == 2)
			{
				state_t target;
				detail::read_bin(is, target);
				tbl.add_action(state, sym, action_shift<state_t>{ target });
			}
			else if (act_idx == 3)
			{
				cfg_rule<T_Symbol> rule;
				detail::read_bin(is, rule.lhs);
				std::size_t rhs_size;
				detail::read_bin(is, rhs_size);
				rule.rhs.resize(rhs_size);
				for (std::size_t k = 0; k < rhs_size; ++k)
				{
					detail::read_bin(is, rule.rhs[k]);
				}
				tbl.add_action(state, sym, action_reduce<T_Symbol>{ std::move(rule) });
			}
		}
	}

	std::size_t goto_tbl_size = 0;
	detail::read_bin(is, goto_tbl_size);

	for (std::size_t i = 0; i < goto_tbl_size; ++i)
	{
		state_t state;
		std::size_t row_size;
		detail::read_bin(is, state);
		detail::read_bin(is, row_size);

		for (std::size_t j = 0; j < row_size; ++j)
		{
			T_Symbol sym;
			state_t target;
			detail::read_bin(is, sym);
			detail::read_bin(is, target);
			tbl.add_goto(state, sym, target);
		}
	}

	return tbl;
}
} // namespace fsm::lr::io

#endif // FSM_LR_TABLE_IO_HPP