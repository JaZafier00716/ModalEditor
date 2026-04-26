cmake_minimum_required(VERSION 4.1)
project(ModalEditor)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_SANITIZER "Enable AddressSanitizer" OFF)

# --- Memory Testing Options ---
if(ENABLE_SANITIZER)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fsanitize=leak -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address -fsanitize=leak)
    endif()
    message(STATUS "AddressSanitizer enabled")
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

# CONFIGURE_DEPENDS ensures CMake re-scans the folders if you add/remove files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "main.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "include/*.h")

# --- Executable Definition ---
# Note: Including HEADERS here makes them show up nicely in IDEs (like CLion or VS)
add_executable(ModalEditor ${SOURCES} ${HEADERS})

# This tells the compiler where to look for #include "File.h"
target_include_directories(ModalEditor PUBLIC include)

find_program(VALGRIND_PATH valgrind)
if(VALGRIND_PATH)
    add_custom_target(memcheck
            COMMAND ${VALGRIND_PATH} --leak-check=full --show-leak-kinds=all --track-origins=yes ./ModalEditor
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running Valgrind memory check..."
            DEPENDS ModalEditor
    )
endif()