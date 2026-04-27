cmake_minimum_required(VERSION 3.14)
project(QuizMasterPro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

include_directories(${GTKMM_INCLUDE_DIRS} ${SQLITE3_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS} ${SQLITE3_LIBRARY_DIRS})

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${GTKMM_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    pthread
    dl
)

if(UNIX AND NOT APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)