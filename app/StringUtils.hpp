#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <regex>
#include <string>

inline std::string trim(std::string const& str)
{
	return std::regex_replace(str, std::regex("^ +| +$|( ) +"), "$1");
}

inline std::string unquote(std::string str)
{
	if (str.length() >= 2 && str.front() == '"' && str.back() == '"')
	{
		return str.substr(1, str.length() - 2);
	}
	return str;
}

#endif // STRING_UTILS_HPP
