#ifndef REGEX_HPP
#define REGEX_HPP

#include <map>
#include <memory>
#include <optional>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include <recognizer.hpp>

namespace fsm
{
namespace details
{
enum class token_type
{
	literal,
	etranslate, // e
	lparen, // (
	rparen, // )
	star, // *
	plus, // +
	pipe, // |
	concat // .
};

struct token
{
	token_type type;
	char value;
};

class regex_parser
{
public:
	struct ast;

	struct symbol
	{
		std::optional<std::string> term;
	};

	struct alteration // |
	{
		std::unique_ptr<ast> lhs;
		std::unique_ptr<ast> rhs;
	};

	struct concatenation // .
	{
		std::unique_ptr<ast> lhs;
		std::unique_ptr<ast> rhs;
	};

	struct kleene_star // *
	{
		std::unique_ptr<ast> child;
	};

	struct kleene_plus // +
	{
		std::unique_ptr<ast> child;
	};

	struct ast
	{
		std::variant<
			symbol,
			std::unique_ptr<alteration>,
			std::unique_ptr<concatenation>,
			std::unique_ptr<kleene_star>,
			std::unique_ptr<kleene_plus>>
			node;

		template <typename T_Node, typename... T_Args>
		static std::unique_ptr<ast> make_unique(T_Args&&... args)
		{
			return std::make_unique<ast>(std::make_unique<T_Node>(std::forward<T_Args>(args)...));
		}
	};

	using output_type = ast;

	[[nodiscard]]
	static ast operator()(std::string const& regex)
	{
		const auto tokens = tokenize(regex);

		const auto processed_tokens = insert_concatenation(tokens);

		const auto postfix = infix_to_postfix(processed_tokens);

		std::stack<std::unique_ptr<ast>> stack;

		for (const auto& [type, value] : postfix)
		{
			if (type == token_type::literal)
			{
				stack.emplace(std::make_unique<ast>(symbol{ std::string(1, value) }));
			}
			else if (type == token_type::etranslate)
			{
				stack.emplace(std::make_unique<ast>(symbol{ std::nullopt }));
			}
			else if (type == token_type::plus)
			{
				if (stack.empty())
				{
					throw std::runtime_error("Parse error: unexpected +");
				}
				auto child = std::move(stack.top());
				stack.pop();
				stack.emplace(ast::make_unique<kleene_plus>(std::move(child)));
			}
			else if (type == token_type::star)
			{
				if (stack.empty())
				{
					throw std::runtime_error("Parse error: unexpected *");
				}
				auto child = std::move(stack.top());
				stack.pop();
				stack.emplace(ast::make_unique<kleene_star>(std::move(child)));
			}
			else if (type == token_type::concat)
			{
				if (stack.size() < 2)
				{
					throw std::runtime_error("Parse error: unexpected concatenation");
				}
				auto rhs = std::move(stack.top());
				stack.pop();
				auto lhs = std::move(stack.top());
				stack.pop();
				stack.emplace(ast::make_unique<concatenation>(std::move(lhs), std::move(rhs)));
			}
			else if (type == token_type::pipe)
			{
				if (stack.size() < 2)
				{
					throw std::runtime_error("Parse error: unexpected |");
				}
				auto rhs = std::move(stack.top());
				stack.pop();
				auto lhs = std::move(stack.top());
				stack.pop();
				stack.emplace(ast::make_unique<alteration>(std::move(lhs), std::move(rhs)));
			}
		}

		if (stack.size() != 1)
		{
			throw std::runtime_error("Invalid regex expression parsing failed.");
		}

		return std::move(*stack.top());
	}

private:
	static std::vector<token> tokenize(const std::string& input)
	{
		std::vector<token> tokens;
		for (size_t i = 0; i < input.length(); ++i)
		{
			if (const char ch = input[i]; ch == '\\')
			{
				if (i + 1 >= input.length())
				{
					throw std::runtime_error("Trailing backslash in regex");
				}
				const char next = input[++i];
				char literal_value = next;

				switch (next)
				{
				case 'n':
					literal_value = '\n';
					break;
				case 'r':
					literal_value = '\r';
					break;
				case 't':
					literal_value = '\t';
					break;
				case '0':
					literal_value = '0';
					break;
				default:
					literal_value = next;
					break;
				}

				tokens.push_back({ token_type::literal, literal_value });
			}
			else if (ch == '(')
			{
				tokens.push_back({ token_type::lparen, ch });
			}
			else if (ch == ')')
			{
				tokens.push_back({ token_type::rparen, ch });
			}
			else if (ch == '*')
			{
				tokens.push_back({ token_type::star, ch });
			}
			else if (ch == '+')
			{
				tokens.push_back({ token_type::plus, ch });
			}
			else if (ch == '|')
			{
				tokens.push_back({ token_type::pipe, ch });
			}
			else
			{
				tokens.push_back({ token_type::literal, ch });
			}
		}
		return tokens;
	}

	static std::vector<token> insert_concatenation(const std::vector<token>& tokens)
	{
		std::vector<token> result;
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			result.push_back(tokens[i]);

			if (i + 1 < tokens.size())
			{
				const auto& curr = tokens[i];
				const auto& next = tokens[i + 1];

				const bool curr_can_concat = (curr.type == token_type::literal
					|| curr.type == token_type::rparen
					|| curr.type == token_type::star
					|| curr.type == token_type::plus);

				const bool next_can_concat = (next.type == token_type::literal
					|| next.type == token_type::lparen);

				if (curr_can_concat && next_can_concat)
				{
					result.push_back({ token_type::concat, '.' });
				}
			}
		}
		return result;
	}

	static std::vector<token> infix_to_postfix(const std::vector<token>& infix)
	{
		std::map<token_type, unsigned> precedence = {
			{ token_type::pipe, 1 },
			{ token_type::concat, 2 },
			{ token_type::star, 3 },
			{ token_type::plus, 3 }
		};

		std::vector<token> postfix;
		std::stack<token> op_stack;

		for (const auto& tok : infix)
		{
			switch (tok.type)
			{
			case token_type::literal:
				postfix.push_back(tok);
				break;

			case token_type::lparen:
				op_stack.push(tok);
				break;

			case token_type::rparen:
				while (!op_stack.empty() && op_stack.top().type != token_type::lparen)
				{
					postfix.push_back(op_stack.top());
					op_stack.pop();
				}
				if (!op_stack.empty())
				{
					op_stack.pop();
				}
				break;

			default: // Operator
				while (!op_stack.empty()
					&& op_stack.top().type != token_type::lparen
					&& precedence[op_stack.top().type] >= precedence[tok.type])
				{
					postfix.push_back(op_stack.top());
					op_stack.pop();
				}
				op_stack.push(tok);
				break;
			}
		}

		while (!op_stack.empty())
		{
			postfix.push_back(op_stack.top());
			op_stack.pop();
		}

		return postfix;
	}
};

class regex_builder
{
public:
	recognizer::state_type operator()(regex_parser::output_type const& state)
	{
		return std::visit(*this, state.node);
	}

	recognizer::state_type operator()(regex_parser::symbol const& sym)
	{
		return create_base_nfa(sym.term);
	}

	recognizer::state_type operator()(std::unique_ptr<regex_parser::alteration> const& node)
	{
		return op_alternate(visit_child(node->lhs), visit_child(node->rhs));
	}

	recognizer::state_type operator()(std::unique_ptr<regex_parser::concatenation> const& node)
	{
		return op_concatenate(visit_child(node->lhs), visit_child(node->rhs));
	}

	recognizer::state_type operator()(std::unique_ptr<regex_parser::kleene_star> const& node)
	{
		return op_kleene_star(visit_child(node->child));
	}

	recognizer::state_type operator()(std::unique_ptr<regex_parser::kleene_plus> const& node)
	{
		return op_kleene_plus(visit_child(node->child));
	}

private:
	std::string new_state_name()
	{
		return "q" + std::to_string(m_state_counter++);
	}

	recognizer::state_type create_base_nfa(std::optional<std::string> const& term)
	{
		recognizer::state_type nfa;
		std::string start = new_state_name();
		std::string final = new_state_name();

		nfa.state_ids = { start, final };
		nfa.initial_state_id = start;
		nfa.final_state_ids = { final };
		nfa.transitions.emplace(std::make_pair(start, term), final);
		nfa.is_deterministic = false;

		return nfa;
	}

	recognizer::state_type op_alternate(const recognizer::state_type& a, const recognizer::state_type& b)
	{
		recognizer::state_type nfa;
		std::string start = new_state_name();
		std::string final = new_state_name();

		nfa.state_ids = a.state_ids;
		nfa.state_ids.insert(b.state_ids.begin(), b.state_ids.end());
		nfa.transitions = a.transitions;
		nfa.transitions.insert(b.transitions.begin(), b.transitions.end());

		nfa.state_ids.insert(start);
		nfa.state_ids.insert(final);
		nfa.initial_state_id = start;
		nfa.final_state_ids = { final };

		nfa.transitions.emplace(std::make_pair(start, std::nullopt), a.initial_state_id);
		nfa.transitions.emplace(std::make_pair(start, std::nullopt), b.initial_state_id);

		for (const auto& f : a.final_state_ids)
		{
			nfa.transitions.emplace(std::make_pair(f, std::nullopt), final);
		}
		for (const auto& f : b.final_state_ids)
		{
			nfa.transitions.emplace(std::make_pair(f, std::nullopt), final);
		}

		nfa.is_deterministic = false;
		return nfa;
	}

	static recognizer::state_type op_concatenate(const recognizer::state_type& a, const recognizer::state_type& b)
	{
		recognizer::state_type nfa;

		nfa.state_ids = a.state_ids;
		nfa.state_ids.insert(b.state_ids.begin(), b.state_ids.end());
		nfa.transitions = a.transitions;
		nfa.transitions.insert(b.transitions.begin(), b.transitions.end());

		nfa.initial_state_id = a.initial_state_id;
		nfa.final_state_ids = b.final_state_ids;

		for (const auto& f : a.final_state_ids)
		{
			nfa.transitions.emplace(std::make_pair(f, std::nullopt), b.initial_state_id);
		}

		nfa.is_deterministic = false;
		return nfa;
	}

	recognizer::state_type op_kleene_star(const recognizer::state_type& a)
	{
		recognizer::state_type nfa;
		std::string start = new_state_name();
		std::string final = new_state_name();

		nfa.state_ids = a.state_ids;
		nfa.transitions = a.transitions;

		nfa.state_ids.insert(start);
		nfa.state_ids.insert(final);
		nfa.initial_state_id = start;
		nfa.final_state_ids = { final };

		nfa.transitions.emplace(std::make_pair(start, std::nullopt), final);
		nfa.transitions.emplace(std::make_pair(start, std::nullopt), a.initial_state_id);

		for (const auto& f : a.final_state_ids)
		{
			nfa.transitions.emplace(std::make_pair(f, std::nullopt), final);
			nfa.transitions.emplace(std::make_pair(f, std::nullopt), a.initial_state_id);
		}

		nfa.is_deterministic = false;
		return nfa;
	}

	recognizer::state_type op_kleene_plus(const recognizer::state_type& a)
	{
		recognizer::state_type nfa;
		std::string start = new_state_name();
		std::string final = new_state_name();

		nfa.state_ids = a.state_ids;
		nfa.transitions = a.transitions;

		nfa.state_ids.insert(start);
		nfa.state_ids.insert(final);
		nfa.initial_state_id = start;
		nfa.final_state_ids = { final };

		nfa.transitions.emplace(std::make_pair(start, std::nullopt), a.initial_state_id);

		for (const auto& f : a.final_state_ids)
		{
			nfa.transitions.emplace(std::make_pair(f, std::nullopt), final);
			nfa.transitions.emplace(std::make_pair(f, std::nullopt), a.initial_state_id);
		}

		nfa.is_deterministic = false;
		return nfa;
	}

	recognizer::state_type visit_child(std::unique_ptr<regex_parser::ast> const& child_node)
	{
		return std::visit(*this, child_node->node);
	}

	size_t m_state_counter = 0;
};
} // namespace details

template <typename T_Parser, typename T_Builder>
class base_regex
{
public:
	explicit base_regex(std::string const& expr, const bool compile_immediately = true)
	{
		if (m_is_compiled = compile_immediately; compile_immediately)
		{
			try
			{
				m_parser_state = std::move(T_Parser{}(expr));
				m_recognizer_state = std::move(T_Builder{}(m_parser_state));
			}
			catch (...)
			{
				m_is_compiled = false;
				throw;
			}
		}
	}

	recognizer compile()
	{
		if (m_is_compiled)
		{
			return recognizer{ m_recognizer_state };
		}

		m_is_compiled = true;
		m_recognizer_state = std::move(T_Builder{}(m_parser_state));

		return recognizer{ m_recognizer_state };
	}

private:
	bool m_is_compiled = false;

	typename T_Parser::output_type m_parser_state;
	recognizer::state_type m_recognizer_state{};
};

using regex = base_regex<details::regex_parser, details::regex_builder>;
} // namespace fsm

#endif // REGEX_HPP
