#include <gtest/gtest.h>

#include <fsm/recognizer.hpp>

using namespace fsm;

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
