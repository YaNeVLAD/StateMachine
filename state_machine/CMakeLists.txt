add_library(state_machine STATIC
        src/state_machine.cpp

        include/base_state_machine.hpp
        include/state_machine_concept.hpp
        include/state_machine_traits.hpp
        include/converter.hpp
)

target_include_directories(state_machine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)