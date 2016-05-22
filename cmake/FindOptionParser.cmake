# - Try to find liboptionparser
# Once done this will define
#  OptionParser_FOUND - System has OptionParser
#  OptionParser_INCLUDE_DIRS - The OptionParser include directories
#  OptionParser_LIBRARIES - The libraries needed to use OptionParser

find_package(PkgConfig)
pkg_check_modules(PC_OptionParser QUIET OptionParser)

find_path(OptionParser_INCLUDE_DIR Options.h
  HINTS ${PC_OptionParser_INCLUDEDIR} ${PC_OptionParser_INCLUDE_DIRS})

find_library(OptionParser_LIBRARY NAMES options
  HINTS ${PC_OptionParser_LIBDIR} ${PC_OptionParser_LIBRARY_DIRS} )

if(OptionParser_FIND_REQUIRED AND "${OptionParser_LIBRARY}" STREQUAL "OptionParser_LIBRARY-NOTFOUND" AND "${OptionParser_INCLUDE_DIR}" STREQUAL "OptionParser_INCLUDE_DIR-NOTFOUND")
	# We have to find it, but did not.
	if(OptionParser_FIND_REQUIRED_MAYBEBUILTIN)
		# User requested fallback to builtin.
		INCLUDE(ExternalProject)

		ExternalProject_Add(
			OptionParser
			GIT_REPOSITORY https://github.com/BGO-OD/OptionParser.git
			CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
			-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
			-DINSTALL_STATIC_LIBS=ON
			LOG_BUILD 1 LOG_CONFIGURE 1 LOG_DOWNLOAD 1 LOG_INSTALL 1
			)
		ExternalProject_Get_Property(OptionParser INSTALL_DIR)
		set(OptionParser_INCLUDE_DIR ${INSTALL_DIR}/include/)
		set(OptionParser_LIBRARY ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}options_static${CMAKE_STATIC_LIBRARY_SUFFIX})
	endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set OptionParser_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(OptionParser  "Could NOT find OptionParser. Install it, or specify the component MAYBEBUILTIN to find_package to allow for builtin-build."
                                  OptionParser_LIBRARY OptionParser_INCLUDE_DIR)

mark_as_advanced(OptionParser_INCLUDE_DIR OptionParser_LIBRARY)

set(OptionParser_LIBRARIES ${OptionParser_LIBRARY})
set(OptionParser_INCLUDE_DIRS ${OptionParser_INCLUDE_DIR})
