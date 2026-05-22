CXX      = g++
MPICXX   = mpicxx
CXXFLAGS = -O3 -Wall -Wextra -std=c++11 -Iinclude

SRC_DIR   = src
BUILD_DIR = build

# Detect OS for cross-platform OpenMP flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    OMP_FLAGS = -fopenmp
else
    OMP_FLAGS = -Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include -L/opt/homebrew/opt/libomp/lib -lomp
endif

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

# ── Hybrid MPI + OpenMP ──
HYBRID_SRCS = $(SRC_DIR)/sobel_hybrid.cpp $(SRC_DIR)/utils.cpp
HYBRID_TARGET = $(BUILD_DIR)/sobel_hybrid

# ── CUDA ──
CUDA_SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/sobel.cpp $(SRC_DIR)/sobel_cuda.cu $(SRC_DIR)/utils.cpp
CUDA_TARGET = $(BUILD_DIR)/sobel_cuda
NVCC = nvcc
NVCCFLAGS = -O3 -std=c++11 -Iinclude -DHAS_CUDA

# ── Generate 4K Utility ──
GEN4K_SRCS = $(SRC_DIR)/generate_4k.cpp
GEN4K_TARGET = $(BUILD_DIR)/generate_4k

all: serial

serial: $(SERIAL_TARGET)
$(SERIAL_TARGET): $(SERIAL_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

openmp: $(OPENMP_TARGET)
$(OPENMP_TARGET): $(OPENMP_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DHAS_OPENMP $(OMP_FLAGS) -o $@ $^

pthreads: $(PTHREADS_TARGET)
$(PTHREADS_TARGET): $(PTHREADS_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DHAS_PTHREADS -lpthread -o $@ $^

mpi: $(MPI_TARGET)
$(MPI_TARGET): $(MPI_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(MPICXX) $(CXXFLAGS) -o $@ $^

hybrid: $(HYBRID_TARGET)
$(HYBRID_TARGET): $(HYBRID_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(MPICXX) $(CXXFLAGS) $(OMP_FLAGS) -o $@ $^

cuda: $(CUDA_TARGET)
$(CUDA_TARGET): $(CUDA_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(NVCC) $(NVCCFLAGS) -o $@ $^

generate_4k: $(GEN4K_TARGET)
$(GEN4K_TARGET): $(GEN4K_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR)

dashboard: serial openmp pthreads mpi hybrid cuda
	@echo "Installing NPM dependencies..."
	npm install
	@echo "Starting dashboard server on port 3000..."
	npm start

.PHONY: all serial openmp pthreads mpi hybrid cuda generate_4k clean run run_4k dashboard

run: serial openmp pthreads
	@echo "--- Running Serial ---"
	./build/sobel_serial data/lenna.png data/output_serial.png serial
	@echo "--- Running OpenMP ---"
	./build/sobel_openmp data/lenna.png data/output_openmp.png openmp 4
	@echo "--- Running Pthreads ---"
	./build/sobel_pthreads data/lenna.png data/output_pthreads.png pthreads 4

run_4k: serial openmp pthreads mpi hybrid generate_4k
	@echo "--- Generating 4K image ---"
	./build/generate_4k
	@echo "--- Running 4K Serial ---"
	./build/sobel_serial data/image_4k.png data/output_serial.png serial
	@echo "--- Running 4K OpenMP ---"
	./build/sobel_openmp data/image_4k.png data/output_openmp.png openmp 4
	@echo "--- Running 4K Pthreads ---"
	./build/sobel_pthreads data/image_4k.png data/output_pthreads.png pthreads 4
	@echo "--- Running 4K MPI ---"
	mpirun --allow-run-as-root -np 4 ./build/sobel_mpi data/image_4k.png data/output_mpi.png data/output_serial.png
	@echo "--- Running 4K Hybrid (MPI + OpenMP) ---"
	OMP_NUM_THREADS=2 mpirun --allow-run-as-root -np 2 ./build/sobel_hybrid data/image_4k.png data/output_hybrid.png data/output_serial.png
