#include "parser.h"
#include <sstream>
#include <iostream>

Parser::Parser(const std::string& filename)
    : filename_(filename), current_(0) {
}

bool Parser::readSourceFile() {
    std::ifstream file(filename_);
    if (!file.is_open()) {
        errorMessage_ = "Cannot open file: " + filename_;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        sourceLines_.push_back(line);
    }

    file.close();

    if (sourceLines_.empty()) {
        errorMessage_ = "File is empty: " + filename_;
        return false;
    }

    return true;
}

bool Parser::parse() {
    if (!readSourceFile()) {
        return false;
    }

    // 读取整个文件内容
    std::stringstream ss;
    for (const auto& line : sourceLines_) {
        ss << line << "\n";
    }

    // 词法分析
    Lexer lexer(ss.str());
    tokens_ = lexer.tokenize();
    current_ = 0;

    // 解析函数声明
    while (!isAtEnd()) {
        // 跳过预处理指令
        if (peek().type == TokenType::PREPROCESSOR) {
            advance();
            continue;
        }

        // 尝试解析函数声明
        if (parseFunctionDeclaration()) {
            // 成功解析一个函数，继续
            continue;
        }

        // 如果不是函数声明，跳过当前token
        advance();
    }

    return true;
}

Token Parser::peek() const {
    if (current_ < tokens_.size()) {
        return tokens_[current_];
    }
    return Token(TokenType::EOF_TOKEN, "", 0, 0);
}

Token Parser::advance() {
    if (current_ < tokens_.size()) {
        return tokens_[current_++];
    }
    return Token(TokenType::EOF_TOKEN, "", 0, 0);
}

bool Parser::isAtEnd() const {
    return current_ >= tokens_.size() ||
           tokens_[current_].type == TokenType::EOF_TOKEN;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return tokens_[current_].type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::isFunctionQualifier(const Token& token) const {
    static const char* qualifiers[] = {
        "static", "inline", "__inline", "__inline__",
        "extern", "register", "__attribute__",
        "__stdcall", "__cdecl", "__fastcall",
        nullptr
    };

    if (token.type != TokenType::KEYWORD) {
        return false;
    }

    for (int i = 0; qualifiers[i] != nullptr; i++) {
        if (token.text == qualifiers[i]) {
            return true;
        }
    }

    return false;
}

std::string Parser::parseReturnType() {
    std::string returnType;

    // 收集所有修饰符和类型
    while (!isAtEnd()) {
        Token tok = peek();

        // 遇到左括号，说明返回类型结束（函数指针）
        if (tok.type == TokenType::LPAREN) {
            break;
        }

        // 遇到标识符，可能是函数名
        if (tok.type == TokenType::IDENTIFIER) {
            // 检查下一个token是否是左括号
            if (current_ + 1 < tokens_.size() &&
                tokens_[current_ + 1].type == TokenType::LPAREN) {
                break; // 这是函数名，不是返回类型
            }
        }

        // 遇到分号或大括号，不是函数声明
        if (tok.type == TokenType::SEMICOLON || tok.type == TokenType::LBRACE) {
            break;
        }

        if (!returnType.empty()) {
            returnType += " ";
        }
        returnType += tok.text;
        advance();
    }

    return returnType;
}

std::string Parser::parseParameterList() {
    if (!match(TokenType::LPAREN)) {
        return "";
    }

    std::string params = "(";
    int depth = 1;
    bool first = true;

    while (!isAtEnd() && depth > 0) {
        Token tok = peek();

        if (tok.type == TokenType::LPAREN) {
            depth++;
        } else if (tok.type == TokenType::RPAREN) {
            depth--;
            if (depth == 0) {
                break;
            }
        }

        if (tok.type == TokenType::LBRACE) {
            // 参数列表不应该有花括号，可能是语法错误
            break;
        }

        if (!first) {
            params += " ";
        }
        params += tok.text;
        first = false;
        advance();
    }

    if (match(TokenType::RPAREN)) {
        params += ")";
    } else {
        params += ")";
    }

    return params;
}

int Parser::findMatchingBrace(int startPos) {
    int depth = 1;

    for (size_t i = startPos + 1; i < tokens_.size(); i++) {
        if (tokens_[i].type == TokenType::LBRACE) {
            depth++;
        } else if (tokens_[i].type == TokenType::RBRACE) {
            depth--;
            if (depth == 0) {
                return static_cast<int>(i);
            }
        }
    }

    return -1;
}

bool Parser::parseFunctionDeclaration() {
    size_t savePos = current_;

    // 跳过函数限定符
    while (!isAtEnd() && isFunctionQualifier(peek())) {
        advance();
    }

    // 解析返回类型
    std::string returnType = parseReturnType();

    if (returnType.empty()) {
        current_ = savePos;
        return false;
    }

    // 检查是否有标识符作为函数名
    if (peek().type != TokenType::IDENTIFIER) {
        current_ = savePos;
        return false;
    }

    std::string funcName = advance().text;

    // 检查是否是函数定义 (...)
    if (!match(TokenType::LPAREN)) {
        current_ = savePos;
        return false;
    }

    // 回退到左括号前，重新解析参数列表
    current_--;

    // 解析参数列表
    std::string params = parseParameterList();

    // 检查是否有函数体 {
    if (!match(TokenType::LBRACE)) {
        // 可能只是函数声明，没有定义
        current_ = savePos;
        return false;
    }

    // 找到匹配的右大括号
    int endTokenPos = findMatchingBrace(static_cast<int>(current_) - 1);
    if (endTokenPos == -1) {
        // 没有找到匹配的右大括号
        current_ = savePos;
        return false;
    }

    // 创建函数信息
    FunctionInfo info;
    info.name = funcName;
    info.returnType = returnType;
    info.signature = returnType + " " + funcName + params;
    info.startLine = tokens_[savePos].line;
    info.endLine = tokens_[endTokenPos].line;

    // 保存函数
    functions_[funcName] = info;

    // 移动到函数体之后
    current_ = endTokenPos + 1;

    return true;
}
