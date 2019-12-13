cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

set(PROJECT "x265gui")
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED on)

project(${PROJECT})

if(UNIX AND NOT APPLE)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
if (MSVC OR MSVC OR MSYS OR MINGW)
   add_definitions(-DWIN64 -D_WIN64 -DBOOST_USE_WINDOWS_H)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")

add_subdirectory(libde265)
#-----------------------------------------------------------------------------
# Qt
#-----------------------------------------------------------------------------
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent
    )


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(HEADER_FILES
   MainWindow.h
   UdpServer.h
   )

set(SOURCE_FILES
   main.cpp
   MainWindow.cpp
   UdpServer.cpp
   )

qt5_wrap_ui(FORMS
   MainWindow.ui
   )

include(Qt5ExternalResources.cmake)
qt5_add_external_resources(resources.rcc resources.qrc)

add_executable(${PROJECT} WIN32
   ${SOURCE_FILES}
   ${HEADER_FILES}
   ${FORMS}
   ${moc_sources}
   ${uic_sources}
   resources.qrc
   )

target_link_libraries (${PROJECT}
   Qt5::Widgets
   Qt5::Network
   Qt5::Concurrent
   Qt5::Core
   Qt5::Gui
   dec265
   )


