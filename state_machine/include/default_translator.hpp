#ifndef DEFAULT_TRANSLATOR_HPP
#define DEFAULT_TRANSLATOR_HPP

#include "concepts.hpp"

#include <stdexcept>

namespace fsm
{

template <concepts::translatable T_Derived>
class default_translator
{
	using machine_traits = state_machine_traits<T_Derived>;
	using translation_traits = translation_traits<T_Derived>;

	using input_type = typename machine_traits::input_type;
	using state_type = typename machine_traits::state_type;

	using find_type = typename translation_traits::find_type;
	using container_type = typename translation_traits::container_type;
	using result_type = typename translation_traits::result_type;

public:
	[[nodiscard]] result_type translate(input_type const& input, state_type const& state)
	{
		const find_type it = translation_traits::find(state, input);

		if (!translation_traits::is_valid(it, state))
		{
			throw std::runtime_error("Undefined transition for input '" + input + "'");
		}

		return it->second;
	}

protected:
	default_translator() = default;
};
} // namespace fsm

#endif // DEFAULT_TRANSLATOR_HPP
