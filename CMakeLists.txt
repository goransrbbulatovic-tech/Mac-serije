cmake_minimum_required(VERSION 3.16)
project(AcmigoIndexer VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 QUIET COMPONENTS Core Gui Widgets Network Sql)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Sql)
    set(QT_VERSION_MAJOR 5)
    message(STATUS "Using Qt5")
endif()

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow_actions.cpp
    src/database.cpp
    src/imdbfetcher.cpp
    src/seriesdialog.cpp
    src/importdialog.cpp
    src/xlsxbridge.cpp
    src/settingsdialog.cpp
    src/seriesfiltermodel.cpp
    src/licensemanager.cpp
    src/licensedialog.cpp
)

set(HEADERS
    src/mainwindow.h
    src/database.h
    src/imdbfetcher.h
    src/seriesdialog.h
    src/importdialog.h
    src/xlsxbridge.h
    src/settingsdialog.h
    src/seriesfiltermodel.h
    src/licensemanager.h
    src/licensedialog.h
)

if(${QT_VERSION_MAJOR} EQUAL 6)
    qt_add_resources(RESOURCES resources.qrc)
    add_executable(AcmigoIndexer ${SOURCES} ${HEADERS} ${RESOURCES})
    target_link_libraries(AcmigoIndexer PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Sql)
else()
    qt5_add_resources(RESOURCES resources.qrc)
    add_executable(AcmigoIndexer ${SOURCES} ${HEADERS} ${RESOURCES})
    target_link_libraries(AcmigoIndexer PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::Sql)
endif()

if(WIN32)
    set_target_properties(AcmigoIndexer PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "AcmigoIndexer")
    add_custom_command(TARGET AcmigoIndexer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/series_seed.db"
            "$<TARGET_FILE_DIR:AcmigoIndexer>/series_seed.db"
        COMMENT "Copying seed database...")
endif()

if(APPLE)
    set_target_properties(AcmigoIndexer PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Acmigo Indexer"
        MACOSX_BUNDLE_BUNDLE_VERSION "2.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "2.0"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.acmigo.indexer"
        OUTPUT_NAME "AcmigoIndexer")
    # On macOS link against Cocoa framework needed by Qt
    target_link_libraries(AcmigoIndexer PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation")
endif()

install(TARGETS AcmigoIndexer DESTINATION bin)
if(WIN32)
    install(FILES series_seed.db DESTINATION bin)
endif()
