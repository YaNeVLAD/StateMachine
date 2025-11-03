#ifndef LABELED_HPP
#define LABELED_HPP

#include <fixed_string.hpp>

namespace fsm
{
template <base_fixed_string T_Name, typename T>
class labeled
{
public:
	using name_type = decltype(T_Name);
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;

	labeled()
		: m_value{}
	{
	}

	template <typename T_Value>
	labeled(T_Value&& value)
		: m_value{ std::forward<T_Value>(value) }
	{
	}

	reference value() { return m_value; }
	const_reference value() const { return m_value; }

	decltype(T_Name) name() const
	{
		return T_Name;
	}

private:
	T m_value;
	static constexpr const auto& name_value = T_Name;
};

template <base_fixed_string T_Name, typename T_Value>
constexpr auto make_labeled(T_Value&& value)
{
	return labeled<T_Name, std::decay_t<T_Value>>{ std::forward<T_Value>(value) };
}

template <base_fixed_string T_Name, typename T_Value, typename... T_Args>
constexpr auto make_labeled(T_Args&&... args)
{
	return labeled<T_Name, std::decay_t<T_Value>>{
		T_Value(std::forward<T_Args>(args)...)
	};
}
} // namespace fsm

template <fsm::fixed_string T_Name, typename T>
std::ostream& operator<<(std::ostream& oss, fsm::labeled<T_Name, T> const& obj)
{
	oss << obj.name().c_str() << " = " << obj.value();

	return oss;
}

template <fsm::fixed_wstring T_Name, typename T>
std::wostream& operator<<(std::wostream& oss, fsm::labeled<T_Name, T> const& obj)
{
	oss << obj.name().c_str() << " = " << obj.value();

	return oss;
}

#endif // LABELED_HPP
