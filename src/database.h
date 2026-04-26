#ifndef DATABASE_H
#define DATABASE_H

#include "models.h"
#include <QString>
#include <mutex>
#include <sqlite3.h>
#include <vector>
#include <functional>

class Database {
public:
    explicit Database(const QString& dbPath = "");
    ~Database();

    // Quiz Sets bilan ishlash
    bool addQuizSet(const std::string& name, const std::vector<Question>& questions);
    bool updateQuizSet(int id, const std::string& name, const std::vector<Question>& questions);
    bool deleteQuizSet(int id);
    std::vector<QuizSet> getAllQuizSets();
    QuizSet getFullQuizSet(int id);
    bool quizSetExists(const std::string& name);

    // Results bilan ishlash
    bool saveResult(const QuizResult& result);
    std::vector<QuizResult> getTopResults(int limit = 10);
    std::vector<QuizResult> getResultsBySet(int setId);
    UserStats getUserStats();
    bool clearResults();

    // Settings
    bool saveSetting(const std::string& key, const std::string& value);
    std::string getSetting(const std::string& key, const std::string& defaultVal = "");
    AppSettings loadSettings();
    bool saveSettings(const AppSettings& settings);

    // Utility methods
    bool isOpen() const { return db != nullptr; }
    QString getLastError() const { return lastError; }
    QString getDbPath() const { return currentPath; }

private:
    sqlite3* db;
    std::mutex dbMutex;
    QString lastError;
    QString currentPath;

    bool openDatabase(const QString& path);
    bool createTables();
    bool createIndexes();
    bool executeSQL(const std::string& sql);
    bool executePrepared(const QString& sql,
                         std::function<void(sqlite3_stmt*)> bindFunc,
                         std::function<void(sqlite3_stmt*)> rowFunc = nullptr);
};

#endif // DATABASE_H