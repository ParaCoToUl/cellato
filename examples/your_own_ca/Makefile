CXX ?= g++
CXX_STD ?= c++20
ROOT_DIR := ../..
BIN_DIR := $(ROOT_DIR)/bin/examples
TARGET := $(BIN_DIR)/your_own_ca

CXXFLAGS ?= -std="$(CXX_STD)" -DNDEBUG -Wall -Wextra -Wpedantic -O2
INCLUDE_DIRS := -I$(ROOT_DIR)/include -I/usr/local/cuda/include
LIB_DIRS := -L/usr/local/cuda/lib64
LIBS := -lcudart

.PHONY: all run clean

all: $(TARGET)

$(TARGET): main.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(INCLUDE_DIRS) $(LIB_DIRS) $(LIBS)

$(BIN_DIR):
	mkdir -p $@

run: $(TARGET)
	$(TARGET)

clean:
	rm -f $(TARGET)
