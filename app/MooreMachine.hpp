#ifndef MOORE_MACHINE_HPP
#define MOORE_MACHINE_HPP

#include "base_state_machine.hpp"
#include "default_translator.hpp"
#include "traits/translation_traits.hpp"

#include <map>
#include <string>
#include <utility>

class MooreMachine;

struct MooreState
{
	using StateId = std::string;
	using Input = std::string;
	using Output = std::string;
	using Transitions = std::map<std::pair<StateId, Input>, StateId>;

	std::set<StateId> states;
	std::map<StateId, Output> outputs;
	Transitions transitions;
	StateId startStateId;
	StateId currentStateId;
};

template <>
struct fsm::state_machine_traits<MooreMachine>
{
	using state_type = MooreState;
	using input_type = MooreState::Input;
	using output_type = MooreState::Output;
};

template <>
struct fsm::translation_traits<MooreMachine>
{
	using find_result_type = MooreState::Transitions::const_iterator;
	using result_type = MooreState::Transitions::mapped_type;

	static find_result_type find(MooreState const& state, MooreState::Input const& input)
	{
		return state.transitions.find({ state.currentStateId, input });
	}

	static bool is_valid(find_result_type const& find_result, MooreState const& state)
	{
		return find_result != state.transitions.end();
	}

	static result_type result(find_result_type const& find_result)
	{
		return find_result->second;
	}
};

class MooreMachine
	: public fsm::base_state_machine<MooreMachine>
	, public fsm::default_translator<MooreMachine>
{
	using Base = base_state_machine;
	using StateId = MooreState::StateId;
	using Input = MooreState::Input;
	using Output = MooreState::Output;

public:
	using TransitionResult = StateId;

	explicit MooreMachine(MooreState const& initialState)
		: Base(initialState)
	{
	}

	explicit MooreMachine(MooreState&& initialState)
		: Base(std::move(initialState))
	{
	}

	[[nodiscard]] Output output_from(TransitionResult const& result) const
	{
		const auto& outputs = state().outputs;
		const auto it = outputs.find(result);

		if (it == outputs.end())
		{
			throw std::runtime_error("Output for state '" + result + "' is not defined");
		}

		return it->second;
	}

	MooreState next_state_from(TransitionResult const& result)
	{
		current_state().currentStateId = result;
		return current_state();
	}
};

template <>
struct fsm::minimization_traits<MooreMachine>
{
	using id_type = std::string;
	using input_type = base_state_machine<MooreMachine>::input_type;

	static std::vector<id_type> get_all_state_ids(MooreState const& state)
	{
		auto const& s = state.states;
		return { s.begin(), s.end() };
	}

	static std::vector<input_type> get_all_inputs(MooreState const& state)
	{
		std::set<input_type> inputs;
		for (auto const& [_, input] : state.transitions | std::views::keys)
		{
			inputs.insert(input);
		}
		return { inputs.begin(), inputs.end() };
	}

	static id_type get_next_state_id(
		MooreState const& state,
		id_type const& current,
		input_type const& input)
	{
		return state.transitions.at({ current, input });
	}

	static bool are_0_equivalent(
		MooreState const& state,
		id_type const& s1,
		id_type const& s2)
	{
		return state.outputs.at(s1) == state.outputs.at(s2);
	}

	static MooreMachine reconstruct_from_partition(
		MooreMachine const& original,
		std::vector<std::set<id_type>> const& partition)
	{
		// Создаем пустое состояние для нового, минимизированного автомата.
		MooreState minimalState;

		// 1. Создаем канонические имена ("s0", "s1", ...) и карту сопоставления.
		std::map<id_type, id_type> old_to_new_id_map;
		for (size_t i = 0; i < partition.size(); ++i)
		{
			const id_type new_id = "s" + std::to_string(i);
			minimalState.states.insert(new_id);

			for (const auto& old_id : partition[i])
			{
				old_to_new_id_map[old_id] = new_id;
			}
		}

		// 2. Определяем новое начальное состояние.
		const id_type& original_start_id = original.state().startStateId;
		minimalState.startStateId = old_to_new_id_map.at(original_start_id);
		minimalState.currentStateId = minimalState.startStateId;

		// 3. Создаем новую карту выходов.
		for (size_t i = 0; i < partition.size(); ++i)
		{
			const id_type new_id = "s" + std::to_string(i);

			// Все состояния в классе 0-эквивалентны, значит у них одинаковый выход.
			// Берем выход у любого состояния-представителя из класса.
			const id_type representative_old_id = *partition[i].begin();
			const auto& output = original.state().outputs.at(representative_old_id);

			minimalState.outputs[new_id] = output;
		}

		// 4. Создаем новые переходы.
		const auto all_inputs = get_all_inputs(original.state());
		for (size_t i = 0; i < partition.size(); ++i)
		{
			const id_type new_from_id = "s" + std::to_string(i);
			const id_type representative_old_id = *partition[i].begin();

			for (const auto& input : all_inputs)
			{
				// Находим оригинальный переход.
				const id_type& original_to_id = original.state().transitions.at({ representative_old_id, input });

				// Находим новое имя для состояния назначения.
				const id_type new_to_id = old_to_new_id_map.at(original_to_id);

				// Добавляем новый переход (без выхода, т.к. это автомат Мура).
				minimalState.transitions[{ new_from_id, input }] = new_to_id;
			}
		}

		// 5. Возвращаем новый автомат.
		return MooreMachine(std::move(minimalState));
	}
};

#endif // MOORE_MACHINE_HPP
