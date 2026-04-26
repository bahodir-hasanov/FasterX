#!/bin/bash
# Quiz Master Pro - Build Script
# Ubuntu/Debian/Fedora/Arch uchun

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

info()  { echo -e "${GREEN}[✅]${NC} $1"; }
warn()  { echo -e "${YELLOW}[⚠️]${NC}  $1"; }
error() { echo -e "${RED}[❌]${NC} $1"; }

echo ""
echo "╔══════════════════════════════════════════╗"
echo "║       QUIZ MASTER PRO - Build Script     ║"
echo "╚══════════════════════════════════════════╝"
echo ""

# Detect distro
if command -v apt-get &>/dev/null; then
    DISTRO="debian"
elif command -v dnf &>/dev/null; then
    DISTRO="fedora"
elif command -v pacman &>/dev/null; then
    DISTRO="arch"
else
    DISTRO="unknown"
fi

install_deps() {
    info "Kerakli paketlar o'rnatilmoqda... ($DISTRO)"
    case $DISTRO in
        debian)
            sudo apt-get update -q
            sudo apt-get install -y g++ cmake libgtkmm-3.0-dev libsqlite3-dev make pkg-config
            ;;
        fedora)
            # Tuzatish 1: g++ -> gcc-c++ (dnf da g++ mavjud emas)
            # Tuzatish 2: pkgconfig -> pkg-config (dnf da pkgconfig mavjud emas)
            sudo dnf install -y gcc-c++ cmake gtkmm30-devel sqlite-devel make pkg-config
            ;;
        arch)
            sudo pacman -Sy --noconfirm gcc cmake gtkmm3 sqlite make pkgconf
            ;;
        *)
            warn "Distro aniqlanmadi. Paketlarni qo'lda o'rnating:"
            echo "  Debian/Ubuntu: g++ cmake libgtkmm-3.0-dev libsqlite3-dev make pkg-config"
            echo "  Fedora: gcc-c++ cmake gtkmm30-devel sqlite-devel make pkg-config"
            echo "  Arch: gcc cmake gtkmm3 sqlite make pkgconf"
            ;;
    esac
}

# Parse arguments
INSTALL_DEPS=false
RUN_AFTER=false
CLEAN=false

for arg in "$@"; do
    case $arg in
        --deps)    INSTALL_DEPS=true ;;
        --run)     RUN_AFTER=true    ;;
        --clean)   CLEAN=true        ;;
        --help|-h)
            echo "Foydalanish: $0 [--deps] [--run] [--clean]"
            echo "  --deps   Kerakli paketlarni o'rnatish"
            echo "  --run    Build dan keyin ishga tushirish"
            echo "  --clean  Build papkasini tozalash"
            exit 0
            ;;
    esac
done

if $INSTALL_DEPS; then
    install_deps
fi

if $CLEAN && [ -d "$BUILD_DIR" ]; then
    info "Build papkasi tozalanmoqda..."
    rm -rf "$BUILD_DIR"
fi

# Build
info "Build papkasi yaratilmoqda..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

info "CMake konfiguratsiyasi..."
cmake "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release

info "Kompilatsiya boshlandi..."
CORES=$(nproc)
info "Paralel build: $CORES core"
make -j"$CORES"

echo ""
echo "╔══════════════════════════════════════════╗"
echo "║          ✅ BUILD MUVAFFAQIYATLI!         ║"
echo "╚══════════════════════════════════════════╝"
echo ""
info "Dastur joylashuvi: $BUILD_DIR/QuizMasterPro"
echo ""
echo "Ishga tushirish uchun:"
echo "  $BUILD_DIR/QuizMasterPro"
echo ""

if $RUN_AFTER; then
    info "Dastur ishga tushirilmoqda..."
    "$BUILD_DIR/QuizMasterPro"
fi