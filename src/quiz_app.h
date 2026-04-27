#pragma once

// ============================================================
//  QUIZ MASTER PRO — Barcha header kodlari
//  Fayl: quiz_app.h
//  C++17 + GTKMM3 + SQLite3
// ============================================================

#include <gtkmm.h>
#include <sqlite3.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ============================================================
//  MODELS
// ============================================================

struct Answer {
    int id = 0;
    std::string text;
    bool isCorrect = false;
};

struct Question {
    int id = 0;
    std::string text;
    std::vector<Answer> answers;
    int orderIndex = 0;
};

struct QuizSet {
    int id = 0;
    std::string name;
    std::vector<Question> questions;
    time_t createdAt = 0;
};

struct QuizResult {
    int quizSetId = 0;
    int score = 0;
    int total = 0;
    double percentage = 0.0;
    time_t completedAt = 0;
    std::string quizName;
};

// ============================================================
//  DATABASE
// ============================================================

class Database {
public:
    explicit Database(const std::string& path = "") {
        std::string dbPath = path.empty() ? getDefaultPath() : path;
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Ma'lumotlar bazasini ochib bo'lmadi: " +
                                     std::string(sqlite3_errmsg(db)));
        }
        sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);
        createTables();
        createIndexes();
    }

    ~Database() {
        if (db) sqlite3_close(db);
    }

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // ---------- Quiz sets ----------

    bool addQuizSet(const std::string& name, const std::vector<Question>& questions) {
        std::lock_guard<std::mutex> lock(dbMutex);
        sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

        sqlite3_stmt* stmt = nullptr;
        const char* insertSet = "INSERT INTO quiz_sets(name) VALUES(?);";
        sqlite3_prepare_v2(db, insertSet, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
            sqlite3_finalize(stmt);
            return false;
        }
        int setId = static_cast<int>(sqlite3_last_insert_rowid(db));
        sqlite3_finalize(stmt);

        int qi = 0;
        for (const auto& q : questions) {
            const char* insertQ =
                "INSERT INTO questions(quiz_set_id, text, order_index) VALUES(?,?,?);";
            sqlite3_prepare_v2(db, insertQ, -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, setId);
            sqlite3_bind_text(stmt, 2, q.text.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 3, qi++);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
                sqlite3_finalize(stmt);
                return false;
            }
            int qId = static_cast<int>(sqlite3_last_insert_rowid(db));
            sqlite3_finalize(stmt);

            for (const auto& a : q.answers) {
                const char* insertA =
                    "INSERT INTO answers(question_id, text, is_correct) VALUES(?,?,?);";
                sqlite3_prepare_v2(db, insertA, -1, &stmt, nullptr);
                sqlite3_bind_int(stmt, 1, qId);
                sqlite3_bind_text(stmt, 2, a.text.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt, 3, a.isCorrect ? 1 : 0);

                if (sqlite3_step(stmt) != SQLITE_DONE) {
                    sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
                    sqlite3_finalize(stmt);
                    return false;
                }
                sqlite3_finalize(stmt);
            }
        }

        sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
        return true;
    }

    std::vector<QuizSet> getAllQuizSets() {
        std::lock_guard<std::mutex> lock(dbMutex);
        std::vector<QuizSet> sets;

        const char* sql =
            "SELECT qs.id, qs.name, qs.created_at,"
            " COUNT(DISTINCT q.id) as qcount"
            " FROM quiz_sets qs"
            " LEFT JOIN questions q ON qs.id = q.quiz_set_id"
            " GROUP BY qs.id ORDER BY qs.created_at DESC;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QuizSet s;
                s.id = sqlite3_column_int(stmt, 0);
                auto* t = sqlite3_column_text(stmt, 1);
                s.name = t ? reinterpret_cast<const char*>(t) : "";
                s.createdAt = sqlite3_column_int64(stmt, 2);
                // question count stored as placeholder; load on demand
                sqlite3_stmt* cntStmt = nullptr;
                const char* cntSql = "SELECT COUNT(*) FROM questions WHERE quiz_set_id=?;";
                sqlite3_prepare_v2(db, cntSql, -1, &cntStmt, nullptr);
                sqlite3_bind_int(cntStmt, 1, s.id);
                if (sqlite3_step(cntStmt) == SQLITE_ROW) {
                    int cnt = sqlite3_column_int(cntStmt, 0);
                    // store count inside dummy questions list for display
                    s.questions.resize(cnt);
                }
                sqlite3_finalize(cntStmt);
                sets.push_back(s);
            }
        }
        sqlite3_finalize(stmt);
        return sets;
    }

    QuizSet getFullQuizSet(int id) {
        std::lock_guard<std::mutex> lock(dbMutex);
        QuizSet set;
        set.id = id;

        sqlite3_stmt* stmt = nullptr;
        const char* getName = "SELECT name FROM quiz_sets WHERE id=?;";
        sqlite3_prepare_v2(db, getName, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            auto* t = sqlite3_column_text(stmt, 0);
            set.name = t ? reinterpret_cast<const char*>(t) : "";
        }
        sqlite3_finalize(stmt);

        const char* getQ =
            "SELECT id, text, order_index FROM questions WHERE quiz_set_id=? ORDER BY order_index;";
        sqlite3_prepare_v2(db, getQ, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, id);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Question q;
            q.id = sqlite3_column_int(stmt, 0);
            auto* qt = sqlite3_column_text(stmt, 1);
            q.text = qt ? reinterpret_cast<const char*>(qt) : "";
            q.orderIndex = sqlite3_column_int(stmt, 2);

            sqlite3_stmt* ansStmt = nullptr;
            const char* getA =
                "SELECT id, text, is_correct FROM answers WHERE question_id=?;";
            sqlite3_prepare_v2(db, getA, -1, &ansStmt, nullptr);
            sqlite3_bind_int(ansStmt, 1, q.id);

            while (sqlite3_step(ansStmt) == SQLITE_ROW) {
                Answer a;
                a.id = sqlite3_column_int(ansStmt, 0);
                auto* at = sqlite3_column_text(ansStmt, 1);
                a.text = at ? reinterpret_cast<const char*>(at) : "";
                a.isCorrect = sqlite3_column_int(ansStmt, 2) == 1;
                q.answers.push_back(a);
            }
            sqlite3_finalize(ansStmt);
            set.questions.push_back(q);
        }
        sqlite3_finalize(stmt);
        return set;
    }

    bool deleteQuizSet(int id) {
        std::lock_guard<std::mutex> lock(dbMutex);
        std::string sql = "DELETE FROM quiz_sets WHERE id=" + std::to_string(id) + ";";
        return execSQL(sql);
    }

    // ---------- Results ----------

    bool saveResult(const QuizResult& result) {
        std::lock_guard<std::mutex> lock(dbMutex);
        const char* sql =
            "INSERT INTO results(quiz_set_id, score, total, percentage) VALUES(?,?,?,?);";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, result.quizSetId);
        sqlite3_bind_int(stmt, 2, result.score);
        sqlite3_bind_int(stmt, 3, result.total);
        sqlite3_bind_double(stmt, 4, result.percentage);
        bool ok = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return ok;
    }

    std::vector<QuizResult> getTopResults(int limit = 10) {
        std::vector<QuizResult> results;
        const char* sql =
            "SELECT r.quiz_set_id, r.score, r.total, r.percentage, r.completed_at,"
            " qs.name FROM results r"
            " LEFT JOIN quiz_sets qs ON r.quiz_set_id = qs.id"
            " ORDER BY r.percentage DESC LIMIT ?;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, limit);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QuizResult r;
                r.quizSetId = sqlite3_column_int(stmt, 0);
                r.score = sqlite3_column_int(stmt, 1);
                r.total = sqlite3_column_int(stmt, 2);
                r.percentage = sqlite3_column_double(stmt, 3);
                r.completedAt = sqlite3_column_int64(stmt, 4);
                auto* nt = sqlite3_column_text(stmt, 5);
                r.quizName = nt ? reinterpret_cast<const char*>(nt) : "—";
                results.push_back(r);
            }
        }
        sqlite3_finalize(stmt);
        return results;
    }

    int getTotalQuizSets() {
        int count = 0;
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT COUNT(*) FROM quiz_sets;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW)
                count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        return count;
    }

private:
    sqlite3* db = nullptr;
    std::mutex dbMutex;

    static std::string getDefaultPath() {
        const char* home = getenv("HOME");
        std::string dir = home ? std::string(home) : ".";
        return dir + "/.quiz_master_pro.db";
    }

    void createTables() {
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS quiz_sets (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                created_at INTEGER DEFAULT (strftime('%s','now'))
            );
            CREATE TABLE IF NOT EXISTS questions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                quiz_set_id INTEGER NOT NULL,
                text TEXT NOT NULL,
                order_index INTEGER DEFAULT 0,
                FOREIGN KEY(quiz_set_id) REFERENCES quiz_sets(id) ON DELETE CASCADE
            );
            CREATE TABLE IF NOT EXISTS answers (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                question_id INTEGER NOT NULL,
                text TEXT NOT NULL,
                is_correct INTEGER DEFAULT 0,
                FOREIGN KEY(question_id) REFERENCES questions(id) ON DELETE CASCADE
            );
            CREATE TABLE IF NOT EXISTS results (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                quiz_set_id INTEGER NOT NULL,
                score INTEGER NOT NULL,
                total INTEGER NOT NULL,
                percentage REAL NOT NULL,
                completed_at INTEGER DEFAULT (strftime('%s','now')),
                FOREIGN KEY(quiz_set_id) REFERENCES quiz_sets(id)
            );
        )";
        execSQL(sql);
    }

    void createIndexes() {
        execSQL("CREATE INDEX IF NOT EXISTS idx_questions_set ON questions(quiz_set_id);");
        execSQL("CREATE INDEX IF NOT EXISTS idx_answers_q ON answers(question_id);");
        execSQL("CREATE INDEX IF NOT EXISTS idx_results_pct ON results(percentage DESC);");
    }

    bool execSQL(const std::string& sql) {
        char* err = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL xato: " << err << "\n";
            sqlite3_free(err);
            return false;
        }
        return true;
    }
};

// ============================================================
//  TXT PARSER
// ============================================================

class TXTParser {
public:
    struct ParseResult {
        bool success = false;
        std::string suggestedName;
        std::vector<Question> questions;
        std::vector<std::string> errors;
    };

    static ParseResult parse(const std::string& filename) {
        ParseResult result;

        // Fayl nomidan nom olish
        size_t slash = filename.find_last_of("/\\");
        size_t dot = filename.find_last_of('.');
        if (slash == std::string::npos) slash = 0; else ++slash;
        result.suggestedName =
            (dot != std::string::npos && dot > slash)
            ? filename.substr(slash, dot - slash)
            : filename.substr(slash);

        std::ifstream file(filename);
        if (!file.is_open()) {
            result.errors.push_back("Faylni ochib bo'lmadi: " + filename);
            return result;
        }

        std::string line;
        std::string currentQuestion;
        std::vector<Answer> currentAnswers;
        bool inBlock = false;
        bool firstSection = false; // question bo'limi
        int lineNo = 0;

        auto saveQuestion = [&]() {
            if (!currentQuestion.empty() && !currentAnswers.empty()) {
                bool hasCorrect = false;
                for (auto& a : currentAnswers) if (a.isCorrect) { hasCorrect = true; break; }
                if (!hasCorrect) {
                    result.errors.push_back(
                        "Savol '" + currentQuestion.substr(0, 30) + "...' to'g'ri javobsiz");
                } else {
                    Question q;
                    q.text = currentQuestion;
                    q.answers = currentAnswers;
                    q.orderIndex = static_cast<int>(result.questions.size());
                    result.questions.push_back(q);
                }
            }
            currentQuestion.clear();
            currentAnswers.clear();
        };

        while (std::getline(file, line)) {
            ++lineNo;
            // trim
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.erase(0,1);
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

            if (line == "++++") {
                if (inBlock) {
                    // blok tugadi
                    saveQuestion();
                    inBlock = false;
                    firstSection = false;
                } else {
                    inBlock = true;
                    firstSection = true;
                }
            } else if (line == "====" && inBlock) {
                firstSection = false; // endi javoblar
            } else if (inBlock) {
                if (firstSection) {
                    if (!currentQuestion.empty()) currentQuestion += " ";
                    currentQuestion += line;
                } else {
                    Answer a;
                    if (!line.empty() && line[0] == '#') {
                        a.text = line.substr(1);
                        a.isCorrect = true;
                    } else {
                        a.text = line;
                        a.isCorrect = false;
                    }
                    if (!a.text.empty())
                        currentAnswers.push_back(a);
                }
            }
        }

        if (inBlock) saveQuestion(); // fayl oxiri

        result.success = !result.questions.empty();
        if (result.questions.empty() && result.errors.empty())
            result.errors.push_back("Fayl bo'sh yoki format noto'g'ri");

        return result;
    }
};

// ============================================================
//  QUIZ ENGINE
// ============================================================

class QuizEngine {
public:
    QuizEngine() : rng(std::random_device{}()) {}

    void startQuiz(const QuizSet& set) {
        activeSet = set;
        currentScore = 0;
        currentPos = 0;
        lastAnswerCorrect = false;
        answered = false;
        currentAnswers.clear();

        shuffledIndices.resize(set.questions.size());
        std::iota(shuffledIndices.begin(), shuffledIndices.end(), 0);
        std::shuffle(shuffledIndices.begin(), shuffledIndices.end(), rng);
    }

    bool hasMore() const {
        return currentPos < static_cast<int>(shuffledIndices.size());
    }

    bool isFinished() const { return !hasMore(); }

    const Question& getCurrentQuestion() const {
        if (!hasMore()) throw std::out_of_range("Savollar tugadi");
        return activeSet.questions[shuffledIndices[currentPos]];
    }

    std::vector<Answer> getShuffledAnswers() {
        auto answers = getCurrentQuestion().answers;
        std::shuffle(answers.begin(), answers.end(), rng);
        currentAnswers = answers;
        return answers;
    }

    bool submitAnswer(int answerIndex) {
        if (answerIndex < 0 || answerIndex >= static_cast<int>(currentAnswers.size()))
            return false;
        bool correct = currentAnswers[answerIndex].isCorrect;
        if (correct) ++currentScore;
        lastAnswerCorrect = correct;
        answered = true;
        return correct;
    }

    void nextQuestion() {
        ++currentPos;
        answered = false;
        currentAnswers.clear();
    }

    int getScore() const { return currentScore; }
    int getTotal() const { return static_cast<int>(shuffledIndices.size()); }
    int getCurrentIndex() const { return currentPos; }
    bool wasLastCorrect() const { return lastAnswerCorrect; }
    bool isAnswered() const { return answered; }

    double getPercentage() const {
        int total = getTotal();
        return total > 0 ? (currentScore * 100.0 / total) : 0.0;
    }

    QuizResult getResult() const {
        QuizResult r;
        r.quizSetId = activeSet.id;
        r.score = currentScore;
        r.total = getTotal();
        r.percentage = getPercentage();
        r.completedAt = time(nullptr);
        r.quizName = activeSet.name;
        return r;
    }

    std::string getGrade() const {
        double pct = getPercentage();
        if (pct >= 95) return "A+";
        if (pct >= 85) return "A";
        if (pct >= 75) return "B";
        if (pct >= 60) return "C";
        if (pct >= 50) return "D";
        return "F";
    }

private:
    QuizSet activeSet;
    std::vector<int> shuffledIndices;
    std::vector<Answer> currentAnswers;
    int currentPos = 0;
    int currentScore = 0;
    bool lastAnswerCorrect = false;
    bool answered = false;
    std::mt19937 rng;
};

// ============================================================
//  MAIN WINDOW
// ============================================================

class MainWindow : public Gtk::Window {
public:
    MainWindow();
    virtual ~MainWindow() = default;

private:
    // ---- UI ----
    Gtk::Box mainVBox{Gtk::ORIENTATION_VERTICAL};
    Gtk::HeaderBar headerBar;
    Gtk::Stack mainStack;

    Gtk::Button btnHome, btnNew, btnImport, btnPlay, btnDelete, btnSettings, btnQuit;

    // Pages
    Gtk::ScrolledWindow swHome, swNew, swImport, swPlay, swDelete, swSettings;

    // ---- Data ----
    Database db;
    QuizEngine engine;

    // ---- New quiz state ----
    struct NewQuizState {
        std::string quizName;
        std::vector<Question> questions;
        Question currentQ;
        std::vector<Gtk::Box*> answerRows;
    } nqs;

    // ---- Play state ----
    int selectedQuizId = -1;
    bool quizActive = false;

    // ---- Widgets stored for dynamic update ----
    // Home
    Gtk::Label* lblStats = nullptr;
    Gtk::ListBox* lbTopResults = nullptr;

    // New
    Gtk::Entry* entQuizName = nullptr;
    Gtk::TextView* tvQuestion = nullptr;
    Gtk::Box* answersBox = nullptr;
    Gtk::Label* lblQCount = nullptr;
    Gtk::ListBox* lbQuestionList = nullptr;

    // Import
    Gtk::Label* lblSelectedFile = nullptr;
    Gtk::Label* lblParseResult = nullptr;
    Gtk::Entry* entImportName = nullptr;
    Gtk::Button* btnDoImport = nullptr;
    TXTParser::ParseResult lastParseResult;

    // Play
    Gtk::ListBox* lbQuizSets = nullptr;
    Gtk::Box* playArea = nullptr;
    Gtk::Label* lblQNum = nullptr;
    Gtk::Label* lblQuestion = nullptr;
    Gtk::Box* answersPlayBox = nullptr;
    Gtk::ProgressBar* pbProgress = nullptr;
    Gtk::Label* lblScore = nullptr;
    Gtk::Button* btnNextQ = nullptr;
    Gtk::Label* lblFeedback = nullptr;

    // Delete
    Gtk::ListBox* lbDeleteSets = nullptr;

    // Settings
    Gtk::Switch* swDarkMode = nullptr;

    // ---- Methods ----
    void buildUI();
    void applyCSS(bool dark = false);

    // Page builders
    Gtk::Widget* buildHomePage();
    Gtk::Widget* buildNewPage();
    Gtk::Widget* buildImportPage();
    Gtk::Widget* buildPlayPage();
    Gtk::Widget* buildDeletePage();
    Gtk::Widget* buildSettingsPage();

    // Home
    void refreshHome();

    // New quiz
    void addAnswerRow(bool correct = false, const std::string& text = "");
    void onSaveQuestion();
    void onSaveQuiz();
    void clearNewQuiz();

    // Import
    void onChooseFile();
    void onDoImport();

    // Play
    void refreshPlayList();
    void startQuiz();
    void showQuestion();
    void onAnswerClicked(int idx);
    void onNextQuestion();

    // Delete
    void refreshDeleteList();
    void onDeleteSelected();

    // Navigation
    void goHome();
    void goNew();
    void goImport();
    void goPlay();
    void goDelete();
    void goSettings();

    // Helpers
    static Gtk::Label* makeLabel(const std::string& markup, bool wrap = false);
    static Gtk::Button* makeButton(const std::string& label, const std::string& cssClass = "");
    Gtk::Widget* makeSeparator();
};

// ---- Implementation ----

inline MainWindow::MainWindow()
    : db()
{
    set_title("Quiz Master Pro");
    set_default_size(900, 700);
    set_size_request(800, 600);
    applyCSS();
    buildUI();
    refreshHome();
    show_all_children();
}

inline void MainWindow::applyCSS(bool dark) {
    auto css = Gtk::CssProvider::create();
    std::string palette = dark
        ? "window { background: #1e1e2e; color: #cdd6f4; }"
          ".card { background: #313244; border-radius: 12px; padding: 16px; margin: 8px; }"
          "entry { background: #313244; color: #cdd6f4; border: 2px solid #45475a; border-radius: 8px; padding: 8px; }"
          "entry:focus { border-color: #cba6f7; }"
          "textview { background: #313244; color: #cdd6f4; }"
          "list { background: #313244; border-radius: 8px; }"
          "list row { padding: 10px; border-bottom: 1px solid #45475a; color: #cdd6f4; }"
          "list row:selected { background: #7287fd; color: white; }"
        : "window { background: #f5f5f5; color: #222; }"
          ".card { background: white; border-radius: 12px; padding: 16px; margin: 8px; }"
          "entry { background: white; color: #222; border: 2px solid #e0e0e0; border-radius: 8px; padding: 8px; }"
          "entry:focus { border-color: #667eea; }"
          "textview { background: white; color: #222; }"
          "list { background: white; border-radius: 8px; }"
          "list row { padding: 10px; border-bottom: 1px solid #eee; }"
          "list row:selected { background: #667eea; color: white; }";

    std::string style = palette + R"(
        headerbar {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 8px;
        }
        headerbar button {
            background: transparent;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 6px 12px;
            margin: 0 3px;
            font-size: 16px;
        }
        headerbar button:hover { background: rgba(255,255,255,0.2); }

        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
        }
        .btn-primary:hover { background: linear-gradient(135deg, #764ba2, #667eea); }

        .btn-danger {
            background: #e74c3c;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
        }
        .btn-danger:hover { background: #c0392b; }

        .btn-success {
            background: #27ae60;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
        }

        .btn-answer {
            background: white;
            color: #333;
            border: 2px solid #e0e0e0;
            border-radius: 10px;
            padding: 12px 20px;
            font-size: 14px;
            margin: 4px;
        }
        .btn-answer:hover { border-color: #667eea; background: #f0f0ff; }
        .btn-correct { background: #d4edda; border-color: #27ae60; color: #155724; }
        .btn-wrong   { background: #f8d7da; border-color: #e74c3c; color: #721c24; }

        progressbar progress {
            background: linear-gradient(90deg, #667eea, #764ba2);
            border-radius: 10px;
        }
        progressbar {
            border-radius: 10px;
            min-height: 14px;
        }

        .page-title {
            font-size: 22px;
            font-weight: bold;
            color: #667eea;
            margin-bottom: 12px;
        }

        .stat-box {
            background: linear-gradient(135deg, #667eea, #764ba2);
            border-radius: 12px;
            padding: 20px;
            margin: 8px;
            color: white;
        }
    )";

    css->load_from_data(style);
    auto screen = Gdk::Screen::get_default();
    Gtk::StyleContext::add_provider_for_screen(
        screen, css, GTK_STYLE_PROVIDER_PRIORITY_USER);
}

inline Gtk::Label* MainWindow::makeLabel(const std::string& markup, bool wrap) {
    auto* lbl = Gtk::make_managed<Gtk::Label>();
    lbl->set_markup(markup);
    lbl->set_halign(Gtk::ALIGN_START);
    if (wrap) lbl->set_line_wrap(true);
    return lbl;
}

inline Gtk::Button* MainWindow::makeButton(const std::string& label, const std::string& cssClass) {
    auto* btn = Gtk::make_managed<Gtk::Button>(label);
    if (!cssClass.empty())
        btn->get_style_context()->add_class(cssClass);
    return btn;
}

inline Gtk::Widget* MainWindow::makeSeparator() {
    auto* sep = Gtk::make_managed<Gtk::Separator>(Gtk::ORIENTATION_HORIZONTAL);
    sep->set_margin_top(10);
    sep->set_margin_bottom(10);
    return sep;
}

// Navigation
inline void MainWindow::goHome()     { refreshHome(); mainStack.set_visible_child("home"); }
inline void MainWindow::goNew()      { clearNewQuiz(); mainStack.set_visible_child("new"); }
inline void MainWindow::goImport()   { mainStack.set_visible_child("import"); }
inline void MainWindow::goPlay()     { refreshPlayList(); mainStack.set_visible_child("play"); }
inline void MainWindow::goDelete()   { refreshDeleteList(); mainStack.set_visible_child("delete"); }
inline void MainWindow::goSettings() { mainStack.set_visible_child("settings"); }

// ---- Build UI ----
inline void MainWindow::buildUI() {
    // Header
    headerBar.set_title("QUIZ MASTER PRO");
    headerBar.set_show_close_button(false);

    btnHome.set_label("🏠"); btnNew.set_label("✨");
    btnImport.set_label("📂"); btnPlay.set_label("🎮");
    btnDelete.set_label("🗑️"); btnSettings.set_label("⚙️");
    btnQuit.set_label("❌");

    btnHome.set_tooltip_text("Bosh sahifa");
    btnNew.set_tooltip_text("Yangi Quiz");
    btnImport.set_tooltip_text("Fayldan import");
    btnPlay.set_tooltip_text("Quiz o'yna");
    btnDelete.set_tooltip_text("O'chirish");
    btnSettings.set_tooltip_text("Sozlamalar");
    btnQuit.set_tooltip_text("Chiqish");

    btnHome.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::goHome));
    btnNew.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::goNew));
    btnImport.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::goImport));
    btnPlay.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::goPlay));
    btnDelete.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::goDelete));
    btnSettings.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::goSettings));
    btnQuit.signal_clicked().connect([this]() { hide(); });

    headerBar.pack_start(btnHome);
    headerBar.pack_start(btnNew);
    headerBar.pack_start(btnImport);
    headerBar.pack_start(btnPlay);
    headerBar.pack_start(btnDelete);
    headerBar.pack_start(btnSettings);
    headerBar.pack_end(btnQuit);
    set_titlebar(headerBar);

    // Pages
    swHome.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    swNew.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    swImport.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    swPlay.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    swDelete.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    swSettings.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    swHome.add(*buildHomePage());
    swNew.add(*buildNewPage());
    swImport.add(*buildImportPage());
    swPlay.add(*buildPlayPage());
    swDelete.add(*buildDeletePage());
    swSettings.add(*buildSettingsPage());

    mainStack.set_transition_type(Gtk::STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    mainStack.set_transition_duration(200);
    mainStack.add(swHome, "home");
    mainStack.add(swNew, "new");
    mainStack.add(swImport, "import");
    mainStack.add(swPlay, "play");
    mainStack.add(swDelete, "delete");
    mainStack.add(swSettings, "settings");
    mainStack.set_visible_child("home");

    mainVBox.pack_start(mainStack, Gtk::PACK_EXPAND_WIDGET);
    add(mainVBox);
}

// ---- HOME ----
inline Gtk::Widget* MainWindow::buildHomePage() {
    auto* grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_margin_top(30);
    grid->set_margin_bottom(30);
    grid->set_margin_start(30);
    grid->set_margin_end(30);
    grid->set_row_spacing(20);
    grid->set_column_spacing(20);
    grid->set_halign(Gtk::ALIGN_CENTER);

    auto* title = makeLabel(
        "<span size='xx-large' weight='bold' foreground='#667eea'>📚 QUIZ MASTER PRO</span>");
    title->set_halign(Gtk::ALIGN_CENTER);
    grid->attach(*title, 0, 0, 2, 1);

    auto* sub = makeLabel(
        "<span size='large' foreground='#888'>\"Bilimingizni sinovdan o'tkazing\"</span>");
    sub->set_halign(Gtk::ALIGN_CENTER);
    grid->attach(*sub, 0, 1, 2, 1);

    // Stat box
    auto* statBox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 4);
    statBox->get_style_context()->add_class("stat-box");
    lblStats = Gtk::make_managed<Gtk::Label>();
    lblStats->set_markup("<span foreground='white' size='large'>📊 Yuklanmoqda...</span>");
    lblStats->set_halign(Gtk::ALIGN_CENTER);
    statBox->pack_start(*lblStats, Gtk::PACK_SHRINK);
    grid->attach(*statBox, 0, 2, 2, 1);

    // Cards (4 ta)
    struct CardInfo { std::string emoji, title, desc; void (MainWindow::*fn)(); };
    std::vector<CardInfo> cards = {
        {"✨", "YANGI QUIZ", "Yangi savollar to'plami", &MainWindow::goNew},
        {"📂", "YUKLASH", "TXT fayldan import", &MainWindow::goImport},
        {"🎮", "QUIZ O'YNA", "Testni boshlash", &MainWindow::goPlay},
        {"🗑️", "O'CHIRISH", "Setlarni o'chirish", &MainWindow::goDelete},
    };

    int col = 0, row = 3;
    for (auto& c : cards) {
        auto* btn = Gtk::make_managed<Gtk::Button>();
        btn->set_size_request(240, 130);
        btn->get_style_context()->add_class("btn-primary");

        auto* vb = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 6);
        vb->set_margin_top(10);
        vb->set_margin_bottom(10);
        vb->set_margin_start(10);
        vb->set_margin_end(10);
        auto* el = makeLabel("<span size='xx-large'>" + c.emoji + "</span>");
        el->set_halign(Gtk::ALIGN_CENTER);
        auto* tl = makeLabel("<span weight='bold' size='large'>" + c.title + "</span>");
        tl->set_halign(Gtk::ALIGN_CENTER);
        auto* dl = makeLabel("<span size='small'>" + c.desc + "</span>");
        dl->set_halign(Gtk::ALIGN_CENTER);
        vb->pack_start(*el); vb->pack_start(*tl); vb->pack_start(*dl);
        btn->add(*vb);

        auto fn = c.fn;
        btn->signal_clicked().connect([this, fn]() { (this->*fn)(); });
        grid->attach(*btn, col, row, 1, 1);
        col = 1 - col;
        if (col == 0) ++row;
    }

    // Top results
    auto* resTitle = makeLabel(
        "<span weight='bold' size='large'>🏆 Top Natijalar</span>");
    resTitle->set_halign(Gtk::ALIGN_CENTER);
    grid->attach(*resTitle, 0, row+1, 2, 1);

    lbTopResults = Gtk::make_managed<Gtk::ListBox>();
    lbTopResults->set_selection_mode(Gtk::SELECTION_NONE);
    lbTopResults->set_margin_top(6);
    grid->attach(*lbTopResults, 0, row+2, 2, 1);

    return grid;
}

inline void MainWindow::refreshHome() {
    if (!lblStats || !lbTopResults) return;
    int total = db.getTotalQuizSets();
    lblStats->set_markup(
        "<span foreground='white' size='large'>📊 Jami quiz setlar: <b>" +
        std::to_string(total) + "</b></span>");

    // Clear top results
    for (auto* ch : lbTopResults->get_children())
        lbTopResults->remove(*ch);

    auto results = db.getTopResults(5);
    if (results.empty()) {
        auto* row = Gtk::make_managed<Gtk::ListBoxRow>();
        auto* lbl = makeLabel("<span foreground='#888'>Hali natija yo'q</span>");
        lbl->set_halign(Gtk::ALIGN_CENTER);
        lbl->set_margin_top(8);
        lbl->set_margin_bottom(8);
        lbl->set_margin_start(8);
        lbl->set_margin_end(8);
        row->add(*lbl);
        lbTopResults->append(*row);
    } else {
        int i = 1;
        for (auto& r : results) {
            auto* row = Gtk::make_managed<Gtk::ListBoxRow>();
            std::string medal = (i == 1) ? "🥇" : (i == 2) ? "🥈" : (i == 3) ? "🥉" : "  ";
            std::string txt = medal + " " + r.quizName + " — " +
                              std::to_string(r.score) + "/" + std::to_string(r.total) +
                              " (" + std::to_string(static_cast<int>(r.percentage)) + "%)";
            auto* lbl = makeLabel(txt);
            lbl->set_margin_top(8);
            lbl->set_margin_bottom(8);
            lbl->set_margin_start(8);
            lbl->set_margin_end(8);
            row->add(*lbl);
            lbTopResults->append(*row);
            ++i;
        }
    }
    lbTopResults->show_all();
}

// ---- NEW QUIZ ----
inline Gtk::Widget* MainWindow::buildNewPage() {
    auto* vb = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 12);
    vb->set_margin_top(24);
    vb->set_margin_bottom(24);
    vb->set_margin_start(24);
    vb->set_margin_end(24);

    vb->pack_start(*makeLabel(
        "<span class='page-title' size='x-large' weight='bold' foreground='#667eea'>✨ Yangi Quiz Yaratish</span>"),
        Gtk::PACK_SHRINK);

    // Quiz name
    auto* nameBox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 8);
    nameBox->pack_start(*makeLabel("<b>Quiz nomi:</b>"), Gtk::PACK_SHRINK);
    entQuizName = Gtk::make_managed<Gtk::Entry>();
    entQuizName->set_placeholder_text("Masalan: Matematika testi");
    entQuizName->set_hexpand(true);
    nameBox->pack_start(*entQuizName, Gtk::PACK_EXPAND_WIDGET);
    vb->pack_start(*nameBox, Gtk::PACK_SHRINK);

    vb->pack_start(*makeSeparator(), Gtk::PACK_SHRINK);
    vb->pack_start(*makeLabel("<b>Savol matni:</b>"), Gtk::PACK_SHRINK);

    tvQuestion = Gtk::make_managed<Gtk::TextView>();
    tvQuestion->set_wrap_mode(Gtk::WRAP_WORD);
    tvQuestion->set_size_request(-1, 80);
    auto* tvFrame = Gtk::make_managed<Gtk::Frame>();
    tvFrame->add(*tvQuestion);
    vb->pack_start(*tvFrame, Gtk::PACK_SHRINK);

    vb->pack_start(*makeLabel("<b>Javoblar (kamida 2 ta, bittasini ✔ to'g'ri deb belgilang):</b>"), Gtk::PACK_SHRINK);

    answersBox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 6);
    vb->pack_start(*answersBox, Gtk::PACK_SHRINK);

    // Default 3 ta javob
    addAnswerRow(false);
    addAnswerRow(true);   // ikkinchisi default to'g'ri
    addAnswerRow(false);

    auto* addAnsBtn = makeButton("+ Javob qo'shish", "btn-primary");
    addAnsBtn->set_halign(Gtk::ALIGN_START);
    addAnsBtn->signal_clicked().connect([this]() { addAnswerRow(); show_all_children(); });
    vb->pack_start(*addAnsBtn, Gtk::PACK_SHRINK);

    auto* addQBtn = makeButton("➕ Savolni saqlash", "btn-success");
    addQBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onSaveQuestion));
    vb->pack_start(*addQBtn, Gtk::PACK_SHRINK);

    vb->pack_start(*makeSeparator(), Gtk::PACK_SHRINK);

    lblQCount = makeLabel("<b>Savollar soni: 0</b>");
    vb->pack_start(*lblQCount, Gtk::PACK_SHRINK);

    lbQuestionList = Gtk::make_managed<Gtk::ListBox>();
    lbQuestionList->set_selection_mode(Gtk::SELECTION_NONE);
    vb->pack_start(*lbQuestionList, Gtk::PACK_SHRINK);

    auto* saveBtn = makeButton("💾 Quizni saqlash", "btn-primary");
    saveBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onSaveQuiz));
    vb->pack_start(*saveBtn, Gtk::PACK_SHRINK);

    return vb;
}

inline void MainWindow::addAnswerRow(bool correct, const std::string& text) {
    auto* hb = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 8);

    auto* chk = Gtk::make_managed<Gtk::CheckButton>("✔ To'g'ri");
    chk->set_active(correct);
    hb->pack_start(*chk, Gtk::PACK_SHRINK);

    auto* ent = Gtk::make_managed<Gtk::Entry>();
    ent->set_placeholder_text("Javob matni...");
    ent->set_hexpand(true);
    if (!text.empty()) ent->set_text(text);
    hb->pack_start(*ent, Gtk::PACK_EXPAND_WIDGET);

    auto* del = Gtk::make_managed<Gtk::Button>("🗑️");
    del->get_style_context()->add_class("btn-danger");
    del->signal_clicked().connect([this, hb]() {
        answersBox->remove(*hb);
        nqs.answerRows.erase(
            std::remove(nqs.answerRows.begin(), nqs.answerRows.end(), hb),
            nqs.answerRows.end());
    });
    hb->pack_start(*del, Gtk::PACK_SHRINK);

    answersBox->pack_start(*hb, Gtk::PACK_SHRINK);
    nqs.answerRows.push_back(hb);
}

inline void MainWindow::onSaveQuestion() {
    std::string qText = tvQuestion->get_buffer()->get_text();
    if (qText.empty()) {
        auto* dlg = new Gtk::MessageDialog(*this, "Savol matnini kiriting!", false,
                                            Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg; return;
    }

    Question q;
    q.text = qText;
    bool hasCorrect = false;

    for (auto* row : nqs.answerRows) {
        auto children = row->get_children();
        if (children.size() < 2) continue;
        auto* chk = dynamic_cast<Gtk::CheckButton*>(children[0]);
        auto* ent = dynamic_cast<Gtk::Entry*>(children[1]);
        if (!ent) continue;
        std::string aText = ent->get_text();
        if (aText.empty()) continue;
        Answer a;
        a.text = aText;
        a.isCorrect = chk && chk->get_active();
        if (a.isCorrect) hasCorrect = true;
        q.answers.push_back(a);
    }

    if (q.answers.size() < 2) {
        auto* dlg = new Gtk::MessageDialog(*this, "Kamida 2 ta javob kiriting!", false,
                                            Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg; return;
    }
    if (!hasCorrect) {
        auto* dlg = new Gtk::MessageDialog(*this, "Kamida 1 ta to'g'ri javobni belgilang!", false,
                                            Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg; return;
    }

    nqs.questions.push_back(q);

    // Update list
    auto* row = Gtk::make_managed<Gtk::ListBoxRow>();
    auto* lbl = makeLabel(
        "<b>" + std::to_string(nqs.questions.size()) + ".</b> " + qText.substr(0, 60) +
        (qText.size() > 60 ? "..." : ""));
    lbl->set_margin_top(6);
    lbl->set_margin_bottom(6);
    lbl->set_margin_start(6);
    lbl->set_margin_end(6);
    row->add(*lbl);
    lbQuestionList->append(*row);
    lbQuestionList->show_all();

    lblQCount->set_markup("<b>Savollar soni: " + std::to_string(nqs.questions.size()) + "</b>");

    // Clear inputs
    tvQuestion->get_buffer()->set_text("");
    for (auto* r : nqs.answerRows) {
        auto ch = r->get_children();
        if (auto* ent = dynamic_cast<Gtk::Entry*>(ch.size() > 1 ? ch[1] : nullptr))
            ent->set_text("");
        if (auto* chk = dynamic_cast<Gtk::CheckButton*>(ch.size() > 0 ? ch[0] : nullptr))
            chk->set_active(false);
    }
}

inline void MainWindow::onSaveQuiz() {
    std::string name = entQuizName->get_text();
    if (name.empty()) {
        auto* dlg = new Gtk::MessageDialog(*this, "Quiz nomini kiriting!", false,
                                            Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg; return;
    }
    if (nqs.questions.empty()) {
        auto* dlg = new Gtk::MessageDialog(*this, "Hech bo'lmasa 1 ta savol qo'shing!", false,
                                            Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg; return;
    }

    if (db.addQuizSet(name, nqs.questions)) {
        auto* dlg = new Gtk::MessageDialog(*this,
            "✅ Quiz saqlandi! (" + std::to_string(nqs.questions.size()) + " savol)",
            false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg;
        clearNewQuiz();
        goHome();
    } else {
        auto* dlg = new Gtk::MessageDialog(*this, "❌ Saqlashda xato!", false,
                                            Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg;
    }
}

inline void MainWindow::clearNewQuiz() {
    nqs.questions.clear();
    nqs.answerRows.clear();
    if (entQuizName) entQuizName->set_text("");
    if (tvQuestion) tvQuestion->get_buffer()->set_text("");
    if (answersBox) {
        for (auto* ch : answersBox->get_children())
            answersBox->remove(*ch);
        addAnswerRow(false); addAnswerRow(true); addAnswerRow(false);
        answersBox->show_all();
    }
    if (lblQCount) lblQCount->set_markup("<b>Savollar soni: 0</b>");
    if (lbQuestionList)
        for (auto* ch : lbQuestionList->get_children())
            lbQuestionList->remove(*ch);
}

// ---- IMPORT ----
inline Gtk::Widget* MainWindow::buildImportPage() {
    auto* vb = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 12);
    vb->set_margin_top(24);
    vb->set_margin_bottom(24);
    vb->set_margin_start(24);
    vb->set_margin_end(24);

    vb->pack_start(*makeLabel(
        "<span size='x-large' weight='bold' foreground='#667eea'>📂 TXT Fayldan Import</span>"),
        Gtk::PACK_SHRINK);

    // Format guide
    auto* frame = Gtk::make_managed<Gtk::Frame>();
    frame->set_label("📄 Format namunasi");
    auto* tv = Gtk::make_managed<Gtk::TextView>();
    tv->set_editable(false);
    tv->get_buffer()->set_text(
        "++++\n"
        "Savol matni shu yerda\n"
        "====\n"
        "Javob 1\n"
        "====\n"
        "#To'g'ri javob  ← # belgisi to'g'ri\n"
        "====\n"
        "Javob 3\n"
        "++++");
    tv->set_size_request(-1, 140);
    frame->add(*tv);
    vb->pack_start(*frame, Gtk::PACK_SHRINK);

    // File chooser
    auto* hb = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 8);
    auto* chooseBtn = makeButton("📁 Faylni tanlash", "btn-primary");
    chooseBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onChooseFile));
    hb->pack_start(*chooseBtn, Gtk::PACK_SHRINK);
    lblSelectedFile = makeLabel("<i>Hech narsa tanlanmadi</i>");
    hb->pack_start(*lblSelectedFile, Gtk::PACK_SHRINK);
    vb->pack_start(*hb, Gtk::PACK_SHRINK);

    lblParseResult = makeLabel("");
    vb->pack_start(*lblParseResult, Gtk::PACK_SHRINK);

    // Quiz name for import
    auto* nb = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 8);
    nb->pack_start(*makeLabel("<b>Quiz nomi:</b>"), Gtk::PACK_SHRINK);
    entImportName = Gtk::make_managed<Gtk::Entry>();
    entImportName->set_hexpand(true);
    nb->pack_start(*entImportName, Gtk::PACK_EXPAND_WIDGET);
    vb->pack_start(*nb, Gtk::PACK_SHRINK);

    btnDoImport = makeButton("📥 Import qilish", "btn-success");
    btnDoImport->set_sensitive(false);
    btnDoImport->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onDoImport));
    vb->pack_start(*btnDoImport, Gtk::PACK_SHRINK);

    return vb;
}

inline void MainWindow::onChooseFile() {
    Gtk::FileChooserDialog dlg("TXT faylni tanlang", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dlg.set_transient_for(*this);
    dlg.add_button("Bekor", Gtk::RESPONSE_CANCEL);
    dlg.add_button("Tanlash", Gtk::RESPONSE_OK);

    auto filter = Gtk::FileFilter::create();
    filter->set_name("Matn fayllari");
    filter->add_pattern("*.txt");
    dlg.add_filter(filter);

    if (dlg.run() == Gtk::RESPONSE_OK) {
        std::string path = dlg.get_filename();
        lblSelectedFile->set_text(path);

        lastParseResult = TXTParser::parse(path);
        if (lastParseResult.success) {
            std::string msg = "✅ " + std::to_string(lastParseResult.questions.size()) +
                              " ta savol topildi";
            if (!lastParseResult.errors.empty())
                msg += " (⚠️ " + std::to_string(lastParseResult.errors.size()) + " ogohlantirish)";
            lblParseResult->set_markup("<span foreground='green'>" + msg + "</span>");
            entImportName->set_text(lastParseResult.suggestedName);
            btnDoImport->set_sensitive(true);
        } else {
            std::string errs;
            for (auto& e : lastParseResult.errors) errs += "• " + e + "\n";
            lblParseResult->set_markup(
                "<span foreground='red'>❌ Xato:\n" + errs + "</span>");
            btnDoImport->set_sensitive(false);
        }
    }
}

inline void MainWindow::onDoImport() {
    std::string name = entImportName->get_text();
    if (name.empty()) name = lastParseResult.suggestedName;

    if (db.addQuizSet(name, lastParseResult.questions)) {
        auto* dlg = new Gtk::MessageDialog(*this,
            "✅ " + std::to_string(lastParseResult.questions.size()) +
            " ta savol import qilindi!",
            false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg;
        btnDoImport->set_sensitive(false);
        lblParseResult->set_text("");
        lblSelectedFile->set_markup("<i>Hech narsa tanlanmadi</i>");
        entImportName->set_text("");
        goHome();
    } else {
        auto* dlg = new Gtk::MessageDialog(*this, "❌ Import xato!", false,
                                            Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg;
    }
}

// ---- PLAY ----
inline Gtk::Widget* MainWindow::buildPlayPage() {
    auto* vb = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 12);
    vb->set_margin_top(24);
    vb->set_margin_bottom(24);
    vb->set_margin_start(24);
    vb->set_margin_end(24);

    vb->pack_start(*makeLabel(
        "<span size='x-large' weight='bold' foreground='#667eea'>🎮 Quiz O'yna</span>"),
        Gtk::PACK_SHRINK);

    vb->pack_start(*makeLabel("<b>Quiz setini tanlang:</b>"), Gtk::PACK_SHRINK);

    lbQuizSets = Gtk::make_managed<Gtk::ListBox>();
    lbQuizSets->set_selection_mode(Gtk::SELECTION_SINGLE);
    lbQuizSets->set_size_request(-1, 160);
    vb->pack_start(*lbQuizSets, Gtk::PACK_SHRINK);

    auto* startBtn = makeButton("🎬 Quizni boshlash", "btn-primary");
    startBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::startQuiz));
    vb->pack_start(*startBtn, Gtk::PACK_SHRINK);

    vb->pack_start(*makeSeparator(), Gtk::PACK_SHRINK);

    // Play area (initially hidden)
    playArea = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 10);

    lblQNum = Gtk::make_managed<Gtk::Label>();
    lblQNum->set_halign(Gtk::ALIGN_END);
    playArea->pack_start(*lblQNum, Gtk::PACK_SHRINK);

    pbProgress = Gtk::make_managed<Gtk::ProgressBar>();
    pbProgress->set_show_text(true);
    playArea->pack_start(*pbProgress, Gtk::PACK_SHRINK);

    lblQuestion = Gtk::make_managed<Gtk::Label>();
    lblQuestion->set_line_wrap(true);
    lblQuestion->set_max_width_chars(60);
    lblQuestion->set_margin_top(10);
    lblQuestion->set_margin_bottom(10);
    playArea->pack_start(*lblQuestion, Gtk::PACK_SHRINK);

    answersPlayBox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 6);
    playArea->pack_start(*answersPlayBox, Gtk::PACK_SHRINK);

    lblFeedback = Gtk::make_managed<Gtk::Label>();
    lblFeedback->set_halign(Gtk::ALIGN_CENTER);
    playArea->pack_start(*lblFeedback, Gtk::PACK_SHRINK);

    auto* scoreBox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
    lblScore = Gtk::make_managed<Gtk::Label>();
    lblScore->set_halign(Gtk::ALIGN_START);
    scoreBox->pack_start(*lblScore, Gtk::PACK_SHRINK);
    playArea->pack_start(*scoreBox, Gtk::PACK_SHRINK);

    btnNextQ = makeButton("⏭️ Keyingi savol", "btn-primary");
    btnNextQ->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onNextQuestion));
    btnNextQ->set_sensitive(false);
    playArea->pack_start(*btnNextQ, Gtk::PACK_SHRINK);

    vb->pack_start(*playArea, Gtk::PACK_SHRINK);

    return vb;
}

inline void MainWindow::refreshPlayList() {
    quizActive = false;
    if (playArea) playArea->set_visible(false);
    if (!lbQuizSets) return;

    for (auto* ch : lbQuizSets->get_children())
        lbQuizSets->remove(*ch);

    auto sets = db.getAllQuizSets();
    for (auto& s : sets) {
        auto* row = Gtk::make_managed<Gtk::ListBoxRow>();
        row->set_data("quiz_id", GINT_TO_POINTER(s.id));
        auto* lbl = makeLabel(
            "🗂️ <b>" + s.name + "</b>  <span foreground='#888'>(" +
            std::to_string(s.questions.size()) + " savol)</span>");
        lbl->set_margin_top(10);
        lbl->set_margin_bottom(10);
        lbl->set_margin_start(10);
        lbl->set_margin_end(10);
        row->add(*lbl);
        lbQuizSets->append(*row);
    }
    lbQuizSets->show_all();
}

inline void MainWindow::startQuiz() {
    auto* row = lbQuizSets->get_selected_row();
    if (!row) {
        auto* dlg = new Gtk::MessageDialog(*this, "Quiz setini tanlang!", false,
                                            Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg; return;
    }

    selectedQuizId = GPOINTER_TO_INT(row->get_data("quiz_id"));
    auto quizSet = db.getFullQuizSet(selectedQuizId);
    if (quizSet.questions.empty()) {
        auto* dlg = new Gtk::MessageDialog(*this, "Bu setda savollar yo'q!", false,
                                            Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg; return;
    }

    engine.startQuiz(quizSet);
    quizActive = true;
    playArea->set_visible(true);
    showQuestion();
}

inline void MainWindow::showQuestion() {
    if (!engine.hasMore()) {
        // Show results
        auto result = engine.getResult();
        db.saveResult(result);

        std::string grade = engine.getGrade();
        std::string msg =
            "🏁 Quiz tugadi!\n\n"
            "✅ To'g'ri: " + std::to_string(result.score) + "/" + std::to_string(result.total) + "\n"
            "📊 Foiz: " + std::to_string(static_cast<int>(result.percentage)) + "%\n"
            "🏆 Baho: " + grade;

        auto* dlg = new Gtk::MessageDialog(*this, msg, false,
                                            Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg;
        playArea->set_visible(false);
        quizActive = false;
        refreshHome();
        goHome();
        return;
    }

    const auto& q = engine.getCurrentQuestion();
    int cur = engine.getCurrentIndex() + 1;
    int tot = engine.getTotal();
    double frac = static_cast<double>(cur - 1) / tot;

    lblQNum->set_markup(
        "<span foreground='#667eea'><b>Savol " + std::to_string(cur) + " / " +
        std::to_string(tot) + "</b></span>");

    pbProgress->set_fraction(frac);
    pbProgress->set_text(std::to_string(static_cast<int>(frac * 100)) + "%");

    lblQuestion->set_markup(
        "<span size='large' weight='bold'>" + q.text + "</span>");

    // Clear answers
    for (auto* ch : answersPlayBox->get_children())
        answersPlayBox->remove(*ch);

    auto answers = engine.getShuffledAnswers();
    for (int i = 0; i < static_cast<int>(answers.size()); ++i) {
        auto* btn = makeButton(answers[i].text, "btn-answer");
        btn->set_hexpand(true);
        int idx = i;
        btn->signal_clicked().connect([this, idx]() { onAnswerClicked(idx); });
        answersPlayBox->pack_start(*btn, Gtk::PACK_SHRINK);
    }
    answersPlayBox->show_all();

    lblFeedback->set_text("");
    lblScore->set_markup(
        "⭐ Ball: <b>" + std::to_string(engine.getScore()) + "/" +
        std::to_string(engine.getTotal()) + "</b>");
    btnNextQ->set_sensitive(false);
}

inline void MainWindow::onAnswerClicked(int idx) {
    if (engine.isAnswered()) return;

    bool correct = engine.submitAnswer(idx);
    lblFeedback->set_markup(
        correct
        ? "<span foreground='green' size='large'><b>✅ To'g'ri!</b></span>"
        : "<span foreground='red' size='large'><b>❌ Noto'g'ri!</b></span>");

    // Color buttons
    auto children = answersPlayBox->get_children();
    auto answers = engine.getShuffledAnswers(); // re-read current (already stored)
    for (int i = 0; i < static_cast<int>(children.size()); ++i) {
        if (auto* btn = dynamic_cast<Gtk::Button*>(children[i])) {
            btn->set_sensitive(false);
        }
    }

    lblScore->set_markup(
        "⭐ Ball: <b>" + std::to_string(engine.getScore()) + "/" +
        std::to_string(engine.getTotal()) + "</b>");

    btnNextQ->set_sensitive(true);
}

inline void MainWindow::onNextQuestion() {
    engine.nextQuestion();
    showQuestion();
}

// ---- DELETE ----
inline Gtk::Widget* MainWindow::buildDeletePage() {
    auto* vb = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 12);
    vb->set_margin_top(24);
    vb->set_margin_bottom(24);
    vb->set_margin_start(24);
    vb->set_margin_end(24);

    vb->pack_start(*makeLabel(
        "<span size='x-large' weight='bold' foreground='#e74c3c'>🗑️ Quiz Setni O'chirish</span>"),
        Gtk::PACK_SHRINK);

    vb->pack_start(*makeLabel("<b>O'chirish uchun setni tanlang:</b>"), Gtk::PACK_SHRINK);

    lbDeleteSets = Gtk::make_managed<Gtk::ListBox>();
    lbDeleteSets->set_selection_mode(Gtk::SELECTION_SINGLE);
    lbDeleteSets->set_size_request(-1, 220);
    vb->pack_start(*lbDeleteSets, Gtk::PACK_SHRINK);

    auto* hb = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
    auto* refreshBtn = makeButton("🔄 Yangilash", "btn-primary");
    refreshBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::refreshDeleteList));

    auto* delBtn = makeButton("🗑️ O'chirish", "btn-danger");
    delBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onDeleteSelected));

    hb->pack_start(*refreshBtn, Gtk::PACK_SHRINK);
    hb->pack_start(*delBtn, Gtk::PACK_SHRINK);
    vb->pack_start(*hb, Gtk::PACK_SHRINK);

    auto* warn = makeLabel(
        "<span foreground='#e74c3c'>⚠️ Diqqat! O'chirilgan setni qayta tiklab bo'lmaydi!</span>",
        true);
    vb->pack_start(*warn, Gtk::PACK_SHRINK);

    return vb;
}

inline void MainWindow::refreshDeleteList() {
    if (!lbDeleteSets) return;
    for (auto* ch : lbDeleteSets->get_children())
        lbDeleteSets->remove(*ch);

    auto sets = db.getAllQuizSets();
    if (sets.empty()) {
        auto* row = Gtk::make_managed<Gtk::ListBoxRow>();
        auto* lbl = makeLabel("<i>Hech qanday quiz yo'q</i>");
        lbl->set_margin_top(10);
        lbl->set_margin_bottom(10);
        lbl->set_margin_start(10);
        lbl->set_margin_end(10);
        row->add(*lbl);
        lbDeleteSets->append(*row);
    }
    for (auto& s : sets) {
        auto* row = Gtk::make_managed<Gtk::ListBoxRow>();
        row->set_data("quiz_id", GINT_TO_POINTER(s.id));
        auto* lbl = makeLabel(
            "📋 <b>" + s.name + "</b>  <span foreground='#888'>(" +
            std::to_string(s.questions.size()) + " savol)</span>");
        lbl->set_margin_top(10);
        lbl->set_margin_bottom(10);
        lbl->set_margin_start(10);
        lbl->set_margin_end(10);
        row->add(*lbl);
        lbDeleteSets->append(*row);
    }
    lbDeleteSets->show_all();
}

inline void MainWindow::onDeleteSelected() {
    auto* row = lbDeleteSets->get_selected_row();
    if (!row) {
        auto* dlg = new Gtk::MessageDialog(*this, "O'chirish uchun set tanlang!", false,
                                            Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
        dlg->run(); dlg->hide(); delete dlg; return;
    }

    int id = GPOINTER_TO_INT(row->get_data("quiz_id"));

    auto* dlg = new Gtk::MessageDialog(*this,
        "Haqiqatan ham o'chirishni xohlaysizmi?",
        false, Gtk::MESSAGE_QUESTION,
        Gtk::BUTTONS_YES_NO, true);
    int resp = dlg->run();
    dlg->hide(); delete dlg;

    if (resp == Gtk::RESPONSE_YES) {
        if (db.deleteQuizSet(id)) {
            refreshDeleteList();
            refreshHome();
        } else {
            auto* err = new Gtk::MessageDialog(*this, "❌ O'chirishda xato!", false,
                                                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
            err->run(); err->hide(); delete err;
        }
    }
}

// ---- SETTINGS ----
inline Gtk::Widget* MainWindow::buildSettingsPage() {
    auto* vb = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 14);
    vb->set_margin_top(24);
    vb->set_margin_bottom(24);
    vb->set_margin_start(24);
    vb->set_margin_end(24);

    vb->pack_start(*makeLabel(
        "<span size='x-large' weight='bold' foreground='#667eea'>⚙️ Sozlamalar</span>"),
        Gtk::PACK_SHRINK);

    // Interface
    auto* ifFrame = Gtk::make_managed<Gtk::Frame>();
    ifFrame->set_label("🎨 Interfeys");
    auto* ifBox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 8);
    ifBox->set_margin_top(12);
    ifBox->set_margin_bottom(12);
    ifBox->set_margin_start(12);
    ifBox->set_margin_end(12);

    auto* dmRow = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
    dmRow->pack_start(*makeLabel("Qorong'i rejim (Dark Mode):"), Gtk::PACK_SHRINK);
    swDarkMode = Gtk::make_managed<Gtk::Switch>();
    swDarkMode->signal_state_set().connect([this](bool state) -> bool {
        applyCSS(state);
        return false;
    }, false);
    dmRow->pack_start(*swDarkMode, Gtk::PACK_SHRINK);
    ifBox->pack_start(*dmRow, Gtk::PACK_SHRINK);
    ifFrame->add(*ifBox);
    vb->pack_start(*ifFrame, Gtk::PACK_SHRINK);

    // DB path
    auto* dbFrame = Gtk::make_managed<Gtk::Frame>();
    dbFrame->set_label("📂 Ma'lumotlar bazasi");
    auto* dbBox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 8);
    dbBox->set_margin_top(12);
    dbBox->set_margin_bottom(12);
    dbBox->set_margin_start(12);
    dbBox->set_margin_end(12);
    auto* pathLbl = makeLabel("Baza fayli: <tt>~/.quiz_master_pro.db</tt>", true);
    dbBox->pack_start(*pathLbl, Gtk::PACK_SHRINK);
    dbFrame->add(*dbBox);
    vb->pack_start(*dbFrame, Gtk::PACK_SHRINK);

    // About
    auto* aboutFrame = Gtk::make_managed<Gtk::Frame>();
    aboutFrame->set_label("ℹ️ Dastur haqida");
    auto* aboutBox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 6);
    aboutBox->set_margin_top(12);
    aboutBox->set_margin_bottom(12);
    aboutBox->set_margin_start(12);
    aboutBox->set_margin_end(12);
    aboutBox->pack_start(*makeLabel("<b>Quiz Master Pro</b> v1.0.0"), Gtk::PACK_SHRINK);
    aboutBox->pack_start(*makeLabel("C++17 + GTKMM3 + SQLite3"), Gtk::PACK_SHRINK);
    aboutBox->pack_start(*makeLabel("Linux da ishlaydi"), Gtk::PACK_SHRINK);
    aboutFrame->add(*aboutBox);
    vb->pack_start(*aboutFrame, Gtk::PACK_SHRINK);

    return vb;
}