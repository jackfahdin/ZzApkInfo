# 依赖版本由 Git 子模块记录，配置阶段不再下载源码。
set(ZZAPK_ZZPURETOOLS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/ZzPureTools")
if(NOT EXISTS "${ZZAPK_ZZPURETOOLS_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "ZzPureTools submodule is missing. Run: git submodule update --init --recursive")
endif()
find_package(Git REQUIRED)
execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${ZZAPK_ZZPURETOOLS_SOURCE_DIR}" rev-parse HEAD
    OUTPUT_VARIABLE ZZAPK_ZZPURETOOLS_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE zzapkRevisionResult)
if(NOT zzapkRevisionResult EQUAL 0)
    message(FATAL_ERROR "Cannot read the ZzPureTools submodule revision")
endif()

# 组件静态链接，Qt 使用共享版 SDK，发布时部署 Qt DLL。
set(BUILD_SHARED_LIBS OFF)
set(ZZ_BUILD_TESTS OFF CACHE BOOL "Disable dependency tests" FORCE)
set(ZZ_BUILD_EXAMPLES OFF CACHE BOOL "Disable dependency examples" FORCE)
set(ZZ_BUILD_BENCHMARKS OFF CACHE BOOL "Disable dependency benchmarks" FORCE)
add_subdirectory("${ZZAPK_ZZPURETOOLS_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/third_party/ZzPureTools" EXCLUDE_FROM_ALL)
