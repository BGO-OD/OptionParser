# Create pkg-config file.

# convert lists of link libraries into -lstdc++ -lm etc..
foreach(LIB ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES} ${PLATFORM_LIBS})
  set(PRIVATE_LIBS "${PRIVATE_LIBS} -l${LIB}")
endforeach()
# Produce a pkg-config file for linking against the shared lib
configure_file("${CMAKE_PROJECT_NAME}.pc.in" "${CMAKE_PROJECT_NAME}.pc" @ONLY)
install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
