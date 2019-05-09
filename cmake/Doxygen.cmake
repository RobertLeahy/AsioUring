if(DOXYGEN_FOUND)
  if(DOXYGEN_DOT_FOUND)
    set(DOXYGEN_CONFIGURE_HAVE_DOT "HAVE_DOT=YES")
    get_filename_component(DOXYGEN_DOT_PATH "${DOXYGEN_DOT_EXECUTABLE}" DIRECTORY)
    file(TO_NATIVE_PATH "${DOXYGEN_DOT_PATH}" DOXYGEN_DOT_PATH)
    set(DOXYGEN_CONFIGURE_DOT_PATH "DOT_PATH=${DOXYGEN_DOT_PATH}")
  else()
    set(DOXYGEN_CONFIGURE_HAVE_DOT "HAVE_DOT=NO")
  endif()
  configure_file(Doxyfile.in "${CMAKE_BINARY_DIR}/Doxyfile")
  add_custom_target(doc
                    ${DOXYGEN_EXECUTABLE}
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                    COMMENT "Running Doxygen" VERBATIM)
  install(CODE "execute_process(COMMAND ${DOXYGEN_EXECUTABLE} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})")
  install(DIRECTORY "${CMAKE_BINARY_DIR}/doc/html/"
          DESTINATION "${CMAKE_INSTALL_DOCDIR}")
endif()
