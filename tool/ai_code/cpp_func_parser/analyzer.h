#ifndef ANALYZER_H
#define ANALYZER_H

#include "function_info.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class Analyzer {
public:
    Analyzer(FunctionMap& functions);

    // 分析所有函数的调用关系
    void analyze();

    // 从源代码分析函数调用
    void analyzeFromSource(const std::vector<std::string>& sourceLines,
                           const std::vector<Token>& tokens);

private:
    FunctionMap& functions_;
    std::unordered_set<std::string> functionNames_;

    // 初始化函数名集合
    void initFunctionNames();

    // 分析单个函数的调用
    void analyzeFunction(const std::string& funcName,
                         const std::vector<std::string>& sourceLines,
                         const std::vector<Token>& tokens);

    // 在指定行范围内查找函数调用
    std::vector<std::string> findCallsInRange(int startLine, int endLine,
                                               const std::vector<std::string>& sourceLines,
                                               const std::vector<Token>& tokens);

    // 判断是否是函数调用
    bool isFunctionCall(const std::string& potentialName,
                       const std::vector<Token>& tokens, size_t pos);

    // 检查token是否在字符串或注释中
    bool isInStringOrComment(const std::vector<std::string>& sourceLines, int line, int col);

    // 过滤非函数调用的标识符
    bool shouldSkipIdentifier(const std::string& id);
};

#endif // ANALYZER_H
