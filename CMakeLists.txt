include_directories("include")
add_library(options Options.cpp)

add_executable(optionExample optionExample.cpp)
target_link_libraries(optionExample options)
install(TARGETS optionExample DESTINATION bin)

add_executable(shellScriptOptionParser shellScriptOptionParser.cpp)
target_link_libraries(shellScriptOptionParser options)
install(TARGETS shellScriptOptionParser DESTINATION bin)
