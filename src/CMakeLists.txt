include_directories("include")
<<<<<<< HEAD
add_library(options Options.cpp)

add_executable(optionExample optionExample.cpp)
target_link_libraries(optionExample options)
install(TARGETS optionExample DESTINATION bin)

add_executable(shellScriptOptionParser shellScriptOptionParser.cpp)
target_link_libraries(shellScriptOptionParser options)
install(TARGETS shellScriptOptionParser DESTINATION bin)

=======
add_library(optionsBuild STATIC Options.cpp)
IF(BUILD_SHARED_LIBS)
	add_library(options SHARED Options.cpp)
	install(TARGETS options DESTINATION lib)
ENDIF(BUILD_SHARED_LIBS)
install(FILES include/Options.h DESTINATION include)

IF(OptionParser_BUILD_EXAMPLES)
	add_executable(optionExample optionExample.cpp)
	target_link_libraries(optionExample optionsBuild)
ENDIF(OptionParser_BUILD_EXAMPLES)
IF(OptionParser_BUILD_EXAMPLES AND OptionParser_INSTALL_EXAMPLES)
	install(TARGETS optionExample DESTINATION bin)
ENDIF(OptionParser_BUILD_EXAMPLES AND OptionParser_INSTALL_EXAMPLES)

add_executable(shellScriptOptionParser shellScriptOptionParser.cpp)
target_link_libraries(shellScriptOptionParser optionsBuild)
install(TARGETS shellScriptOptionParser DESTINATION bin)

IF(OptionParser_INSTALL_EXAMPLES)
	install(FILES shellScriptExample DESTINATION bin)
ENDIF(OptionParser_INSTALL_EXAMPLES)

>>>>>>> a785e29573b3b2fc1f4b0315f6b77fbaa4787504
#set(BANG ${CMAKE_INSTALL_PREFIX}/bin/)
#configure_file(shebangTest shebangTest @ONLY)
#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/shebangTest  DESTINATION bin PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_#READ GROUP_EXECUTE GROUP_READ GROUP_WRITE WORLD_READ WORLD_EXECUTE)
