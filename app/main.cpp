#include <fsm.hpp>

#include <iostream>

#include "printers/MealyToDot.hpp"
#include "printers/MooreToDot.hpp"
#include "readers/MealyFromDot.hpp"
#include "readers/MooreFromDot.hpp"

template <size_t N, typename T_Char>
struct base_fixed_string
{
	using traits_type = std::char_traits<T_Char>;

	T_Char data[N]{};

	constexpr base_fixed_string(const T_Char (&str)[N])
	{
		traits_type::copy(data, str, N);
	}

	[[nodiscard]] constexpr const T_Char* c_str() const
	{
		return data;
	}

	[[nodiscard]] constexpr size_t size() const
	{
		return N - 1;
	}

	template <size_t R>
	constexpr auto operator<=>(base_fixed_string<R, T_Char> const& rhs) const
	{
		return std::lexicographical_compare_three_way(
			data, data + N,
			rhs.data, rhs.data + R);
	}
};

template <size_t N>
using fixed_string = base_fixed_string<N, char>;

template <size_t N>
using fixed_wstring = base_fixed_string<N, char>;

template <size_t N>
base_fixed_string(const char (&str)[N]) -> base_fixed_string<N, char>;

template <size_t N>
base_fixed_string(const wchar_t (&str)[N]) -> base_fixed_string<N, wchar_t>;

template <typename T_Char, size_t N>
std::basic_ostream<T_Char>& operator<<(std::ostream& os, const base_fixed_string<N, T_Char>& str)
{
	os << str.c_str();
	return os;
}

template <typename T, base_fixed_string T_Name>
class Labeled
{
public:
	using name_type = decltype(T_Name);
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;

	Labeled()
		: value{}
	{
	}

	Labeled(value_type value)
		: value{ std::move(value) }
	{
	}

	reference Value() { return value; }
	const_reference Value() const { return value; }

	const char* Name() const
	{
		return name_value.c_str();
	}

private:
	T value;
	static constexpr base_fixed_string name_value = T_Name;
};

template <typename T, fixed_string T_Name>
std::ostream& operator<<(std::ostream& oss, Labeled<T, T_Name> const& obj)
{
	oss << obj.Name() << " = " << obj.Value();

	return oss;
}

struct Person
{
	Labeled<std::string, "name"> name;
	Labeled<int, "age"> age;

	Person(std::string name, const int age)
		: name{ std::move(name) }
		, age{ age }
	{
	}
};

void print_person(const Person& person)
{
	std::cout << person.name << std::endl;
	std::cout << person.age << std::endl;
}

int main()
{
	using namespace std::literals;

	const std::string moore_file2 = "res/moore2.dot";
	const std::string mealy_file2 = "res/mealy2.dot";
	const std::string recognizer_file = "res/recognizer.dot";
#if 0
	Labeled<int, "gg"> label{ 333 };
	Labeled<int, "gg"> same_label{ 42 };
	Labeled<float, "gg"> label2{ 25.f };

	std::cout << label.Value() << std::endl;
	std::cout << label.Name() << std::endl;

	std::cout << label2.Value() << std::endl;
	std::cout << label2.Name() << std::endl;

	const Person p1{ "Vosya", 42 };
	const Person p2{ "Oleg", 66 };

	print_person(p1);
	print_person(p2);

	constexpr fixed_string str1 = "a";
	constexpr fixed_string str2 = "ba";

	static_assert(str1 < str2);

	static_assert(std::is_same_v<decltype(label), decltype(same_label)>);
#endif
#if 1
	// auto mealy = fsm::from_dot<fsm::mealy_machine>("mealy.dot");
	// auto moore = fsm::from_dot<fsm::moore_machine>("moore.dot");
	// auto recognizer = fsm::from_dot<fsm::recognizer>("recognizer.dot");
	//
	// fsm::to_dot(mealy, "out_mealy.dot");
	// fsm::to_dot(moore, "out_moore.dot");
	// fsm::to_dot(recognizer, "out_recognizer.dot");

	try
	{
		{
			std::cout << "Minimization Test Moore" << std::endl;
			auto moore = CreateMooreMachineFromDot(moore_file2);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl;
			std::cout << moore.handle_input("z1") << std::endl;
			std::cout << moore.handle_input("z2") << std::endl;
			std::cout << moore.handle_input("z2") << std::endl;
			std::cout << moore.handle_input("z1") << std::endl;
			std::cout << moore.handle_input("z2") << std::endl;
			std::cout << moore.handle_input("z1") << std::endl;
			std::cout << moore.handle_input("z1") << std::endl;
			std::cout << moore.handle_input("z2") << std::endl;

			auto minMoore = fsm::minimize(moore);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl;
			std::cout << minMoore.handle_input("z1") << std::endl;
			std::cout << minMoore.handle_input("z2") << std::endl;
			std::cout << minMoore.handle_input("z2") << std::endl;
			std::cout << minMoore.handle_input("z1") << std::endl;
			std::cout << minMoore.handle_input("z2") << std::endl;
			std::cout << minMoore.handle_input("z1") << std::endl;
			std::cout << minMoore.handle_input("z1") << std::endl;
			std::cout << minMoore.handle_input("z2") << std::endl;

			ExportMooreMachineToDot(minMoore, "min_moore2.dot");
		}

		{
			std::cout << "Minimization Test Mealy" << std::endl;
			auto mealy = CreateMealyMachineFromDot(mealy_file2);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl;
			std::cout << mealy.handle_input("z1") << std::endl;
			std::cout << mealy.handle_input("z2") << std::endl;
			std::cout << mealy.handle_input("z2") << std::endl;
			std::cout << mealy.handle_input("z1") << std::endl;
			std::cout << mealy.handle_input("z2") << std::endl;
			std::cout << mealy.handle_input("z1") << std::endl;
			std::cout << mealy.handle_input("z1") << std::endl;
			std::cout << mealy.handle_input("z2") << std::endl;

			auto minMealy = fsm::minimize(mealy);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl;
			std::cout << minMealy.handle_input("z1") << std::endl;
			std::cout << minMealy.handle_input("z2") << std::endl;
			std::cout << minMealy.handle_input("z2") << std::endl;
			std::cout << minMealy.handle_input("z1") << std::endl;
			std::cout << minMealy.handle_input("z2") << std::endl;
			std::cout << minMealy.handle_input("z1") << std::endl;
			std::cout << minMealy.handle_input("z1") << std::endl;
			std::cout << minMealy.handle_input("z2") << std::endl;

			ExportMealyMachineToDot(minMealy, "min_mealy2.dot");
		}

		{
			std::cout << "Recognizer test" << std::endl;
			auto recognizer = fsm::recognizer::from_dot(recognizer_file);
			std::cout << "is_deterministic " << std::boolalpha << recognizer.is_deterministic() << std::endl;
			auto dr = fsm::determinize(recognizer);
			auto mdr = fsm::minimize(dr);

			dr.to_dot("out_recognizer.dot");
			mdr.to_dot("out_recognizer2.dot");
		}
	}
	catch (std::exception const& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return 0;
#endif
}
