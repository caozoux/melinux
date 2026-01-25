#include "analyzer.h"
#include <algorithm>
#include <sstream>

// C关键字（用于过滤）
static const char* cKeywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default",
    "do", "double", "else", "enum", "extern", "float", "for", "goto",
    "if", "inline", "int", "long", "register", "restrict", "return",
    "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while", "while", "for",
    "if", "else", "return", "sizeof", "typeof", "__attribute__",
    nullptr
};

Analyzer::Analyzer(FunctionMap& functions)
    : functions_(functions) {
}

void Analyzer::initFunctionNames() {
    functionNames_.clear();
    for (const auto& pair : functions_) {
        functionNames_.insert(pair.first);
    }
}

void Analyzer::analyze() {
    initFunctionNames();
    // 这里需要sourceLines和tokens，由analyzeFromSource提供
}

void Analyzer::analyzeFromSource(const std::vector<std::string>& sourceLines,
                                  const std::vector<Token>& tokens) {
    initFunctionNames();

    // 为每个函数分析调用关系
    for (auto& pair : functions_) {
        const std::string& funcName = pair.first;
        FunctionInfo& info = pair.second;

        // 只分析有函数体的函数
        if (info.startLine > 0 && info.endLine > info.startLine) {
            analyzeFunction(funcName, sourceLines, tokens);
        }
    }
}

void Analyzer::analyzeFunction(const std::string& funcName,
                               const std::vector<std::string>& sourceLines,
                               const std::vector<Token>& tokens) {
    auto it = functions_.find(funcName);
    if (it == functions_.end()) {
        return;
    }

    FunctionInfo& info = it->second;

    // 查找函数体范围内的所有函数调用
    std::vector<std::string> calls = findCallsInRange(
        info.startLine, info.endLine, sourceLines, tokens);

    info.calls = calls;
}

std::vector<std::string> Analyzer::findCallsInRange(int startLine, int endLine,
                                                     const std::vector<std::string>& sourceLines,
                                                     const std::vector<Token>& tokens) {
    std::vector<std::string> calls;
    std::unordered_set<std::string> seenCalls; // 避免重复调用

    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& token = tokens[i];

        // 检查token是否在目标行范围内
        // 跳过函数声明所在行（startLine），只分析函数体
        if (token.line <= startLine || token.line > endLine) {
            continue;
        }

        // 只处理标识符
        if (token.type != TokenType::IDENTIFIER) {
            continue;
        }

        const std::string& id = token.text;

        // 跳过已知的关键字
        if (shouldSkipIdentifier(id)) {
            continue;
        }

        // 检查是否是函数调用（后面紧跟左括号）
        if (i + 1 < tokens.size() && tokens[i + 1].type == TokenType::LPAREN) {
            // 检查这个标识符是否在已定义的函数中
            if (functionNames_.find(id) != functionNames_.end()) {
                // 这是一个函数调用
                std::string callKey = id + "@" + std::to_string(token.line);
                if (seenCalls.find(callKey) == seenCalls.end()) {
                    calls.push_back(id);
                    seenCalls.insert(callKey);
                }
            }
        }

        // 检查指针调用: ptr->func(
        if (i + 2 < tokens.size() &&
            tokens[i + 1].type == TokenType::ARROW &&
            tokens[i + 2].type == TokenType::IDENTIFIER) {
            const std::string& method = tokens[i + 2].text;
            // 如果这个方法名也是已定义的函数，记录它
            if (functionNames_.find(method) != functionNames_.end()) {
                // 检查后面是否有左括号
                if (i + 3 < tokens.size() && tokens[i + 3].type == TokenType::LPAREN) {
                    std::string callKey = method + "@" + std::to_string(token.line);
                    if (seenCalls.find(callKey) == seenCalls.end()) {
                        calls.push_back(method);
                        seenCalls.insert(callKey);
                    }
                }
            }
        }

        // 检查成员调用: obj.func(
        if (i + 2 < tokens.size() &&
            tokens[i + 1].type == TokenType::DOT &&
            tokens[i + 2].type == TokenType::IDENTIFIER) {
            const std::string& method = tokens[i + 2].text;
            if (functionNames_.find(method) != functionNames_.end()) {
                if (i + 3 < tokens.size() && tokens[i + 3].type == TokenType::LPAREN) {
                    std::string callKey = method + "@" + std::to_string(token.line);
                    if (seenCalls.find(callKey) == seenCalls.end()) {
                        calls.push_back(method);
                        seenCalls.insert(callKey);
                    }
                }
            }
        }
    }

    return calls;
}

bool Analyzer::isFunctionCall(const std::string& potentialName,
                              const std::vector<Token>& tokens, size_t pos) {
    // 检查后面是否紧跟左括号
    if (pos + 1 >= tokens.size()) {
        return false;
    }

    return tokens[pos + 1].type == TokenType::LPAREN;
}

bool Analyzer::isInStringOrComment(const std::vector<std::string>& sourceLines,
                                    int line, int col) {
    if (line < 1 || line > static_cast<int>(sourceLines.size())) {
        return false;
    }

    const std::string& lineStr = sourceLines[line - 1];
    if (col < 1 || col > static_cast<int>(lineStr.length())) {
        return false;
    }

    // 简单检查：检查位置前的引号数量
    bool inString = false;
    bool inSingleLineComment = false;

    for (size_t i = 0; i < static_cast<size_t>(col - 1); i++) {
        if (i + 1 < lineStr.length() && lineStr[i] == '/' && lineStr[i + 1] == '/') {
            inSingleLineComment = true;
            break;
        }
        if (lineStr[i] == '"' && (i == 0 || lineStr[i - 1] != '\\')) {
            inString = !inString;
        }
    }

    return inString || inSingleLineComment;
}

bool Analyzer::shouldSkipIdentifier(const std::string& id) {
    // 检查是否是C关键字
    for (int i = 0; cKeywords[i] != nullptr; i++) {
        if (id == cKeywords[i]) {
            return true;
        }
    }

    // 检查是否是类型名
    static const char* types[] = {
        "int", "char", "short", "long", "float", "double",
        "void", "signed", "unsigned", "bool", "_Bool",
        "size_t", "ssize_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        nullptr
    };

    for (int i = 0; types[i] != nullptr; i++) {
        if (id == types[i]) {
            return true;
        }
    }

    return false;
}
