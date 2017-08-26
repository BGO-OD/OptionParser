# Check if cmake has the required version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0 FATAL_ERROR)

# Set project name
PROJECT(OptionParser)

OPTION(OptionParser_BUILD_EXAMPLES "Build optionParserExample" ON)
OPTION(OptionParser_INSTALL_EXAMPLES "Install optionParserExample" OFF)
OPTION(BUILD_SHARED_LIBS "Build and install the dynamic library" ON)
OPTION(INSTALL_STATIC_LIBS "Install the static library" OFF)
OPTION(INSTALL_DOCUMENTATION "Also install the HTML based API documentation (you first need to make the doc-target)" OFF)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

include(GNUInstallDirs)

include(CheckCXXCompilerFlag)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)
MESSAGE("Selected build type: ${CMAKE_BUILD_TYPE}")


CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
SET(CXX_STD_FLAG "")
if(COMPILER_SUPPORTS_CXX11)
	SET(CXX_STD_FLAG "-std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	SET(CXX_STD_FLAG "-std=c++0x")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STD_FLAG}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wunused -Woverloaded-virtual -Wshadow -Wextra -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -Wvla -Wsuggest-attribute=noreturn")
# Get latest tagged version.
INCLUDE(GetLatestTag)

# Check for sensible build system
if (UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIS_NONBROKEN_SYSTEM")
endif()

# Check for timezone files
find_path(TZFILE_PATH "UTC" "/usr/share/zoneinfo" "/usr/lib/zoneinfo")
if (TZFILE_PATH) 
  message(STATUS "the time zone info files will be saerched in ${TZFILE_PATH}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZFILE_PATH='\"${TZFILE_PATH}/\"'")
endif()

# Set variables for packaging. 
SET(PROJECT_DESCRIPTION "templated C++ command line option parser with executable to be used in shell scripts")
SET(PROJECT_VERSION "${${CMAKE_PROJECT_NAME}_LATEST_TAG}")

# Write and install pkg-config file. 
INCLUDE(WritePkgConfig)

# Install CMake find + external build macro. 
set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/cmake/OptionParser)
install(FILES cmake/FindOptionParser.cmake DESTINATION ${ConfigPackageLocation})

# add a target to generate API documentation with Doxygen
find_package(Doxygen)

if(DOXYGEN_FOUND)
  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  configure_file(${doxyfile_in} ${doxyfile} @ONLY)

  add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)

  IF(INSTALL_DOCUMENTATION)
     install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION ${CMAKE_INSTALL_DOCDIR} OPTIONAL)
  ENDIF()
ELSE()
  message(STATUS "Doxygen not found, not adding 'doc' target.")
endif()


# Finally, the main compilation... 
ADD_SUBDIRECTORY(src)
