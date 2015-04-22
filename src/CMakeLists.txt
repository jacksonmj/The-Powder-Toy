
file(GLOB libpowder_SRC
	"*.c"
	"*.cpp"
	"common/*.cpp"
	"lib/resampler/*.cpp"
	"simulation/*.cpp"
	"simulation/*/*.cpp"
)
file(GLOB main_SRC
	"main.c"
)
list(REMOVE_ITEM libpowder_SRC ${main_SRC})

set(powder_SRC "main.c")


include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckSymbolExists)


# TODO: remove this once everything is C++, this is needed because the C++ stuff interacts with stuff in .c files and not all the headers for .c files have extern "C"
set_source_files_properties(${powder_SRC} PROPERTIES LANGUAGE CXX )
set_source_files_properties(${libpowder_SRC} PROPERTIES LANGUAGE CXX )

add_library(PowderToy ${libpowder_SRC})
target_include_directories(PowderToy PUBLIC ../includes)
target_include_directories(PowderToy PUBLIC .)

add_executable(powder WIN32 ${powder_SRC})
target_link_libraries(powder PowderToy)


option(PreferStatic "Prefer linking to static libraries in most cases" ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES_ORIG ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(CMAKE_FIND_LIBRARY_SUFFIXES_ONLY_STATIC .a;)

macro(clear_package_vars VarsPrefix)
	unset(${VarsPrefix}_INCLUDE_DIR CACHE)
	unset(${VarsPrefix}_LIBRARIES CACHE)
	unset(${VarsPrefix}_LIBRARY CACHE)
	unset(${VarsPrefix}_LIBRARY_DEBUG CACHE)
	unset(${VarsPrefix}_LIBRARY_RELEASE CACHE)
endmacro(clear_package_vars)

macro(find_package_PreferStatic PackageName PackageFoundVar VarsPrefix)
	if(PreferStatic)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ONLY_STATIC})
		find_package(${PackageName})
		set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ORIG})
	endif(PreferStatic)
	if(${PackageFoundVar})
		set(${PackageFoundVar}_STATIC TRUE)
	else()
		clear_package_vars(${VarsPrefix})
		find_package(${PackageName} REQUIRED)
		if(${PackageFoundVar})
			set(${PackageFoundVar}_STATIC FALSE)
		else()
			clear_package_vars(${VarsPrefix})
		endif()
	endif()
endmacro(find_package_PreferStatic)

macro(find_optional_package_PreferStatic PackageName PackageFoundVar VarsPrefix)
	if(PreferStatic)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ONLY_STATIC})
		find_package(${PackageName})
		set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ORIG})
	endif(PreferStatic)
	if(${PackageFoundVar})
		set(${PackageFoundVar}_STATIC TRUE)
	else()
		clear_package_vars(${VarsPrefix})
		find_package(${PackageName})
		if(${PackageFoundVar})
			set(${PackageFoundVar}_STATIC FALSE)
		else()
			clear_package_vars(${VarsPrefix})
		endif()
	endif()
endmacro(find_optional_package_PreferStatic)


if(PreferStatic AND (${CMAKE_SYSTEM_NAME} MATCHES "Windows"))
	if(CMAKE_COMPILER_IS_GNUCC)
		set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -static-libstdc++ -static-libgcc")
	endif(CMAKE_COMPILER_IS_GNUCC)
	if(CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -static-libstdc++ -static-libgcc")
	endif(CMAKE_COMPILER_IS_GNUCXX)
endif(PreferStatic AND (${CMAKE_SYSTEM_NAME} MATCHES "Windows"))


find_package_PreferStatic(GnuRegex GNUREGEX_FOUND GNUREGEX)
target_include_directories(PowderToy PUBLIC ${GNUREGEX_INCLUDE_DIR})
target_link_libraries(PowderToy ${GNUREGEX_LIBRARIES})


if(PreferStatic)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ONLY_STATIC})
	find_package(SDL)
	if(SDL_FOUND)
		CHECK_LIBRARY_EXISTS("${SDL_LIBRARY}" SDL_Init "" SDL_WORKS)
		if(NOT SDL_WORKS)
			if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
				find_package(SDL_static_extra)
				if(NOT SDL_STATIC_EXTRA_LIBRARIES)
					set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ORIG})
					find_package(SDL_static_extra REQUIRED)
				endif()
			else()
				# static SDL lib is a bit broken on some distros (many unsatisfied dependencies, sorting them all out would be too much work - pulseaudio, x11, libcaca, etc)
				unset(SDL_FOUND)
				unset(SDL_LIBRARY_TEMP CACHE)
				unset(SDL_LIBRARY CACHE)
			endif()
		endif(NOT SDL_WORKS)
	endif(SDL_FOUND)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ORIG})
endif(PreferStatic)
if(NOT SDL_FOUND)
	find_package(SDL REQUIRED)
endif(NOT SDL_FOUND)

target_include_directories(PowderToy PUBLIC ${SDL_INCLUDE_DIR})
target_link_libraries(PowderToy ${SDL_LIBRARY};${SDL_STATIC_EXTRA_LIBRARIES})


find_package(Threads REQUIRED)
target_link_libraries(PowderToy ${CMAKE_THREAD_LIBS_INIT})


option(GravityFFT "Enable FFTs for Newtonian gravity (makes it faster)" ON)
if (GravityFFT)
	find_optional_package_PreferStatic(FFTW3F FFTW3F_FOUND FFTW3F)
	if (FFTW3F_FOUND)
		target_include_directories(PowderToy PUBLIC ${FFTW3F_INCLUDE_DIR})
		target_link_libraries(PowderToy ${FFTW3F_LIBRARIES})
		add_definitions(-DGRAVFFT)
	else (FFTW3F_FOUND)
		message(STATUS "Package FFTW3F not found, compiling without FFTs for Newtonian gravity")
	endif (FFTW3F_FOUND)
endif (GravityFFT)


# lm

find_package_PreferStatic(BZip2 BZIP2_FOUND BZIP2)
target_include_directories(PowderToy PUBLIC ${BZIP2_INCLUDE_DIR})
target_link_libraries(PowderToy ${BZIP2_LIBRARIES})


option(LuaConsole "Enable Lua console" ON)
if(LuaConsole)
	if(PreferStatic AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		# For Linux, find maths library first, because otherwise find_package(Lua51) will fail to find the static Lua library if there isn't a static libm
		find_library(LUA_MATH_LIBRARY m)
		# Also need libdl
		find_library(LUA_EXTRA_LIBS dl)
		if(LUA_EXTRA_LIBS STREQUAL "LUA_EXTRA_LIBS-NOTFOUND")
			message(STATUS "Could not find libdl, trying dynamic lua51")
			find_package(Lua51)
		else()
			find_optional_package_PreferStatic(Lua51 LUA51_FOUND LUA)
		endif()
	else()
		find_optional_package_PreferStatic(Lua51 LUA51_FOUND LUA)
	endif()

	if(LUA51_FOUND)
		target_include_directories(PowderToy PUBLIC ${LUA_INCLUDE_DIR})
		target_link_libraries(PowderToy ${LUA_LIBRARIES};${LUA_EXTRA_LIBS})
		add_definitions(-DLUACONSOLE)
	else()
		message(STATUS "Package lua51 not found, compiling without Lua console")
	endif()
endif(LuaConsole)


# X11 clipboard
set(CMAKE_REQUIRED_INCLUDES ${SDL_INCLUDE_DIR})
CHECK_SYMBOL_EXISTS(SDL_VIDEO_DRIVER_X11 SDL/SDL.h SDL_IS_X11)
if(SDL_IS_X11)
	find_package(X11)
	if(X11_FOUND)
		message(STATUS "X11 clipboard enabled")
		target_include_directories(PowderToy PUBLIC ${X11_X11_INCLUDE_PATH})
		target_link_libraries(PowderToy ${X11_X11_LIB})
		add_definitions(-DCLIPBOARD_X11=1)
	endif()
endif(SDL_IS_X11)


if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	find_package(ClockGettime REQUIRED)
	target_link_libraries(PowderToy ${CLOCK_GETTIME_LIBRARIES})
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# Winsock
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	find_package_PreferStatic(Winsock WINSOCK_FOUND WINSOCK)
	target_include_directories(PowderToy PUBLIC ${WINSOCK_INCLUDE_DIR})
	target_link_libraries(PowderToy ${WINSOCK_LIBRARIES})
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")



