cmake_minimum_required(VERSION 3.16)
project(rtl_variant8 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(rtl_variant8
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)
target_link_libraries(rtl_variant8 PRIVATE Qt6::Widgets)
set_target_properties(rtl_variant8 PROPERTIES WIN32_EXECUTABLE TRUE)

option(BUILD_TESTING "Build simulation checks" OFF)
if(BUILD_TESTING)
    enable_testing()
    qt_add_executable(check_route
        tests/check_route.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
    )
    target_include_directories(check_route PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(check_route PRIVATE Qt6::Widgets)
    add_test(NAME route_and_rtl COMMAND check_route -platform offscreen)
endif()
