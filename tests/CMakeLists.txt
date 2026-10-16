add_executable(core_tests CoreTests.cpp)
target_link_libraries(core_tests PRIVATE zzapk_core Qt6::Test)
add_test(NAME core COMMAND core_tests)
add_executable(fake_aapt FakeAapt.cpp)
target_link_libraries(fake_aapt PRIVATE Qt6::Core)
# 独立测试目录，绝不覆盖交付的 Android 工具。
set_target_properties(fake_aapt PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin" OUTPUT_NAME aapt SUFFIX .exe)
add_custom_command(TARGET fake_aapt POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "$<TARGET_FILE:fake_aapt>" "$<TARGET_FILE_DIR:fake_aapt>/aapt2.exe")
add_executable(analyzer_tests AnalyzerTests.cpp)
target_link_libraries(analyzer_tests PRIVATE zzapk_core Qt6::Test)
set_target_properties(analyzer_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
add_dependencies(analyzer_tests fake_aapt)
add_test(NAME analyzer COMMAND analyzer_tests)
