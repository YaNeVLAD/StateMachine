#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <stdexcept>

#include "lexer.hpp"

#include "Lang.hpp"

// lexer вызывать по одному токену
// Ограничить длину идентификатора и значения чисел
namespace parser
{
using TokenType = my_lang::TokenType;
using Token = fsm::token<TokenType>;
using Lexer = fsm::lexer<TokenType>;

class Parser
{
public:
	explicit Parser(Lexer& lexer)
		: m_lexer(lexer)
	{
	}

	void parse()
	{
		if (!m_lexer.peek().has_value())
		{
			return;
		}

		parse_program();
	}

private:
	Lexer& m_lexer;

	[[nodiscard]] std::optional<Token> peek() const
	{
		return m_lexer.peek();
	}

	[[nodiscard]] bool match(const TokenType type) const
	{
		if (const auto token = peek(); token && token->type == type)
		{
			m_lexer.next();

			return true;
		}

		return false;
	}

	void consume(const TokenType type, const std::string& err_msg) const
	{
		const auto token = peek();
		if (token && token->type == type)
		{
			m_lexer.next();
		}

		const std::string found = token ? std::string(token->lexeme) : "EOF";
		const size_t line = token ? token->line : 0;

		throw std::runtime_error("Syntax Error at line " + std::to_string(line) + ": " + err_msg + ". Found: " + found);
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
		while (true)
		{
			auto t = peek();
			if (!t.has_value())
			{
				break;
			}

			if (t->type == TokenType::KwVar)
			{
				parse_var();
				consume(TokenType::Semicolon, "Expected ';' after var definition");
			}
			else if (t->type == TokenType::Identifier)
			{
				parse_consts();
				consume(TokenType::Semicolon, "Expected ';' after const definition");
			}
			else
			{
				break;
			}
		}
	}

	void parse_var() const
	{
		consume(TokenType::KwVar, "Expected 'var'");
		parse_id_list();
		consume(TokenType::Colon, "Expected ':'");
		parse_type();
	}

	void parse_id_list() const
	{
		consume(TokenType::Identifier, "Expected identifier");
		while (match(TokenType::Comma))
		{
			consume(TokenType::Identifier, "Expected identifier after ','");
		}
	}

	void parse_type() const
	{
		if (match(TokenType::KwInt) || match(TokenType::KwFloat))
		{
			return;
		}
		throw std::runtime_error("Expected type (int or float)");
	}

	void parse_consts()
	{
		consume(TokenType::Identifier, "Expected identifier for constant");
		consume(TokenType::Assign, "Expected '='");
		parse_expression();
	}

	void parse_statements()
	{
		while (true)
		{
			auto t = peek();
			if (!t.has_value())
			{
				throw std::runtime_error("Unexpected EOF inside statements");
			}
			if (t->type == TokenType::KwEnd)
			{
				break;
			}

			parse_statement();

			if (peek()->type == TokenType::Semicolon)
			{
				consume(TokenType::Semicolon, "Expected ';'");
			}
			else
			{
				if (peek()->type != TokenType::KwEnd)
				{
					throw std::runtime_error("Expected ';' between statements");
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
			const auto t = peek();
			const std::string val = t ? std::string(t->lexeme) : "EOF";
			throw std::runtime_error("Unexpected token in expression: " + val);
		}
	}
};
} // namespace parser

#endif // PARSER_HPP