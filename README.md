# Quiz App - C++ da Grafik Interfeysli Desktop Ilovasi

## 📋 Loyiha haqida

Bu **C++** tilida yozilgan, **GTKMM** (GTK3) grafik kutubxonasidan foydalanuvchi to'liq huquqli **DESKTOP** ilovasi. Ilova Linux operatsion tizimida ishlaydi va quyidagi imkoniyatlarni taqdim etadi:

- 📁 **.txt fayllardan** testlarni avtomatik o'qish
- ✍️ **Yangi savollar** to'plamini interaktiv yaratish
- 🎲 **Random tartibda** savol va javoblarni berish
- 📊 **Ball va foiz** hisoblash
- 🗑️ **Savollar to'plamini** o'chirish

---

## 💻 Tizim talablari

| Komponent | Minimal talab |
|-----------|---------------|
| Operatsion tizim | Linux (Ubuntu 20.04+, Debian 11+, Fedora 35+) |
| RAM | 512 MB |
| Disk hajmi | 100 MB |
| Grafik muhit | X11 yoki Wayland |

---

## 🚀 O'rnatish va ishga tushirish

### 1. Kerakli paketlarni o'rnatish

**Ubuntu/Debian uchun:**
```bash
sudo apt update
sudo apt install -y \
    g++ \
    cmake \
    libgtkmm-3.0-dev \
    libsqlite3-dev \
    make

**fedora uchun:**
sudo dnf install -y \
    gcc-c++ \
    cmake \
    gtkmm30-devel \
    sqlite-devel \
    make