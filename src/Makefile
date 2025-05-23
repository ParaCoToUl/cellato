CXX = g++
CXX_STD = c++20
NVCC = nvcc
CXXFLAGS = -std=$(CXX_STD) -Wall -Wextra -Wpedantic -O3 -I../include
NVCC_CC_BIN = $(shell which $(CXX))
NVCCFLAGS = -std=$(CXX_STD) -O3 -I../include -Wno-deprecated-gpu-targets --expt-relaxed-constexpr -ccbin $(NVCC_CC_BIN) -Xcompiler -std=$(CXX_STD) -Xcompiler -Wall -Xcompiler -Wextra -Xcompiler -Wpedantic -Xcompiler -O3
CMAKE = cmake

CUDA_HOME = /usr/local/cuda

INCLUDE_DIRS = -I../include -I$(DEP_DIR)/kokkos/include -I$(CUDA_HOME)/include
LIB_DIRS = -L$(DEP_DIR)/kokkos/lib -L$(CUDA_HOME)/lib64
LIBS = -lcudart

# Directories
SRC_DIR = .
OBJ_DIR = ../bin
BIN_DIR = ../bin
DEP_DIR = ../_deps
VENDOR_DIR = ../vendor
CUDA_DIRS = ../include/evaluators ../include/traversers/cuda
TEST_DIR = $(SRC_DIR)

# Source and object files
MAIN_SRC = $(SRC_DIR)/main.cpp
MAIN_OBJ = $(OBJ_DIR)/main.o
TEST_SRC = $(SRC_DIR)/test_runner.cpp
TEST_OBJ = $(OBJ_DIR)/test_runner.o

# CUDA source and object files
CUDA_SRCS = $(foreach dir,$(CUDA_DIRS),$(wildcard $(dir)/*.cu))
CUDA_OBJS = $(patsubst %.cu,$(OBJ_DIR)/%.o,$(notdir $(CUDA_SRCS)))

# Main targets
TARGET = $(BIN_DIR)/cellib_experiments
TEST_TARGET = $(BIN_DIR)/cellib_tests

# Rules
all: directories $(TARGET) $(TEST_TARGET)

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

install_deps: install_kokkos

install_kokkos:
	@echo "Installing Kokkos dependencies..."
	@mkdir -p $(DEP_DIR) $(DEP_DIR)/kokkos
	@$(CMAKE) -B $(DEP_DIR)/kokkos -S $(VENDOR_DIR)/kokkos -D CMAKE_BUILD_TYPE=Release -D CMAKE_CXX_COMPILER=$(CXX) -D CMAKE_CXX_STANDARD=20 -D CMAKE_CUDA_COMPILER=$(NVCC) -D CMAKE_CUDA_HOST_COMPILER=$(NVCC_CC_BIN) -D Kokkos_ENABLE_CUDA=ON -D Kokkos_ENABLE_OPENMP=ON -D Kokkos_ENABLE_SERIAL=ON -D Kokkos_ENABLE_TESTS=OFF -D Kokkos_ARCH_NATIVE=ON -D Kokkos_ENABLE_DEPRECATED_CODE_4=ON -D CMAKE_INSTALL_PREFIX=$(DEP_DIR)
	@$(CMAKE) --build $(DEP_DIR)/kokkos --target install
	@echo "Kokkos dependencies installed."

# Add directories dependency to object file creation rules
$(MAIN_OBJ): $(MAIN_SRC) | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_DIRS)

$(TEST_OBJ): $(TEST_SRC) | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_DIRS)

# Pattern rule for CUDA files with vpath to find them
vpath %.cu $(CUDA_DIRS)
$(OBJ_DIR)/%.o: %.cu | directories
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

# Create separate executables for main app and tests
$(TARGET): $(MAIN_OBJ) $(CUDA_OBJS)
	$(NVCC) $(NVCCFLAGS) $^ -o $@ $(LIB_DIRS) $(LIBS)

$(TEST_TARGET): $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIB_DIRS) $(LIBS)

# Debug rule to print variables
print-%:
	@echo $* = $($*)

run: $(TARGET)
	$(TARGET) $(ARGS)

unit_test: $(TEST_TARGET)
	$(TEST_TARGET) $(ARGS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(DEP_DIR)

.PHONY: all run test clean directories print-% install_deps install_kokkos
