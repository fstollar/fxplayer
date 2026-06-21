add_executable(render main.c)
target_link_libraries(render PRIVATE fx::core m)

set(CHECK ${CMAKE_SOURCE_DIR}/cmake/check_sha256.cmake)

# ---- S3M reference (TEST.S3M) ----
set(REF_S3M ${CMAKE_SOURCE_DIR}/tests/reference_renders/TEST.S3M)
set(OUT_S3M ${CMAKE_CURRENT_BINARY_DIR}/out_s3m.wav)
set(SHA_S3M 5edd49d54cd161dfac8c88ffbec30bf6fcadea4a2e3cd053db3dbb1c68c30834)

add_test(NAME render_s3m  COMMAND render "${REF_S3M}" "${OUT_S3M}" 1441792)
add_test(NAME compare_s3m COMMAND ${CMAKE_COMMAND} -DFILE=${OUT_S3M} -DEXPECTED_SHA256=${SHA_S3M} -P ${CHECK})
set_tests_properties(compare_s3m PROPERTIES DEPENDS render_s3m)

# ---- MOD reference (hul.mod — 8-channel "8CHN" Amiga module) ----
set(REF_MOD ${CMAKE_SOURCE_DIR}/tests/reference_renders/hul.mod)
set(OUT_MOD ${CMAKE_CURRENT_BINARY_DIR}/out_mod.wav)
set(SHA_MOD dfa8b22ccc142463dc837d82bcee9411d491d6ec906c489633abfef6a80dec3a)

add_test(NAME render_mod  COMMAND render "${REF_MOD}" "${OUT_MOD}" 1441792)
add_test(NAME compare_mod COMMAND ${CMAKE_COMMAND} -DFILE=${OUT_MOD} -DEXPECTED_SHA256=${SHA_MOD} -P ${CHECK})
set_tests_properties(compare_mod PROPERTIES DEPENDS render_mod)

# ---- 669 reference (purple.669) ----
set(REF_669 ${CMAKE_SOURCE_DIR}/tests/reference_renders/purple.669)
set(OUT_669 ${CMAKE_CURRENT_BINARY_DIR}/out_669.wav)
set(SHA_669 10089bc3010836b22cd9afe89a74b27524060afb96c953fa9efbfc32a752ff5d)

add_test(NAME render_669  COMMAND render "${REF_669}" "${OUT_669}" 1441792)
add_test(NAME compare_669 COMMAND ${CMAKE_COMMAND} -DFILE=${OUT_669} -DEXPECTED_SHA256=${SHA_669} -P ${CHECK})
set_tests_properties(compare_669 PROPERTIES DEPENDS render_669)
