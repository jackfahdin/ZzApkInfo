include(FetchContent)
# 固定源码提交；本地可用 FETCHCONTENT_SOURCE_DIR_ZZPURETOOLS 指向同版本源码。
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build bundled framework statically")
set(ZZ_BUILD_TESTS OFF CACHE BOOL "Disable dependency tests" FORCE)
set(ZZ_BUILD_EXAMPLES OFF CACHE BOOL "Disable dependency examples" FORCE)
set(ZZ_BUILD_BENCHMARKS OFF CACHE BOOL "Disable dependency benchmarks" FORCE)
FetchContent_Declare(zzpuretools
    URL https://github.com/jackfahdin/ZzPureTools/archive/9077bbebd972a5a1dc1e7115a6a5b23f0132af20.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
FetchContent_MakeAvailable(zzpuretools)
