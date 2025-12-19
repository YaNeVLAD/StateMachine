#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <stdexcept>
#include <vector>

#include "lexer.hpp"

#include "Lang.hpp"

namespace parser
{
using Token = fsm::token<TokenType>;

class Parser
{
public:
	explicit Parser(std::vector<Token> tokens)
		: m_tokens(std::move(tokens))
		, m_pos(0)
	{
	}

	void parse()
	{
		if (m_tokens.empty())
		{
			return;
		}
		parse_program();
	}

private:
	std::vector<Token> m_tokens;
	size_t m_pos;

	[[nodiscard]] const Token& peek() const
	{
		if (m_pos >= m_tokens.size())
		{
			throw std::runtime_error("Unexpected end of file");
		}
		return m_tokens[m_pos];
	}

	bool match(const TokenType type)
	{
		if (m_pos < m_tokens.size() && m_tokens[m_pos].type == type)
		{
			m_pos++;
			return true;
		}
		return false;
	}

	const Token& consume(const TokenType type, const std::string& err_msg)
	{
		if (match(type))
		{
			return m_tokens[m_pos - 1];
		}
		throw std::runtime_error("Syntax Error at line " + std::to_string(peek().line) + ": " + err_msg + ". Found: " + std::string(peek().lexeme));
	}

	void parse_program()
	{
		consume(TokenType::KwMain, "Expected 'main'");
		parse_body();
		consume(TokenType::KwEnd, "Expected 'end'");
		consume(TokenType::Dot, "Expected '.' after end");
		std::cout << "Program parsed successfully!" << std::endl;
	}

	void parse_body()
	{
		parse_defines();

		consume(TokenType::KwBegin, "Expected 'begin'");
		parse_statements();
	}

	void parse_defines()
	{
		while (peek().type != TokenType::KwBegin && peek().type != TokenType::KwEnd)
		{
			if (peek().type == TokenType::KwVar)
			{
				parse_var();
			}
			else if (peek().type == TokenType::Identifier)
			{
				parse_consts();
			}
			else
			{
				throw std::runtime_error("Expected variable or constant definition");
			}

			consume(TokenType::Semicolon, "Expected ';' after definition");
		}
	}

	void parse_var()
	{
		consume(TokenType::KwVar, "Expected 'var'");
		parse_id_list();
		consume(TokenType::Colon, "Expected ':'");
		parse_type();
	}

	void parse_id_list()
	{
		consume(TokenType::Identifier, "Expected identifier");
		while (match(TokenType::Comma))
		{
			consume(TokenType::Identifier, "Expected identifier after ','");
		}
	}

	void parse_type()
	{
		if (match(TokenType::KwInt) || match(TokenType::KwFloat))
		{
			return;
		}
		throw std::runtime_error("Expected type (int or float)");
	}

	void parse_consts()
	{
		parse_const();
	}

	void parse_const()
	{
		consume(TokenType::Identifier, "Expected identifier for constant");
		consume(TokenType::Assign, "Expected '='");
		parse_expression();
	}

	void parse_statements()
	{
		while (peek().type != TokenType::KwEnd)
		{
			parse_statement();
			if (peek().type == TokenType::Semicolon)
			{
				consume(TokenType::Semicolon, "Expected ';'");
			}
			else
			{
				if (peek().type != TokenType::KwEnd)
				{
					consume(TokenType::Semicolon, "Expected ';'");
				}
			}
		}
	}

	void parse_statement()
	{
		consume(TokenType::Identifier, "Expected identifier in assignment");
		consume(TokenType::Assign, "Expected '='");
		parse_expression();
	}

	void parse_expression()
	{
		parse_T();
		while (match(TokenType::Plus))
		{
			parse_T();
		}
	}

	void parse_T()
	{
		parse_F();
		while (match(TokenType::Star))
		{
			parse_F();
		}
	}

	void parse_F()
	{
		if (match(TokenType::Minus))
		{
			parse_F();
		}
		else if (match(TokenType::LParen))
		{
			parse_expression();
			consume(TokenType::RParen, "Expected ')'");
		}
		else if (match(TokenType::Identifier))
		{
		}
		else if (match(TokenType::Number))
		{
		}
		else
		{
			throw std::runtime_error("Unexpected token in expression: " + std::string(peek().lexeme));
		}
	}
};
} // namespace parser

#endif // PARSER_HPP