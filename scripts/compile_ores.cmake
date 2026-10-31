# Fail closed: only genuine Oreslang source -> real LLVM compiler IR counts.
foreach(required ORE_COMPILER ORE_SOURCE ORE_IR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing ${required}")
  endif()
endforeach()
if(NOT EXISTS "${ORE_SOURCE}")
  message(FATAL_ERROR "Missing real Oreslang physics source: ${ORE_SOURCE}")
endif()
execute_process(
  COMMAND "${ORE_COMPILER}" --target=host "${ORE_SOURCE}"
  RESULT_VARIABLE status
  OUTPUT_FILE "${ORE_IR}"
  ERROR_VARIABLE diagnostics
)
if(NOT status EQUAL 0)
  file(REMOVE "${ORE_IR}")
  message(FATAL_ERROR "Native Oreslang LLVM compilation failed: ${diagnostics}")
endif()
file(READ "${ORE_IR}" ir)
foreach(symbol physics_advance_axis physics_reflect_axis physics_gravity)
  string(FIND "${ir}" "define i64 @${symbol}(" position)
  if(position EQUAL -1)
    file(REMOVE "${ORE_IR}")
    message(FATAL_ERROR "Native LLVM IR missing actual exported guest function ${symbol}")
  endif()
endforeach()
