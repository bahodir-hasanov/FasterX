#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>
#include <ctime>
#include <QString>

// ========== JAVOB STRUKTURA ==========
struct Answer {
    int id;
    std::string text;
    bool isCorrect;

    Answer() : id(-1), isCorrect(false) {}
    Answer(int id, const std::string& text, bool isCorrect)
        : id(id), text(text), isCorrect(isCorrect) {}
};

// ========== SAVOL STRUKTURA ==========
struct Question {
    int id;
    std::string text;
    std::vector<Answer> answers;
    int orderIndex;

    Question() : id(-1), orderIndex(0) {}
    Question(int id, const std::string& text, int order)
        : id(id), text(text), orderIndex(order) {}

    bool isValid() const {
        if (text.empty()) return false;
        if (answers.size() < 2 || answers.size() > 6) return false;
        
        bool hasCorrect = false;
        for (const auto& ans : answers) {
            if (ans.isCorrect) {
                hasCorrect = true;
                break;
            }
        }
        return hasCorrect;
    }
};

// ========== QUIZ SETI STRUKTURA ==========
struct QuizSet {
    int id;
    std::string name;
    std::vector<Question> questions;
    time_t createdAt;
    time_t updatedAt;

    QuizSet() : id(-1), createdAt(0), updatedAt(0) {}
    QuizSet(int id, const std::string& name)
        : id(id), name(name), createdAt(time(nullptr)), updatedAt(time(nullptr)) {}

    int getQuestionCount() const {
        return static_cast<int>(questions.size());
    }
};

// ========== QUIZ NATIJASI STRUKTURA ==========
struct QuizResult {
    int id;
    int quizSetId;
    std::string quizSetName;
    int score;
    int total;
    double percentage;
    time_t completedAt;
    std::vector<bool> correctAnswers;

    QuizResult()
        : id(-1), quizSetId(-1), quizSetName(""), score(0), total(0), percentage(0.0), completedAt(0) {}

    QuizResult(int quizSetId, int score, int total)
        : quizSetId(quizSetId), score(score), total(total),
          percentage(total > 0 ? (score * 100.0 / total) : 0.0),
          completedAt(time(nullptr)) {}

    QString getGrade() const {
        if (percentage >= 90) return "A+ (Mukammal!)";
        if (percentage >= 85) return "A (Ajoyib!)";
        if (percentage >= 75) return "B (Yaxshi)";
        if (percentage >= 60) return "C (Qoniqarli)";
        return "D (Ko'proq o'qish kerak)";
    }
};

// ========== FOYDALANUVCHI STATISTIKASI ==========
struct UserStats {
    int totalQuizzes;
    int totalQuestions;
    int correctAnswers;
    double averageScore;
    std::vector<QuizResult> recentResults;

    UserStats()
        : totalQuizzes(0), totalQuestions(0), correctAnswers(0), averageScore(0.0) {}

    void updateFromResults(const std::vector<QuizResult>& results) {
        totalQuizzes = results.size();
        totalQuestions = 0;
        correctAnswers = 0;
        double totalScore = 0.0;

        for (const auto& result : results) {
            totalQuestions += result.total;
            correctAnswers += result.score;
            totalScore += result.percentage;
        }

        averageScore = totalQuizzes > 0 ? totalScore / totalQuizzes : 0.0;
        recentResults = results;
    }
};

// ========== ILOVANI SOZLAMALARI ==========
struct AppSettings {
    bool darkMode;
    std::string language;
    int fontSize;
    int answerTimeSeconds;
    bool allowRepeat;
    bool soundEnabled;
    std::string dbPath;

    AppSettings() 
        : darkMode(false), language("uz"), fontSize(14),
          answerTimeSeconds(30), allowRepeat(true),
          soundEnabled(false), dbPath("%APPDATA%/QuizMasterPro/quiz.db") {}
};

#endif // MODELS_H
