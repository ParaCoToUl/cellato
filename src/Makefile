CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O2 -I../include

# Directories
SRC_DIR = .
OBJ_DIR = ../bin
BIN_DIR = ../bin

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Main target
TARGET = $(BIN_DIR)/gol

# Rules
all: directories $(TARGET)

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

run: all
	$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all run clean directories
