#!/usr/bin/env bash
# ============================================================
#  Quiz Master Pro — Build Script (Fedora)
#  Foydalanish:
#    ./build.sh              # faqat build
#    ./build.sh --deps       # paketlarni o'rnatib build
#    ./build.sh --deps --run # o'rnatib, build qilib, ishlatib
#    ./build.sh --clean      # tozalab qayta build
#    ./build.sh --run        # build qilib ishlatish
# ============================================================
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

echo "=============================="
echo "  Quiz Master Pro — Builder"
echo "  (Fedora)"
echo "=============================="

if [ $DEPS -eq 1 ]; then
    echo ""
    echo "📦 Kerakli paketlar o'rnatilmoqda (dnf)..."
    sudo dnf install -y \
        gcc-c++ \
        cmake \
        make \
        gtkmm30-devel \
        sqlite-devel \
        pkgconfig
    echo "✅ Paketlar o'rnatildi!"
fi

if [ $CLEAN -eq 1 ] && [ -d build ]; then
    echo ""
    echo "🧹 Eski build tozalanmoqda..."
    rm -rf build
    echo "✅ Tozalandi!"
fi

echo ""
echo "🔨 Kompilyatsiya boshlanmoqda..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

echo ""
echo "✅ Build muvaffaqiyatli tugadi!"
echo "   Fayl: $(pwd)/QuizMasterPro"

if [ $RUN -eq 1 ]; then
    echo ""
    echo "🚀 Dastur ishga tushirilmoqda..."
    ./QuizMasterPro
fi