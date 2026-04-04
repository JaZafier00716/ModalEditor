cmake_minimum_required(VERSION 4.1)
project(ModalEditor)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ModalEditor main.cpp
        include/Editor.h
        src/Editor.cpp
        include/TerminalManagement.cpp
        include/TerminalManagement.h
        include/EditorMode.cpp
        include/EditorMode.h)
