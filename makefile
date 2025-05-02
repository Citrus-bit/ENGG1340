# Compiler and options
CXX       := g++
CXXFLAGS  := -std=c++17 -O2 -Wall -Wextra

# Executable name
TARGET    := tower_defense

# Source file list
SRCS      := main.cpp io.cpp

# Object files automatically derived from SRCS
OBJS      := $(SRCS:.cpp=.o)

# Default target: compile and link
all: $(TARGET)

# Linking stage: generate executable from .o files
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilation rule: .cpp → .o
# If your source files include other headers (like io.h), you can add dependencies here:
%.o: %.cpp io.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean intermediate files and executable
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)

# Run the program (optional)
.PHONY: run
run: $(TARGET)
	./$(TARGET)
