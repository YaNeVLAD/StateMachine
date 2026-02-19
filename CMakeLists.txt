cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(
        StateMachine
        VERSION "1.0.0"
        LANGUAGES CXX
)

add_library(FSM INTERFACE)
add_library(FSM::FSM ALIAS FSM)

target_include_directories(
        FSM
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(FSM INTERFACE cxx_std_23)

if (FSM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

include(GNUInstallDirs)
install(TARGETS FSM EXPORT FSMTargets)
install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT FSMTargets
        FILE FSMConfig.cmake
        NAMESPACE FSM::
        DESTINATION lib/cmake/FSM
)