#ifndef FUNCTION_INFO_H
#define FUNCTION_INFO_H

#include <string>
#include <vector>
#include <unordered_map>

// Token类型枚举
enum class TokenType {
    UNKNOWN,
    KEYWORD,        // static, inline, const, volatile等
    IDENTIFIER,     // 标识符
    TYPE_NAME,      // 类型名 (int, char, void, struct name等)
    OPERATOR,       // 运算符
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]
    SEMICOLON,      // ;
    COMMA,          // ,
    ASTERISK,       // *
    AMPERSAND,      // &
    ARROW,          // ->
    DOT,            // .
    STRING_LITERAL, // 字符串字面量
    NUMBER,         // 数字
    PREPROCESSOR,   // 预处理指令 #开头的
    COMMENT,        // 注释
    WHITESPACE,     // 空白字符
    NEWLINE,        // 换行
    EOF_TOKEN
};

// Token结构
struct Token {
    TokenType type;
    std::string text;
    int line;
    int column;

    Token() : type(TokenType::UNKNOWN), line(0), column(0) {}
    Token(TokenType t, const std::string& txt, int l, int c)
        : type(t), text(txt), line(l), column(c) {}
};

// 函数信息结构
struct FunctionInfo {
    std::string name;           // 函数名
    std::string returnType;     // 返回类型
    std::string signature;      // 完整签名
    int startLine;              // 起始行号
    int endLine;                // 结束行号
    std::vector<std::string> calls;  // 调用的函数列表（按顺序）

    FunctionInfo() : startLine(0), endLine(0) {}

    FunctionInfo(const std::string& n, const std::string& ret,
                 const std::string& sig, int start)
        : name(n), returnType(ret), signature(sig),
          startLine(start), endLine(0) {}
};

// 使用unordered_map作为hash数组
using FunctionMap = std::unordered_map<std::string, FunctionInfo>;

#endif // FUNCTION_INFO_H
