#ifndef FILE_PARSER_H
#define FILE_PARSER_H

#include "models.h"
#include <QString>
#include <vector>

class TXTParser {
public:
    struct ParseResult {
        bool success;
        QString quizName;
        std::vector<Question> questions;
        std::vector<QString> errors;
        int lineNumber;

        ParseResult() : success(false), lineNumber(0) {}
    };

    static ParseResult parse(const QString& filename);
    static std::string getFormatExample();

private:
    static QString trim(const QString& s);
    static QString extractNameFromPath(const QString& path);
};

#endif // FILE_PARSER_H
