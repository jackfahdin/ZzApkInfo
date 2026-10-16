add_executable(core_tests CoreTests.cpp)
target_link_libraries(core_tests PRIVATE zzapk_core Qt6::Test)
add_test(NAME core COMMAND core_tests)
