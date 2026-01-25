#include "lexer.h"
#include <algorithm>
#include <cstring>

// C关键字集合
static const char* keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default",
    "do", "double", "else", "enum", "extern", "float", "for", "goto",
    "if", "inline", "int", "long", "register", "restrict", "return",
    "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while", "_Nonnull", "_Nullable",
    "__attribute__", "__extension__", "__asm__", "__asm", "__typeof__",
    "typeof", "__inline", "__inline__", "__forceinline", nullptr
};

// 判断是否是关键字
static bool isKeyword(const std::string& str) {
    for (int i = 0; keywords[i] != nullptr; i++) {
        if (str == keywords[i]) {
            return true;
        }
    }
    return false;
}

// 判断是否是类型名
static bool isTypeName(const std::string& str) {
    static const char* types[] = {
        "void", "char", "short", "int", "long", "float", "double",
        "signed", "unsigned", "bool", "_Bool", "size_t", "ssize_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "u8", "u16", "u32", "u64", "s8", "s16", "s32", "s64",
        nullptr
    };
    for (int i = 0; types[i] != nullptr; i++) {
        if (str == types[i]) {
            return true;
        }
    }
    return false;
}

Lexer::Lexer(const std::string& source)
    : source_(source), pos_(0), line_(1), column_(1) {
}

void Lexer::reset() {
    pos_ = 0;
    line_ = 1;
    column_ = 1;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[pos_];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = source_[pos_++];
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.length();
}

bool Lexer::isIdentifierStart(char c) const {
    return std::isalpha(c) || c == '_';
}

bool Lexer::isIdentifierChar(char c) const {
    return std::isalnum(c) || c == '_';
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && std::isspace(peek()) && peek() != '\n') {
        advance();
    }
}

void Lexer::skipLineComment() {
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

void Lexer::skipBlockComment() {
    while (!isAtEnd()) {
        if (peek() == '*' && pos_ + 1 < source_.length() && source_[pos_ + 1] == '/') {
            advance(); // *
            advance(); // /
            return;
        }
        advance();
    }
}

Token Lexer::readString() {
    int startLine = line_;
    int startCol = column_;
    char quote = advance(); // " or '
    std::string str;
    str += quote;

    while (!isAtEnd()) {
        char c = peek();
        if (c == '\\') {
            // 转义字符
            str += advance();
            if (!isAtEnd()) {
                str += advance();
            }
        } else if (c == quote) {
            str += advance();
            return Token(TokenType::STRING_LITERAL, str, startLine, startCol);
        } else {
            str += advance();
        }
    }
    return Token(TokenType::STRING_LITERAL, str, startLine, startCol);
}

Token Lexer::readNumber() {
    int startLine = line_;
    int startCol = column_;
    std::string num;

    while (!isAtEnd() && (std::isdigit(peek()) || peek() == '.' ||
                          peek() == 'x' || peek() == 'X' ||
                          (peek() >= 'a' && peek() <= 'f') ||
                          (peek() >= 'A' && peek() <= 'F') ||
                          peek() == 'L' || peek() == 'U' || peek() == 'l' || peek() == 'u')) {
        num += advance();
    }

    return Token(TokenType::NUMBER, num, startLine, startCol);
}

Token Lexer::readIdentifierOrKeyword() {
    int startLine = line_;
    int startCol = column_;
    std::string id;

    while (!isAtEnd() && isIdentifierChar(peek())) {
        id += advance();
    }

    if (isKeyword(id)) {
        return Token(TokenType::KEYWORD, id, startLine, startCol);
    } else if (isTypeName(id)) {
        return Token(TokenType::TYPE_NAME, id, startLine, startCol);
    } else {
        return Token(TokenType::IDENTIFIER, id, startLine, startCol);
    }
}

Token Lexer::readPreprocessor() {
    int startLine = line_;
    int startCol = column_;
    std::string dir;
    dir += advance(); // #

    while (!isAtEnd() && peek() != '\n') {
        dir += advance();
    }

    return Token(TokenType::PREPROCESSOR, dir, startLine, startCol);
}

Token Lexer::readOperator() {
    int startLine = line_;
    int startCol = column_;
    char c = advance();

    switch (c) {
        case '(':
            return Token(TokenType::LPAREN, "(", startLine, startCol);
        case ')':
            return Token(TokenType::RPAREN, ")", startLine, startCol);
        case '{':
            return Token(TokenType::LBRACE, "{", startLine, startCol);
        case '}':
            return Token(TokenType::RBRACE, "}", startLine, startCol);
        case '[':
            return Token(TokenType::LBRACKET, "[", startLine, startCol);
        case ']':
            return Token(TokenType::RBRACKET, "]", startLine, startCol);
        case ';':
            return Token(TokenType::SEMICOLON, ";", startLine, startCol);
        case ',':
            return Token(TokenType::COMMA, ",", startLine, startCol);
        case '*':
            return Token(TokenType::ASTERISK, "*", startLine, startCol);
        case ':':
            return Token(TokenType::OPERATOR, ":", startLine, startCol);
        case '=':
            if (peek() == '=') {
                advance();
                return Token(TokenType::OPERATOR, "==", startLine, startCol);
            }
            return Token(TokenType::OPERATOR, "=", startLine, startCol);
        case '!':
            if (peek() == '=') {
                advance();
                return Token(TokenType::OPERATOR, "!=", startLine, startCol);
            }
            return Token(TokenType::OPERATOR, "!", startLine, startCol);
        case '<':
            if (peek() == '=') {
                advance();
                return Token(TokenType::OPERATOR, "<=", startLine, startCol);
            } else if (peek() == '<') {
                advance();
                return Token(TokenType::OPERATOR, "<<", startLine, startCol);
            }
            return Token(TokenType::OPERATOR, "<", startLine, startCol);
        case '>':
            if (peek() == '=') {
                advance();
                return Token(TokenType::OPERATOR, ">=", startLine, startCol);
            } else if (peek() == '>') {
                advance();
                return Token(TokenType::OPERATOR, ">>", startLine, startCol);
            }
            return Token(TokenType::OPERATOR, ">", startLine, startCol);
        case '+':
            if (peek() == '+') {
                advance();
                return Token(TokenType::OPERATOR, "++", startLine, startCol);
            } else if (peek() == '=') {
                advance();
                return Token(TokenType::OPERATOR, "+=", startLine, startCol);
            }
            return Token(TokenType::OPERATOR, "+", startLine, startCol);
        case '-':
            if (peek() == '-') {
                advance();
                return Token(TokenType::OPERATOR, "--", startLine, startCol);
            } else if (peek() == '=') {
                advance();
                return Token(TokenType::OPERATOR, "-=", startLine, startCol);
            } else if (peek() == '>') {
                advance();
                return Token(TokenType::ARROW, "->", startLine, startCol);
            }
            return Token(TokenType::OPERATOR, "-", startLine, startCol);
        case '|':
            if (peek() == '|') {
                advance();
                return Token(TokenType::OPERATOR, "||", startLine, startCol);
            } else if (peek() == '=') {
                advance();
                return Token(TokenType::OPERATOR, "|=", startLine, startCol);
            }
            return Token(TokenType::OPERATOR, "|", startLine, startCol);
        case '&':
            if (peek() == '&') {
                advance();
                return Token(TokenType::OPERATOR, "&&", startLine, startCol);
            } else if (peek() == '=') {
                advance();
                return Token(TokenType::OPERATOR, "&=", startLine, startCol);
            }
            return Token(TokenType::AMPERSAND, "&", startLine, startCol);
        case '^':
            if (peek() == '=') {
                advance();
                return Token(TokenType::OPERATOR, "^=", startLine, startCol);
            }
            return Token(TokenType::OPERATOR, "^", startLine, startCol);
        case '%':
            if (peek() == '=') {
                advance();
                return Token(TokenType::OPERATOR, "%=", startLine, startCol);
            }
            return Token(TokenType::OPERATOR, "%", startLine, startCol);
        case '/':
            if (peek() == '=') {
                advance();
                return Token(TokenType::OPERATOR, "/=", startLine, startCol);
            }
            return Token(TokenType::OPERATOR, "/", startLine, startCol);
        case '.':
            return Token(TokenType::DOT, ".", startLine, startCol);
        default:
            return Token(TokenType::UNKNOWN, std::string(1, c), startLine, startCol);
    }
}

Token Lexer::getNextToken() {
    while (!isAtEnd()) {
        char c = peek();

        // 跳过空白字符
        if (std::isspace(c)) {
            if (c == '\n') {
                int line = line_;
                advance();
                return Token(TokenType::NEWLINE, "\n", line, column_);
            }
            skipWhitespace();
            continue;
        }

        // 处理注释
        if (c == '/' && pos_ + 1 < source_.length()) {
            char next = source_[pos_ + 1];
            if (next == '/') {
                skipLineComment();
                continue;
            } else if (next == '*') {
                skipBlockComment();
                continue;
            }
        }

        // 预处理指令
        if (c == '#') {
            return readPreprocessor();
        }

        // 字符串字面量
        if (c == '"' || c == '\'') {
            return readString();
        }

        // 数字
        if (std::isdigit(c)) {
            return readNumber();
        }

        // 标识符或关键字
        if (isIdentifierStart(c)) {
            return readIdentifierOrKeyword();
        }

        // 运算符和分隔符
        return readOperator();
    }

    return Token(TokenType::EOF_TOKEN, "", line_, column_);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    reset();

    while (true) {
        Token token = getNextToken();
        if (token.type == TokenType::NEWLINE) {
            // 保留换行用于错误报告，但不添加到token流
            continue;
        }
        if (token.type == TokenType::EOF_TOKEN) {
            tokens.push_back(token);
            break;
        }
        tokens.push_back(token);
    }

    return tokens;
}
