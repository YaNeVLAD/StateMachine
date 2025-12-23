#ifndef LEXER_HPP
#define LEXER_HPP

#include <string_view>

#include "minimization.hpp"
#include "recognizer.hpp"
#include "regex.hpp"

namespace fsm
{
template <typename T_Type>
struct token
{
	T_Type type;
	std::string_view lexeme;

	size_t line{};
	size_t column{};
	size_t offset{};
};

template <typename T_TokenType>
class lexer
{
public:
	using token_type = token<T_TokenType>;

	struct rule
	{
		T_TokenType type;
		fsm::recognizer machine;

		bool skip{};
		size_t priority{};
	};

	explicit lexer(std::string_view source)
		: lexer(source, {})
	{
	}

	lexer(const std::string_view source, std::vector<rule> const& rules)
		: m_source{ source }
		, m_rules{ rules }
	{
	}

	lexer& add_rule(
		std::string const& expression,
		T_TokenType type,
		const bool skip = false)
	{
		fsm::regex regex(expression);
		auto nfa = regex.compile();
		auto min_dfa = fsm::minimize(fsm::determinize(nfa));

		m_rules.emplace_back(type, std::move(min_dfa), skip, m_rules.size());

		m_peek_buffer.reset();

		return *this;
	}

	lexer& change_source(const std::string_view source, const bool with_clear = true)
	{
		m_source = source;

		m_cursor = 0;
		m_line = 1;
		m_column = 1;

		m_peek_buffer.reset();

		if (with_clear)
		{
			m_rules.clear();
		}

		return *this;
	}

	std::optional<token_type> peek()
	{
		if (m_peek_buffer.has_value())
		{
			m_peek_buffer = read_next_token();
		}

		return m_peek_buffer;
	}

	std::optional<token_type> next()
	{
		if (m_peek_buffer)
		{
			auto token = m_peek_buffer;
			m_peek_buffer.reset();

			return token;
		}

		return read_next_token();
	}

	std::vector<token_type> tokenize()
	{
		std::vector<token_type> tokens;
		while (auto token = next())
		{
			tokens.emplace_back(*token);
		}

		return tokens;
	}

private:
	struct match_result
	{
		rule const* matched_rule;

		size_t length{};
	};

	std::string_view m_source;
	std::vector<rule> m_rules;

	size_t m_cursor = 0;
	size_t m_line = 1;
	size_t m_column = 1;

	std::optional<token_type> m_peek_buffer{};

	std::optional<token_type> read_next_token()
	{
		while (m_cursor < m_source.length())
		{
			auto match = find_longest_match();

			if (!match)
			{
				std::string error = "Unexpected character '";
				error += m_source[m_cursor];
				error += std::format("' at line {}, column {}", m_line, m_column);
				throw std::runtime_error(error);
			}

			size_t start_line = m_line;
			size_t start_col = m_column;
			size_t start_offset = m_cursor;

			advance_cursor(match->length);

			if (match->matched_rule->skip)
			{
				continue;
			}

			return token_type{
				match->matched_rule->type,
				m_source.substr(start_offset, match->length),
				start_line,
				start_col,
				start_offset
			};
		}

		return std::nullopt;
	}

	std::optional<match_result> find_longest_match()
	{
		const rule* best_rule = nullptr;
		size_t max_len = 0;

		// Перебираем все правила (в идеале это можно оптимизировать через объединенный автомат)
		for (const auto& rule : m_rules)
		{
			size_t current_len = 0;
			size_t last_final_len = 0;
			bool reached_dead_end = false;

			// Создаем копию машины для симуляции прохода
			// (Важно: recognizer должен быть cheap-to-copy или использовать COW,
			// иначе здесь будет просадка по performance. В рамках текущей реализации копируем)
			fsm::recognizer machine_run = rule.machine;

			for (size_t i = m_cursor; i < m_source.length(); ++i)
			{
				const char input_char = m_source[i];
				std::string input_str(1, input_char);

				try
				{
					const bool is_final_now = machine_run.handle_input(std::optional{ input_str });
					current_len++;

					if (is_final_now)
					{
						last_final_len = current_len;
					}
				}
				catch (...)
				{
					reached_dead_end = true;
					break;
				}
			}

			if (last_final_len > max_len)
			{
				max_len = last_final_len;
				best_rule = &rule;
			}
		}

		if (max_len > 0 && best_rule != nullptr)
		{
			return match_result{ best_rule, max_len };
		}

		return std::nullopt;
	}

	void advance_cursor(const size_t length)
	{
		for (size_t i = 0; i < length; ++i)
		{
			if (m_source[m_cursor] == '\n')
			{
				++m_line;
				m_column = 1;
			}
			else
			{
				++m_column;
			}
			++m_cursor;
		}
	}
};

} // namespace fsm

#endif // LEXER_HPP
