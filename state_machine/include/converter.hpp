#ifndef STATE_MACHINE_CONVERTER_HPP
#define STATE_MACHINE_CONVERTER_HPP

namespace fsm
{
template <typename T_From, std::convertible_to<T_From> T_To>
struct converter
{
	[[nodiscard]] T_To operator()(T_From const& from) const
	{
		static_assert(std::convertible_to<T_To, T_From>, "TODO: Explain why");

		return static_cast<T_To>(from);
	}
};
} // namespace fsm

#endif // STATE_MACHINE_CONVERTER_HPP
