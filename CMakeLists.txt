cmake_minimum_required(VERSION 3.24)
project(HardwareLimiter LANGUAGES CXX)

if(NOT WIN32)
    message(WARNING "HardwareLimiter throttling is Windows-only; non-Windows builds run in diagnostic mode.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(HardwareLimiter WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/MainWindow.cpp
    src/BenchmarkRunner.cpp
    src/HardwareInfo.cpp
    src/ProfileLoader.cpp
    src/ProfileEngine.cpp
    src/PowerThrottler.cpp
    include/MainWindow.hpp
)

target_include_directories(HardwareLimiter PRIVATE include)

target_link_libraries(HardwareLimiter PRIVATE Qt6::Widgets)

if(WIN32)
    target_link_libraries(HardwareLimiter PRIVATE
        wbemuuid
        dxgi
        d3d11
        d3dcompiler
        user32
        gdi32
        shell32
        ole32
        oleaut32
    )
endif()

add_custom_target(CopyProfiles ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/profiles.json
        $<TARGET_FILE_DIR:HardwareLimiter>/profiles.json
    COMMENT "Copying profile bundle"
)
add_dependencies(HardwareLimiter CopyProfiles)
