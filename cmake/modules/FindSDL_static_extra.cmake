# Find some extra libraries needed when linking SDL statically on Windows
#
#  SDL_STATIC_EXTRA_LIBRARIES   - List of libraries to link
#  SDL_STATIC_EXTRA_FOUND       - True if all the necessary libraries were found

include(FindPackageHandleStandardArgs)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	find_library(SDL_STATIC_EXTRA_LIBRARY_DXGUID NAMES dxguid
		PATHS
		C:/MinGW/lib/)
	find_library(SDL_STATIC_EXTRA_LIBRARY_WINMM NAMES winmm
		PATHS
		C:/MinGW/lib/)
	if(SDL_STATIC_EXTRA_LIBRARY_DXGUID AND SDL_STATIC_EXTRA_LIBRARY_WINMM)
		set(SDL_STATIC_EXTRA_LIBRARIES "${SDL_STATIC_EXTRA_LIBRARY_DXGUID};${SDL_STATIC_EXTRA_LIBRARY_WINMM}")
	endif()
	find_package_handle_standard_args (SDL_static_extra DEFAULT_MSG SDL_STATIC_EXTRA_LIBRARY_DXGUID SDL_STATIC_EXTRA_LIBRARY_WINMM SDL_STATIC_EXTRA_LIBRARIES)
else(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	# Do nothing if not on Windows
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

mark_as_advanced(SDL_STATIC_EXTRA_LIBRARY_DXGUID SDL_STATIC_EXTRA_LIBRARY_WINMM SDL_STATIC_EXTRA_LIBRARIES)
