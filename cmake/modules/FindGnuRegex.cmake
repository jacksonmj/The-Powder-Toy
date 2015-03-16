# - Find GnuRegex
# Find the native GnuRegex includes and library
#
#  GNUREGEX_INCLUDE_DIR    - where to find regex.h
#  GNUREGEX_LIBRARIES   - List of libraries when using GnuRegex.
#  GNUREGEX_FOUND       - True if GnuRegex found.

include (FindPackageHandleStandardArgs)

find_path (GNUREGEX_INCLUDE_DIR regex.h)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	# Windows, library needed
	find_library(GNUREGEX_LIBRARIES NAMES regex gnurx)
	find_package_handle_standard_args (GnuRegex DEFAULT_MSG GNUREGEX_LIBRARIES GNUREGEX_INCLUDE_DIR)
else(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	# library not needed
	find_package_handle_standard_args (GnuRegex DEFAULT_MSG GNUREGEX_INCLUDE_DIR)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

mark_as_advanced (GNUREGEX_LIBRARIES GNUREGEX_INCLUDE_DIR)
