#ifndef DOT_HPP
#define DOT_HPP

#include <istream>
#include <optional>
#include <ostream>
#include <string>

namespace fsm
{
template <typename T>
T dot(std::istream&)
{
	static_assert(sizeof(T) == 0, "To use fsm::dot(istream&), "
								  "provide a template specialization for your type.");
	return T{};
}

template <typename T>
void dot(std::ostream&, T const&)
{
	static_assert(sizeof(T) == 0, "To use fsm::dot(ostream&, T), "
								  "provide a template specialization for your type.");
}

namespace details
{
static constexpr auto EDGE_REGEX{ "ABOBA" };
static constexpr auto NODE_REGEX{ "ABOBA" };

inline std::string unquote(std::string const& str)
{
	if (str.length() >= 2 && str.front() == '"' && str.back() == '"')
	{
		return str.substr(1, str.length() - 2);
	}
	return str;
}

inline std::string quote(std::string const& s)
{
	return "\"" + s + "\"";
}

template <typename T_From, typename T_To, typename T_Value>
std::ostream&
print_edge(std::ostream& os, T_From const& from, T_To const& to, std::optional<T_Value> const& value)
{
	os << "    " << from
	   << " -> " << to;

	if (value.has_value())
	{
		os << " [label = " << *value << "]";
	}

	os << ";\n";

	return os;
}

template <typename T_State, typename... T_Args>
std::ostream& print_node(std::ostream& os, T_State const& state_id, T_Args&&... args)
{
	os << "    " << state_id;

	if (sizeof...(T_Args) > 0)
	{
		os << " [";

		std::size_t n{ 0 };
		((os << args << (++n != sizeof...(T_Args) ? " " : "")), ...);

		os << "]";
	}

	os << ";\n";

	return os;
}
} // namespace details
} // namespace fsm

#endif // DOT_HPP
