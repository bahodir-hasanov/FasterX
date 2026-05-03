// ============================================================
//  QUIZ MASTER PRO — Entry Point
//  Fayl: main.cpp
//
//  Build:
//    mkdir build && cd build
//    cmake .. -DCMAKE_BUILD_TYPE=Release
//    make -j$(nproc)
//    ./QuizMasterPro
//
//  Yoki: ./build.sh --deps --run
// ============================================================

#include "quiz_app.h"
#include <gtkmm/application.h>
#include <iostream>

int main(int argc, char *argv[]) {
  auto app = Gtk::Application::create(argc, argv, "uz.quiz.masterpro");

  try {
    MainWindow window;
    return app->run(window);
  } catch (const std::exception &e) {
    std::cerr << "❌ Xato: " << e.what() << std::endl;
    return 1;
  }
}

// ============================================================
//  CMakeLists.txt  (bu fayl alohida: CMakeLists.txt nomi bilan saqlang)
// ============================================================
/*

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
    # quiz_app.h — header only, alohida compile shart emas
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

*/

// ============================================================
//  build.sh  (bu fayl alohida: build.sh nomi bilan saqlang)
// ============================================================
/*

#!/usr/bin/env bash
set -e

DEPS=0
CLEAN=0
RUN=0

for arg in "$@"; do
    case $arg in
        --deps)  DEPS=1  ;;
        --clean) CLEAN=1 ;;
        --run)   RUN=1   ;;
    esac
done

if [ $DEPS -eq 1 ]; then
    echo "📦 Kerakli paketlar o'rnatilmoqda..."
    if command -v apt &>/dev/null; then
        sudo apt update
        sudo apt install -y g++ cmake libgtkmm-3.0-dev libsqlite3-dev pkg-config
make elif command -v dnf &>/dev/null; then sudo dnf install -y gcc-c++ cmake
gtkmm30-devel sqlite-devel pkgconfig make elif command -v pacman &>/dev/null;
then sudo pacman -S --needed gcc cmake gtkmm3 sqlite pkgconf make else echo "⚠️
Paket menejeri aniqlanmadi. Qo'lda o'rnating." fi fi

if [ $CLEAN -eq 1 ] && [ -d build ]; then
    echo "🧹 Eski build tozalanmoqda..."
    rm -rf build
fi

echo "🔨 Build boshlandi..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
echo "✅ Build muvaffaqiyatli!"

if [ $RUN -eq 1 ]; then
    echo "🚀 Ishga tushirilmoqda..."
    ./QuizMasterPro
fi

*/