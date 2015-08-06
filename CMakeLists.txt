cmake_minimum_required(VERSION 3.0)
add_executable(ttest src/main.cpp)
target_include_directories(ttest PRIVATE ${CMAKE_SOURCE_DIR}/include)
