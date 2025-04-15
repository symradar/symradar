# set(ENV{"JSON_USE_EXCEPTION"} "0")
add_definitions(-DJSON_USE_EXCEPTION=0)

klee_add_component(kleeExternal
  jsoncpp.cpp
)