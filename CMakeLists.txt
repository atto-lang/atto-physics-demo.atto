cmake_minimum_required(VERSION 3.20)
project(oreslang_physics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(ORES_PHYSICS_GUI "Build native Qt Quick desktop frontend" ON)
option(ORES_PHYSICS_GUI_PROOF "Include opt-in graphical mouse/render proof harness" OFF)
option(ORES_PHYSICS_QUEUE_ONLY "Build only the compiler-independent snapshot ring contract" OFF)
# Clang optimization applied to the actual compiler-generated Oreslang guest IR.
# CMAKE_BUILD_TYPE alone does NOT control the native guest object.
set(ORES_PHYSICS_GUEST_OPT_LEVEL "2" CACHE STRING "Guest IR object Clang optimization: 0 or 2")
set_property(CACHE ORES_PHYSICS_GUEST_OPT_LEVEL PROPERTY STRINGS 0 2)
if(NOT ORES_PHYSICS_GUEST_OPT_LEVEL MATCHES "^(0|2)$")
  message(FATAL_ERROR "ORES_PHYSICS_GUEST_OPT_LEVEL must be exactly 0 or 2")
endif()

find_package(Threads REQUIRED)
enable_testing()
add_executable(physics_snapshot_ring_contract tests/snapshot_ring_contract.cpp)
target_include_directories(physics_snapshot_ring_contract PRIVATE src/native)
target_link_libraries(physics_snapshot_ring_contract PRIVATE Threads::Threads)
target_compile_options(physics_snapshot_ring_contract PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
add_test(NAME snapshot_ring_no_guest_contract COMMAND physics_snapshot_ring_contract)
add_executable(physics_snapshot_layout_contract tests/snapshot_layout_contract.cpp)
target_include_directories(physics_snapshot_layout_contract PRIVATE src/native)
target_compile_options(physics_snapshot_layout_contract PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
add_test(NAME snapshot_aosoa_layout_no_guest_contract COMMAND physics_snapshot_layout_contract)
add_test(NAME llvm_ir_abi_preflight_no_guest_contract
  COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/ir_signature_contract.cmake")
# Separate compiler-independent static ABI checks from real Clang IR parsing.
find_program(ORE_HOST_IR_CLANG NAMES clang)
if(ORE_HOST_IR_CLANG)
  add_test(NAME llvm_ir_native_parser_no_guest_contract
    COMMAND "${CMAKE_COMMAND}" "-DORE_TEST_CLANG=${ORE_HOST_IR_CLANG}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/ir_native_parser_contract.cmake")
endif()
add_test(NAME llvm_ir_compilation_fail_closed_no_guest_contract
  COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_failure_cleanup_contract.cmake")
add_executable(physics_numeric_envelope_contract tests/physics_domain_contract.cpp)
target_include_directories(physics_numeric_envelope_contract PRIVATE src/native)
target_compile_options(physics_numeric_envelope_contract PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
add_test(NAME physics_numeric_envelope_no_guest_contract COMMAND physics_numeric_envelope_contract)
add_test(NAME guest_optimization_config_no_guest_contract
  COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/guest_optimization_contract.cmake")
if(UNIX)
  find_program(ORE_TEST_BASH NAMES bash)
  if(ORE_TEST_BASH)
    add_test(NAME pinned_guest_runner_rejection_no_guest_contract
      COMMAND "${ORE_TEST_BASH}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/pinned_guest_runner_contract.sh")
  endif()
endif()

if(ORES_PHYSICS_QUEUE_ONLY AND ORES_PHYSICS_GUI)
  message(FATAL_ERROR "Queue-only verification requires -DORES_PHYSICS_GUI=OFF")
endif()

if(NOT ORES_PHYSICS_QUEUE_ONLY)
  find_program(ORESLANG_LLVM_COMPILER NAMES oreslang-llvmc REQUIRED)
  find_program(ORE_CLANG NAMES clang REQUIRED)

  set(ORE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/physics/physics.atto")
  set(ORE_IR "${CMAKE_CURRENT_BINARY_DIR}/native-physics.ll")
  set(ORE_OBJ "${CMAKE_CURRENT_BINARY_DIR}/native-physics.o")

  add_custom_command(OUTPUT "${ORE_IR}"
    COMMAND "${CMAKE_COMMAND}"
            "-DORE_COMPILER=${ORESLANG_LLVM_COMPILER}"
            "-DORE_CLANG=${ORE_CLANG}"
            "-DORE_SOURCE=${ORE_SOURCE}"
            "-DORE_IR=${ORE_IR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/scripts/compile_ores.cmake"
    DEPENDS "${ORE_SOURCE}" "${ORESLANG_LLVM_COMPILER}" "${ORE_CLANG}"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/compile_ores.cmake"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_ores_ir.cmake"
    VERBATIM)
  # A reconfigure changing 0 <-> 2 must invalidate an existing guest .o.
  # CMake writes this stamp only when its contents change.
  set(ORE_OPT_STAMP "${CMAKE_CURRENT_BINARY_DIR}/native-physics-opt-level.txt")
  file(GENERATE OUTPUT "${ORE_OPT_STAMP}" CONTENT "${ORES_PHYSICS_GUEST_OPT_LEVEL}\n")
  add_custom_command(OUTPUT "${ORE_OBJ}"
    COMMAND "${ORE_CLANG}" "-O${ORES_PHYSICS_GUEST_OPT_LEVEL}" -x ir -c "${ORE_IR}" -o "${ORE_OBJ}"
    DEPENDS "${ORE_IR}" "${ORE_CLANG}" "${ORE_OPT_STAMP}" VERBATIM)
  set_source_files_properties("${ORE_OBJ}" PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)

  add_library(ores_physics_core STATIC src/native/PhysicsCore.cpp)
  target_include_directories(ores_physics_core PUBLIC src/native)
  target_compile_options(ores_physics_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_library(ores_native_guest STATIC "${ORE_OBJ}")
  set_target_properties(ores_native_guest PROPERTIES LINKER_LANGUAGE CXX)

  add_executable(physics_native_contract tests/native_contract.cpp)
  target_link_libraries(physics_native_contract PRIVATE ores_physics_core ores_native_guest)
  add_test(NAME real_oreslang_llvm_physics_contract COMMAND physics_native_contract)
  add_executable(physics_spsc_contract
    tests/spsc_contract.cpp src/native/PhysicsEngine.cpp)
  target_link_libraries(physics_spsc_contract PRIVATE
    ores_physics_core ores_native_guest Threads::Threads)
  add_test(NAME bounded_spsc_physics_smoke COMMAND physics_spsc_contract)
  add_executable(physics_engine_lifecycle_contract
    tests/engine_lifecycle_contract.cpp src/native/PhysicsEngine.cpp)
  target_link_libraries(physics_engine_lifecycle_contract PRIVATE
    ores_physics_core ores_native_guest Threads::Threads)
  target_compile_options(physics_engine_lifecycle_contract PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
  add_test(NAME real_oreslang_llvm_physics_lifecycle_contract
    COMMAND physics_engine_lifecycle_contract)
  set_tests_properties(real_oreslang_llvm_physics_lifecycle_contract
    PROPERTIES TIMEOUT 80)

  if(ORES_PHYSICS_GUI)
    set(CMAKE_AUTOMOC ON)
    find_package(Qt6 6.2 REQUIRED COMPONENTS Quick Qml QuickControls2)
    qt_add_executable(oreslang_physics
      src/native/main.cpp
      src/native/PhysicsCanvas.cpp
      src/native/PhysicsCanvas.hpp
      src/native/PhysicsEngine.cpp
      src/native/PhysicsEngine.hpp)
    set_target_properties(oreslang_physics PROPERTIES MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_BUNDLE_NAME "AttoLang Physics"
      MACOSX_BUNDLE_GUI_IDENTIFIER "org.attolang.physics")
    qt_add_resources(oreslang_physics "physics_qml"
      PREFIX "/"
      FILES ui/Main.qml)
    target_link_libraries(oreslang_physics PRIVATE
      Qt6::Quick Qt6::Qml Qt6::QuickControls2 Threads::Threads
      ores_physics_core ores_native_guest)
    if(ORES_PHYSICS_GUI_PROOF)
      find_package(Qt6 REQUIRED COMPONENTS Test)
      target_sources(oreslang_physics PRIVATE tests/gui_proof.cpp)
      target_link_libraries(oreslang_physics PRIVATE Qt6::Test)
      target_compile_definitions(oreslang_physics PRIVATE ORES_PHYSICS_GUI_PROOF=1)
    endif()
  endif()
endif()
