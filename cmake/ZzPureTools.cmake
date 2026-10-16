include(FetchContent)
# 固定源码提交；本地可用 FETCHCONTENT_SOURCE_DIR_ZZPURETOOLS 指向同版本源码。
# 组件静态链接，Qt 仍使用官方共享版 SDK，发布时只需部署 Qt DLL。
set(BUILD_SHARED_LIBS OFF)
set(ZZ_BUILD_TESTS OFF CACHE BOOL "Disable dependency tests" FORCE)
set(ZZ_BUILD_EXAMPLES OFF CACHE BOOL "Disable dependency examples" FORCE)
set(ZZ_BUILD_BENCHMARKS OFF CACHE BOOL "Disable dependency benchmarks" FORCE)
FetchContent_Declare(zzpuretools
    URL https://github.com/jackfahdin/ZzPureTools/archive/9077bbebd972a5a1dc1e7115a6a5b23f0132af20.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
FetchContent_MakeAvailable(zzpuretools)
