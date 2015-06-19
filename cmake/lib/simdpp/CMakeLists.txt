cmake_minimum_required(VERSION 2.8.8)
project(simdpp_builder CXX)
include(ExternalProject)
find_package(Git REQUIRED)

# installing libsimdpp is currently broken, so just add source folder to include paths

ExternalProject_Add(
	simdpp
	PREFIX ${CMAKE_BINARY_DIR}/simdpp
	GIT_REPOSITORY https://github.com/jacksonmj/libsimdpp.git
	GIT_TAG storeu
	TIMEOUT 30
	UPDATE_COMMAND ${GIT_EXECUTABLE} pull
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(simdpp source_dir)
set(SIMDPP_INCLUDE_DIR ${source_dir} CACHE INTERNAL "Path to include folder for libsimdpp")

