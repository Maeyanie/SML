cmake_minimum_required(VERSION 3.10)
include(FindPkgConfig)
include(CheckCXXSourceCompiles)

project(SML)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

pkg_search_module(SPARSEHASH libsparsehash)
if(SPARSEHASH_FOUND)
	option(USE_SPARSEHASH "Use Google Sparsehash" ON)
endif()

check_cxx_source_compiles("#include <filesystem>\nint main(void) { new std::filesystem::path(); }" HAVE_BUILTIN_FS)
if(NOT HAVE_BUILTIN_FS)
	set(CMAKE_REQUIRED_LIBRARIES "stdc++fs")
	check_cxx_source_compiles("#include <filesystem>\nint main(void) { new std::filesystem::path(); }" HAVE_STDCXXFS_FS)
	
	if(NOT HAVE_STDCXXFS_FS)
		set(CMAKE_REQUIRED_LIBRARIES "c++fs")
		check_cxx_source_compiles("#include <filesystem>\nint main(void) { new std::filesystem::path(); }" HAVE_CXXFS_FS)
	endif()
endif()

configure_file(config.h.in config.h)

add_executable(sml2stl sml2stl.cpp crc32c.c mesh.cpp stripsearch.cpp sml.cpp stl.cpp)
target_include_directories(sml2stl PUBLIC "${PROJECT_BINARY_DIR}")

add_executable(stl2sml stl2sml.cpp crc32c.c mesh.cpp stripsearch.cpp sml.cpp stl.cpp)
target_include_directories(stl2sml PUBLIC "${PROJECT_BINARY_DIR}")

add_executable(sml2obj sml2obj.cpp crc32c.c mesh.cpp stripsearch.cpp sml.cpp obj.cpp)
target_include_directories(sml2obj PUBLIC "${PROJECT_BINARY_DIR}")

add_executable(obj2sml obj2sml.cpp crc32c.c mesh.cpp stripsearch.cpp sml.cpp obj.cpp)
target_include_directories(obj2sml PUBLIC "${PROJECT_BINARY_DIR}")