CXX = g++
CXX_STD = c++20
NVCC = nvcc
CXXFLAGS = -std=$(CXX_STD) -Wall -Wextra -Wpedantic -O3 -I../include
NVCC_CC_BIN = $(shell which $(CXX))
NVCCFLAGS = -std=$(CXX_STD) -O3 -I../include -Wno-deprecated-gpu-targets --expt-relaxed-constexpr -ccbin $(NVCC_CC_BIN) -Xcompiler -std=$(CXX_STD) -Xcompiler -Wall -Xcompiler -Wextra -Xcompiler -O3
CMAKE = cmake

CUDA_VERSION = 12.6

CUDA_HOME = /usr/local/cuda

INCLUDE_DIRS = -I../include -I$(CUDA_HOME)/include
LIB_DIRS = -L$(CUDA_HOME)/lib64
LIBS = -lcudart

KOKKOS_HOME_INCLUDE = $(DEP_DIR)/include
KOKKOS_HOME_LIB = $(DEP_DIR)/lib ${DEP_DIR}/lib64

HALIDE_HOME_INCLUDE = "/usr/include/halide19"
HALIDE_HOME_LIB = "/usr/lib/x86_64-linux-gnu/halide19"

CUDA_DRIVER_LIB = $(shell pkg-config --libs cuda-$(CUDA_VERSION))

ENABLE_KOKKOS = OFF
ENABLE_HALIDE = OFF
ENABLE_AN5D = OFF

# Directories
SRC_DIR = .
OBJ_DIR = ../bin
BIN_DIR = ../bin
DEP_DIR = ../_deps
VENDOR_DIR = ../vendor
CUDA_DIRS = ../include/evaluators ../include/traversers/cuda ./game_of_life ./greenberg ./wire ./fire
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

ifeq ($(ENABLE_KOKKOS), ON)
DEFINES += -DENABLE_KOKKOS
INCLUDE_DIRS += $(KOKKOS_HOME_INCLUDE:%=-I%)
LIB_DIRS += $(KOKKOS_HOME_LIB:%=-L%)
LIBS += -lkokkoscore $(CUDA_DRIVER_LIB)
EXTRA_OBJS += $(OBJ_DIR)/kokkos_game_of_life_runner.o $(OBJ_DIR)/kokkos_fire_runner.o # $(OBJ_DIR)/kokkos_greenberg_runner.o $(OBJ_DIR)/kokkos_wire_runner.o

$(OBJ_DIR)/kokkos_%_runner.o: $(SRC_DIR)/_relwork/kokkos/kokkos_%_runner.cu | directories
	$(NVCC) $(NVCCFLAGS) -c $< -o $@ $(DEFINES) $(KOKKOS_HOME_INCLUDE:%=-I%) -extended-lambda

endif

ifeq ($(ENABLE_HALIDE), ON)
DEFINES += -DENABLE_HALIDE
INCLUDE_DIRS += $(HALIDE_HOME_INCLUDE:%=-I%)
LIB_DIRS += $(HALIDE_HOME_LIB:%=-L%)
LIBS += -lHalide19
EXTRA_OBJS += $(OBJ_DIR)/halide_game_of_life_runner.o # $(OBJ_DIR)/halide_fire_runner.o $(OBJ_DIR)/halide_greenberg_runner.o $(OBJ_DIR)/halide_wire_runner.o

$(OBJ_DIR)/halide_%_runner.o: $(SRC_DIR)/_relwork/halide/halide_%_runner.cpp | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(DEFINES) $(INCLUDE_DIRS)
endif

ifeq ($(ENABLE_AN5D), ON)
DEFINES += -DENABLE_AN5D
endif

# Rules
all: directories $(TARGET) $(TEST_TARGET)

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

install_deps: install_kokkos install_halide

install_kokkos: $(VENDOR_DIR)/kokkos
	@echo "Installing Kokkos dependencies..."
	@mkdir -p $(DEP_DIR) $(DEP_DIR)/kokkos
	@$(CMAKE) -B $(DEP_DIR)/kokkos -S $(VENDOR_DIR)/kokkos \
		-D CMAKE_BUILD_TYPE=Release \
		-D CMAKE_CXX_COMPILER=$(NVCC_CC_BIN) \
		-D CMAKE_CXX_STANDARD=20 \
		-D CMAKE_CUDA_COMPILER=$(NVCC) \
		-D CMAKE_CUDA_HOST_COMPILER=$(NVCC_CC_BIN) \
		-D Kokkos_ENABLE_CUDA=ON \
		-D Kokkos_ENABLE_CUDA_LAMBDA=ON \
		-D Kokkos_ENABLE_SERIAL=ON \
		-D Kokkos_ENABLE_TESTS=OFF \
		-D Kokkos_ARCH_NATIVE=ON \
		-D Kokkos_ENABLE_DEPRECATED_CODE_4=ON \
		-D CMAKE_INSTALL_PREFIX=$(DEP_DIR)
	@$(CMAKE) --build $(DEP_DIR)/kokkos
	@$(CMAKE) --install $(DEP_DIR)/kokkos --prefix=$(DEP_DIR)
	@echo "Kokkos dependencies installed."

# Install LLVM dependencies according to https://github.com/halide/Halide?tab=readme-ov-file#acquiring-llvm
install_llvm: $(VENDOR_DIR)/llvm-project
	@echo "Installing LLVM dependencies..."
	@mkdir -p $(DEP_DIR) $(DEP_DIR)/llvm-build
	@$(CMAKE) -B $(DEP_DIR)/llvm-build -S $(VENDOR_DIR)/llvm-project/llvm \
		-D CMAKE_BUILD_TYPE=Release \
		-D LLVM_ENABLE_PROJECTS="clang;lld;clang-tools-extra" \
		-D LLVM_ENABLE_RUNTIMES=compiler-rt \
		-D LLVM_TARGETS_TO_BUILD="WebAssembly;X86;AArch64;ARM;Hexagon;NVPTX;PowerPC;RISCV" \
		-D LLVM_ENABLE_ASSERTIONS=ON \
		-D LLVM_ENABLE_EH=ON \
		-D LLVM_ENABLE_RTTI=ON \
		-D LLVM_ENABLE_HTTPLIB=OFF \
		-D LLVM_ENABLE_LIBEDIT=OFF \
		-D LLVM_ENABLE_LIBXML2=OFF \
		-D LLVM_ENABLE_TERMINFO=OFF \
		-D LLVM_ENABLE_ZLIB=OFF \
		-D LLVM_ENABLE_ZSTD=OFF \
		-D LLVM_BUILD_32_BITS=OFF
	@$(CMAKE) --build $(DEP_DIR)/llvm-build --parallel $(shell nproc)
	@$(CMAKE) --install $(DEP_DIR)/llvm-build --prefix=$(DEP_DIR)
	@echo "LLVM dependencies installed."

install_halide: install_llvm $(VENDOR_DIR)/Halide
	@echo "Installing Halide dependencies..."
	@mkdir -p $(DEP_DIR) $(DEP_DIR)/halide
	@$(CMAKE) -B $(DEP_DIR)/halide -S $(VENDOR_DIR)/Halide \
		-D CMAKE_BUILD_TYPE=Release \
		-D Halide_LLVM_ROOT=$(shell realpath $(DEP_DIR))
	@$(CMAKE) --build $(DEP_DIR)/halide
	@echo "Halide dependencies installed."

# Add directories dependency to object file creation rules
$(MAIN_OBJ): $(MAIN_SRC) | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_DIRS) $(DEFINES)

$(TEST_OBJ): $(TEST_SRC) | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_DIRS) $(DEFINES)

# Pattern rule for CUDA files with vpath to find them
vpath %.cu $(CUDA_DIRS)
$(OBJ_DIR)/%.o: %.cu | directories
	$(NVCC) $(NVCCFLAGS) -c $< -o $@ $(DEFINES)

# Create separate executables for main app and tests
$(TARGET): $(MAIN_OBJ) $(CUDA_OBJS) $(EXTRA_OBJS)
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

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(DEP_DIR)

.PHONY: all run test clean directories print-% install_deps install_kokkos install_llvm install_halide
