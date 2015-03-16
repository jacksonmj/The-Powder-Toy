file(GLOB_RECURSE powdertests_SRC
	"*.cpp"
)

add_executable(powdertests ${powdertests_SRC})

find_path(CATCH_INCLUDE_DIR catch.hpp PATHS ../extlib/include)
if(NOT CATCH_INCLUDE_DIR)
	add_subdirectory(../lib/catch ${CMAKE_BINARY_DIR}/lib)
endif()

add_dependencies(powdertests catch)
target_include_directories(powdertests PRIVATE ${CATCH_INCLUDE_DIR})
target_link_libraries(powdertests PowderToy)

add_test(NAME PowderTests COMMAND powdertests)
