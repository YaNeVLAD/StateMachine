#ifndef FSM_SYMBOL_FORMATTER_HPP
#define FSM_SYMBOL_FORMATTER_HPP

#include "concepts.hpp"

#include <format>
#include <sstream>
#include <string>

namespace fsm
{
template <typename T>
struct symbol_formatter
{
	std::string operator()(const T& value) const
	{
		if constexpr (std::formattable<T, char>)
		{
			return std::format("{}", value);
		}
		else if constexpr (concepts::streamable<T>)
		{
			std::ostringstream oss;
			oss << value;
			return oss.str();
		}
		else if constexpr (std::constructible_from<std::string, T>)
		{
			return static_cast<std::string>(value);
		}
		else
		{
			return "<unprintable_symbol>";
		}
	}
};
} // namespace fsm

#endif // FSM_SYMBOL_FORMATTER_HPP
