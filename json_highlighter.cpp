#include "json_highlighter.h"

JsonHighlighter::JsonHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // key（"xxx":）
    keyFormat.setForeground(Qt::blue);

    // string value
    stringFormat.setForeground(Qt::darkGreen);

    // number
    numberFormat.setForeground(Qt::darkMagenta);

    // bool / null
    keywordFormat.setForeground(Qt::darkRed);
    // keywordPattern = QRegularExpression("\\b(true|false|null)\\b");
    keywordPattern = QRegularExpression(R"(\b(true|false|null)\b)");
}

void JsonHighlighter::highlightBlock(const QString &text)
{
    // -------- key --------
    // QRegularExpression keyRegex("\".*?\"(?=\\s*:)"); // "key":
    QRegularExpression keyRegex(R"("[^"\\]*"(?=\s*:))"); // "key":
    auto keyMatches = keyRegex.globalMatch(text);
    while (keyMatches.hasNext()) {
        auto m = keyMatches.next();
        setFormat(m.capturedStart(), m.capturedLength(), keyFormat);
    }

    // -------- string value --------
    // QRegularExpression stringRegex(":\\s*\".*?\"");
    QRegularExpression stringRegex(R"(:\s*"[^"\\]*")");
    auto strMatches = stringRegex.globalMatch(text);
    while (strMatches.hasNext()) {
        auto m = strMatches.next();
        setFormat(m.capturedStart(), m.capturedLength(), stringFormat);
    }

    // -------- number --------
    // QRegularExpression numberRegex("\\b-?\\d+(\\.\\d+)?\\b");
    QRegularExpression numberRegex(R"(\b-?\d+(\.\d+)?\b)");
    auto numMatches = numberRegex.globalMatch(text);
    while (numMatches.hasNext()) {
        auto m = numMatches.next();
        setFormat(m.capturedStart(), m.capturedLength(), numberFormat);
    }

    // -------- true / false / null --------
    auto kwMatches = keywordPattern.globalMatch(text);
    while (kwMatches.hasNext()) {
        auto m = kwMatches.next();
        setFormat(m.capturedStart(), m.capturedLength(), keywordFormat);
    }
}