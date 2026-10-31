# Static LLVM IR ABI preflight. This is not compiler or guest execution proof.
if(NOT DEFINED ORE_IR OR "${ORE_IR}" STREQUAL "" OR NOT EXISTS "${ORE_IR}")
  message(FATAL_ERROR "Missing compiler-produced LLVM IR file: ${ORE_IR}")
endif()
file(READ "${ORE_IR}" ir)

# The C++ caller requires unmangled C-ABI functions with signed i64 returns
# and the specified count of i64 arguments. In particular, do not accept a
# mere substring in a comment, a declaration, or a non-default calling convention.
foreach(signature IN ITEMS "physics_advance_axis|4" "physics_reflect_axis|4" "physics_gravity|3" "physics_discs_overlap|6")
  string(REPLACE "|" ";" parts "${signature}")
  list(GET parts 0 symbol)
  list(GET parts 1 arity)
  # The signature must open a function body; a bodyless 'define' header
  # is not executable LLVM IR. Exclude braces that occur in IR comments.
  # Permit ABI-neutral i64 range return metadata (seen at Clang O2),
  # LLVM function attributes and a newline before the body opener.
  # Never permit internal linkage or non-default calling conventions.
  set(pattern "(^|\n)[ \t]*define[ \t]+(dso_local[ \t]+)?(noundef[ \t]+)?(range\\(i64[ \t]+-?[0-9]+,[ \t]*-?[0-9]+\\)[ \t]+)?i64[ \t]+@${symbol}\\(([^\r\n)]*)\\)[^\r\n;{}]*(\r?\n[ \t]*)?\\{")
  string(REGEX MATCH "${pattern}" definition "${ir}")
  if(definition STREQUAL "")
    message(FATAL_ERROR "LLVM IR missing externally callable default-ABI i64 definition of ${symbol}")
  endif()
  set(argtext "${CMAKE_MATCH_5}")
  string(REPLACE "," ";" args "${argtext}")
  list(LENGTH args found_arity)
  if(NOT found_arity EQUAL arity)
    message(FATAL_ERROR "LLVM IR ${symbol} arity ${found_arity}, expected ${arity}")
  endif()
  foreach(arg IN LISTS args)
    # The native C++ caller passes plain i64 values. ABI-affecting parameter
    # attributes (zeroext/signext/inreg/etc.) must not pass on type alone.
    # Accept only bare i64, optional noundef, and an optional SSA name.
    if(NOT arg MATCHES "^[ \t]*i64([ \t]+noundef)?([ \t]+%[A-Za-z0-9_.$-]+)?[ \t]*$")
      message(FATAL_ERROR "LLVM IR ${symbol} has a non-C-ABI i64 parameter: ${arg}")
    endif()
  endforeach()
endforeach()
