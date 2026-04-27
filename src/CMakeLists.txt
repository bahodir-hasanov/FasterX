cmake_minimum_required(VERSION 3.14)
project(QuizMasterPro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Cross compile uchun
if(WIN32)
    set(CMAKE_SYSTEM_NAME Windows)
endif()

# pkg-config
find_package(PkgConfig REQUIRED)

# MUHIM: mingw pkg-config ishlatish
if(WIN32)
    set(ENV{PKG_CONFIG_LIBDIR} "/usr/x86_64-w64-mingw32/sys-root/mingw/lib/pkgconfig")
endif()

pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

include_directories(
    ${GTKMM_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
)

link_directories(
    ${GTKMM_LIBRARY_DIRS}
    ${SQLITE3_LIBRARY_DIRS}
)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${GTKMM_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

# Linux uchun
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} pthread dl)
endif()