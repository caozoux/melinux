# Makefile for C Function Parser

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
TARGET = cpp_parser
SRCDIR = .
OBJDIR = obj

# 源文件
SOURCES = main.cpp lexer.cpp parser.cpp analyzer.cpp

# 目标文件
OBJECTS = $(addprefix $(OBJDIR)/, $(SOURCES:.cpp=.o))

# 头文件依赖
HEADERS = function_info.h lexer.h parser.h analyzer.h

# 默认目标
all: $(TARGET)

# 创建目标文件目录
$(OBJDIR):
	mkdir -p $(OBJDIR)

# 链接目标文件
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "Build complete: $(TARGET)"

# 编译源文件
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -rf $(OBJDIR) $(TARGET)
	@echo "Clean complete"

# 运行测试
test: $(TARGET)
	./$(TARGET)

# 安装（可选）
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# 卸载
uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean test install uninstall
