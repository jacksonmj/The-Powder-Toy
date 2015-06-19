file(GLOB_RECURSE powdertests_SRC
	"*.cpp"
)

add_executable(powdertests ${powdertests_SRC})

find_path(CATCH_INCLUDE_DIR catch.hpp PATHS ../lib/include)
if(NOT CATCH_INCLUDE_DIR)
	set(CATCH_EXTPROJECT 1 CACHE INTERNAL "Use ExternalProject for Catch")
endif()

if(CATCH_EXTPROJECT)
	message(STATUS "Using ExternalProject for Catch")
	add_subdirectory(../cmake/lib/catch ${CMAKE_BINARY_DIR}/lib/catch)
	add_dependencies(powdertests catch)
endif()

target_include_directories(powdertests PRIVATE ${CATCH_INCLUDE_DIR})
target_link_libraries(powdertests PowderToy)

add_test(NAME PowderTests COMMAND powdertests)
