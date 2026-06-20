# Usage: cmake -DFILE=<path> -DEXPECTED_SHA256=<hash> -P check_sha256.cmake
if(NOT DEFINED FILE OR NOT DEFINED EXPECTED_SHA256)
    message(FATAL_ERROR "Usage: cmake -DFILE=<path> -DEXPECTED_SHA256=<hash> -P check_sha256.cmake")
endif()
file(SHA256 "${FILE}" ACTUAL)
if(NOT ACTUAL STREQUAL EXPECTED_SHA256)
    message(FATAL_ERROR
        "SHA256 mismatch for ${FILE}\n"
        "  expected: ${EXPECTED_SHA256}\n"
        "  actual:   ${ACTUAL}")
endif()
message(STATUS "SHA256 OK: ${FILE}")
