#ifndef REGEX_HPP
#define REGEX_HPP

#include <string>

#include <recognizer.hpp>

namespace fsm
{
class base_regex
{
public:
	virtual ~base_regex() = default;

	explicit base_regex(std::string const& expr)
	{
		const std::string processed = preprocess(expr);
		const std::string postfix = infix_to_postfix(processed);

		m_state = construct_recognizer(postfix);
	}

	[[nodiscard]] recognizer recognizer() const
	{
		return fsm::recognizer{ m_state };
	}

private:
	recognizer_state m_state;
	size_t m_state_counter{};

	std::string new_state_name()
	{
		return "q" + std::to_string(m_state_counter++);
	}

	static bool is_operand(const char ch)
	{
		return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
	}

	// a(b|c)* -> a.(b|c)*
	static std::string preprocess(const std::string& infix)
	{
		std::string result;
		for (size_t i = 0; i < infix.length(); ++i)
		{
			result += infix[i];

			// Insert '.' if:
			// ab -> a.b
			// )a ->).a
			// *a -> *.a
			// a( -> a.(
			// ) ->)(->).(
			// ) -> *(-> *.(

			if (i + 1 < infix.length())
			{
				const char current = infix[i];
				const char next = infix[i + 1];

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

	recognizer_state construct_recognizer(const std::string& postfix)
	{
		std::stack<recognizer_state> nfa_stack;

		for (char c : postfix)
		{
			if (is_operand(c))
			{
				std::optional<std::string> term = (c == 'e')
					? std::nullopt
					: std::optional(std::string(1, c));
				nfa_stack.push(create_base_nfa(term));
			}
			else if (c == '*')
			{
				recognizer_state a = nfa_stack.top();
				nfa_stack.pop();
				nfa_stack.push(op_kleene_star(a));
			}
			else if (c == '.')
			{
				recognizer_state b = nfa_stack.top();
				nfa_stack.pop();
				recognizer_state a = nfa_stack.top();
				nfa_stack.pop();
				nfa_stack.push(op_concatenate(a, b));
			}
			else if (c == '|')
			{
				recognizer_state b = nfa_stack.top();
				nfa_stack.pop();
				recognizer_state a = nfa_stack.top();
				nfa_stack.pop();
				nfa_stack.push(op_alternate(a, b));
			}
		}

		if (nfa_stack.size() != 1)
		{
			throw std::runtime_error("Invalid regex expression parsing failed.");
		}

		return nfa_stack.top();
	}

	recognizer_state create_base_nfa(std::optional<std::string> term)
	{
		recognizer_state nfa;
		std::string start = new_state_name();
		std::string final = new_state_name();

		nfa.state_ids = { start, final };
		nfa.initial_state_id = start;
		nfa.final_state_ids = { final };
		nfa.transitions.emplace(std::make_pair(start, term), final);
		nfa.is_deterministic = false;

		return nfa;
	}

	recognizer_state op_alternate(const recognizer_state& a, const recognizer_state& b)
	{
		recognizer_state nfa;
		std::string start = new_state_name();
		std::string final = new_state_name();

		// 1. Копируем все состояния и переходы
		nfa.state_ids = a.state_ids;
		nfa.state_ids.insert(b.state_ids.begin(), b.state_ids.end());
		nfa.transitions = a.transitions;
		nfa.transitions.insert(b.transitions.begin(), b.transitions.end());

		// 2. Добавляем новые start/final состояния
		nfa.state_ids.insert(start);
		nfa.state_ids.insert(final);
		nfa.initial_state_id = start;
		nfa.final_state_ids = { final };

		// 3. Новые эпсилон-переходы
		// От new_start к старым start'ам
		nfa.transitions.emplace(std::make_pair(start, std::nullopt), a.initial_state_id);
		nfa.transitions.emplace(std::make_pair(start, std::nullopt), b.initial_state_id);

		// От старых final'ов к new_final
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

	static recognizer_state op_concatenate(const recognizer_state& a, const recognizer_state& b)
	{
		recognizer_state nfa;

		// 1. Копируем все состояния и переходы
		nfa.state_ids = a.state_ids;
		nfa.state_ids.insert(b.state_ids.begin(), b.state_ids.end());
		nfa.transitions = a.transitions;
		nfa.transitions.insert(b.transitions.begin(), b.transitions.end());

		// 2. Start - от A, Final - от B
		nfa.initial_state_id = a.initial_state_id;
		nfa.final_state_ids = b.final_state_ids;

		// 3. Связываем: все final-состояния A -> initial-состояние B
		for (const auto& f : a.final_state_ids)
		{
			nfa.transitions.emplace(std::make_pair(f, std::nullopt), b.initial_state_id);
		}

		nfa.is_deterministic = false;
		return nfa;
	}

	recognizer_state op_kleene_star(const recognizer_state& a)
	{
		recognizer_state nfa;
		std::string start = new_state_name();
		std::string final = new_state_name();

		// 1. Копируем состояния и переходы
		nfa.state_ids = a.state_ids;
		nfa.transitions = a.transitions;

		// 2. Добавляем новые start/final
		nfa.state_ids.insert(start);
		nfa.state_ids.insert(final);
		nfa.initial_state_id = start;
		nfa.final_state_ids = { final };

		// 3. Новые эпсилон-переходы
		// а) Пропустить (0 раз)
		nfa.transitions.emplace(std::make_pair(start, std::nullopt), final);
		// б) Войти в автомат
		nfa.transitions.emplace(std::make_pair(start, std::nullopt), a.initial_state_id);

		// в) Выйти из автомата
		// г) Вернуться в начало (цикл)
		for (const auto& f : a.final_state_ids)
		{
			nfa.transitions.emplace(std::make_pair(f, std::nullopt), final);
			nfa.transitions.emplace(std::make_pair(f, std::nullopt), a.initial_state_id);
		}

		nfa.is_deterministic = false;
		return nfa;
	}
};
} // namespace fsm

#endif // REGEX_HPP
