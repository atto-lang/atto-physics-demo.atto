# Fixture-only negative contract for scripts/verify_ores_ir.cmake.
# No compiler invocation, Oreslang guest execution, or LLVM linking occurs here.
set(verifier "${CMAKE_CURRENT_LIST_DIR}/../scripts/verify_ores_ir.cmake")
set(fixture "${CMAKE_CURRENT_BINARY_DIR}/ores_physics_ir_preflight_fixture.ll")
set(valid [=[
define dso_local i64 @physics_advance_axis(i64 noundef %0, i64 %1, i64 %2, i64 %3) {
  ret i64 0
}
define i64 @physics_reflect_axis(i64 %0, i64 %1, i64 %2, i64 %3) {
  ret i64 0
}
define noundef i64 @physics_gravity(i64 %0, i64 %1, i64 %2) {
  ret i64 0
}
]=])
function(check label contents accepted)
  file(WRITE "${fixture}" "${contents}")
  execute_process(COMMAND "${CMAKE_COMMAND}" "-DORE_IR=${fixture}" -P "${verifier}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
  if(accepted)
    if(NOT result EQUAL 0)
      message(FATAL_ERROR "Accepted ABI fixture ${label} rejected: ${stderr}")
    endif()
  elseif(result EQUAL 0)
    message(FATAL_ERROR "Invalid ABI fixture ${label} unexpectedly passed")
  endif()
endfunction()
check("expected ABI" "${valid}" TRUE)
string(REPLACE "define noundef i64 @physics_gravity" "; define noundef i64 @physics_gravity" bad "${valid}")
check("commented-out definition" "${bad}" FALSE)
string(REPLACE "define i64 @physics_reflect_axis" "declare i64 @physics_reflect_axis" bad "${valid}")
check("declaration is not a definition" "${bad}" FALSE)
string(REPLACE "physics_advance_axis(i64" "physics_advance_axis(i32" bad "${valid}")
check("wrong parameter width" "${bad}" FALSE)
string(REPLACE "physics_gravity(i64 %0, i64 %1, i64 %2)" "physics_gravity(i64 %0, i64 %1)" bad "${valid}")
check("wrong parameter count" "${bad}" FALSE)
string(REPLACE "define i64 @physics_reflect_axis" "define fastcc i64 @physics_reflect_axis" bad "${valid}")
check("non-default calling convention" "${bad}" FALSE)
string(REPLACE "define i64 @physics_reflect_axis" "define internal i64 @physics_reflect_axis" bad "${valid}")
check("internal linkage" "${bad}" FALSE)
file(REMOVE "${fixture}")
message(STATUS "IR_SIGNATURE_STATIC_NO_GUEST_CONTRACT_PASS")
