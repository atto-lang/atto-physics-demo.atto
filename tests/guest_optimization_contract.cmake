# CMake-only configuration contract; never runs or substitutes a guest compiler.
set(source_dir "${CMAKE_CURRENT_LIST_DIR}/..")
set(build_root "${CMAKE_CURRENT_BINARY_DIR}/guest_opt_config_contract")
foreach(level IN ITEMS 0 2)
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${source_dir}"
    -B "${build_root}/valid_${level}" -DORES_PHYSICS_GUI=OFF
    -DORES_PHYSICS_QUEUE_ONLY=ON "-DORES_PHYSICS_GUEST_OPT_LEVEL=${level}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT "${status}" STREQUAL "0")
    message(FATAL_ERROR "Valid O${level} configuration rejected: ${err}")
  endif()
  file(READ "${build_root}/valid_${level}/CMakeCache.txt" cache)
  if(NOT cache MATCHES "ORES_PHYSICS_GUEST_OPT_LEVEL:STRING=${level}($|\n)")
    message(FATAL_ERROR "Expected O${level} cache entry not found")
  endif()
endforeach()
foreach(invalid IN ITEMS 1 3 -O2 02)
  string(MAKE_C_IDENTIFIER "${invalid}" label)
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${source_dir}"
    -B "${build_root}/invalid_${label}" -DORES_PHYSICS_GUI=OFF
    -DORES_PHYSICS_QUEUE_ONLY=ON "-DORES_PHYSICS_GUEST_OPT_LEVEL=${invalid}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if("${status}" STREQUAL "0")
    message(FATAL_ERROR "Invalid guest optimization '${invalid}' unexpectedly accepted")
  endif()
  if(NOT err MATCHES "ORES_PHYSICS_GUEST_OPT_LEVEL must be exactly 0 or 2")
    message(FATAL_ERROR "Invalid mode '${invalid}' rejected for unexpected reason: ${err}")
  endif()
endforeach()
# Configure the REAL production CMake graph without building/running a guest.
# CMake itself is an inert placeholder to satisfy find_program; it is never
# invoked through ORE_CLANG or ORESLANG_LLVM_COMPILER in this test.
set(reconfigure_build "${build_root}/same_directory")
set(stamp "${reconfigure_build}/native-physics-opt-level.txt")
foreach(level IN ITEMS 0 2)
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${source_dir}"
    -B "${reconfigure_build}" -DORES_PHYSICS_GUI=OFF
    -DORES_PHYSICS_QUEUE_ONLY=OFF
    "-DORES_PHYSICS_GUEST_OPT_LEVEL=${level}"
    "-DORESLANG_LLVM_COMPILER=${CMAKE_COMMAND}"
    "-DORE_CLANG=${CMAKE_COMMAND}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT "${status}" STREQUAL "0")
    message(FATAL_ERROR "Production graph O${level} configuration failed: ${err}")
  endif()
  if(NOT EXISTS "${stamp}")
    message(FATAL_ERROR "Missing optimization stamp ${stamp}")
  endif()
  file(READ "${stamp}" actual)
  string(STRIP "${actual}" stripped)
  if(NOT stripped STREQUAL "${level}")
    message(FATAL_ERROR "Reconfigured O${level} stamp wrong: '${actual}'")
  endif()
endforeach()
message(STATUS "GUEST_OPT_CONFIG_NO_GUEST_CONTRACT_PASS")
