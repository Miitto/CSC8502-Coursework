function(enable_warnings target)
  target_compile_options(${target} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd5050 /wd4458>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror -Wno-language-extension-token>
)
endfunction()

function(disable_warnings target)
  target_compile_options(${target} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W0>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-w>
  )
endfunction()
