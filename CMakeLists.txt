# Check if cmake has the required version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0 FATAL_ERROR)

# Set project name
PROJECT(OptionParser)

OPTION(OptionParser_BUILD_EXAMPLES "Build optionParserExample" ON)
OPTION(OptionParser_INSTALL_EXAMPLES "Install optionParserExample" OFF)
OPTION(BUILD_SHARED_LIBS "Build the dynamic libraries" ON)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

ADD_SUBDIRECTORY(src)

# Get latest tagged version.
find_package(Git QUIET) # present in 2.8.8
set(OPTIONPARSER_LATEST_TAG "0.0")
if(GIT_EXECUTABLE AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
	execute_process(
		COMMAND
		${GIT_EXECUTABLE} describe --tags --abbrev=0
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		OUTPUT_VARIABLE OPTIONPARSER_LATEST_TAG
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if("" STREQUAL "${OPTIONPARSER_LATEST_TAG}")
		# No tag yet...
		set(OPTIONPARSER_LATEST_TAG "0.0")
	endif()
	MESSAGE(STATUS "Latest tag: ${OPTIONPARSER_LATEST_TAG}")
endif()

# Create pkg-config file.

# convert lists of link libraries into -lstdc++ -lm etc..
foreach(LIB ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES} ${PLATFORM_LIBS})
  set(PRIVATE_LIBS "${PRIVATE_LIBS} -l${LIB}")
endforeach()
# Produce a pkg-config file for linking against the shared lib
configure_file("OptionParser.pc.in" "OptionParser.pc" @ONLY)
install(FILES       "${CMAKE_CURRENT_BINARY_DIR}/OptionParser.pc"
  DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
