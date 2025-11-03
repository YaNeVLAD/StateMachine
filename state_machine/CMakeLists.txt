set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(state_machine INTERFACE
        mealy/mealy_state.hpp
        mealy/mealy_state_machine_traits.hpp
        mealy/mealy_translation_traits.hpp
        mealy/minimization.hpp
        moore/moore_state.hpp
        moore/moore_state_machine_traits.hpp
        moore/moore_translation_traits.hpp
        recognizer.hpp
        fsm.hpp
        traits/dot_traits.hpp
        recognizer.hpp
        dot.hpp
        label.hpp
        fixed_string.hpp
)

target_include_directories(state_machine INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(state_machine INTERFACE
        base_state_machine.hpp
        concepts.hpp
        traits/state_machine_traits.hpp
        converter.hpp
        default_translator.hpp
        traits/minimization_traits.hpp
        minimization.hpp
)
