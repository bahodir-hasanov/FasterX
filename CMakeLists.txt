cmake_minimum_required(VERSION 3.16)
project(QuizMasterPro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6 o'rnatish
find_package(Qt6 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Sql 
    Concurrent
    REQUIRED
)

find_package(SQLite3 REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Header files
set(HEADERS
    src/models.h
    src/database.h
    src/quiz_engine.h
    src/file_parser.h
    src/main_window.h
)

# Source files
set(SOURCES
    src/main.cpp
    src/main_window.cpp
    src/database.cpp
    src/quiz_engine.cpp
    src/file_parser.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Concurrent
    SQLite::SQLite3
)

# Windows specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    
    # MSVC compiler flags
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4 /EHsc)
    endif()
endif()

# Print config summary
message(STATUS "=== Quiz Master Pro Build Config ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "GTKMM: ${GTKMM_VERSION}")
message(STATUS "SQLite3: ${SQLITE3_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")