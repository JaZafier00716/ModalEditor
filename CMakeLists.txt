cmake_minimum_required(VERSION 4.1)
project(ModalEditor)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_SANITIZER "Enable AddressSanitizer" OFF)

if(ENABLE_SANITIZER)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fsanitize=leak -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address -fsanitize=leak)
    endif()
    message(STATUS "AddressSanitizer enabled")
endif()

add_executable(ModalEditor main.cpp
        src/Editor.cpp
        src/TerminalManagement.cpp
        src/EditorMode.cpp
        src/CursorController.cpp
        include/Editor.h
        include/TerminalManagement.h
        include/EditorMode.h
        include/CursorController.h
)

find_program(VALGRIND_PATH valgrind)
if(VALGRIND_PATH)
    add_custom_target(memcheck
            COMMAND ${VALGRIND_PATH} --leak-check=full --show-leak-kinds=all --track-origins=yes ./ModalEditor
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running Valgrind memory check..."
            DEPENDS ModalEditor
    )
endif()