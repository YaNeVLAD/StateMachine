add_library(state_machine INTERFACE
        include/mealy/mealy_state.hpp
        include/mealy/mealy_state_machine_traits.hpp
        include/mealy/mealy_translation_traits.hpp
        include/mealy/minimization.hpp
        include/moore/moore_state.hpp
        include/moore/moore_state_machine_traits.hpp
        include/moore/moore_translation_traits.hpp
        include/recognizer.hpp
        include/fsm.hpp
)

target_include_directories(state_machine INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(state_machine INTERFACE
        include/base_state_machine.hpp
        include/concepts.hpp
        include/traits/state_machine_traits.hpp
        include/converter.hpp
        include/default_translator.hpp
        include/traits/minimization_traits.hpp
        include/minimization.hpp
)
