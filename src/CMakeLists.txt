
set(LIBBASE base)
set(LIBTHIRDPARTY thirdparty)

set( DRIVER_OBJS
     driver.c
     wavfile.c
     playlist.c
   )

set(LIBDRIVER driver)
add_library(${LIBDRIVER} ${LIBTYPE} ${DRIVER_OBJS})
target_link_libraries(${LIBDRIVER} ${LIBBASE})

function(CREATE_UTIL TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.c)
    target_link_libraries(${TEST_NAME} ${LIBTHIRDPARTY} ${LIBBASE} ${LIBDRIVER} ${AAX_LIBRARY} ${XML_LIBRARY} ${KERNEL_LIBRARY} ${EXTRA_LIBS})
    set_target_properties(${TEST_NAME} PROPERTIES
            COMPILE_DEFINITIONS  "SRC_PATH=\"${PROJECT_SOURCE_DIR}/sounds\"")
endfunction()

CREATE_UTIL(aaxinfo)
install(TARGETS aaxinfo
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT Applications
)
CREATE_UTIL(aaxplay)
install(TARGETS aaxplay
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT Applications
)
CREATE_UTIL(aaxcvt)
install(TARGETS aaxcvt
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT Applications
)

CREATE_UTIL(aaxsstandardize)
install(TARGETS aaxsstandardize
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT Applications
)
