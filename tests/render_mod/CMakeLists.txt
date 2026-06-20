add_executable(render_mod main.c)
target_link_libraries(render_mod PRIVATE fx::core m)

# TODO: add a reference MOD to tests/reference_renders/ and run:
#   tests/render-dosbox.sh --native <module.mod> <seconds>
# to generate the reference WAV, then fill in REF_MOD, REF_SHA, and MAX_FRAMES.
#
# set(REF_MOD  ${CMAKE_SOURCE_DIR}/tests/reference_renders/TEST.MOD)
# set(OUT_WAV  ${CMAKE_CURRENT_BINARY_DIR}/out.wav)
# set(REF_SHA  <sha256-of-reference-wav>)
# set(MAX_FRAMES <n>)   # 48000 * seconds * (render seconds cap)
#
# add_test(NAME render_mod
#     COMMAND render_mod "${REF_MOD}" "${OUT_WAV}" ${MAX_FRAMES})
# add_test(NAME compare_mod
#     COMMAND ${CMAKE_COMMAND}
#         -DFILE=${OUT_WAV}
#         -DEXPECTED_SHA256=${REF_SHA}
#         -P ${CMAKE_SOURCE_DIR}/cmake/check_sha256.cmake)
# set_tests_properties(compare_mod PROPERTIES DEPENDS render_mod)
