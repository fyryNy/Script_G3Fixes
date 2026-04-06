add_library(Script_G3Fixes SHARED)

target_link_libraries(Script_G3Fixes PRIVATE g3sdk::SDK)

target_gothic_script_options(Script_G3Fixes)

target_tree_sources(Script_G3Fixes
  PRIVATE
    Script_G3Fixes.cpp
    Script_G3Fixes.h
    resource.h
)