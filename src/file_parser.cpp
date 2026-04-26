#include "file_parser.h"
#include <fstream>

// ----------- HELPERS -----------

QString TXTParser::trim(const QString& s) {
    return s.trimmed();
}

QString TXTParser::extractNameFromPath(const QString& path) {
    int lastSlash = std::max(path.lastIndexOf('/'), path.lastIndexOf('\\'));
    QString filename = (lastSlash != -1) ? path.mid(lastSlash + 1) : path;
    int lastDot = filename.lastIndexOf('.');
    return (lastDot != -1) ? filename.left(lastDot) : filename;
}

// ----------- MAIN PARSER -----------

TXTParser::ParseResult TXTParser::parse(const QString& filename) {
    ParseResult result;
    result.success = false;
    result.quizName = extractNameFromPath(filename);

    std::ifstream file(filename.toStdString());
    if (!file.is_open()) {
        result.errors.push_back("Faylni ochib bo'lmadi: " + filename);
        return result;
    }

    std::string line;
    int lineNum = 0;

    enum State { IDLE, IN_QUESTION, IN_ANSWERS };
    State state = IDLE;

    QString currentQuestionText;
    std::vector<Answer> currentAnswers;
    bool foundCorrect = false;
    int questionStartLine = 0;
    int answerIdCounter = 1;
    int questionCount = 0;

    auto finalizeQuestion = [&]() {
        if (currentQuestionText.isEmpty()) return;

        if (currentAnswers.size() < 2) {
            result.errors.push_back(
                QString("Savol #%1 uchun kamida 2 ta javob kerak (qator %2)")
                    .arg(questionCount + 1)
                    .arg(questionStartLine)
            );
            return;
        }

        if (!foundCorrect) {
            currentAnswers[0].isCorrect = true;
        }

        Question q;
        q.id = questionCount + 1;
        q.text = currentQuestionText;
        q.answers = currentAnswers;
        q.orderIndex = questionCount;

        result.questions.push_back(q);
        questionCount++;

        currentQuestionText.clear();
        currentAnswers.clear();
        foundCorrect = false;
    };

    while (std::getline(file, line)) {
        lineNum++;

        QString trimmed = trim(QString::fromStdString(line));
        if (trimmed.isEmpty()) continue;

        if (trimmed == "++++") {
            if (state == IDLE) {
                state = IN_QUESTION;
                questionStartLine = lineNum;
            } else if (state == IN_ANSWERS) {
                finalizeQuestion();
                state = IDLE;
            }
            continue;
        }

        if (trimmed == "====" || trimmed == "----") {
            if (state == IN_QUESTION) {
                state = IN_ANSWERS;
            }
            continue;
        }

        if (state == IN_QUESTION) {
            if (!currentQuestionText.isEmpty())
                currentQuestionText += "\n";

            currentQuestionText += trimmed;
        }
        else if (state == IN_ANSWERS) {
            Answer a;
            a.id = answerIdCounter++;

            if (trimmed.startsWith("#")) {
                a.text = trim(trimmed.mid(1));
                a.isCorrect = true;
                foundCorrect = true;
            } else {
                a.text = trimmed;
                a.isCorrect = false;
            }

            if (!a.text.isEmpty()) {
                currentAnswers.push_back(a);
            }
        }
    }

    // Fayl oxirida ++++ bo‘lmasa ham ishlaydi
    if (state == IN_ANSWERS && !currentQuestionText.isEmpty()) {
        finalizeQuestion();
    }

    if (result.questions.empty()) {
        result.errors.push_back("Faylda hech qanday savol topilmadi.");
        return result;
    }

    result.success = true;
    return result;
}

// ----------- WRAPPER -----------

TXTParser::ParseResult TXTParser::parse(const std::string& filename) {
    return parse(QString::fromStdString(filename));
}