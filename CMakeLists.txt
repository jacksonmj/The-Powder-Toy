cmake_minimum_required(VERSION 2.8.11)

project(powder)

# TODO: mingw windres?
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

include(FancyOptions)

if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -Wno-c++11-narrowing")
elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c++11")
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-c++11-narrowing")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()


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
	if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast")
	elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -ffast-math -ftree-vectorize -funsafe-math-optimizations")
	endif()
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -ftree-vectorize -funsafe-math-optimizations")
	endif()
endif(Optimisations)

option(OptimisationsX86 "Enable x86 specific optimisations, such as MMX and SSE" ON)
if (OptimisationsX86)
	if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
	elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmmx -msse -msse2 -DX86 -DX86_SSE2")
	endif()
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmmx -msse -msse2 -DX86 -DX86_SSE2")
	endif()
endif(OptimisationsX86)

option(CheckPartsAlloc "Enable checking of particle allocation/freeing" OFF)
if (CheckPartsAlloc)
	add_definitions(-DDEBUG_PARTSALLOC=1)
endif(CheckPartsAlloc)

option(CheckBounds "Enable extra bounds checking which should, in the absence of bugs, be unnecessary" OFF)
if (CheckBounds)
	add_definitions(-DDEBUG_BOUNDSCHECK=1)
endif(CheckBounds)

option(NoWarnings "Disable compiler warnings" OFF)
if(NoWarnings)
	if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
	elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
	endif()
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
	endif()
else(NoWarnings)
	if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
	elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
	endif()
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	endif()
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

