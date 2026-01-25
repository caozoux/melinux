#ifndef PARSER_H
#define PARSER_H

#include "function_info.h"
#include "lexer.h"
#include <string>
#include <vector>
#include <fstream>

class Parser {
public:
    Parser(const std::string& filename);
    ~Parser() = default;

    // 解析文件，提取所有函数声明
    bool parse();

    // 获取解析结果
    const FunctionMap& getFunctions() const { return functions_; }

    // 获取原始token流（供analyzer使用）
    const std::vector<Token>& getTokens() const { return tokens_; }

    // 获取源代码行（供analyzer使用）
    const std::vector<std::string>& getSourceLines() const { return sourceLines_; }

    std::string getError() const { return errorMessage_; }

private:
    std::string filename_;
    std::vector<Token> tokens_;
    size_t current_;
    FunctionMap functions_;
    std::vector<std::string> sourceLines_;
    std::string errorMessage_;

    // Token操作
    Token peek() const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);

    // 解析函数声明
    bool parseFunctionDeclaration();

    // 辅助函数
    bool isFunctionQualifier(const Token& token) const;
    std::string parseReturnType();
    std::string parseParameterList();
    int findMatchingBrace(int startPos);

    // 读取源文件
    bool readSourceFile();
};

#endif // PARSER_H
