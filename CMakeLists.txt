cmake_minimum_required(VERSION 2.8.11)

project(powder)

# TODO: mingw windres?
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

include(FancyOptions)

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
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -ftree-vectorize -funsafe-math-optimizations")
	endif()
endif(Optimisations)

option(TargetArch "Target architecture) - sets appropriate instruction set flags (native/x86/x86-legacy/rpi/rpi2/none)" "native")
if (TargetArch STREQUAL "native")
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
	endif()
elseif (TargetArch STREQUAL "x86")
	# Reasonably widespread x86 instruction set extensions
	# Note: AMD64 has SSE and SSE2 as core instructions, so guaranteed to be present on 64 bit
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmmx -msse -msse2")
	endif()
elseif (TargetArch STREQUAL "x86-legacy")
	# x86 with no extensions, so compatible with ancient 32 bit CPUs
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i586")
elseif (TargetArch STREQUAL "rpi")
	# Raspberry pi 1 and 2
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard")
	endif()
elseif (TargetArch STREQUAL "rpi2")
	# Raspberry pi 2
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard")
	endif()
elseif (TargetArch STREQUAL "none")
	# Don't add any flags
else()
	message(WARNING "Unrecognised TargetArch")
endif()

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
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
	endif()
else(NoWarnings)
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
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

