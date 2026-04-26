#include "file_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

QString TXTParser::trim(const QString& s) {
    return s.trimmed();
}

QString TXTParser::extractNameFromPath(const QString& path) {
    int lastSlash = std::max(path.lastIndexOf('/'), path.lastIndexOf('\\'));
    QString filename = (lastSlash != -1) ? path.mid(lastSlash + 1) : path;
    int lastDot = filename.lastIndexOf('.');
    return (lastDot != -1) ? filename.left(lastDot) : filename;
}

// Format:
// ++++
// Savol matni
// ====
// Javob 1
// ====
// #To'g'ri javob
// ====
// Javob 3
// ++++
TXTParser::ParseResult TXTParser::parse(const QString& filename) {
    ParseResult result;
    result.success = false;
    result.quizName = extractNameFromPath(filename);

    std::ifstream file(filename.toStdString());
    if (!file.is_open()) {
        result.errors.push_back(QString::fromLocal8Bit("Faylni ochib bo'lmadi: ") + filename);
        return result;
    }

    std::string line;
    int lineNum = 0;

    enum State { IDLE, IN_QUESTION, IN_ANSWERS };
    State state = IDLE;

    std::string currentQuestionText;
    std::vector<Answer> currentAnswers;
    bool foundCorrect = false;
    int questionStartLine = 0;
    int answerIdCounter = 1;
    int questionCount = 0;

    auto finalizeQuestion = [&]() {
        if (currentQuestionText.empty()) return;
        if (currentAnswers.size() < 2) {
            result.errors.push_back(QString("Savol #%1 uchun kamida 2 ta javob kerak (qator %2)")
                                        .arg(questionCount + 1)
                                        .arg(questionStartLine));
            return;
        }
        if (!foundCorrect) {
            // Mark first as correct
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
        result.lineNumber = lineNum;

        // Trim the line
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(start, end - start + 1);

        if (trimmed == "++++") {
            if (state == IDLE) {
                state = IN_QUESTION;
                questionStartLine = lineNum;
                currentQuestionText.clear();
                currentAnswers.clear();
                foundCorrect = false;
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
            if (!currentQuestionText.empty()) currentQuestionText += "\n";
            currentQuestionText += trimmed;
        } else if (state == IN_ANSWERS) {
            if (trimmed.empty()) continue;

            Answer a;
            a.id = answerIdCounter++;

            if (!trimmed.empty() && trimmed[0] == '#') {
                // Correct answer
                a.text = trimmed.substr(1);
                // Trim the answer text
                size_t ansStart = a.text.find_first_not_of(" \t");
                if (ansStart != std::string::npos) {
                    size_t ansEnd = a.text.find_last_not_of(" \t\r\n");
                    a.text = a.text.substr(ansStart, ansEnd - ansStart + 1);
                }
                a.isCorrect = true;
                foundCorrect = true;
            } else {
                a.text = trimmed;
                a.isCorrect = false;
            }

            if (!a.text.empty()) {
                currentAnswers.push_back(a);
            }
        }
    }

    // Handle case where file doesn't end with ++++
    if (state == IN_ANSWERS && !currentQuestionText.empty()) {
        finalizeQuestion();
    }

    if (result.questions.empty()) {
        result.errors.push_back(QString::fromLocal8Bit("Faylda hech qanday savol topilmadi. Format to'g'ri ekanligini tekshiring."));
        return result;
    }

    result.success = true;
    return result;
}

// Format:
// ++++
// Savol matni
// ====
// Javob 1
// ====
// #To'g'ri javob
// ====
// Javob 3
// ++++
TXTParser::ParseResult TXTParser::parse(const std::string& filename) {
    ParseResult result;
    result.success = false;
    result.quizName = extractNameFromPath(filename);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        result.errors.push_back("Faylni ochib bo'lmadi: " + filename);
        return result;
    }
    
    std::string line;
    int lineNum = 0;
    
    enum State { IDLE, IN_QUESTION, IN_ANSWERS };
    State state = IDLE;
    
    std::string currentQuestionText;
    std::vector<Answer> currentAnswers;
    bool foundCorrect = false;
    int questionStartLine = 0;
    int answerIdCounter = 1;
    
    auto finalizeQuestion = [&]() {
        if (currentQuestionText.empty()) return;
        if (currentAnswers.size() < 2) {
            result.warnings.push_back("Savol " + std::to_string(result.questions.size() + 1) + 
                                       " uchun kamida 2 ta javob kerak (qator " + 
                                       std::to_string(questionStartLine) + ")");
            return;
        }
        if (!foundCorrect) {
            result.warnings.push_back("Savol " + std::to_string(result.questions.size() + 1) + 
                                       " uchun to'g'ri javob belgilanmagan (qator " + 
                                       std::to_string(questionStartLine) + ")");
            // Mark first as correct to not break it
            currentAnswers[0].isCorrect = true;
        }
        
        Question q;
        q.id = (int)result.questions.size() + 1;
        q.text = currentQuestionText;
        q.answers = currentAnswers;
        q.orderIndex = (int)result.questions.size();
        result.questions.push_back(q);
        
        currentQuestionText.clear();
        currentAnswers.clear();
        foundCorrect = false;
    };
    
    while (std::getline(file, line)) {
        lineNum++;
        std::string trimmed = trim(line);
        result.lineCount = lineNum;
        
        if (trimmed.empty()) continue;
        
        if (trimmed == "++++") {
            if (state == IDLE) {
                // Start of a new question block
                state = IN_QUESTION;
                questionStartLine = lineNum;
                currentQuestionText.clear();
                currentAnswers.clear();
                foundCorrect = false;
            } else if (state == IN_ANSWERS) {
                // End of question block
                finalizeQuestion();
                state = IDLE;
            }
            continue;
        }
        
        if (trimmed == "====" || trimmed == "---") {
            if (state == IN_QUESTION) {
                state = IN_ANSWERS;
            }
            // separator between answers - just continue
            continue;
        }
        
        if (state == IN_QUESTION) {
            if (!currentQuestionText.empty()) currentQuestionText += "\n";
            currentQuestionText += trimmed;
        } else if (state == IN_ANSWERS) {
            if (trimmed.empty()) continue;
            
            Answer a;
            a.id = answerIdCounter++;
            
            if (!trimmed.empty() && trimmed[0] == '#') {
                // Correct answer
                a.text = trim(trimmed.substr(1));
                a.isCorrect = true;
                foundCorrect = true;
            } else {
                a.text = trimmed;
                a.isCorrect = false;
            }
            
            if (!a.text.empty()) {
                currentAnswers.push_back(a);
            }
        }
    }
    
    // Handle case where file doesn't end with ++++
    if (state == IN_ANSWERS && !currentQuestionText.empty()) {
        finalizeQuestion();
    }
    
    if (result.questions.empty()) {
        result.errors.push_back("Faylda hech qanday savol topilmadi. Format to'g'ri ekanligini tekshiring.");
        return result;
    }
    
    result.success = true;
    return result;
}

std::string TXTParser::getFormatExample() {
    return R"(++++
Savol matni bu yerga yoziladi
====
Birinchi javob
====
#To'g'ri javob (#belgisi bilan boshlanadi)
====
Uchinchi javob
====
To'rtinchi javob
++++
++++
Ikkinchi savol matni
====
Javob A
====
#To'g'ri javob B
====
Javob C
++++)";
}