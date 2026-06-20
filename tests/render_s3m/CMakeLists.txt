add_executable(render_s3m main.c)
target_link_libraries(render_s3m PRIVATE fx::core m)

set(REF_S3M  ${CMAKE_SOURCE_DIR}/tests/reference_renders/TEST.S3M)
set(OUT_WAV  ${CMAKE_CURRENT_BINARY_DIR}/out.wav)
set(REF_SHA  5edd49d54cd161dfac8c88ffbec30bf6fcadea4a2e3cd053db3dbb1c68c30834)

add_test(NAME render_s3m
    COMMAND render_s3m "${REF_S3M}" "${OUT_WAV}" 1441792)

add_test(NAME compare_s3m
    COMMAND ${CMAKE_COMMAND}
        -DFILE=${OUT_WAV}
        -DEXPECTED_SHA256=${REF_SHA}
        -P ${CMAKE_SOURCE_DIR}/cmake/check_sha256.cmake)

set_tests_properties(compare_s3m PROPERTIES DEPENDS render_s3m)
