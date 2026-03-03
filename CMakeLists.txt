cmake_minimum_required(VERSION 3.10)

set(PATCH_VERSION "1" CACHE STRING "Номер патч-версии")
project(bayan VERSION 0.0.${PATCH_VERSION})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -pedantic -Werror)

find_package(Boost REQUIRED COMPONENTS
    filesystem
    program_options
    regex
)

add_executable(bayan
    main.cpp
    hasher.cpp
    scanner.cpp
    duplicates.cpp
)

target_include_directories(bayan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bayan PRIVATE
    Boost::filesystem
    Boost::program_options
    Boost::regex
)
