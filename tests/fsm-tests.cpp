#include <gtest/gtest.h>
#include <stack>

#include <fsm/cfg.hpp>
#include <fsm/integer_symbol_generator.hpp>
#include <fsm/ll1/accepts.hpp>
#include <fsm/ll1/table.hpp>
#include <fsm/ll1/table_builder.hpp>
#include <fsm/ll1/table_io.hpp>
#include <fsm/ll1/table_printer.hpp>
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

TEST(StringSymbolGeneratorTest, ConstructorArgs)
{
	string_symbol_generator<std::string> gen{
		'_',
		"Terminal_",
		"Intermediate_"
	};
	const std::string old_start = "S";

	const std::set<std::string> nts = { "S_", "S__" };

	const std::string new_start = gen.next_start_symbol(old_start, nts);
	EXPECT_EQ(new_start, "S___");

	EXPECT_EQ(gen.next_terminal_proxy("x"), "Terminal_x");

	EXPECT_EQ(gen.next_intermediate(), "Intermediate_1");
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

// S -> A | B | a
// A -> a B | b S
// B -> A B | B a | B b
// C -> A d | a
TEST(CFGTest, ReduceGrammar)
{
	std::string grammar_str
		= "S -> A | B | a\n"
		  "A -> a B | b S\n"
		  "B -> A B | B a | B b\n"
		  "C -> A d | a\n";

	std::stringstream ss(grammar_str);

	const auto grammar = cfg_load(ss);

	std::cout << "--- Original Grammar ---\n";
	grammar.print();

	const auto reduced = grammar | reduce_grammar;

	std::cout << "\n--- Reduced Grammar ---\n";
	reduced.print();
}

// E -> E + T | T
// T -> T * F | F
// F -> ( E ) | id
TEST(CFGTest, LeftRecursionElimination)
{
	const cfg grammar(
		{ "E", "T", "F" },
		{ "+", "*", "id", "(", ")" },
		{ { "E", { "E", "+", "T" } },
			{ "E", { "T" } },
			{ "T", { "T", "*", "F" } },
			{ "T", { "F" } },
			{ "F", { "(", "E", ")" } },
			{ "F", { "id" } } },
		"E");

	std::cout << "--- Original Grammar ---\n";
	grammar.print();

	const auto top_down_ready = grammar
		| reduce_grammar
		| remove_left_recursion;

	std::cout << "\n--- Without Left Recursion ---\n";
	top_down_ready.print();
}

// S -> a B S | b
// B -> c | ε
TEST(LL1TableTest, BuildValidTable)
{
	using namespace std::literals;
	const basic_cfg<std::string> g(
		{ "S", "B" },
		{ "a", "b", "c" },
		{ { "S", { "a", "B", "S" } },
			{ "S", { "b" } },
			{ "B", { "c" } },
			{ "B", {} } },
		"S");

	const auto table = ll1::table_builder(g, "ε"s, "$"s).build();

	// S rules
	ASSERT_TRUE(table.has_rule("S", "a"));
	EXPECT_EQ(table.at("S", "a").rhs[0], "a"); // M[S, a] = S -> a B S

	ASSERT_TRUE(table.has_rule("S", "b"));
	EXPECT_EQ(table.at("S", "b").rhs[0], "b"); // M[S, b] = S -> b

	// B rules
	ASSERT_TRUE(table.has_rule("B", "c"));
	EXPECT_EQ(table.at("B", "c").rhs[0], "c"); // M[B, c] = B -> c

	// B -> ε applies for symbols from FOLLOW(B) = {a, b}
	ASSERT_TRUE(table.has_rule("B", "a"));
	EXPECT_TRUE(table.at("B", "a").is_epsilon()); // M[B, a] = B -> ε

	ASSERT_TRUE(table.has_rule("B", "b"));
	EXPECT_TRUE(table.at("B", "b").is_epsilon()); // M[B, b] = B -> ε
}

// S -> a B S | b
// B -> c | ε
TEST(LL1TableTest, BuildTableWithCustomType)
{
	struct MySymbol
	{
		std::string value;
		bool is_terminal;

		bool operator<(const MySymbol& other) const
		{
			return value < other.value;
		}

		bool operator==(const MySymbol& other) const
		{
			return value == other.value;
		}
	};

	using Symbol = MySymbol;

	Symbol S{ "S", false }, B{ "B", false };
	Symbol a{ "a", true }, b{ "b", true }, c{ "c", true };
	Symbol eps{ "ε", true }, dollar{ "$", true };

	const basic_cfg g(
		{ S, B },
		{ a, b, c },
		{ { S, { a, B, S } },
			{ S, { b } },
			{ B, { c } },
			{ B, {} } },
		S);

	const auto table = ll1::table_builder(g, eps, dollar).build();

	// S
	ASSERT_TRUE(table.has_rule(S, a));
	EXPECT_EQ(table.at(S, a).rhs[0], a); // M[S, a] = S -> a B S

	ASSERT_TRUE(table.has_rule(S, b));
	EXPECT_EQ(table.at(S, b).rhs[0], b); // M[S, b] = S -> b

	// B
	// FOLLOW(B) includes 'a' (from S -> a B S) and 'b' (from S -> b)
	ASSERT_TRUE(table.has_rule(B, c));
	EXPECT_EQ(table.at(B, c).rhs[0], c); // M[B, c] = B -> c

	// E-rules check
	ASSERT_TRUE(table.has_rule(B, a));
	EXPECT_TRUE(table.at(B, a).is_epsilon());

	ASSERT_TRUE(table.has_rule(B, b));
	EXPECT_TRUE(table.at(B, b).is_epsilon());

	// Unexisting rule test
	EXPECT_FALSE(table.has_rule(S, c));
}

// S -> a B S | b
// B -> c | ε
TEST(LL1TableTest, PrintCustomTypeTableWithDefaultFormatter)
{
	struct MySymbol
	{
		std::string value;
		bool is_terminal{};

		bool operator<(const MySymbol& other) const
		{
			return value < other.value;
		}

		bool operator==(const MySymbol& other) const
		{
			return value == other.value;
		}

		explicit operator std::string() const
		{
			return value;
		}
	};

	using Symbol = MySymbol;

	Symbol S{ "S", false }, B{ "B", false };
	Symbol a{ "a", true }, b{ "b", true }, c{ "c", true };
	Symbol eps{ "ε", true }, dollar{ "$", true };

	const basic_cfg g(
		{ S, B },
		{ a, b, c },
		{ { S, { a, B, S } },
			{ S, { b } },
			{ B, { c } },
			{ B, {} } },
		S);

	const auto table = ll1::table_builder(g, eps, dollar).build();

	std::ostringstream os;
	ll1::print_table(table, os);
	EXPECT_TRUE(os.str().find("<unprintable_symbol>") == std::string::npos);
}

// S -> a B S | b
// B -> c | ε
#if 0
TEST(LL1TableTest, PrintCustomTypeTableWithCustomFormatterAndSettings)
{
	using namespace std::literals;
	const basic_cfg<std::string> g(
		{ "S", "B" },
		{ "a", "b", "c" },
		{ { "S", { "a", "B", "S" } },
			{ "S", { "b" } },
			{ "B", { "c" } },
			{ "B", {} } },
		"S");

	const auto table = ll1::table_builder(g, "e"s, "$"s).build();

	std::ostringstream os;
	ll1::table_printer_settings settings;
	settings.table_header = "Custom LL1 table header";
	settings.epsilon_str = "Custom Epsilon symbol";
	settings.separator = " --->>> ";

	auto formatter = [](const std::string& s) {
		return std::quoted(s);
	};

	ll1::print_table(table, os, settings, formatter);
	EXPECT_TRUE(os.str().find("<unprintable_symbol>") == std::string::npos);
	EXPECT_TRUE(os.str().find("Custom LL1 table header") != std::string::npos);
	EXPECT_TRUE(os.str().find("Custom Epsilon symbol") != std::string::npos);
	EXPECT_TRUE(os.str().find(" --->>> ") != std::string::npos);
	EXPECT_TRUE(os.str().find("\"S\"") != std::string::npos);
}
#endif

// S -> AB
// S -> PQx
// A -> xy | m
// B -> bC
// C -> bC | e
// P -> pP | e
// Q -> qQ | e
// Conflict in S and A non-terminals -> symbol x
TEST(LL1TableTest, ExceptionOnConflict1)
{
	using namespace std::literals;
	std::ifstream file("res/ll1_grammar_1.txt");
	const basic_cfg<std::string> g = cfg_load(file);

	EXPECT_THROW(
		{
			const auto table = ll1::table_builder(g, "ε"s, "$"s).build();
		},
		std::runtime_error);
}

// A -> BCc | lDB
// B -> \e | bCDE
// C -> DaB | ca
// E -> lAf | c
// D -> \e | dD
TEST(LL1TableTest, CorrectLL1GrammarTest)
{
	using namespace std::literals;
	std::ifstream file("res/ll1_grammar_2.txt");
	const basic_cfg<std::string> g = cfg_load(file);

	const auto table = ll1::table_builder(g, "ε"s, "$"s).build();
	ll1::print_table(table);
}

// S -> a S A | \e
// A -> b B | \e
// B -> b d | \e
TEST(LL1TableTest, ExceptionOnConflict2)
{
	using namespace std::literals;
	std::ifstream file("res/ll1_grammar_3.txt");
	const basic_cfg<std::string> g = cfg_load(file);

	EXPECT_THROW(
		{
			const auto table = ll1::table_builder(g, "ε"s, "$"s).build();
		},
		std::runtime_error);
}

// PROGRAM -> begin d ; X end
// X -> d ; X | s Y
// Y -> ; s Y | \e
TEST(LL1TableTest, ExceptionOnConflict3)
{
	using namespace std::literals;

	std::ifstream file("res/ll1_grammar_4.txt");
	const basic_cfg<std::string> g = cfg_load(file);

	const auto table = ll1::table_builder(g, "ε"s, "$"s).build();
	ll1::print_table(table);
}

// S -> a B S | b
// B -> c | ε
TEST(LL1TableTest, BuildIntegerTypeValidTable)
{
	// 10 -> 1 11 10 | 2
	// 11 -> 3 | ε
	const basic_cfg g(
		{ 10, 11 },
		{ 1, 2, 3 },
		{ { 10, { 1, 11, 10 } },
			{ 10, { 2 } },
			{ 11, { 3 } },
			{ 11, {} } },
		10);

	const auto table = ll1::table_builder(g, 0, 256).build();

	// S rules
	ASSERT_TRUE(table.has_rule(10, 1));
	EXPECT_EQ(table.at(10, 1).rhs[0], 1); // M[S, a] = S -> a B S

	ASSERT_TRUE(table.has_rule(10, 2));
	EXPECT_EQ(table.at(10, 2).rhs[0], 2); // M[S, b] = S -> b

	// B rules
	ASSERT_TRUE(table.has_rule(11, 3));
	EXPECT_EQ(table.at(11, 3).rhs[0], 3); // M[B, c] = B -> c

	// B -> ε applies for symbols from FOLLOW(B) = {a, b}
	ASSERT_TRUE(table.has_rule(11, 1));
	EXPECT_TRUE(table.at(11, 1).is_epsilon()); // M[B, a] = B -> ε

	ASSERT_TRUE(table.has_rule(11, 2));
	EXPECT_TRUE(table.at(11, 2).is_epsilon()); // M[B, b] = B -> ε
}

// FIRST/FIRST Conflict:
// S -> a | a b
TEST(LL1TableTest, CollisionFirstFirst)
{
	using namespace std::literals;
	const basic_cfg<std::string> g(
		{ "S" },
		{ "a", "b" },
		{ { "S", { "a" } },
			{ "S", { "a", "b" } } },
		"S");

	EXPECT_THROW(
		{
			auto table = ll1::table_builder(g, "ε"s, "$"s).build();
		},
		std::runtime_error);
}

// Left recursion grammar:
// E -> E + T | T
TEST(LL1TableTest, CollisionLeftRecursion)
{
	using namespace std::literals;

	basic_cfg<std::string> g(
		{ "E", "T" },
		{ "+", "id" },
		{ { "E", { "E", "+", "T" } },
			{ "E", { "T" } },
			{ "T", { "id" } } },
		"E");

	EXPECT_THROW(
		{
			auto table = ll1::table_builder(g, "ε"s, "$"s).build();
		},
		std::runtime_error);
}

// S -> a B S | b
// B -> c | ε
TEST(LL1TableBuilderTest, BuildValidTable)
{
	using namespace std::literals;
	basic_cfg<std::string> g(
		{ "S", "B" },
		{ "a", "b", "c" },
		{ { "S", { "a", "B", "S" } },
			{ "S", { "b" } },
			{ "B", { "c" } },
			{ "B", {} } },
		"S");

	auto table = ll1::table_builder(g, "ε"s, "$"s).build();

	ASSERT_TRUE(table.has_rule("S", "a"));
	EXPECT_EQ(table.at("S", "a").rhs[0], "a");

	ASSERT_TRUE(table.has_rule("S", "b"));
	EXPECT_EQ(table.at("S", "b").rhs[0], "b");

	ASSERT_TRUE(table.has_rule("B", "c"));
	EXPECT_EQ(table.at("B", "c").rhs[0], "c");

	// Rule B -> ε for FOLLOW(B) = {a, b}
	ASSERT_TRUE(table.has_rule("B", "a"));
	EXPECT_TRUE(table.at("B", "a").is_epsilon());

	ASSERT_TRUE(table.has_rule("B", "b"));
	EXPECT_TRUE(table.at("B", "b").is_epsilon());

	EXPECT_EQ(table.epsilon(), "ε");
	EXPECT_EQ(table.end_marker(), "$");
}

// S -> a | a b
TEST(LL1TableBuilderTest, ThrowsOnCollisionByDefault)
{
	basic_cfg<std::string> g(
		{ "S" },
		{ "a", "b" },
		{ { "S", { "a" } },
			{ "S", { "a", "b" } } },
		"S");

	auto builder = ll1::table_builder(g)
					   .with_epsilon("ε")
					   .with_end_marker("$");

	EXPECT_THROW({ auto table = builder.build(); }, std::runtime_error);
}

// S -> i S E | a
// E -> e S | ε
// Conflict FIRST/FOLLOW for non-terminal E for terminal 'e'.
TEST(LL1TableBuilderTest, KeepFirstStrategyAndWarningCallback)
{
	basic_cfg<std::string> g(
		{ "S", "E" },
		{ "i", "a", "e" },
		{ { "S", { "i", "S", "E" } },
			{ "S", { "a" } },
			{ "E", { "e", "S" } },
			{ "E", {} } },
		"S");

	std::string warning_message;
	auto warn_cb = [&](const std::string& msg) {
		warning_message = msg;
	};

	auto table = ll1::table_builder(g)
					 .with_epsilon("ε")
					 .with_end_marker("$")
					 .with_collision_strategy(ll1::collision_strategy::keep_first)
					 .on_warning(warn_cb)
					 .build();

	EXPECT_FALSE(warning_message.empty());
	EXPECT_NE(warning_message.find("[WARNING] Conflict ignored (keep_first)"), std::string::npos);

	// Check that first rule remained (E -> e S) in M[E][e]
	ASSERT_TRUE(table.has_rule("E", "e"));
	auto rule = table.at("E", "e");
	ASSERT_TRUE(rule.is_epsilon());
}

// S -> a
TEST(LL1TableBuilderTest, SupportsCustomTypes)
{
	struct CustomSymbol
	{
		int id{};
		std::string name;

		bool operator==(const CustomSymbol& other) const { return id == other.id; }
		bool operator<(const CustomSymbol& other) const { return id < other.id; }
	};

	auto formatter = [](const CustomSymbol& s) -> std::string {
		return s.name;
	};

	CustomSymbol S{ 1, "S" };
	CustomSymbol a{ 2, "a" };
	CustomSymbol eps{ 0, "ε" };
	CustomSymbol eof{ 99, "$" };

	basic_cfg g(
		{ S }, { a },
		{ { S, { a } } },
		S);

	auto table = ll1::table_builder(g)
					 .with_epsilon(eps)
					 .with_end_marker(eof)
					 .build(formatter);

	ASSERT_TRUE(table.has_rule(S, a));
	EXPECT_EQ(table.at(S, a).rhs[0].id, 2);
}

struct VmRow
{
	std::string symbol;
	std::set<std::string> lookaheads;
	int next_row; // -1 equals NULL
	int stack_row; // -1 equals NULL
	bool is_epsilon;
	bool err;
	bool end;
	bool shift;
};

TEST(LL1, ExecutionProof)
{
	const std::vector<VmRow> optimized_table = {
		/* 0*/ { "c", { "c" }, 1, -1, false, true, false, true },
		/* 1*/ { "B", { "" }, 3, 2, false, true, false, false },
		/* 2*/ { "d", { "d" }, -1, -1, false, true, true, true },
		/* 3*/ { "ε", { "d", "g" }, -1, -1, true, false, true, false },
		/* 4*/ { "f", { "f" }, 6, -1, false, false, false, true },
		/* 5*/ { "k", { "k" }, -1, -1, false, true, true, true },
		/* 6*/ { "B", { "" }, 3, 7, false, true, false, false },
		/* 7*/ { "g", { "g" }, -1, -1, false, true, true, true },
		/* 8*/ { "a", { "a" }, 9, -1, false, true, false, true },
		/* 9*/ { "A", { "" }, 0, 10, false, true, false, false },
		/* 10*/ { "b", { "b" }, -1, -1, false, true, true, true }
	};

	// Input: a c f k g d b $
	const std::vector<std::string> input = { "a", "c", "f", "k", "g", "d", "b", "$" };
	int token_ptr = 0; // Index of S

	std::stack<int> ret_stack;
	int pc = 8;

	bool success = false;

	while (pc != -1)
	{
		const auto& row = optimized_table[pc];
		const std::string& current_token = input[token_ptr];

		const bool la_match = row.lookaheads.empty() || row.lookaheads.contains(current_token) || row.lookaheads.contains("");

		if (!la_match)
		{
			if (!row.err)
			{
				pc++; // Fallthrough
				continue;
			}
			FAIL() << "Синтаксическая ошибка на токене: " << current_token << " (PC=" << pc << ")";
		}

		if (row.shift && !row.is_epsilon)
		{
			token_ptr++;
		}

		if (row.stack_row != -1)
		{
			ret_stack.push(row.stack_row);
		}

		if (row.next_row != -1)
		{
			pc = row.next_row;
		}
		else if (!ret_stack.empty())
		{
			pc = ret_stack.top();
			ret_stack.pop();
		}
		else
		{
			pc = -1;
			success = true;
		}
	}

	EXPECT_TRUE(success);
	EXPECT_EQ(input[token_ptr], "$");
}

TEST(LL1, AcceptanceTest1)
{
	using namespace std::literals;
	std::ifstream file("res/ll1_grammar_test.txt");
	auto g = cfg_load(file) | remove_left_recursion | left_factor;
	auto table = ll1::table_builder(g).with_epsilon("ε").with_end_marker("$").build();

	// ------------------------------------------------------------------------
	// 1: while (a+b*c(c+a)) do a(a+b,a)=a
	// ------------------------------------------------------------------------
	std::vector<std::string> input_1 = {
		"while", "(", "a", "+", "b", "*", "c", "(", "c", "+", "a", ")", ")",
		"do", "a", "(", "a", "+", "b", ",", "a", ")", "=", "a"
	};
	EXPECT_TRUE(ll1::accepts(table, "S"s, "ε"s, "$"s, input_1, &std::cerr, &std::cout));

	// ------------------------------------------------------------------------
	// 2: while (a+b*(a+c)) do a (a, a+b) = b
	// ------------------------------------------------------------------------
	std::vector<std::string> input_2 = {
		"while", "(", "a", "+", "b", "*", "(", "a", "+", "c", ")", ")",
		"do", "a", "(", "a", ",", "a", "+", "b", ")", "=", "b"
	};
	EXPECT_TRUE(ll1::accepts(table, "S"s, "ε"s, "$"s, input_2, &std::cerr, &std::cout));

	// ------------------------------------------------------------------------
	// 3: while (a+b*c(c+a)) do a(a+b a)=a
	// ------------------------------------------------------------------------
	std::vector<std::string> input_3 = {
		"while", "(", "a", "+", "b", "*", "c", "(", "c", "+", "a", ")", ")",
		"do", "a", "(", "a", "+", "b", "a", ")", "=", "a"
	};
	EXPECT_FALSE(ll1::accepts(table, "S"s, "ε"s, "$"s, input_3, &std::cerr, &std::cout));
}

// S -> type I = T B
// T -> int | record I : T B end
// B -> \e | ; I : T B
// I -> a | b | c
TEST(LL1, AcceptanceTest2)
{
	using namespace std::literals;
	std::ifstream file("res/ll1_grammar_test2.txt");
	auto g = cfg_load(file) | remove_left_recursion | left_factor | reduce_grammar;
	auto table = ll1::table_builder(g).with_epsilon("ε").with_end_marker("$").build();

	std::vector<std::string> input_sequence = {
		"type", "a", "=", "record", "a", ":", "int", ";", "b", ":", "int", "end"
	};
	EXPECT_TRUE(ll1::accepts(table, "S"s, "ε"s, "$"s, input_sequence, &std::cerr, &std::cout));

	std::vector<std::string> input_sequence2 = {
		"type", "a", "=", "int", ";", "b", ":", "record", "a", ":", "int", "end"
	};
	EXPECT_TRUE(ll1::accepts(table, "S"s, "ε"s, "$"s, input_sequence2, &std::cerr, &std::cout));
}

// F -> function I ( I ) S; I := E end
// S -> ; I := E S | ε
// E -> E * I | E + I | I
// I -> a | b
TEST(LL1, AcceptanceTest4)
{
	using namespace std::literals;
	std::ifstream file("res/ll1_grammar_test4.txt");
	auto g = cfg_load(file) | remove_left_recursion | left_factor;
	auto table = ll1::table_builder(g).with_epsilon("ε").with_end_marker("$").build();

	std::vector<std::string> input_sequence = {
		"function", "a", "(", "b", ")", ";", "a", ":=", "a", "*", "a", ";", "a", ":=", "b", "*", "a", "end"
	};
	EXPECT_TRUE(ll1::accepts(table, "F"s, "ε"s, "$"s, input_sequence, &std::cerr, &std::cout));

	std::vector<std::string> input_sequence2 = {
		"function", "b", "(", "a", ")", ";", "a", ":=", "a", "+", "a", "*", "a", ";", "a", ":=", "b", "end"
	};
	EXPECT_TRUE(ll1::accepts(table, "F"s, "ε"s, "$"s, input_sequence2, &std::cerr, &std::cout));
}

TEST(LL1Table, ProgrammingLanguageGrammarTest)
{
	using namespace std::literals;
	const auto EPSILON = "ε"s;
	const auto END_MARKER = "$"s;

	std::ifstream file("res/cfg_prog_lang.txt");
	auto g = cfg_load(file) | remove_left_recursion | left_factor;
	auto table = ll1::table_builder(g).with_epsilon(EPSILON).with_end_marker(END_MARKER).build();

	ll1::print_table(table, std::cout, { .format = ll1::table_format::compiled_table });

	/*
		package org.example;
		import java.util.*;

		@Inject
		public class MyClass <T> {
			private val id: Int = intCon;
		}

		internal compile_time fun List<String>.printAll(prefix: String): Null {
			for (i in intCon .. intCon) {
				print(prefix);
			}
			return null;
		}

		var flag = true;
	*/
	std::vector<std::string> tokens = {
		// package org.example;
		"package", "ident", ".", "ident", ";",

		// import java.util.*;
		"import", "ident", ".", "ident", ".", "*", ";",

		// @Inject public class MyClass <T> {
		"@", "ident",
		"public", "class", "ident", "<", "ident", ">", "{",

		// private val id : Int = intCon ;
		"private", "val", "ident", ":", "ident", "=", "intCon", ";",

		"}", // class end

		// internal compile_time fun List<String>.printAll(prefix: String): Null {
		"internal", "compile_time", "fun", "ident", "<", "ident", ">", ".", "ident", "(", "ident", ":", "ident", ")", ":", "ident", "{",

		// for (i in intCon .. intCon) {
		"for", "(", "ident", "in", "intCon", "..", "intCon", ")", "{",

		// print (prefix) ;
		"ident", "(", "ident", ")", ";",

		"}",

		// return null ;
		"return", "null", ";",

		"}", // function end

		// var flag = true ;
		"var", "ident", "=", "true", ";"
	};

	EXPECT_TRUE(ll1::accepts(table, "Program"s, EPSILON, END_MARKER, tokens, &std::cerr));
}

// F -> function I ( I ) S; I := E end
// S -> ; I := E S | ε
// E -> E * I | E + I | I
// I -> a | b
TEST(LL1Table, SaveAndLoadTest)
{
	using namespace std::literals;
	std::ifstream file("res/ll1_grammar_test4.txt");
	auto g = cfg_load(file) | remove_left_recursion | left_factor;
	auto table = ll1::table_builder(g).with_epsilon("ε").with_end_marker("$").build();

	// --- TEXT ---
	{
		std::ofstream out("res/out_table.txt");
		ll1::io::save_to_text(table, out);
	}
	{
		std::ifstream in("res/out_table.txt");
		auto loaded_table = fsm::ll1::io::load_from_text<std::string>(in);

		EXPECT_EQ(loaded_table.epsilon(), "ε");
		EXPECT_EQ(loaded_table.end_marker(), "$");
	}

	// --- BINARY ---
	{
		std::ofstream out("res/out_table.bin", std::ios::binary);
		ll1::io::save_to_binary(table, out);
	}
	{
		std::ifstream in("res/out_table.bin", std::ios::binary);
		auto loaded_table = ll1::io::load_from_binary<std::string>(in);

		EXPECT_EQ(loaded_table.epsilon(), "ε");
		EXPECT_EQ(loaded_table.end_marker(), "$");
	}
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