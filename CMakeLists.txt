cmake_minimum_required(VERSION 3.24)
project(nvc
        VERSION 0.1.0
        LANGUAGES C)
# C99 ISO standard
set(CMAKE_C_STANDARD 11)
# Require standard
set(CMAKE_C_STANDARD_REQUIRED ON)
# set release build
set(CMAKE_BUILD_TYPE Release)
# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# create executable from src/
include_directories("${PROJECT_SOURCE_DIR}/include")
add_executable(${PROJECT_NAME}
        include/nvc_compiler.h
        include/nvc_ast.h
        include/nvc_lexer.h
        src/nvc_compiler.c
        src/main.c
        src/nvc_lexer.c
        src/nvc_ast.c
        include/nvc_output.h
        src/nvc_output.c)