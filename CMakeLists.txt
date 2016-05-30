# Check if cmake has the required version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0 FATAL_ERROR)

# Set project name
PROJECT(OptionParser)

OPTION(OptionParser_BUILD_EXAMPLES "Build optionParserExample" ON)
OPTION(OptionParser_INSTALL_EXAMPLES "Install optionParserExample" OFF)
OPTION(BUILD_SHARED_LIBS "Build and install the dynamic library" ON)
OPTION(INSTALL_STATIC_LIBS "Install the static library" OFF)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

include(CheckCXXCompilerFlag)

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

# Get latest tagged version.
INCLUDE(GetLatestTag)

# Set variables for packaging. 
SET(PROJECT_DESCRIPTION "templated C++ command line option parser with executable to be used in shell scripts")
SET(PROJECT_VERSION "${${CMAKE_PROJECT_NAME}_LATEST_TAG}")

# Write and install pkg-config file. 
INCLUDE(WritePkgConfig)

# Install CMake find + external build macro. 
set(ConfigPackageLocation lib/cmake/OptionParser)
install(FILES cmake/FindOptionParser.cmake DESTINATION ${ConfigPackageLocation})

# Finally, the main compilation... 
ADD_SUBDIRECTORY(src)
