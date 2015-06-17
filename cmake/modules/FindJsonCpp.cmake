# - Find jsoncpp
#
#  JSONCPP_INCLUDE_DIR    - where to find gearman.h
#  JSONCPP_LIBRARIES   - libraries to link with
#  JSONCPP_FOUND       - True if found.

include (FindPackageHandleStandardArgs)

find_path(JSONCPP_INCLUDE_DIR NAMES json/json.h PATH_SUFFIXES jsoncpp PATHS ${CMAKE_SOURCE_DIR}/lib)

find_path(JSONCPP_BUILTIN_CPP NAMES jsoncpp.cpp PATHS ${CMAKE_SOURCE_DIR}/lib NO_DEFAULT_PATH)

if(JSONCPP_BUILTIN_CPP)
	message(STATUS "Using JsonCpp from lib folder")
	add_library(builtin_lib_jsoncpp ${CMAKE_SOURCE_DIR}/lib/jsoncpp.cpp)
	set(JSONCPP_LIBRARIES builtin_lib_jsoncpp)
else()
	find_library(JSONCPP_LIBRARIES jsoncpp)
endif()

find_package_handle_standard_args (JsonCpp DEFAULT_MSG JSONCPP_LIBRARIES JSONCPP_INCLUDE_DIR)
mark_as_advanced(JSONCPP_LIBRARIES JSONCPP_INCLUDE_DIR)
