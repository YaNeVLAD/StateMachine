#include "lang/Lang.hpp"
#include "lang/Parser.hpp"
#include "lexer.hpp"

#include <fsm.hpp>

#include <regex.hpp>
#include <regular_grammar.hpp>

#include <iostream>

#include "readers/MealyFromDot.hpp"
#include "readers/MooreFromDot.hpp"

template <typename TokenType, typename Mapper>
void LoadRulesFromFile(fsm::lexer<TokenType>& lexer, const std::string& filename, Mapper mapper)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open lexer rules file: " + filename);
	}

	std::string line;
	size_t line_num = 0;

	while (std::getline(file, line))
	{
		line_num++;
		std::string clean_line = trim(line);

		if (clean_line.empty() || clean_line[0] == '#')
		{
			continue;
		}

		std::stringstream ss(clean_line);
		std::string token_name_str;
		std::string regex_part;
		bool skip = false;

		ss >> token_name_str;

		if (token_name_str == "%skip")
		{
			skip = true;
			if (!(ss >> token_name_str))
			{
				throw std::runtime_error("Syntax error in rules file at line " + std::to_string(line_num) + ": expected token name after %skip");
			}
		}

		std::getline(ss, regex_part);
		regex_part = trim(regex_part);

		if (regex_part.empty())
		{
			throw std::runtime_error("Syntax error at line " + std::to_string(line_num) + ": empty regex for token " + token_name_str);
		}

		try
		{
			TokenType type = mapper(token_name_str);

			lexer.add_rule(regex_part, type, skip);
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("Error processing rule at line " + std::to_string(line_num) + ": " + e.what());
		}
	}
}

int main()
{
	using namespace std::literals;

	try
	{
		std::ifstream src("res/lang_src.txt");
		std::stringstream buffer;

		buffer << src.rdbuf();
		std::string source_code = buffer.str();

		fsm::lexer<TokenType> lexer(source_code);
		LoadRulesFromFile(lexer, "res/lang_grammar.txt", Mapper());

		const auto tokens = lexer.tokenize();

		parser::Parser parser(tokens);
		parser.parse();
	}
	catch (std::exception const& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return 0;
}
