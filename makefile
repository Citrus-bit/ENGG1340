# ��������ѡ��
CXX       := g++
CXXFLAGS  := -std=c++17 -O2 -Wall -Wextra

# ��ִ���ļ�����
TARGET    := tower_defense

# Դ�ļ��б�
SRCS      := main.cpp io.cpp

# �Զ��� SRCS �Ƶ��������ļ��б�
OBJS      := $(SRCS:.cpp=.o)

# Ĭ��Ŀ�꣺���벢����
all: $(TARGET)

# ���ӽ׶Σ��� .o �ļ����ɿ�ִ��
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# �������.cpp �� .o
# ������Դ�ļ��ﻹ include ������ͷ���� io.h��������������ͳһ���������
%.o: %.cpp io.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# �����м��ļ��Ϳ�ִ��
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)

# ���г��򣨿�ѡ��
.PHONY: run
run: $(TARGET)
	./$(TARGET)
