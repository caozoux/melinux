#ifndef LEXER_H
#define LEXER_H

#include "function_info.h"
#include <string>
#include <vector>
#include <cctype>

class Lexer {
public:
    Lexer(const std::string& source);
    ~Lexer() = default;

    // 获取所有token
    std::vector<Token> tokenize();

    // 获取下一个token
    Token getNextToken();

    // 重置lexer到开始位置
    void reset();

private:
    std::string source_;
    size_t pos_;
    int line_;
    int column_;

    // 辅助函数
    char peek() const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    Token readPreprocessor();
    Token readString();
    Token readNumber();
    Token readIdentifierOrKeyword();
    Token readOperator();

    // 判断字符类型
    bool isIdentifierStart(char c) const;
    bool isIdentifierChar(char c) const;
};

#endif // LEXER_H
