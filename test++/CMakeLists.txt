
set(LIBBASE base)
set(LIBDRIVER driver)

FUNCTION(CREATE_CPP_TEST TEST_NAME)
    ADD_EXECUTABLE(${TEST_NAME} ${TEST_NAME}.cpp)
    TARGET_LINK_LIBRARIES(${TEST_NAME} ${LIBDRIVER} ${LIBBASE} ${AAX_LIBRARY} ${EXTRA_LIBS} ${XML_LIBRARY})
    SET_TARGET_PROPERTIES(${TEST_NAME} PROPERTIES
            COMPILE_DEFINITIONS  "SRC_PATH=\"${PROJECT_SOURCE_DIR}/sounds\"")
ENDFUNCTION()


CREATE_CPP_TEST(aaxinfo++)
CREATE_CPP_TEST(aaxplay++)
CREATE_CPP_TEST(testplay++)
CREATE_CPP_TEST(testinit++)
CREATE_CPP_TEST(testmono3d++)
CREATE_CPP_TEST(testmath++)
CREATE_CPP_TEST(teststream++)
CREATE_CPP_TEST(testtying++)
CREATE_CPP_TEST(testmusicstream++)
CREATE_CPP_TEST(testdistortion_frame++)
CREATE_CPP_TEST(testmono3d_reverb++)
CREATE_CPP_TEST(testmono3d_frame_reverb++)
