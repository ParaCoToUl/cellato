CXX = g++
CXX_STD = c++20
NVCC = nvcc
PYTHON = python3
CXXFLAGS = -std="$(CXX_STD)" -DNDEBUG -Wall -Wextra -Wpedantic -O3 -march=native -mtune=native
NVCC_CC_BIN = $(shell which $(CXX))
NVCCFLAGS = -std="$(CXX_STD)" -DNDEBUG -Wreorder -Wext-lambda-captures-this -O3 -arch=native -Wno-deprecated-gpu-targets --expt-relaxed-constexpr -ccbin "$(NVCC_CC_BIN)" -Xcompiler -std="$(CXX_STD)",-Wall,-Wextra,-march=native,-mtune=native

CUDA_HOME = /usr/local/cuda

INCLUDE_DIRS = -Iinclude -Isrc -Isrc/automata -I"$(CUDA_HOME)/include"
LIB_DIRS = -L"$(CUDA_HOME)/lib64"
LIBS = -lcudart

DEFINES =
ifeq ($(BUILD_TYPE), BENCHMARK)
	DEFINES += -DBENCHMARK_COMPILE
else ifeq ($(BUILD_TYPE), VERIFICATION)
	DEFINES += -DVERIFICATION_COMPILE
endif

# Directories
OBJ_DIR = bin/obj
BIN_DIR = bin
GENERATED_CUDA_INSTANTIATION_DIR = $(BIN_DIR)/generated/cuda_instantiations
GENERATED_CUDA_INSTANTIATION_MK = $(GENERATED_CUDA_INSTANTIATION_DIR)/cuda_instantiations.mk

ifneq ($(MAKECMDGOALS),clean)
-include $(GENERATED_CUDA_INSTANTIATION_MK)
endif

# Source and object files
MAIN_SRC = src/app/main.cpp
MAIN_OBJ = $(OBJ_DIR)/$(MAIN_SRC:.cpp=.o)
TEST_SRC = tests/test_runner.cpp
TEST_OBJ = $(OBJ_DIR)/$(TEST_SRC:.cpp=.o)

# CUDA source and object files
GENERIC_CUDA_SRCS = $(wildcard src/cellato/traversers/cuda/*.cu)
AUTOMATA_CUDA_SRCS = $(wildcard src/automata/*/*.cu)
CUDA_SRCS = $(GENERIC_CUDA_SRCS) $(AUTOMATA_CUDA_SRCS) $(GENERATED_CUDA_INSTANTIATION_SRCS)
CUDA_OBJS = $(patsubst %.cu,$(OBJ_DIR)/%.o,$(CUDA_SRCS))

# Main targets
TARGET = $(BIN_DIR)/cellato
TEST_TARGET = $(BIN_DIR)/cellato_tests

# Rules
all: directories $(TARGET) $(TEST_TARGET)

benchmark:
	@$(MAKE) -j4 -B all BUILD_TYPE=BENCHMARK

verify:
	@$(MAKE) -j4 -B all BUILD_TYPE=VERIFICATION


directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Add directories dependency to object file creation rules
$(GENERATED_CUDA_INSTANTIATION_MK): tools/generate_cuda_instantiations.py src/automata/registry.hpp src/cuda_instantiation/template.cuh | directories
	$(PYTHON) tools/generate_cuda_instantiations.py --registry src/automata/registry.hpp --output-dir $(GENERATED_CUDA_INSTANTIATION_DIR) --make-fragment $@
	@touch $@

$(GENERATED_CUDA_INSTANTIATION_SRCS): $(GENERATED_CUDA_INSTANTIATION_MK)

$(OBJ_DIR)/%.o: %.cpp | directories
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_DIRS) $(DEFINES)

$(OBJ_DIR)/%.o: %.cu | directories
	@mkdir -p $(dir $@)
	$(NVCC) $(NVCCFLAGS) -c $< -o $@ $(INCLUDE_DIRS) $(DEFINES)

# Create separate executables for main app and tests
$(TARGET): $(MAIN_OBJ) $(CUDA_OBJS) | $(GENERATED_CUDA_INSTANTIATION_MK)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIB_DIRS) $(LIBS)

$(TEST_TARGET): $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIB_DIRS) $(LIBS)

# Debug rule to print variables
print-%:
	@echo $* = $($*)

run: $(TARGET)
	$(TARGET) $(ARGS)

unit_test: $(TEST_TARGET)
	$(TEST_TARGET) $(ARGS)

test: unit_test

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all run test clean directories print-%
