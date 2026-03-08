# WebServer Makefile

# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -std=c++11 -Wall -O2 -pthread

# MySQL配置
MYSQL_CFLAGS = $(shell mysql_config --cflags)
MYSQL_LIBS = $(shell mysql_config --libs)

# 包含路径
INCLUDES = -I./include

# 源文件
SRCS = src/main.cpp src/http_conn.cpp src/mysql_conn.cpp

# 目标文件
OBJS = $(SRCS:.cpp=.o)

# 可执行文件
TARGET = a.out

# 默认目标
all: $(TARGET)

# 链接
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(MYSQL_LIBS)

# 编译
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(MYSQL_CFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJS)

# 重新编译
rebuild: clean all

# 运行
run: $(TARGET)
	./$(TARGET) 10000

# 调试编译
debug: CXXFLAGS = -std=c++11 -Wall -g -pthread
debug: $(TARGET)

.PHONY: all clean rebuild run debug
