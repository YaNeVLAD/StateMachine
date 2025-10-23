include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
if (WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif ()
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(fsm-tests fsm-tests.cpp)

add_custom_command(
        TARGET fsm-tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res"
        "$<TARGET_FILE_DIR:fsm-tests>/res"
        COMMENT "Copying resources..."
)

target_link_libraries(
        fsm-tests PRIVATE
        GTest::gtest_main
        state_machine
)
include(GoogleTest)

gtest_discover_tests(fsm-tests)