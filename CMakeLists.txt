cmake_minimum_required(VERSION 3.31)
project(StateMachine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(state_machine)
add_subdirectory(app)