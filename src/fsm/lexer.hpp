#ifndef FSM_LEXER_HPP
#define FSM_LEXER_HPP

#include "minimization.hpp"
#include "recognizer.hpp"
#include "regex.hpp"

#include <string_view>

namespace fsm
{
struct fsm_regex_matcher final
{
	recognizer recognizer;

	static fsm_regex_matcher compile(const std::string& pattern)
	{
		regex re(pattern);
		const auto nfa = re.compile();
		return fsm_regex_matcher{ minimize(determinize(nfa)) };
	}

	[[nodiscard]] std::size_t
	find_match(const std::string_view source, const std::size_t start_pos) const
	{
		fsm::recognizer machine_run = recognizer;
		std::size_t last_final_len = 0;

		for (std::size_t i = start_pos; i < source.length(); ++i)
		{
			try
			{
				if (machine_run.handle_input(std::make_optional(std::string(1, source[i]))))
				{
					last_final_len = i - start_pos + 1;
				}
			}
			catch (...)
			{
				break;
			}
		}

		return last_final_len;
	}
};

struct std_regex_matcher final
{
	std::regex regex;

	static std_regex_matcher compile(const std::string& pattern)
	{
		return { std::regex(pattern, std::regex::optimize) };
	}

	[[nodiscard]] std::size_t
	find_match(const std::string_view source, const std::size_t start_pos) const
	{
		constexpr auto flags = std::regex_constants::match_continuous;
		if (std::cmatch match; std::regex_search(
				source.data() + start_pos,
				source.data() + source.size(),
				match, regex, flags))
		{
			return match.length();
		}

		return 0;
	}
};

template <typename T_Type>
struct token
{
	T_Type type;
	std::string_view lexeme;

	size_t line{};
	size_t column{};
	size_t offset{};
};

template <typename T_TokenType, typename T_Matcher = fsm_regex_matcher>
class lexer
{
public:
	using token_type = token<T_TokenType>;

	struct rule
	{
		T_TokenType type;
		T_Matcher matcher;

		bool skip{};
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
		m_rules.emplace_back(type, T_Matcher::compile(expression), skip);

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

		for (const auto& rule : m_rules)
		{
			const std::size_t current_len = rule.matcher.find_match(m_source, m_cursor);

			if (current_len > max_len)
			{
				max_len = current_len;
				best_rule = &rule;
			}
		}

		if (max_len > 0)
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

#endif // FSM_LEXER_HPP
