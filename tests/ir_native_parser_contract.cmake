# Host-only LLVM parser regression: never compile Oreslang or run guest physics.
# Exercise the exact CMake publication script using an IR-emitting test stub
# solely to prove malformed IR cannot become a published guest artifact.
if(NOT DEFINED ORE_TEST_CLANG OR NOT EXISTS "${ORE_TEST_CLANG}")
  message(FATAL_ERROR "A real Clang executable is required for IR parsing test")
endif()
set(root "${CMAKE_CURRENT_LIST_DIR}/..")
set(verifier "${root}/scripts/verify_ores_ir.cmake")
set(compile_script "${root}/scripts/compile_ores.cmake")
set(source "${root}/src/physics/physics.ores")
set(work "${CMAKE_CURRENT_BINARY_DIR}/ir_native_parser_negative")
file(MAKE_DIRECTORY "${work}")
set(valid [=[
define i64 @physics_advance_axis(i64 %0, i64 %1, i64 %2, i64 %3) {
  ret i64 0
}
define i64 @physics_reflect_axis(i64 %0, i64 %1, i64 %2, i64 %3) {
  ret i64 0
}
define i64 @physics_gravity(i64 %0, i64 %1, i64 %2) {
  ret i64 0
}
define i64 @physics_discs_overlap(i64 %0, i64 %1, i64 %2, i64 %3, i64 %4, i64 %5) {
  ret i64 0
}
]=])
set(valid_file "${work}/valid-reference.ll")
set(valid_obj "${work}/valid-reference.o")
file(WRITE "${valid_file}" "${valid}")
execute_process(
  COMMAND "${ORE_TEST_CLANG}" -x ir -c "${valid_file}" -o "${valid_obj}"
  RESULT_VARIABLE valid_status ERROR_VARIABLE valid_error)
if(NOT "${valid_status}" STREQUAL "0" OR NOT EXISTS "${valid_obj}")
  message(FATAL_ERROR "Real Clang failed the valid C-reference IR control: ${valid_error}")
endif()
file(REMOVE "${valid_obj}")

# This module has four plausible function headers/bodies, but an undefined SSA
# operand makes it invalid LLVM IR. The structural ABI gate must accept it,
# demonstrating why native parsing is separately mandatory.
string(REPLACE
  "define i64 @physics_gravity(i64 %0, i64 %1, i64 %2) {\n  ret i64 0"
  "define i64 @physics_gravity(i64 %0, i64 %1, i64 %2) {\n  ret i64 %undefined"
  invalid "${valid}")
if(invalid STREQUAL valid)
  message(FATAL_ERROR "Malformed fixture was not generated")
endif()
set(invalid_file "${work}/malformed-reference.ll")
file(WRITE "${invalid_file}" "${invalid}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" "-DORE_IR=${invalid_file}" -P "${verifier}"
  RESULT_VARIABLE static_status ERROR_VARIABLE static_error)
if(NOT "${static_status}" STREQUAL "0")
  message(FATAL_ERROR "Malformed fixture did not reach the native parser: ${static_error}")
endif()
set(invalid_obj "${work}/malformed-reference.o")
execute_process(
  COMMAND "${ORE_TEST_CLANG}" -x ir -c "${invalid_file}" -o "${invalid_obj}"
  RESULT_VARIABLE invalid_status ERROR_VARIABLE invalid_error)
file(REMOVE "${invalid_obj}")
if("${invalid_status}" STREQUAL "0")
  message(FATAL_ERROR "Real Clang unexpectedly accepted undefined SSA operand")
endif()

# Stub stands in for compiler stdout ONLY in a negative host-only fixture.
# The native Oreslang guest is never built or credited from this test.
set(stub "${work}/emit-malformed-reference.sh")
file(WRITE "${stub}" "#!/bin/sh\ncat '${invalid_file}'\n")
file(CHMOD "${stub}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
set(final "${work}/published.ll")
foreach(suffix "" ".candidate" ".verify-probe.o")
  file(WRITE "${final}${suffix}" "STALE_OUTPUT_MUST_BE_REMOVED\n")
endforeach()
execute_process(
  COMMAND "${CMAKE_COMMAND}" "-DORE_COMPILER=${stub}" "-DORE_CLANG=${ORE_TEST_CLANG}"
    "-DORE_SOURCE=${source}" "-DORE_IR=${final}" -P "${compile_script}"
  RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
if("${status}" STREQUAL "0" OR NOT err MATCHES "Native LLVM parse/codegen verification failed")
  message(FATAL_ERROR "Malformed IR did not fail at the native parser: ${err}")
endif()
foreach(suffix "" ".candidate" ".verify-probe.o")
  if(EXISTS "${final}${suffix}")
    message(FATAL_ERROR "Rejected guest artifact remained: ${final}${suffix}")
  endif()
endforeach()

# A zero-exit Clang substitute that emits no object must fail closed too.
find_program(ORE_TEST_TRUE NAMES true)
if(ORE_TEST_TRUE)
  set(stub_valid "${work}/emit-valid-reference.sh")
  file(WRITE "${stub_valid}" "#!/bin/sh\ncat '${valid_file}'\n")
  file(CHMOD "${stub_valid}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DORE_COMPILER=${stub_valid}"
      "-DORE_CLANG=${ORE_TEST_TRUE}" "-DORE_SOURCE=${source}"
      "-DORE_IR=${final}" -P "${compile_script}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if("${status}" STREQUAL "0" OR NOT err MATCHES "produced no object")
    message(FATAL_ERROR "No-object Clang substitute bypassed fail-closed gate: ${err}")
  endif()
  foreach(suffix "" ".candidate" ".verify-probe.o")
    if(EXISTS "${final}${suffix}")
      message(FATAL_ERROR "Zero-output verification left artifact: ${final}${suffix}")
    endif()
  endforeach()
endif()
# Successful synthetic IR is only a host publication-path positive control.
# It must never be reported as an Oreslang guest compilation or native test.
set(stub_valid "${work}/emit-valid-reference.sh")
file(WRITE "${stub_valid}" "#!/bin/sh\ncat '${valid_file}'\n")
file(CHMOD "${stub_valid}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
execute_process(
  COMMAND "${CMAKE_COMMAND}" "-DORE_COMPILER=${stub_valid}"
    "-DORE_CLANG=${ORE_TEST_CLANG}" "-DORE_SOURCE=${source}"
    "-DORE_IR=${final}" -P "${compile_script}"
  RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT "${status}" STREQUAL "0" OR NOT EXISTS "${final}")
  message(FATAL_ERROR "Valid host-only reference IR was not published: ${err}")
endif()
file(SHA256 "${valid_file}" expected_reference_sha)
file(SHA256 "${final}" published_reference_sha)
if(NOT expected_reference_sha STREQUAL published_reference_sha)
  message(FATAL_ERROR "Host-only IR publication changed the verified source")
endif()
foreach(suffix ".candidate" ".verify-probe.o")
  if(EXISTS "${final}${suffix}")
    message(FATAL_ERROR "Successful publication left a scratch artifact: ${final}${suffix}")
  endif()
endforeach()
file(REMOVE "${final}")
message(STATUS "LLVM_NATIVE_PARSER_NO_GUEST_CONTRACT_PASS")
