CXX = g++
NVCC = nvcc
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O3 -I../include
NVCCFLAGS = -std=c++17 -O3 -I../include

# Directories
SRC_DIR = .
OBJ_DIR = ../bin
BIN_DIR = ../bin
CUDA_DIR = ../include/evaluators
TEST_DIR = $(SRC_DIR)

# Source and object files
MAIN_SRC = $(SRC_DIR)/main.cpp
MAIN_OBJ = $(OBJ_DIR)/main.o
TEST_SRC = $(SRC_DIR)/test_runner.cpp
TEST_OBJ = $(OBJ_DIR)/test_runner.o

# CUDA source and object files
CUDA_SRCS = $(wildcard $(CUDA_DIR)/*.cu)
CUDA_OBJS = $(patsubst $(CUDA_DIR)/%.cu,$(OBJ_DIR)/%.o,$(CUDA_SRCS))

# Main targets
TARGET = $(BIN_DIR)/cellib_experiments
TEST_TARGET = $(BIN_DIR)/cellib_tests

# Rules
all: directories $(TARGET) $(TEST_TARGET)

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Add directories dependency to object file creation rules
$(MAIN_OBJ): $(MAIN_SRC) | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_OBJ): $(TEST_SRC) | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(CUDA_DIR)/%.cu | directories
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

# Create separate executables for main app and tests
$(TARGET): $(MAIN_OBJ) $(CUDA_OBJS)
	$(NVCC) $(NVCCFLAGS) $^ -o $@

$(TEST_TARGET): $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

run: $(TARGET)
	$(TARGET)

test: $(TEST_TARGET)
	$(TEST_TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all run test clean directories
