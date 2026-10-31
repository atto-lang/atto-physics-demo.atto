# Fail closed: publish only compiler-produced LLVM IR that passed ABI verification.
foreach(required ORE_COMPILER ORE_SOURCE ORE_IR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing ${required}")
  endif()
endforeach()
if(NOT EXISTS "${ORE_SOURCE}")
  message(FATAL_ERROR "Missing real Oreslang physics source: ${ORE_SOURCE}")
endif()
get_filename_component(ore_source_absolute "${ORE_SOURCE}" REALPATH)
get_filename_component(ore_ir_absolute "${ORE_IR}" ABSOLUTE)
if(ore_source_absolute STREQUAL ore_ir_absolute)
  message(FATAL_ERROR "Refusing to overwrite the Oreslang physics source with generated LLVM IR")
endif()

# Never leave an old or unverified final IR output behind on failure.
# The temporary file is in the same directory for an atomic rename on success.
set(ore_candidate "${ORE_IR}.candidate")
file(REMOVE "${ORE_IR}" "${ore_candidate}")
execute_process(
  COMMAND "${ORE_COMPILER}" --target=host "${ORE_SOURCE}"
  RESULT_VARIABLE status
  OUTPUT_FILE "${ore_candidate}"
  ERROR_VARIABLE diagnostics
)
if(NOT "${status}" STREQUAL "0")
  file(REMOVE "${ORE_IR}" "${ore_candidate}")
  message(FATAL_ERROR "Native Oreslang LLVM compilation failed: ${diagnostics} (${status})")
endif()

# Invoke the verifier out of process so FATAL_ERROR cannot bypass cleanup.
execute_process(
  COMMAND "${CMAKE_COMMAND}" "-DORE_IR=${ore_candidate}"
          -P "${CMAKE_CURRENT_LIST_DIR}/verify_ores_ir.cmake"
  RESULT_VARIABLE verify_status
  OUTPUT_VARIABLE verify_output
  ERROR_VARIABLE verify_error
)
if(NOT "${verify_status}" STREQUAL "0")
  file(REMOVE "${ORE_IR}" "${ore_candidate}")
  message(FATAL_ERROR "Oreslang LLVM IR ABI verification failed: ${verify_error} (${verify_status})")
endif()
file(RENAME "${ore_candidate}" "${ORE_IR}")
