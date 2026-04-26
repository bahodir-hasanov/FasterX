#pragma once

#include <QString>
#include <vector>
#include <string>

struct Answer {
    int id;
    QString text;
    bool isCorrect;
};

struct Question {
    int id;
    QString text;
    std::vector<Answer> answers;
    int orderIndex;
};

class TXTParser {
public:
    struct ParseResult {
        bool success;
        QString quizName;
        std::vector<Question> questions;
        std::vector<QString> errors;
    };

    static ParseResult parse(const QString& filename);
    static ParseResult parse(const std::string& filename);

    static QString trim(const QString& s);
    static QString extractNameFromPath(const QString& path);
};