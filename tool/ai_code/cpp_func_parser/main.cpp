#include "function_info.h"
#include "parser.h"
#include "analyzer.h"
#include <iostream>
#include <sstream>
#include <iomanip>

class CFunctionParser {
public:
    CFunctionParser() : analyzed_(false) {}

    // 加载并解析C文件
    bool loadFile(const std::string& filename) {
        filename_ = filename;
        analyzed_ = false;

        Parser parser(filename);
        if (!parser.parse()) {
            std::cerr << "Error parsing file: " << parser.getError() << std::endl;
            return false;
        }

        functions_ = parser.getFunctions();
        sourceLines_ = parser.getSourceLines();
        tokens_ = parser.getTokens();

        std::cout << "Loaded " << functions_.size()
                  << " functions from " << filename << std::endl;
        return true;
    }

    // 分析函数调用关系
    void analyze() {
        if (functions_.empty()) {
            std::cout << "No functions loaded. Use 'load <file>' first." << std::endl;
            return;
        }

        Analyzer analyzer(functions_);
        analyzer.analyzeFromSource(sourceLines_, tokens_);
        analyzed_ = true;

        std::cout << "Function call analysis completed." << std::endl;
    }

    // 打印所有函数声明
    void dump() {
        if (functions_.empty()) {
            std::cout << "No functions loaded. Use 'load <file>' first." << std::endl;
            return;
        }

        std::cout << "\n=== Function Declarations (" << functions_.size() << ") ===\n" << std::endl;
        std::cout << std::left << std::setw(40) << "Function Name"
                  << std::setw(15) << "Return Type"
                  << "Location" << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        for (const auto& pair : functions_) {
            const FunctionInfo& info = pair.second;
            std::cout << std::left << std::setw(40) << info.name
                      << std::setw(15) << info.returnType
                      << "Line " << info.startLine;

            if (info.endLine > 0) {
                std::cout << " - " << info.endLine;
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    // 显示特定函数的详细信息
    void show(const std::string& funcName) {
        auto it = functions_.find(funcName);
        if (it == functions_.end()) {
            std::cout << "Function '" << funcName << "' not found." << std::endl;
            std::cout << "Use 'dump' to list all functions." << std::endl;
            return;
        }

        const FunctionInfo& info = it->second;

        std::cout << "\n=== Function: " << info.name << " ===" << std::endl;
        std::cout << "Signature: " << info.signature << std::endl;
        std::cout << "Location:  Line " << info.startLine;
        if (info.endLine > 0) {
            std::cout << " - " << info.endLine;
        }
        std::cout << std::endl;

        if (!analyzed_) {
            std::cout << "\nCall analysis not run yet. Use 'analyze' command first." << std::endl;
            return;
        }

        if (info.calls.empty()) {
            std::cout << "\nNo function calls detected." << std::endl;
        } else {
            std::cout << "\nCalls (" << info.calls.size() << "):" << std::endl;
            for (size_t i = 0; i < info.calls.size(); i++) {
                std::cout << "  " << (i + 1) << ". " << info.calls[i] << std::endl;
            }
        }
        std::cout << std::endl;
    }

    // 显示调用关系图
    void callGraph() {
        if (!analyzed_) {
            std::cout << "Call analysis not run yet. Use 'analyze' command first." << std::endl;
            return;
        }

        std::cout << "\n=== Call Graph ===\n" << std::endl;

        for (const auto& pair : functions_) {
            const FunctionInfo& info = pair.second;

            if (info.calls.empty()) {
                continue;
            }

            std::cout << info.name << " ->";
            for (const auto& call : info.calls) {
                std::cout << " " << call;
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    // 显示统计信息
    void stats() {
        std::cout << "\n=== Statistics ===" << std::endl;
        std::cout << "Total functions: " << functions_.size() << std::endl;

        if (analyzed_) {
            int totalCalls = 0;
            int maxCalls = 0;
            std::string mostActive;

            for (const auto& pair : functions_) {
                const FunctionInfo& info = pair.second;
                totalCalls += static_cast<int>(info.calls.size());
                if (static_cast<int>(info.calls.size()) > maxCalls) {
                    maxCalls = static_cast<int>(info.calls.size());
                    mostActive = info.name;
                }
            }

            std::cout << "Total function calls: " << totalCalls << std::endl;
            std::cout << "Average calls per function: "
                      << (functions_.empty() ? 0 : totalCalls / static_cast<int>(functions_.size()))
                      << std::endl;

            if (!mostActive.empty()) {
                std::cout << "Most active function: " << mostActive
                          << " (" << maxCalls << " calls)" << std::endl;
            }
        }
        std::cout << std::endl;
    }

private:
    std::string filename_;
    FunctionMap functions_;
    std::vector<std::string> sourceLines_;
    std::vector<Token> tokens_;
    bool analyzed_;
};

// 打印帮助信息
void printHelp() {
    std::cout << "\n=== C Function Parser Commands ===\n" << std::endl;
    std::cout << "  load <file.c>       - Load and parse a C source file" << std::endl;
    std::cout << "  dump                - Print all function declarations" << std::endl;
    std::cout << "  analyze             - Analyze function call relationships" << std::endl;
    std::cout << "  show <func_name>    - Show details of a specific function" << std::endl;
    std::cout << "  graph               - Display call graph" << std::endl;
    std::cout << "  stats               - Show statistics" << std::endl;
    std::cout << "  help                - Show this help message" << std::endl;
    std::cout << "  quit                - Exit the program\n" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== C Function Parser ===" << std::endl;
    std::cout << "Type 'help' for available commands\n" << std::endl;

    CFunctionParser parser;

    // 如果命令行提供了文件名，直接加载
    if (argc > 1) {
        parser.loadFile(argv[1]);
    }

    // 交互式命令循环
    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "quit" || command == "exit" || command == "q") {
            break;
        } else if (command == "help" || command == "h" || command == "?") {
            printHelp();
        } else if (command == "load") {
            std::string filename;
            iss >> filename;
            if (filename.empty()) {
                std::cout << "Usage: load <filename>" << std::endl;
            } else {
                parser.loadFile(filename);
            }
        } else if (command == "dump") {
            parser.dump();
        } else if (command == "analyze") {
            parser.analyze();
        } else if (command == "show") {
            std::string funcName;
            iss >> funcName;
            if (funcName.empty()) {
                std::cout << "Usage: show <function_name>" << std::endl;
            } else {
                parser.show(funcName);
            }
        } else if (command == "graph") {
            parser.callGraph();
        } else if (command == "stats") {
            parser.stats();
        } else {
            std::cout << "Unknown command: " << command
                      << ". Type 'help' for available commands." << std::endl;
        }
    }

    return 0;
}
