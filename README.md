# 🎯 Quiz Master Pro — C++ Desktop Ilovasi

Professional **C++17 + GTKMM3 + SQLite3** bilan yozilgan to'liq funksional Linux desktop quiz ilovasi.

---

## ✨ Yangiliklar va mukammalashtirishlar

| Xususiyat | Tavsif |
|-----------|--------|
| 🎨 Dark Mode | Qorong'i rejim (sozlamalar orqali) |
| 💾 Doimiy saqlash | SQLite3 — WAL rejimi, barcha ma'lumotlar saqlanadi |
| 🔀 Random savollar | Savollar va javoblar random tartibda |
| 📊 Natijalar tarixi | Eng yaxshi 10 natija ko'rsatiladi |
| ⚙️ Sozlamalar | Til, shrift, javob vaqti, tovush |
| 🔒 Thread-safe | Mutex bilan ma'lumotlar bazasi xavfsizligi |
| 📁 TXT parser | Xatoliklarni aniqlab beruvchi to'liq parser |
| 🏆 Baholar | A+, A, B, C, D baho tizimi |

---

## 📁 Loyiha tuzilmasi

```
QuizMasterPro/
├── CMakeLists.txt          # Build tizimi
├── build.sh                # Qulay build script
├── namuna_test.txt         # Namuna test fayli
├── quiz-master.desktop     # Linux launcher
└── src/
    ├── models.h            # Ma'lumotlar modellari
    ├── database.h/cpp      # SQLite3 database layer
    ├── quiz_engine.h/cpp   # Quiz logikasi
    ├── file_parser.h/cpp   # TXT fayl parser
    ├── main_window.h/cpp   # GTKMM asosiy oyna
    └── main.cpp            # Entry point
```

---

## 🚀 O'rnatish

### 1-usul: Build script (tavsiya etiladi)

```bash
# Paketlar o'rnatish + build + ishga tushirish
chmod +x build.sh
./build.sh --deps --run

# Faqat build
./build.sh

# Tozalab qayta build
./build.sh --clean
```

### 2-usul: Qo'lda

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y g++ cmake libgtkmm-3.0-dev libsqlite3-dev pkg-config make

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./QuizMasterPro
```

**Fedora:**
```bash
sudo dnf install -y gcc-c++ cmake gtkmm30-devel sqlite-devel pkgconfig make

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./QuizMasterPro
```

---

## 📄 TXT Fayl Formati

```
++++
Savol matni
====
Birinchi javob
====
#To'g'ri javob  ← # belgisi to'g'riligini bildiradi
====
Uchinchi javob
++++
```

`namuna_test.txt` faylida tayyor misol bor.

---

## 🎮 Sahifalar

| Sahifa | Tavsif |
|--------|--------|
| 🏠 Home | Bosh sahifa, statistika |
| ✨ Yangi Quiz | Interaktiv quiz yaratish |
| 📂 Import | TXT fayldan import |
| 🎮 O'ynash | Quiz o'ynash, natijalar |
| 🗑️ O'chirish | Quizlarni o'chirish |
| ⚙️ Sozlamalar | Dark mode, til, vaqt |

---

## 🔧 Tizim talablari

| Komponent | Talab |
|-----------|-------|
| OS | Linux (Ubuntu 20.04+) |
| RAM | 256 MB |
| Disk | 50 MB |
| Display | X11 yoki Wayland |
| Compiler | GCC 9+ yoki Clang 10+ |

---

## 📝 Litsenziya

MIT License — erkin foydalaning va o'zgartiring.