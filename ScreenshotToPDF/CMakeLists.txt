cmake_minimum_required(VERSION 3.16)
project(ScreenshotToPDF VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    PrintSupport
    HttpServer
)

qt_standard_project_setup()

qt_add_executable(ScreenshotToPDF
    main.cpp
)

target_link_libraries(ScreenshotToPDF PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::HttpServer
)

qt_finalize_executable(ScreenshotToPDF)
