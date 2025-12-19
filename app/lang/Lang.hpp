#ifndef C_LEXER_HPP
#define C_LEXER_HPP

#include <map>
#include <stdexcept>
#include <string>

enum class TokenType
{
	// Ключевые слова
	KwMain, // main
	KwBegin, // begin
	KwEnd, // end
	KwVar, // var
	KwInt, // int
	KwFloat, // float

	// Идентификаторы и числа
	Identifier,
	Number,

	// Операторы и пунктуация
	Assign, // =
	Plus, // +
	Minus, // -
	Star, // *
	Slash, // /

	Dot, // .
	Comma, // ,
	Colon, // :
	Semicolon, // ;
	LParen, // (
	RParen, // )

	Whitespace,
	Unknown
};

inline std::string to_string(TokenType t)
{
	switch (t)
	{
	case TokenType::KwMain:
		return "main";
	case TokenType::KwBegin:
		return "begin";
	case TokenType::KwEnd:
		return "end";
	case TokenType::KwVar:
		return "var";
	case TokenType::KwInt:
		return "int";
	case TokenType::KwFloat:
		return "float";
	case TokenType::Identifier:
		return "id";
	case TokenType::Number:
		return "number";
	case TokenType::Assign:
		return "=";
	case TokenType::Plus:
		return "+";
	case TokenType::Minus:
		return "-";
	case TokenType::Star:
		return "*";
	case TokenType::Slash:
		return "/";
	case TokenType::Dot:
		return ".";
	case TokenType::Comma:
		return ",";
	case TokenType::Colon:
		return ":";
	case TokenType::Semicolon:
		return ";";
	case TokenType::LParen:
		return "(";
	case TokenType::RParen:
		return ")";
	default:
		return "unknown";
	}
}

inline auto Mapper()
{
	return [](const std::string& name) -> TokenType {
		static const std::map<std::string, TokenType> map = {
			{ "KW_MAIN", TokenType::KwMain }, { "KW_BEGIN", TokenType::KwBegin },
			{ "KW_END", TokenType::KwEnd }, { "KW_VAR", TokenType::KwVar },
			{ "KW_INT", TokenType::KwInt }, { "KW_FLOAT", TokenType::KwFloat },
			{ "ASSIGN", TokenType::Assign }, { "PLUS", TokenType::Plus },
			{ "MINUS", TokenType::Minus }, { "STAR", TokenType::Star },
			{ "SLASH", TokenType::Slash }, { "DOT", TokenType::Dot },
			{ "COMMA", TokenType::Comma }, { "COLON", TokenType::Colon },
			{ "SEMICOLON", TokenType::Semicolon }, { "LPAREN", TokenType::LParen },
			{ "RPAREN", TokenType::RParen }, { "NUMBER", TokenType::Number },
			{ "IDENTIFIER", TokenType::Identifier }, { "SPACE", TokenType::Whitespace }
		};
		if (map.contains(name))
		{
			return map.at(name);
		}
		throw std::runtime_error("Unknown token: " + name);
	};
}

#endif // C_LEXER_HPP
