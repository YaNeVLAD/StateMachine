#ifndef STATE_MACHINE_CONVERTER_HPP
#define STATE_MACHINE_CONVERTER_HPP

#include <concepts>

namespace fsm
{
template <typename T_From, typename T_To>
struct converter
{
	template <typename... Args>
	[[nodiscard]] static T_To operator()(T_From const& from, Args&&...)
	{
		static_assert(std::convertible_to<T_To, T_From>, "TODO: Explain why");

		return static_cast<T_To>(from);
	}
};
} // namespace fsm

#endif // STATE_MACHINE_CONVERTER_HPP
