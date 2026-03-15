#ifndef FSM_LL1_TABLE_IO_HPP
#define FSM_LL1_TABLE_IO_HPP

#include "../cfg/basic_cfg.hpp"
#include "table.hpp"

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace fsm::ll1::io
{
template <typename T_Symbol, typename T_Compare>
void save_to_text(const table<T_Symbol, T_Compare>& tbl, std::ostream& os)
{
	os << tbl.epsilon() << "\n";
	os << tbl.end_marker() << "\n";

	size_t total_entries = 0;
	for (const auto& [lhs, row] : tbl.entries())
	{
		total_entries += row.size();
	}

	os << total_entries << "\n";

	// Format: LHS Terminal RhsSize RhsSym1 RhsSym2 ...
	for (const auto& [lhs, row] : tbl.entries())
	{
		for (const auto& [term, rule] : row)
		{
			os << lhs << " " << term << " " << rule.rhs.size();
			for (const auto& sym : rule.rhs)
			{
				os << " " << sym;
			}
			os << "\n";
		}
	}
}

template <typename T_Symbol, typename T_Compare = std::less<T_Symbol>>
table<T_Symbol, T_Compare> load_from_text(std::istream& is)
{
	table<T_Symbol, T_Compare> tbl;
	T_Symbol eps, eof;

	if (!(is >> eps >> eof))
	{
		return tbl;
	}

	tbl.set_epsilon(eps).set_end_marker(eof);

	size_t total_entries;
	is >> total_entries;

	for (size_t i = 0; i < total_entries; ++i)
	{
		T_Symbol lhs, term;
		size_t rhs_size;
		is >> lhs >> term >> rhs_size;

		cfg_rule<T_Symbol> rule{ lhs, {} };
		rule.rhs.reserve(rhs_size);

		for (size_t j = 0; j < rhs_size; ++j)
		{
			T_Symbol sym;
			is >> sym;
			rule.rhs.push_back(sym);
		}

		tbl.add_entry(lhs, term, rule);
	}

	return tbl;
}

namespace detail
{
template <typename T>
void write_bin(std::ostream& os, const T& val)
{
	if constexpr (std::is_same_v<T, std::string>)
	{
		const size_t sz = val.size();
		os.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
		os.write(val.data(), sz);
	}
	else
	{
		os.write(reinterpret_cast<const char*>(&val), sizeof(val));
	}
}

template <typename T>
void read_bin(std::istream& is, T& val)
{
	if constexpr (std::is_same_v<T, std::string>)
	{
		size_t sz = 0;
		is.read(reinterpret_cast<char*>(&sz), sizeof(sz));
		val.resize(sz);
		is.read(val.data(), sz);
	}
	else
	{
		is.read(reinterpret_cast<char*>(&val), sizeof(val));
	}
}
} // namespace detail

template <typename T_Symbol, typename T_Compare>
void save_to_binary(const table<T_Symbol, T_Compare>& tbl, std::ostream& os)
{
	detail::write_bin(os, tbl.epsilon());
	detail::write_bin(os, tbl.end_marker());

	size_t total_entries = 0;
	for (const auto& [lhs, row] : tbl.entries())
	{
		total_entries += row.size();
	}

	detail::write_bin(os, total_entries);

	for (const auto& [lhs, row] : tbl.entries())
	{
		for (const auto& [term, rule] : row)
		{
			detail::write_bin(os, lhs);
			detail::write_bin(os, term);

			size_t rhs_size = rule.rhs.size();
			detail::write_bin(os, rhs_size);
			for (const auto& sym : rule.rhs)
			{
				detail::write_bin(os, sym);
			}
		}
	}
}

template <typename T_Symbol, typename T_Compare = std::less<T_Symbol>>
table<T_Symbol, T_Compare> load_from_binary(std::istream& is)
{
	table<T_Symbol, T_Compare> tbl;
	T_Symbol eps, eof;

	detail::read_bin(is, eps);
	detail::read_bin(is, eof);
	tbl.set_epsilon(eps).set_end_marker(eof);

	size_t total_entries = 0;
	detail::read_bin(is, total_entries);

	for (size_t i = 0; i < total_entries; ++i)
	{
		T_Symbol lhs, term;
		detail::read_bin(is, lhs);
		detail::read_bin(is, term);

		size_t rhs_size = 0;
		detail::read_bin(is, rhs_size);

		cfg_rule<T_Symbol> rule{ lhs, {} };
		rule.rhs.resize(rhs_size);

		for (size_t j = 0; j < rhs_size; ++j)
		{
			detail::read_bin(is, rule.rhs[j]);
		}

		tbl.add_entry(lhs, term, rule);
	}

	return tbl;
}
} // namespace fsm::ll1::io

#endif // FSM_LL1_TABLE_IO_HPP
