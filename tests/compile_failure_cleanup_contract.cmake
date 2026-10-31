# Negative-only host test: must not produce or reuse any guest IR.
# Deliberately exercises compiler failure and successful non-IR output.
set(compiler_script "${CMAKE_CURRENT_LIST_DIR}/../scripts/compile_ores.cmake")
set(source "${CMAKE_CURRENT_LIST_DIR}/../src/physics/physics.ores")
set(artifact "${CMAKE_CURRENT_BINARY_DIR}/physics_compile_failure_probe.ll")

function(expect_rejection label executable)
  file(WRITE "${artifact}" "STALE_FINAL_ARTIFACT_MUST_BE_REMOVED\n")
  file(WRITE "${artifact}.candidate" "STALE_CANDIDATE_MUST_BE_REMOVED\n")
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
      "-DORE_COMPILER=${executable}" "-DORE_SOURCE=${source}"
      "-DORE_IR=${artifact}" -P "${compiler_script}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err
  )
  if("${status}" STREQUAL "0")
    message(FATAL_ERROR "${label}: rejected compilation unexpectedly succeeded")
  endif()
  if(EXISTS "${artifact}" OR EXISTS "${artifact}.candidate")
    message(FATAL_ERROR "${label}: stale or rejected guest IR was left on disk")
  endif()
endfunction()

# CMake with unsupported --target=host is an intentionally failing executable,
# not a substitute Oreslang compiler.
expect_rejection("compiler nonzero exit" "${CMAKE_COMMAND}")
if(UNIX)
  find_program(ORE_TRUE_EXECUTABLE NAMES true REQUIRED)
  # 'true' exits successfully but emits no IR: ABI verifier must reject it.
  expect_rejection("invalid IR from zero-exit tool" "${ORE_TRUE_EXECUTABLE}")
endif()
message(STATUS "ORE_IR_FAIL_CLOSED_NO_GUEST_CONTRACT_PASS")
