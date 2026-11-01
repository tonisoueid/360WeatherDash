cmake_minimum_required(VERSION 3.21)
project(360WeatherDash VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(360WeatherDash
    src/main.cpp
    src/Theme.h
    src/AppIcon.h
    src/StationData.h
    src/StationRegistry.h
    src/StationRegistry.cpp
    src/WeatherService.h
    src/WeatherService.cpp
    src/widgets/Panel.h
    src/widgets/Panel.cpp
    src/widgets/WeatherGlyph.h
    src/widgets/ExpandButton.h
    src/widgets/BodySplitter.h
    src/widgets/TrendChart.h
    src/widgets/TrendChart.cpp
    src/StationCard.h
    src/StationCard.cpp
    src/EventLog.h
    src/EventLog.cpp
    src/AdminDialog.h
    src/AdminDialog.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

target_include_directories(360WeatherDash PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(360WeatherDash PRIVATE Qt6::Widgets Qt6::Network)

# GUI subsystem on Windows: no console window behind the dashboard.
if(WIN32)
    set_target_properties(360WeatherDash PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS 360WeatherDash BUNDLE DESTINATION . RUNTIME DESTINATION bin)
