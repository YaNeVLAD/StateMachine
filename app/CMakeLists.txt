set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(my_app main.cpp
        converters/MealyToMoore.hpp
        converters/MooreToMealy.hpp
        printers/MealyToDot.hpp
        printers/MooreToDot.hpp
        readers/MealyFromDot.hpp
        readers/MooreFromDot.hpp
        StringUtils.hpp)

add_custom_command(
        TARGET my_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res"
        "$<TARGET_FILE_DIR:my_app>/res"
        COMMENT "Copying resources..."
)

target_link_libraries(my_app PRIVATE state_machine)