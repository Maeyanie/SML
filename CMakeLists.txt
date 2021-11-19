cmake_minimum_required(VERSION 3.10)

project(SML)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FindPkgConfig)
pkg_search_module(SPARSEHASH libsparsehash)
if(SPARSEHASH_FOUND)
	option(USE_SPARSEHASH "Use Google Sparsehash" ON)
endif()

configure_file(config.h.in config.h)

add_executable(sml2stl sml2stl.cpp crc32c.c mesh.cpp stripsearch.cpp sml.cpp stl.cpp)
target_include_directories(sml2stl PUBLIC "${PROJECT_BINARY_DIR}")

add_executable(stl2sml stl2sml.cpp crc32c.c mesh.cpp stripsearch.cpp sml.cpp stl.cpp)
target_include_directories(stl2sml PUBLIC "${PROJECT_BINARY_DIR}")