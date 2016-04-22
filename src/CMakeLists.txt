include_directories("include")
add_library(options Options.cpp)

add_executable(optionExample optionExample.cpp)
target_link_libraries(optionExample options)
install(TARGETS optionExample DESTINATION bin)

add_executable(shellScriptOptionParser shellScriptOptionParser.cpp)
target_link_libraries(shellScriptOptionParser options)
install(TARGETS shellScriptOptionParser DESTINATION bin)

#set(BANG ${CMAKE_INSTALL_PREFIX}/bin/)
#configure_file(shebangTest shebangTest @ONLY)
#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/shebangTest  DESTINATION bin PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_#READ GROUP_EXECUTE GROUP_READ GROUP_WRITE WORLD_READ WORLD_EXECUTE)
