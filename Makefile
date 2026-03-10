CXX      = g++
MPICXX   = mpicxx
CXXFLAGS = -O3 -Wall -Wextra -std=c++11 -Iinclude

SRC_DIR   = src
BUILD_DIR = build

# ── Serial baseline ──
SERIAL_SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/sobel.cpp $(SRC_DIR)/utils.cpp
SERIAL_TARGET = $(BUILD_DIR)/sobel_serial

# ── OpenMP ──
OPENMP_SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/sobel.cpp $(SRC_DIR)/sobel_openmp.cpp $(SRC_DIR)/utils.cpp
OPENMP_TARGET = $(BUILD_DIR)/sobel_openmp

# ── Pthreads ──
PTHREADS_SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/sobel.cpp $(SRC_DIR)/sobel_pthreads.cpp $(SRC_DIR)/utils.cpp
PTHREADS_TARGET = $(BUILD_DIR)/sobel_pthreads

# ── MPI ──
MPI_SRCS = $(SRC_DIR)/sobel_mpi.cpp $(SRC_DIR)/utils.cpp
MPI_TARGET = $(BUILD_DIR)/sobel_mpi

all: serial

serial: $(SERIAL_TARGET)
$(SERIAL_TARGET): $(SERIAL_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

openmp: $(OPENMP_TARGET)
$(OPENMP_TARGET): $(OPENMP_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DHAS_OPENMP -fopenmp -o $@ $^

pthreads: $(PTHREADS_TARGET)
$(PTHREADS_TARGET): $(PTHREADS_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DHAS_PTHREADS -lpthread -o $@ $^

mpi: $(MPI_TARGET)
$(MPI_TARGET): $(MPI_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(MPICXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all serial openmp pthreads mpi clean
