# 编译器与选项
CXX       := g++
CXXFLAGS  := -std=c++17 -O2 -Wall -Wextra

# 可执行文件名称
TARGET    := tower_defense

# 源文件列表
SRCS      := main.cpp io.cpp

# 自动由 SRCS 推导出对象文件列表
OBJS      := $(SRCS:.cpp=.o)

# 默认目标：编译并链接
all: $(TARGET)

# 链接阶段：由 .o 文件生成可执行
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 编译规则：.cpp → .o
# 如果你的源文件里还 include 了其他头（如 io.h），可以在这里统一添加依赖：
%.o: %.cpp io.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理中间文件和可执行
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)

# 运行程序（可选）
.PHONY: run
run: $(TARGET)
	./$(TARGET)
