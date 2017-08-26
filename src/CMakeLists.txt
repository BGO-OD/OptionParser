include_directories("include")
add_library(options_static STATIC Options.cpp OptionsChrono.cpp OptionsWithForeignParser.cpp OptionsForTApplication.cpp)
set_target_properties(options_static PROPERTIES OUTPUT_NAME options)
IF(INSTALL_STATIC_LIBS)
	install(TARGETS options_static DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDIF(INSTALL_STATIC_LIBS)
IF(BUILD_SHARED_LIBS)
	add_library(options SHARED Options.cpp OptionsChrono.cpp)
	install(TARGETS options DESTINATION ${CMAKE_INSTALL_LIBDIR})
	set_property(TARGET options PROPERTY VERSION ${PROJECT_VERSION})
ENDIF(BUILD_SHARED_LIBS)
INSTALL(
	FILES
	include/Options.h include/OptionsChrono.h include/OptionsWithForeignParser.h include/OptionsForTApplication.h
	DESTINATION
	${CMAKE_INSTALL_INCLUDEDIR})

IF(OptionParser_BUILD_EXAMPLES)
	add_executable(optionExample optionExample.cpp)
	target_link_libraries(optionExample options_static)
ENDIF(OptionParser_BUILD_EXAMPLES)
IF(OptionParser_BUILD_EXAMPLES AND OptionParser_INSTALL_EXAMPLES)
	install(TARGETS optionExample DESTINATION ${CMAKE_INSTALL_BINDIR})
ENDIF(OptionParser_BUILD_EXAMPLES AND OptionParser_INSTALL_EXAMPLES)

add_executable(shellScriptOptionParser shellScriptOptionParser.cpp)
target_link_libraries(shellScriptOptionParser options_static)
install(TARGETS shellScriptOptionParser DESTINATION ${CMAKE_INSTALL_BINDIR})

IF(OptionParser_INSTALL_EXAMPLES)
	install(FILES shellScriptExample DESTINATION ${CMAKE_INSTALL_BINDIR})
ENDIF(OptionParser_INSTALL_EXAMPLES)

#set(BANG ${CMAKE_INSTALL_PREFIX}/bin/)
#configure_file(shebangTest shebangTest @ONLY)
#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/shebangTest  DESTINATION ${CMAKE_INSTALL_BINDIR} PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_#READ GROUP_EXECUTE GROUP_READ GROUP_WRITE WORLD_READ WORLD_EXECUTE)
