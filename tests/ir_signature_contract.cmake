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
define i64 @physics_discs_overlap(i64 %0, i64 %1, i64 %2, i64 %3, i64 %4, i64 %5) {
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
# Clang O2 may annotate a narrowed i64 return with valid range metadata;
# that changes optimization information, not symbol calling conventions.
string(REPLACE "define i64 @physics_discs_overlap"
  "define range(i64 0, 2) i64 @physics_discs_overlap" optimized "${valid}")
string(REPLACE "physics_discs_overlap(i64 %0, i64 %1, i64 %2, i64 %3, i64 %4, i64 %5) {"
  "physics_discs_overlap(i64 %0, i64 %1, i64 %2, i64 %3, i64 %4, i64 %5) local_unnamed_addr #0 {"
  optimized "${optimized}")
check("Clang O2 integer range return" "${optimized}" TRUE)
string(REPLACE "range(i64 0, 2)" "range(i32 0, 2)" bad "${optimized}")
check("incorrect range return integer width" "${bad}" FALSE)
string(REPLACE "range(i64 0, 2)" "zeroext" bad "${optimized}")
check("ABI-altering zeroext return" "${bad}" FALSE)
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
string(REPLACE "physics_discs_overlap(i64 %0, i64 %1, i64 %2, i64 %3, i64 %4, i64 %5)"
  "physics_discs_overlap(i64 %0, i64 %1, i64 %2, i64 %3, i64 %4)" bad "${valid}")
check("collision ABI wrong arity" "${bad}" FALSE)
string(REPLACE "define i64 @physics_discs_overlap" "; define i64 @physics_discs_overlap" bad "${valid}")
check("collision ABI comment only" "${bad}" FALSE)
# A signature is insufficient without an actual opening function body.
# Previously these malformed IR headers passed despite having no native code.
string(REPLACE "physics_reflect_axis(i64 %0, i64 %1, i64 %2, i64 %3) {"
  "physics_reflect_axis(i64 %0, i64 %1, i64 %2, i64 %3)" bad "${valid}")
check("bodyless define header" "${bad}" FALSE)
# Semicolons begin LLVM IR comments, so a brace after one is not a body.
string(REPLACE "physics_discs_overlap(i64 %0, i64 %1, i64 %2, i64 %3, i64 %4, i64 %5) {"
  "physics_discs_overlap(i64 %0, i64 %1, i64 %2, i64 %3, i64 %4, i64 %5) ; {"
  bad "${valid}")
check("opening brace in comment" "${bad}" FALSE)
# ABI-mutating parameter attributes are not C++ plain i64, despite i64 type.
string(REPLACE "i64 noundef %0, i64 %1" "i64 zeroext %0, i64 %1" bad "${valid}")
check("zeroext parameter ABI" "${bad}" FALSE)
string(REPLACE "i64 noundef %0, i64 %1" "i64 signext %0, i64 %1" bad "${valid}")
check("signext parameter ABI" "${bad}" FALSE)
string(REPLACE "i64 noundef %0, i64 %1" "i64 inreg %0, i64 %1" bad "${valid}")
check("inreg parameter ABI" "${bad}" FALSE)
string(REPLACE "i64 noundef %0, i64 %1" "i64 noundef inreg %0, i64 %1" bad "${valid}")
check("mixed noundef and inreg parameter ABI" "${bad}" FALSE)
# LLVM-native SSA names are allowed without affecting the ABI.
string(REPLACE "i64 noundef %0, i64 %1" "i64 noundef %physics.arg, i64 %1" named "${valid}")
check("named SSA parameter" "${named}" TRUE)
file(REMOVE "${fixture}")
message(STATUS "IR_SIGNATURE_STATIC_NO_GUEST_CONTRACT_PASS")
