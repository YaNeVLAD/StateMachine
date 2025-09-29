#ifndef DEFAULT_TRANSLATOR_HPP
#define DEFAULT_TRANSLATOR_HPP

#include "concepts.hpp"

#include <stdexcept>

namespace fsm
{

template <
	concepts::translatable T_Derived,
	typename T_MachineTraits = state_machine_traits<T_Derived>,
	typename T_Traits = translation_traits<T_Derived>>
class default_translator
{
	using machine_traits = T_MachineTraits;
	using translation_traits = T_Traits;

	using input_type = typename machine_traits::input_type;
	using state_type = typename machine_traits::state_type;

	using find_result_type = typename translation_traits::find_result_type;
	using result_type = typename translation_traits::result_type;

public:
	[[nodiscard]] static result_type
	translate(input_type const& input, state_type const& state) noexcept(false)
	{
		using traits = translation_traits;

		const find_result_type res = traits::find(state, input);

		if (!traits::is_valid(res, state))
		{
			throw std::runtime_error("Undefined transition for input '" + input + "'");
		}

		return traits::result(res);
	}

protected:
	default_translator() = default;
};
} // namespace fsm

#endif // DEFAULT_TRANSLATOR_HPP
