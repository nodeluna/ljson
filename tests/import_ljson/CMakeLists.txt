cmake_minimum_required(VERSION 3.28)

project(ljson_test_import_module LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra -pedantic -O1)

set(output_dir "${PROJECT_SOURCE_DIR}/bin")
set(mods_dir "${PROJECT_SOURCE_DIR}/../../mod")
file(GLOB_RECURSE MODS ../../mod/*.cpp)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${output_dir}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options(-fdiagnostics-color)
endif()

add_executable(${PROJECT_NAME} main.cpp) 

include_directories(${PROJECT_SOURCE_DIR}/../../include)

target_sources(${PROJECT_NAME}
	PUBLIC
	FILE_SET CXX_MODULES
	BASE_DIRS ${mods_dir}
	FILES ${MODS}
)
