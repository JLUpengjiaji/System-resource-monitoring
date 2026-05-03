cmake_minimum_required(VERSION 3.16)

project(SystemMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

set(SOURCES
    src/main.cpp
    src/SystemMonitor.cpp
    src/SystemMonitor.h
    src/SystemInfo.cpp
    src/SystemInfo.h
    src/ConfigManager.cpp
    src/ConfigManager.h
    src/TrendChart.cpp
    src/TrendChart.h
    src/ProcessList.cpp
    src/ProcessList.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
)

add_executable(SystemMonitor ${SOURCES})

target_link_libraries(SystemMonitor PRIVATE
    Qt5::Core
    Qt5::Widgets
)

if(WIN32)
    target_link_libraries(SystemMonitor PRIVATE
        pdh
        psapi
        iphlpapi
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(SystemMonitor PRIVATE
        procps
    )
endif()

set_target_properties(SystemMonitor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(MSVC)
    set_target_properties(SystemMonitor PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
