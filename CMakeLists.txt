cmake_minimum_required(VERSION 2.8.11)

project(powder)

# TODO: mingw windres?
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

include(FancyOptions)

if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c++11")
endif(CMAKE_COMPILER_IS_GNUCC)
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif(CMAKE_COMPILER_IS_GNUCXX)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	add_definitions(-DLIN32)
	add_definitions(-DLINUX)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	add_definitions(-DWIN32)
	add_definitions(-DWINDOWS)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_definitions(-DMACOSX)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


option(Optimisations "Enable optimisations" ON)
if (Optimisations)
	if(CMAKE_COMPILER_IS_GNUCC)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -ffast-math -ftree-vectorize -funsafe-math-optimizations")
	endif(CMAKE_COMPILER_IS_GNUCC)
	if(CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -ftree-vectorize -funsafe-math-optimizations")
	endif(CMAKE_COMPILER_IS_GNUCXX)
endif(Optimisations)

option(OptimisationsX86 "Enable x86 specific optimisations, such as MMX and SSE" ON)
if (OptimisationsX86)
	if(CMAKE_COMPILER_IS_GNUCC)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmmx -msse -msse2 -DX86 -DX86_SSE2")
	endif(CMAKE_COMPILER_IS_GNUCC)
	if(CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmmx -msse -msse2 -DX86 -DX86_SSE2")
	endif(CMAKE_COMPILER_IS_GNUCXX)
endif(OptimisationsX86)

option(CheckPartsAlloc "Enable checking of particle allocation/freeing" OFF)
if (CheckPartsAlloc)
	add_definitions(-DDEBUG_PARTSALLOC=1)
endif(CheckPartsAlloc)

option(NoWarnings "Disable compiler warnings" OFF)
if (NoWarnings)
	if(CMAKE_COMPILER_IS_GNUCC)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
	endif(CMAKE_COMPILER_IS_GNUCC)
	if(CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
	endif(CMAKE_COMPILER_IS_GNUCXX)
else(NoWarnings)
	if(CMAKE_COMPILER_IS_GNUCC)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-narrowing")
	endif(CMAKE_COMPILER_IS_GNUCC)
	if(CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-narrowing")
	endif(CMAKE_COMPILER_IS_GNUCXX)
endif(NoWarnings)


if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

add_subdirectory(src)



fuzzyoption(Tests "Enable tests (on/off/maybe, maybe=only if test framework is already installed)" "maybe")

if(${Tests} STREQUAL "maybe")
	find_path(CATCH_ALREADY_INSTALLED catch.hpp PATHS extlib/include)
	if(CATCH_ALREADY_INSTALLED)
		set(Tests true)
	else()
		set(Tests false)
	endif()
	unset(CATCH_ALREADY_INSTALLED CACHE)
endif()

if(Tests)
	message(STATUS "Tests enabled")
	add_subdirectory(tests)
	enable_testing()
else()
	message(STATUS "Tests disabled")
endif()

