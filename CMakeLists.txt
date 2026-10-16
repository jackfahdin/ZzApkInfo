cmake_minimum_required(VERSION 3.25)
project(ZzApkInfo VERSION 1.0.0 LANGUAGES CXX)

option(ZZAPK_DEVELOPER_CHECKS "Allow non-Windows builds for development checks only" OFF)
option(ZZAPK_BUILD_APP "Build the desktop application" ON)
if(NOT WIN32 AND NOT ZZAPK_DEVELOPER_CHECKS)
    message(FATAL_ERROR "ZzApkInfo only supports Windows. Use ZZAPK_DEVELOPER_CHECKS for local tests.")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(MSVC)
    add_compile_options(/utf-8 /permissive- /Zc:__cplusplus)
endif()
include(CTest)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Concurrent)

if(ZZAPK_BUILD_APP)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Gui Widgets Svg)
    include(cmake/ZzPureTools.cmake)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${PROJECT_SOURCE_DIR}" rev-parse HEAD
        OUTPUT_VARIABLE ZZAPK_SOURCE_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
endif()

add_library(zzapk_core STATIC src/core/BadgingParser.cpp src/core/FileHasher.cpp src/core/ApkAnalyzer.cpp src/core/FileStager.cpp)
target_include_directories(zzapk_core PUBLIC src)
target_link_libraries(zzapk_core PUBLIC Qt6::Core Qt6::Concurrent)
if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
if(ZZAPK_BUILD_APP)
    add_library(zzapk_ui STATIC src/ui/ApkInfoPage.cpp src/ui/ApplicationSetup.cpp)
    target_include_directories(zzapk_ui PUBLIC src)
    target_link_libraries(zzapk_ui PUBLIC zzapk_core Zz::PureTools Zz::FluentUI Zz::WindowKit)
    qt_add_executable(ZzApkInfo WIN32 src/main.cpp resources/app.qrc)
    target_link_libraries(ZzApkInfo PRIVATE zzapk_ui)
    target_compile_definitions(ZzApkInfo PRIVATE ZZAPK_VERSION="${PROJECT_VERSION}")
    add_custom_command(TARGET ZzApkInfo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/aapt.exe"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/aapt2.exe"
            "$<TARGET_FILE_DIR:ZzApkInfo>"
    )
    if(WIN32)
        enable_language(RC)
        configure_file(cmake/version.rc.in "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)
        target_sources(ZzApkInfo PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
        get_target_property(qmakePath Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(qtBinDirectory "${qmakePath}" DIRECTORY)
        find_program(ZZAPK_DUMPBIN dumpbin REQUIRED)
        execute_process(COMMAND "${qmakePath}" -query QT_INSTALL_PLUGINS
            OUTPUT_VARIABLE ZZAPK_QT_PLUGINS_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY)
        configure_file(cmake/DeployWindows.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/DeployWindows.cmake" @ONLY)
        # 只安装本项目的 Runtime 组件，排除子模块的头文件、静态库和示例。
        install(CODE "
            execute_process(COMMAND \"${CMAKE_COMMAND}\"
                \"-DAPP=$<TARGET_FILE:ZzApkInfo>\" \"-DCONFIG=\${CMAKE_INSTALL_CONFIG_NAME}\"
                -P \"${CMAKE_CURRENT_BINARY_DIR}/DeployWindows.cmake\"
                COMMAND_ERROR_IS_FATAL ANY)
        " COMPONENT Runtime)
        install(DIRECTORY "${CMAKE_BINARY_DIR}/dist/" DESTINATION . COMPONENT Runtime)
        set(CPACK_GENERATOR ZIP)
        set(CPACK_PACKAGE_NAME "ZzApkInfo")
        set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
        set(CPACK_PACKAGE_FILE_NAME "ZzApkInfo-${PROJECT_VERSION}-windows-x64")
        set(CPACK_PACKAGE_DIRECTORY "${PROJECT_SOURCE_DIR}/dist")
        set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
        set(CPACK_BUILD_CONFIG Release)
        set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};ZzApkInfo;Runtime;/")
        configure_file(cmake/FinalizePackage.cmake.in "${CMAKE_BINARY_DIR}/FinalizePackage.cmake" @ONLY)
        set(CPACK_POST_BUILD_SCRIPTS "${CMAKE_BINARY_DIR}/FinalizePackage.cmake")
        include(CPack)
        add_custom_target(package-windows
            COMMAND "${CMAKE_CPACK_COMMAND}" --config "${CMAKE_BINARY_DIR}/CPackConfig.cmake" -C Release
            DEPENDS ZzApkInfo VERBATIM)
    endif()
    if(BUILD_TESTING)
        add_test(NAME startup COMMAND ZzApkInfo --smoke)
        set_tests_properties(startup PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
        add_executable(window_tests tests/WindowTests.cpp resources/app.qrc)
        target_compile_definitions(window_tests PRIVATE ZZAPK_VERSION="${PROJECT_VERSION}")
        target_link_libraries(window_tests PRIVATE zzapk_ui Qt6::Test)
        set_target_properties(window_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")
        add_dependencies(window_tests fake_aapt)
        add_test(NAME window COMMAND window_tests -o window-tests.txt,txt -o -,txt)
        set_tests_properties(window PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    endif()
endif()
