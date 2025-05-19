CXX = g++
NVCC = nvcc
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O3 -I../include
NVCCFLAGS = -std=c++17 -O3 -I../include

# Directories
SRC_DIR = .
OBJ_DIR = ../bin
BIN_DIR = ../bin
CUDA_DIR = ../include/evaluators

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# CUDA source and object files
CUDA_SRCS = $(wildcard $(CUDA_DIR)/*.cu)
CUDA_OBJS = $(patsubst $(CUDA_DIR)/%.cu,$(OBJ_DIR)/%.o,$(CUDA_SRCS))

# Main target
TARGET = $(BIN_DIR)/cellib_experiments

# Rules
all: directories $(TARGET)

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(CUDA_DIR)/%.cu
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

$(TARGET): $(OBJS) $(CUDA_OBJS)
	$(NVCC) $(NVCCFLAGS) $^ -o $@

run: all
	$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all run clean directories
