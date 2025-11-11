#ifndef REGEX_HPP
#define REGEX_HPP

#include <string>

#include <recognizer.hpp>

namespace fsm
{
namespace details
{
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

	struct ast
	{
		std::variant<
			symbol,
			std::unique_ptr<alteration>,
			std::unique_ptr<concatenation>,
			std::unique_ptr<kleene_star>>
			node;

		template <typename T_Node, typename... T_Args>
		static std::unique_ptr<ast> make_unique(T_Args&&... args)
		{
			return std::make_unique<ast>(std::make_unique<T_Node>(std::forward<T_Args>(args)...));
		}
	};

	using output_type = ast;

	[[nodiscard]]
	static ast operator()(std::string const& infix)
	{
		const std::string processed = preprocess(infix);
		const std::string postfix = infix_to_postfix(processed);

		std::stack<std::unique_ptr<ast>> stack;

		for (const char ch : postfix)
		{
			if (is_operand(ch))
			{
				const auto term = (ch == 'e')
					? std::nullopt
					: std::optional{ std::string(1, ch) };

				stack.emplace(std::make_unique<ast>(symbol{ term }));
			}
			else if (ch == '*')
			{
				auto child = std::move(stack.top());
				stack.pop();

				stack.emplace(ast::make_unique<kleene_star>(std::move(child)));
			}
			else if (ch == '.')
			{
				auto rhs = std::move(stack.top());
				stack.pop();
				auto lhs = std::move(stack.top());
				stack.pop();

				stack.emplace(ast::make_unique<concatenation>(std::move(lhs), std::move(rhs)));
			}
			else if (ch == '|')
			{
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
	static bool is_operand(const char ch)
	{
		return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
	}

	static std::string preprocess(const std::string& infix)
	{
		std::string result;
		for (size_t i = 0; i < infix.length(); ++i)
		{
			result += infix[i];
			if (i + 1 < infix.length())
			{
				const char current = infix[i];
				const char next = infix[i + 1];

				// Insert '.' if:
				// ab -> a.b
				// )a ->).a
				// *a -> *.a
				// a( -> a.(
				// ) ->)(->).(
				// ) -> *(-> *.(

				if ((is_operand(current) || current == ')' || current == '*')
					&& (is_operand(next) || next == '('))
				{
					result += '.';
				}
			}
		}

		return result;
	}

	static std::string infix_to_postfix(const std::string& infix)
	{
		std::map<char, unsigned> precedence = { { '|', 1 }, { '.', 2 }, { '*', 3 } };
		std::string postfix;
		std::stack<char> op_stack;

		for (char ch : infix)
		{
			if (is_operand(ch))
			{
				postfix += ch;
			}
			else if (ch == '(')
			{
				op_stack.push(ch);
			}
			else if (ch == ')')
			{
				while (!op_stack.empty() && op_stack.top() != '(')
				{
					postfix += op_stack.top();
					op_stack.pop();
				}
				op_stack.pop();
			}
			else
			{
				while (!op_stack.empty() && op_stack.top() != '('
					&& precedence[op_stack.top()] >= precedence[ch])
				{
					postfix += op_stack.top();
					op_stack.pop();
				}
				op_stack.push(ch);
			}
		}

		while (!op_stack.empty())
		{
			postfix += op_stack.top();
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

	recognizer::state_type visit_child(std::unique_ptr<regex_parser::ast> const& child_node)
	{
		return std::visit(*this, child_node->node);
	}

	size_t m_state_counter = 0;
};
} // namespace details

template <typename T_Parser, typename T_Builder>
class base_regex
	: T_Parser
	, T_Builder
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
