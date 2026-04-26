#include "database.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <iostream>
#include <stdexcept>
#include <mutex>

Database::Database(const QString& dbPath) : db(nullptr) {
    QString path = dbPath;
    if (path.isEmpty()) {
        // Windows default: AppData
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(appDataPath);
        if (!dir.exists()) {
            dir.mkpath(appDataPath);
        }
        path = appDataPath + "/quiz_master.db";
    }
    
    currentPath = path;

    if (!openDatabase(path)) {
        throw std::runtime_error(std::string("Ma'lumotlar bazasini ochib bo'lmadi: ") + lastError.toStdString());
    }
    createTables();
    createIndexes();
}

Database::~Database() {
    if (db) sqlite3_close(db);
}

bool Database::openDatabase(const QString& path) {
    int rc = sqlite3_open(path.toStdString().c_str(), &db);
    if (rc != SQLITE_OK) {
        lastError = QString::fromLocal8Bit(sqlite3_errmsg(db));
        sqlite3_close(db);
        db = nullptr;
        return false;
    }
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    return true;
}

bool Database::createTables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS quiz_sets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            created_at INTEGER DEFAULT (strftime('%s','now')),
            updated_at INTEGER DEFAULT (strftime('%s','now'))
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
            quiz_set_id INTEGER,
            quiz_set_name TEXT,
            score INTEGER NOT NULL,
            total INTEGER NOT NULL,
            percentage REAL NOT NULL,
            completed_at INTEGER DEFAULT (strftime('%s','now')),
            FOREIGN KEY(quiz_set_id) REFERENCES quiz_sets(id) ON DELETE SET NULL
        );
        
        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );
    )";
    return executeSQL(sql);
}

bool Database::createIndexes() {
    executeSQL("CREATE INDEX IF NOT EXISTS idx_questions_set ON questions(quiz_set_id);");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_answers_question ON answers(question_id);");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_results_date ON results(completed_at DESC);");
    executeSQL("CREATE INDEX IF NOT EXISTS idx_results_pct ON results(percentage DESC);");
    return true;
}

bool Database::executeSQL(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        lastError = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::addQuizSet(const std::string& name, const std::vector<Question>& questions) {
    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    sqlite3_stmt* stmt;
    const char* insertSet = "INSERT INTO quiz_sets (name) VALUES (?);";
    if (sqlite3_prepare_v2(db, insertSet, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        lastError = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    int setId = (int)sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    int orderIdx = 0;
    for (const auto& q : questions) {
        const char* insertQ = "INSERT INTO questions (quiz_set_id, text, order_index) VALUES (?, ?, ?);";
        sqlite3_prepare_v2(db, insertQ, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, setId);
        sqlite3_bind_text(stmt, 2, q.text.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, orderIdx++);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }
        int qId = (int)sqlite3_last_insert_rowid(db);
        sqlite3_finalize(stmt);

        for (const auto& a : q.answers) {
            const char* insertA = "INSERT INTO answers (question_id, text, is_correct) VALUES (?, ?, ?);";
            sqlite3_prepare_v2(db, insertA, -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, qId);
            sqlite3_bind_text(stmt, 2, a.text.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 3, a.isCorrect ? 1 : 0);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
                return false;
            }
            sqlite3_finalize(stmt);
        }
    }
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    return true;
}

bool Database::deleteQuizSet(int id) {
    std::lock_guard<std::mutex> lock(dbMutex);
    std::string sql = "DELETE FROM quiz_sets WHERE id = " + std::to_string(id) + ";";
    return executeSQL(sql);
}

bool Database::quizSetExists(const std::string& name) {
    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM quiz_sets WHERE name = ?;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count > 0;
}

std::vector<QuizSet> Database::getAllQuizSets() {
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<QuizSet> sets;

    const char* sql = R"(
        SELECT qs.id, qs.name, COUNT(DISTINCT q.id) as qcount, qs.created_at
        FROM quiz_sets qs
        LEFT JOIN questions q ON qs.id = q.quiz_set_id
        GROUP BY qs.id
        ORDER BY qs.created_at DESC;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            QuizSet s;
            s.id = sqlite3_column_int(stmt, 0);
            const char* nm = (const char*)sqlite3_column_text(stmt, 1);
            s.name = nm ? nm : "";
            // Store question count in a temporary Question to avoid refetch
            int qcount = sqlite3_column_int(stmt, 2);
            s.questions.resize(qcount); // placeholder for count
            s.createdAt = sqlite3_column_int(stmt, 3);
            sets.push_back(s);
        }
    }
    sqlite3_finalize(stmt);
    return sets;
}

QuizSet Database::getFullQuizSet(int id) {
    QuizSet set;
    set.id = id;

    sqlite3_stmt* stmt;
    const char* getName = "SELECT name, created_at FROM quiz_sets WHERE id = ?;";
    sqlite3_prepare_v2(db, getName, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* nm = (const char*)sqlite3_column_text(stmt, 0);
        set.name = nm ? nm : "";
        set.createdAt = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);

    const char* getQ = "SELECT id, text, order_index FROM questions WHERE quiz_set_id = ? ORDER BY order_index;";
    sqlite3_prepare_v2(db, getQ, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Question q;
        q.id = sqlite3_column_int(stmt, 0);
        const char* qtxt = (const char*)sqlite3_column_text(stmt, 1);
        q.text = qtxt ? qtxt : "";
        q.orderIndex = sqlite3_column_int(stmt, 2);

        sqlite3_stmt* ansStmt;
        const char* getA = "SELECT id, text, is_correct FROM answers WHERE question_id = ?;";
        sqlite3_prepare_v2(db, getA, -1, &ansStmt, nullptr);
        sqlite3_bind_int(ansStmt, 1, q.id);
        while (sqlite3_step(ansStmt) == SQLITE_ROW) {
            Answer a;
            a.id = sqlite3_column_int(ansStmt, 0);
            const char* atxt = (const char*)sqlite3_column_text(ansStmt, 1);
            a.text = atxt ? atxt : "";
            a.isCorrect = sqlite3_column_int(ansStmt, 2) == 1;
            q.answers.push_back(a);
        }
        sqlite3_finalize(ansStmt);
        set.questions.push_back(q);
    }
    sqlite3_finalize(stmt);
    return set;
}

bool Database::saveResult(const QuizResult& result) {
    std::lock_guard<std::mutex> lock(dbMutex);
    const char* sql = "INSERT INTO results (quiz_set_id, quiz_set_name, score, total, percentage) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, result.quizSetId);
    sqlite3_bind_text(stmt, 2, result.quizSetName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, result.score);
    sqlite3_bind_int(stmt, 4, result.total);
    sqlite3_bind_double(stmt, 5, result.percentage);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<QuizResult> Database::getTopResults(int limit) {
    std::vector<QuizResult> results;
    std::string sql = "SELECT id, quiz_set_id, quiz_set_name, score, total, percentage, completed_at "
                      "FROM results ORDER BY percentage DESC, completed_at DESC LIMIT " + std::to_string(limit) + ";";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QuizResult r;
        r.id = sqlite3_column_int(stmt, 0);
        r.quizSetId = sqlite3_column_int(stmt, 1);
        const char* nm = (const char*)sqlite3_column_text(stmt, 2);
        r.quizSetName = nm ? nm : "O'chirilgan";
        r.score = sqlite3_column_int(stmt, 3);
        r.total = sqlite3_column_int(stmt, 4);
        r.percentage = sqlite3_column_double(stmt, 5);
        r.completedAt = sqlite3_column_int(stmt, 6);
        results.push_back(r);
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<QuizResult> Database::getResultsBySet(int setId) {
    std::vector<QuizResult> results;
    const char* sql = "SELECT id, score, total, percentage, completed_at FROM results WHERE quiz_set_id = ? ORDER BY completed_at DESC;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, setId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QuizResult r;
        r.id = sqlite3_column_int(stmt, 0);
        r.quizSetId = setId;
        r.score = sqlite3_column_int(stmt, 1);
        r.total = sqlite3_column_int(stmt, 2);
        r.percentage = sqlite3_column_double(stmt, 3);
        r.completedAt = sqlite3_column_int(stmt, 4);
        results.push_back(r);
    }
    sqlite3_finalize(stmt);
    return results;
}

UserStats Database::getUserStats() {
    UserStats stats;
    sqlite3_stmt* stmt;
    
    const char* sql = "SELECT COUNT(*), SUM(score), SUM(total), AVG(percentage) FROM results;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stats.totalQuizzes = sqlite3_column_int(stmt, 0);
        stats.correctAnswers = sqlite3_column_int(stmt, 1);
        stats.totalQuestions = sqlite3_column_int(stmt, 2);
        stats.averageScore = sqlite3_column_double(stmt, 3);
    }
    sqlite3_finalize(stmt);
    
    stats.recentResults = getTopResults(5);
    return stats;
}

bool Database::clearResults() {
    return executeSQL("DELETE FROM results;");
}

bool Database::saveSetting(const std::string& key, const std::string& value) {
    const char* sql = "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_STATIC);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::string Database::getSetting(const std::string& key, const std::string& defaultVal) {
    const char* sql = "SELECT value FROM settings WHERE key = ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    std::string result = defaultVal;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* val = (const char*)sqlite3_column_text(stmt, 0);
        if (val) result = val;
    }
    sqlite3_finalize(stmt);
    return result;
}

AppSettings Database::loadSettings() {
    AppSettings s;
    s.darkMode = getSetting("dark_mode", "0") == "1";
    s.language = getSetting("language", "uz");
    s.fontSize = std::stoi(getSetting("font_size", "14"));
    s.answerTimeSeconds = std::stoi(getSetting("answer_time", "30"));
    s.allowRepeat = getSetting("allow_repeat", "1") == "1";
    s.soundEnabled = getSetting("sound", "0") == "1";
    s.dbPath = getDbPath().toStdString();
    return s;
}

bool Database::saveSettings(const AppSettings& s) {
    saveSetting("dark_mode", s.darkMode ? "1" : "0");
    saveSetting("language", s.language);
    saveSetting("font_size", std::to_string(s.fontSize));
    saveSetting("answer_time", std::to_string(s.answerTimeSeconds));
    saveSetting("allow_repeat", s.allowRepeat ? "1" : "0");
    saveSetting("sound", s.soundEnabled ? "1" : "0");
    return true;
}