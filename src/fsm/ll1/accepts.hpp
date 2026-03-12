#ifndef FSM_LL1_ACCEPTS_HPP
#define FSM_LL1_ACCEPTS_HPP

#include "../symbol_formatter.hpp"
#include "table.hpp"

#include <iomanip>

namespace fsm::ll1
{

template <typename T_Symbol, typename T_Compare, typename F = symbol_formatter<T_Symbol>>
bool accepts(
	const table<T_Symbol, T_Compare>& tbl,
	const T_Symbol& start_symbol,
	const T_Symbol& epsilon_symbol,
	const T_Symbol& eof_marker,
	const std::vector<T_Symbol>& input_sequence,
	std::ostream* err_out = nullptr,
	std::ostream* trace_out = nullptr,
	F formatter = {})
{
	std::vector<T_Symbol> st;
	st.push_back(eof_marker);
	st.push_back(start_symbol);

	size_t input_ptr = 0;
	size_t step = 1;

	auto epsilon = tbl.epsilon();
	if (trace_out)
	{
		*trace_out << "\n--- LL(1) Parsing Trace ---\n"
				   << std::left
				   << std::setw(5) << "Step" << " | "
				   << std::setw(30) << "Stack (Left to right)" << " | "
				   << std::setw(30) << "Input" << " | "
				   << "Action\n"
				   << std::string(90, '-') << "\n";
	}

	while (!st.empty())
	{
		T_Symbol top = st.back();

		T_Symbol current_token = (input_ptr < input_sequence.size())
			? input_sequence[input_ptr]
			: eof_marker;

		if (trace_out)
		{
			std::string stack_str, input_str;

			for (auto it = st.rbegin(); it != st.rend(); ++it)
			{
				stack_str += formatter(*it) + " ";
			}
			if (stack_str.length() > 28)
			{
				stack_str = stack_str.substr(0, 25) + "...";
			}

			for (std::size_t i = input_ptr; i < input_sequence.size(); ++i)
			{
				input_str += formatter(input_sequence[i]) + " ";
			}
			if (input_ptr >= input_sequence.size())
			{
				input_str += formatter(eof_marker);
			}
			if (input_str.length() > 28)
			{
				input_str = input_str.substr(0, 25) + "...";
			}

			*trace_out << std::left
					   << std::setw(5) << step++ << " | "
					   << std::setw(30) << stack_str << " | "
					   << std::setw(30) << input_str << " | ";
		}

		if (top == epsilon_symbol)
		{
			st.pop_back();
			if (trace_out)
			{
				*trace_out << "Removing "
						   << formatter(epsilon_symbol) << "\n";
			}
			continue;
		}
		if (!tbl.entries().contains(top))
		{
			if (top == current_token)
			{
				st.pop_back();
				if (current_token != eof_marker || input_ptr < input_sequence.size())
				{
					input_ptr++;
				}
				if (trace_out)
				{
					*trace_out << "Match (Moving forward): '" << formatter(current_token) << "'\n";
				}
			}
			else
			{
				if (trace_out)
				{
					*trace_out << "ERROR: Terminals don't match\n";
				}
				if (err_out)
				{
					*err_out << "[Syntax error] Position " << input_ptr
							 << ". Expected: '" << formatter(top)
							 << "', Found: '" << formatter(current_token) << "'.\n";
				}

				return false;
			}
		}
		else
		{
			if (tbl.has_rule(top, current_token))
			{
				const auto& rule = tbl.at(top, current_token);
				st.pop_back();

				for (auto it = rule.rhs.rbegin(); it != rule.rhs.rend(); ++it)
				{
					if (*it != epsilon_symbol)
					{
						st.push_back(*it);
					}
				}

				if (trace_out)
				{
					*trace_out << "Rule: " << formatter(rule.lhs) << " -> ";
					if (rule.rhs.empty())
					{
						*trace_out << formatter(epsilon_symbol);
					}
					else
					{
						for (size_t i = 0; i < rule.rhs.size(); ++i)
						{
							*trace_out << formatter(rule.rhs[i]) << (i + 1 < rule.rhs.size() ? " " : "");
						}
					}
					*trace_out << "\n";
				}
			}
			else
			{
				if (trace_out)
				{
					*trace_out << "ERROR: No rule in table\n";
				}
				if (err_out)
				{
					*err_out << "[Syntax error] Position " << input_ptr
							 << " (Processing '" << formatter(top) << "'). "
							 << "Expected on of: { ";

					if (tbl.entries().contains(top))
					{
						bool first = true;
						for (const auto& [term, _] : tbl.entries().at(top))
						{
							if (!first)
							{
								*err_out << ", ";
							}
							*err_out << "'" << formatter(term) << "'";
							first = false;
						}
					}
					*err_out << " }, Found: '" << formatter(current_token) << "'.\n";
				}

				return false;
			}
		}
	}

	const bool success = input_ptr >= input_sequence.size()
		|| (input_ptr == input_sequence.size() - 1 && input_sequence.back() == eof_marker);
	if (!success && err_out)
	{
		*err_out << "[Syntax error] Stack is empty but input is not finished. "
				 << "Extra token on position " << input_ptr << ": '"
				 << formatter(input_sequence[input_ptr]) << "'.\n";
	}

	if (trace_out)
	{
		if (success)
		{
			*trace_out << "--- Processing finished ---\n\n";
		}
		else
		{
			*trace_out << "--- Processing failed ---\n\n";
		}
	}

	return success;
}

} // namespace fsm::ll1

#endif // FSM_LL1_ACCEPTS_HPP
