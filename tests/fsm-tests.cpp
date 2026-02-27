#include <gtest/gtest.h>

#include <fsm/cfg.hpp>
#include <fsm/integer_symbol_generator.hpp>
#include <fsm/recognizer.hpp>
#include <fsm/string_symbol_generator.hpp>

using namespace fsm;
using namespace fsm::transforms;

recognizer_state SimpleRecognizerState()
{
	recognizer_state state;
	state.state_ids = { "q0", "q1" };
	state.initial_state_id = "q0";
	state.current_state_id = "q0";
	state.final_state_ids = { "q1" };
	state.is_deterministic = true;
	state.transitions.emplace(std::make_pair("q0", std::make_optional<std::string>("a")), "q1");
	state.transitions.emplace(std::make_pair("q1", std::make_optional<std::string>("b")), "q0");
	return state;
}

TEST(Recognizer, ConstructorCopy)
{
	recognizer_state state = SimpleRecognizerState();
	recognizer r(state);
	EXPECT_EQ(r.state().initial_state_id, "q0");
	EXPECT_EQ(r.state().final_state_ids.size(), 1);
}

TEST(Recognizer, ConstructorMove)
{
	recognizer_state state = SimpleRecognizerState();
	recognizer r(std::move(state));
	EXPECT_EQ(r.state().initial_state_id, "q0");
}

TEST(Recognizer, ConstructorMealy)
{
	mealy_state ms;
	ms.state_ids = { "q0", "q1" };
	ms.initial_state_id = "q0";
	ms.current_state_id = "q0";
	ms.transitions.emplace(std::make_pair("q0", "a"), std::make_pair("q1", "out"));
	ms.transitions.emplace(std::make_pair("q1", "b"), std::make_pair("q0", "out"));
	std::set<std::string> finals = { "q1" };
	recognizer r(mealy_machine(ms), finals);
	EXPECT_EQ(r.state().final_state_ids.size(), 1);
}

TEST(Recognizer, ConstructorMoore)
{
	moore_state ms;
	ms.state_ids = { "q0", "q1" };
	ms.initial_state_id = "q0";
	ms.current_state_id = "q0";
	ms.transitions.emplace(std::make_pair("q0", "a"), "q1");
	ms.transitions.emplace(std::make_pair("q1", "b"), "q0");
	std::set<std::string> finals = { "q1" };
	recognizer r(moore_machine(ms), finals);
	EXPECT_EQ(r.state().final_state_ids.size(), 1);
}

TEST(Recognizer, HandleInputVector)
{
	recognizer r(SimpleRecognizerState());
	const std::vector inputs = { std::make_optional<std::string>("a") };
	const bool result = r.handle_input(inputs);
	EXPECT_TRUE(result);
}

TEST(Recognizer, HandleInputVariadic)
{
	recognizer r(SimpleRecognizerState());
	const bool result = r.handle_input(std::make_optional<std::string>("a"));
	EXPECT_TRUE(result);
}

TEST(Recognizer, IsDeterministic)
{
	recognizer r(SimpleRecognizerState());
	EXPECT_TRUE(r.is_deterministic());
}

TEST(Recognizer, FromDotAndToDot)
{
	recognizer_state state = SimpleRecognizerState();
	recognizer r(state);
	const std::string filename = "res/test_recognizer.dot";
	std::ofstream file(filename);
	dot(file, r);
	recognizer r2 = recognizer::from_dot(filename);
	EXPECT_EQ(r2.state().state_ids.size(), 2);
	remove(filename.c_str());
}

TEST(Recognizer, OutputFrom)
{
	recognizer r(SimpleRecognizerState());
	EXPECT_TRUE(r.handle_input(std::make_optional<std::string>("a")));
}

TEST(Recognizer, IsFinal)
{
	recognizer r(SimpleRecognizerState());
	EXPECT_TRUE(r.handle_input(std::make_optional<std::string>("a")));
	EXPECT_TRUE(r.state().final_state_ids.contains("q1"));
}

TEST(Recognizer, RecognizeVariadic)
{
	recognizer r(SimpleRecognizerState());
	const bool result = recognize(r, "a");
	EXPECT_TRUE(result);
}

TEST(Recognizer, RecognizeVector)
{
	recognizer r(SimpleRecognizerState());
	const std::vector inputs = { std::make_optional<std::string>("a") };
	const bool result = recognize(r, inputs);
	EXPECT_TRUE(result);
}

TEST(Recognizer, Determinize)
{
	recognizer r(SimpleRecognizerState());
	auto dr = determinize(r);
	EXPECT_TRUE(dr.is_deterministic());
}

mealy_state SimpleMealyState()
{
	mealy_state s;
	s.state_ids = { "s0", "s1" };
	s.initial_state_id = "s0";
	s.current_state_id = "s0";
	s.transitions.emplace(std::make_pair(std::string("s0"), std::string("a")),
		std::make_pair(std::string("s1"), std::string("out1")));
	s.transitions.emplace(std::make_pair(std::string("s1"), std::string("b")),
		std::make_pair(std::string("s0"), std::string("out2")));
	return s;
}

TEST(MealyMachine, ConstructorCopy)
{
	const mealy_state ms = SimpleMealyState();
	const mealy_machine m(ms);
	EXPECT_EQ(m.state().initial_state_id, "s0");
	EXPECT_EQ(m.state().state_ids.size(), 2u);
}

TEST(MealyMachine, ConstructorMove)
{
	mealy_state ms = SimpleMealyState();
	const mealy_machine m(std::move(ms));
	EXPECT_EQ(m.state().initial_state_id, "s0");
}

TEST(MealyMachine, HandleInputSingleTransition)
{
	mealy_machine m(SimpleMealyState());
	const auto out = m.handle_input("a");
	EXPECT_EQ(out, "out1");
	EXPECT_EQ(m.state().current_state_id, "s1");
}

TEST(MealyMachine, HandleInputSequence)
{
	mealy_machine m(SimpleMealyState());
	const auto out1 = m.handle_input("a");
	EXPECT_EQ(out1, "out1");
	const auto out2 = m.handle_input("b");
	EXPECT_EQ(out2, "out2");
	EXPECT_EQ(m.state().current_state_id, "s0");
}

void verify_is_cnf(const cfg& g, const bool allows_epsilon)
{
	const auto& start = g.start_symbol();
	for (const auto& r : g.rules())
	{
		// 1. Стартовый символ не должен встречаться в правых частях
		for (const auto& sym : r.rhs)
		{
			EXPECT_NE(sym, start)
				<< "Rule " << r.lhs << " contains start symbol '" << start << "' on RHS!";
		}

		// 2. Проверка формы правил
		if (r.is_epsilon())
		{
			EXPECT_TRUE(allows_epsilon && r.lhs == start)
				<< "Epsilon rule allowed ONLY for start symbol if language contains epsilon. "
				<< "Found invalid epsilon rule: " << r.lhs << " -> e";
		}
		else if (r.rhs.size() == 1)
		{
			EXPECT_TRUE(g.is_terminal(r.rhs[0]))
				<< "Rule of length 1 must map to a terminal! Found: "
				<< r.lhs << " -> " << r.rhs[0];
		}
		else if (r.rhs.size() == 2)
		{
			EXPECT_TRUE(g.is_non_terminal(r.rhs[0]) && g.is_non_terminal(r.rhs[1]))
				<< "Rule of length 2 must map to two non-terminals! Found: "
				<< r.lhs << " -> " << r.rhs[0] << " " << r.rhs[1];
		}
		else
		{
			FAIL() << "Rule length > 2 is not allowed in CNF! Found rule length: " << r.rhs.size();
		}
	}
}

// E -> E+E | E*E | (E) | a
TEST(CFGTransformsTest, Grammar1)
{
	const cfg grammar(
		{ "E" },
		{ "+", "*", "(", ")", "a" },
		{ { "E", { "E", "+", "E" } },
			{ "E", { "E", "*", "E" } },
			{ "E", { "(", "E", ")" } },
			{ "E", { "a" } } },
		"E");

	const cfg cnf = grammar | to_chomsky_normal_form;

	verify_is_cnf(cnf, false);
}

// S -> aSb | aSc | cSb | e
TEST(CFGTransformsTest, Grammar2)
{
	const cfg grammar(
		{ "S" },
		{ "a", "b", "c" },
		{
			{ "S", { "a", "S", "b" } },
			{ "S", { "a", "S", "c" } },
			{ "S", { "c", "S", "b" } },
			{ "S", {} } // e-rule
		},
		"S");

	const cfg cnf = grammar | to_chomsky_normal_form;

	verify_is_cnf(cnf, true);
}

// P -> AB | BG
// A -> aA | e
// B -> c | bB
// G -> c | bA
TEST(CFGTransformsTest, Grammar3)
{
	const cfg grammar(
		{ "P", "A", "B", "G" },
		{ "a", "b", "c" },
		{ { "P", { "A", "B" } },
			{ "P", { "B", "G" } },
			{ "A", { "a", "A" } },
			{ "A", {} }, // e-rule
			{ "B", { "c" } },
			{ "B", { "b", "B" } },
			{ "G", { "c" } },
			{ "G", { "b", "A" } } },
		"P");

	const cfg cnf = grammar | to_chomsky_normal_form;

	verify_is_cnf(cnf, false);
}

// S -> abSa | aaAb | b
// A -> baAb | b
TEST(CFGTransformsTest, Grammar4)
{
	const cfg grammar(
		{ "S", "A" },
		{ "a", "b" },
		{ { "S", { "a", "b", "S", "a" } },
			{ "S", { "a", "a", "A", "b" } },
			{ "S", { "b" } },
			{ "A", { "b", "a", "A", "b" } },
			{ "A", { "b" } } },
		"S");

	const cfg cnf = grammar | to_chomsky_normal_form;

	verify_is_cnf(cnf, false);
}

TEST(IntegerSymbolGeneratorLogicTest, FillUpToTypeLimit)
{
	using SymbolType = uint8_t;
	basic_cfg<SymbolType> cfg;
	integer_symbol_generator<SymbolType> gen;

	for (SymbolType i = 0; i < 250; ++i)
	{
		cfg.add_terminal(i);
	}
	cfg.set_start_symbol(0);

	const SymbolType start = gen.next_start_symbol(0, cfg.terminals());
	EXPECT_EQ(start, 250);
	cfg.add_non_terminal(start);

	EXPECT_EQ(gen.next_intermediate(), 251);
	EXPECT_EQ(gen.next_intermediate(), 252);
	EXPECT_EQ(gen.next_intermediate(), 253);
	EXPECT_EQ(gen.next_intermediate(), 254);
	EXPECT_EQ(gen.next_intermediate(), 255);

	EXPECT_THROW({ gen.next_intermediate(); }, std::overflow_error);
}

TEST(IntegerSymbolGeneratorLogicTest, RespectExistingMaxSymbol)
{
	using SymbolType = int;
	basic_cfg<SymbolType> cfg;
	integer_symbol_generator<SymbolType> gen;

	cfg.add_terminal(1);
	cfg.add_terminal(5000);
	cfg.add_non_terminal(0);

	std::set<SymbolType> all_symbols = cfg.terminals();
	all_symbols.insert(cfg.non_terminals().begin(), cfg.non_terminals().end());

	const SymbolType next = gen.next_start_symbol(0, all_symbols);

	EXPECT_EQ(next, 5001);
}

TEST(IntegerSymbolGeneratorLogicTest, UnsignedInitialization)
{
	using SymbolType = unsigned int;
	integer_symbol_generator<SymbolType> gen;

	const SymbolType next = gen.next_start_symbol(0, {});
	EXPECT_LT(next, 1000000);
}

TEST(StringSymbolGeneratorTest, BasicGenerationChar)
{
	string_symbol_generator<std::string> gen;

	EXPECT_EQ(gen.next_terminal_proxy("abc"), "T_abc");

	EXPECT_EQ(gen.next_intermediate(), "C_1");
	EXPECT_EQ(gen.next_intermediate(), "C_2");
}

TEST(StringSymbolGeneratorTest, StartSymbolCollision)
{
	string_symbol_generator<std::string> gen;
	const std::string old_start = "S";

	const std::set<std::string> nts = { "S'", "S''" };

	const std::string new_start = gen.next_start_symbol(old_start, nts);
	EXPECT_EQ(new_start, "S'''");
}

TEST(StringSymbolGeneratorTest, WideStringSupport)
{
	string_symbol_generator<std::wstring> gen;

	EXPECT_EQ(gen.next_terminal_proxy(L"x"), L"T_x");

	EXPECT_EQ(gen.next_intermediate(), L"C_1");
}

TEST(StringSymbolGeneratorTest, UTF8Support)
{
	string_symbol_generator<std::u8string> gen;

	EXPECT_EQ(gen.next_terminal_proxy(u8"id"), u8"T_id");
	EXPECT_EQ(gen.next_intermediate(), u8"C_1");
}

TEST(StringSymbolGeneratorTest, UTF16Support)
{
	string_symbol_generator<std::u16string> gen;

	EXPECT_EQ(gen.next_terminal_proxy(u"id"), u"T_id");
	EXPECT_EQ(gen.next_intermediate(), u"C_1");
}

TEST(StringSymbolGeneratorTest, UTF32Support)
{
	string_symbol_generator<std::u32string> gen;

	EXPECT_EQ(gen.next_terminal_proxy(U"id"), U"T_id");
	EXPECT_EQ(gen.next_intermediate(), U"C_1");
}

// E -> E+E | E*E | (E) | a
// w1 = a+(a+a)
// w2 = (a+a)*a
TEST(CYKAlgorithmTest, Grammar1)
{
	const cfg grammar(
		{ "E" },
		{ "+", "*", "(", ")", "a" },
		{ { "E", { "E", "+", "E" } },
			{ "E", { "E", "*", "E" } },
			{ "E", { "(", "E", ")" } },
			{ "E", { "a" } } },
		"E");

	const auto cnf = grammar | to_chomsky_normal_form;

	EXPECT_TRUE(algorithms::cyk(cnf, "a+(a+a)")) << "w1 should belong to the grammar";
	EXPECT_TRUE(algorithms::cyk(cnf, "(a+a)*a")) << "w2 should belong to the grammar";

	EXPECT_FALSE(algorithms::cyk(cnf, "a+*a")) << "Invalid expression should be rejected";
}

// S -> aSb | aSc | cSb | e
// w1 = aaacbbcb
// w2 = ccaaaccbb
TEST(CYKAlgorithmTest, Grammar2)
{
	const cfg grammar(
		{ "S" },
		{ "a", "b", "c" },
		{
			{ "S", { "a", "S", "b" } },
			{ "S", { "a", "S", "c" } },
			{ "S", { "c", "S", "b" } },
			{ "S", {} } // e-rule
		},
		"S");

	const auto cnf = grammar | to_chomsky_normal_form;

	EXPECT_TRUE(algorithms::cyk(cnf, "aaacbbcb")) << "w1 belongs to the grammar";

	EXPECT_FALSE(algorithms::cyk(cnf, "ccaaaccbb")) << "w2 does NOT belong to the grammar (odd length)";
}

// P -> AB | BG
// A -> aA | e
// B -> c | bB
// G -> c | bA
// w1 = bbbcaaa
// w2 = bbbcbaa
TEST(CYKAlgorithmTest, Grammar3)
{
	const cfg grammar(
		{ "P", "A", "B", "G" },
		{ "a", "b", "c" },
		{ { "P", { "A", "B" } },
			{ "P", { "B", "G" } },
			{ "A", { "a", "A" } },
			{ "A", {} }, // e-rule
			{ "B", { "c" } },
			{ "B", { "b", "B" } },
			{ "G", { "c" } },
			{ "G", { "b", "A" } } },
		"P");

	const auto cnf = grammar | to_chomsky_normal_form;

	EXPECT_FALSE(algorithms::cyk(cnf, "bbbcaaa")) << "w1 does NOT belong to the grammar";
	EXPECT_TRUE(algorithms::cyk(cnf, "bbbcbaa")) << "w2 belongs to the grammar (P => BG => bbbc baa)";
}

// S -> abSa | aaAb | b
// A -> baAb | b
// w1 = abaababbba
// w2 = aabababbbb
TEST(CYKAlgorithmTest, Task5_LongRules)
{
	const cfg grammar(
		{ "S", "A" },
		{ "a", "b" },
		{ { "S", { "a", "b", "S", "a" } },
			{ "S", { "a", "a", "A", "b" } },
			{ "S", { "b" } },
			{ "A", { "b", "a", "A", "b" } },
			{ "A", { "b" } } },
		"S");

	const auto cnf = grammar | to_chomsky_normal_form;

	// w1: S => abSa => ab(aaAb)a => abaa(baAb)ba => abaaba(b)ba = abaababbba
	// w2: S => aaAb => aa(baAb)b => aaba(baAb)bb => aababa(b)bbb = aabababbbb
	EXPECT_TRUE(algorithms::cyk(cnf, "abaababbba")) << "w1 belongs to the grammar";
	EXPECT_TRUE(algorithms::cyk(cnf, "aabababbbb")) << "w2 belongs to the grammar";

	EXPECT_FALSE(algorithms::cyk(cnf, "aba")) << "Invalid word should be rejected";
}

#if 0

TEST(CYKTest, LoadFromFile)
{
	std::ifstream file("res/cfg_grammar.txt");
	const auto grammar = cfg_load(file);

	std::cout << "--- Loaded Grammar ---\n";
	grammar.print();

	const auto cnf = grammar | to_chomsky_normal_form;

	std::cout << "\n--- CNF Grammar ---\n";
	cnf.print();

	const std::vector<std::string> word = {};
	const std::string word_str = "bbbcaaa";
	const auto res = algorithms::cyk(cnf, word_str);
	algorithms::print_cyk_table(res, word_str);

	if (res)
	{
		std::cout << "\nWord is VALID.\n";
	}
	else
	{
		std::cout << "\nWord is INVALID.\n";
	}
}

TEST(CFGTest, LoadFromFile)
{
	std::ifstream file("res/cfg_grammar.txt");
	const auto grammar = cfg_load(file);

	std::cout << "--- Loaded Grammar ---\n";
	grammar.print();

	const auto reduced = grammar
		| remove_epsilon_rules
		| remove_unit_rules
		| remove_useless_symbols
		| merge_equivalent_symbols;

	std::cout << "--- Reduced grammar ---\n";
	reduced.print();
}

#endif