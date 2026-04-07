#ifndef JSON_HIGHLIGHTER_H
#define JSON_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>

class JsonHighlighter : public QSyntaxHighlighter
{
public:
    JsonHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat keyFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat keywordFormat;
    QRegularExpression keywordPattern;
};

#endif // JSON_HIGHLIGHTER_H
