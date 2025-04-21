#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_executable(uni-klee
  main.cpp
)

set(KLEE_LIBS
  kleeCore
)

set_target_properties(uni-klee 
  PROPERTIES LINK_FLAGS "-pthread"
)


find_package(OpenSSL REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})

target_link_libraries(uni-klee 
  ${KLEE_LIBS}
  ${OPENSSL_LIBRARIES}
)

install(TARGETS uni-klee RUNTIME DESTINATION bin)

# The KLEE binary depends on the runtimes
add_dependencies(uni-klee BuildKLEERuntimes)



###Format
# add_custom_target(clang-format 
#   COMMENT "Formatting source code using scripts/clang-format.py" 
#   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} 
#   COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/clang-format.py
#   RESULT_VARIABLE CLANG_FORMAT_RESULT
#   OUTPUT_VARIABLE CLANG_FORMAT_OUTPUT
#   ERROR_VARIABLE CLANG_FORMAT_ERROR
# )

# add_dependencies(uni-klee clang-format)
# if (CLANG_FORMAT_RESULT EQUAL 0)
#   message(STATUS ${CLANG_FORMAT_OUTPUT})
# else()
#   message(STATUS ${CLANG_FORMAT_ERROR})
# endif()