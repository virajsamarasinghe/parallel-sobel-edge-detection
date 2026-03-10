CXX      = g++
CXXFLAGS = -O3 -Wall -Wextra -std=c++11 -Iinclude
BUILD    = build

# ─────────────────────────────────────────────────────────────────────────────
# Three separate binaries, each compiled with the right flags
# ─────────────────────────────────────────────────────────────────────────────
all: $(BUILD)/sobel_serial $(BUILD)/sobel_pthreads $(BUILD)/sobel_openmp

# ── Serial ───────────────────────────────────────────────────────────────────
$(BUILD)/sobel_serial: $(BUILD)/serial/main.o \
                       $(BUILD)/serial/sobel.o \
                       $(BUILD)/serial/utils.o
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/serial/main.o:  src/main.cpp        | $(BUILD)/serial
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/serial/sobel.o: src/sobel.cpp       | $(BUILD)/serial
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/serial/utils.o: src/utils.cpp       | $(BUILD)/serial
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ── Pthreads ─────────────────────────────────────────────────────────────────
$(BUILD)/sobel_pthreads: $(BUILD)/pthreads/main.o \
                         $(BUILD)/pthreads/sobel.o \
                         $(BUILD)/pthreads/sobel_pthreads.o \
                         $(BUILD)/pthreads/utils.o
	$(CXX) $(CXXFLAGS) -pthread -o $@ $^

$(BUILD)/pthreads/main.o: src/main.cpp             | $(BUILD)/pthreads
	$(CXX) $(CXXFLAGS) -DHAS_PTHREADS -c $< -o $@

$(BUILD)/pthreads/sobel.o: src/sobel.cpp           | $(BUILD)/pthreads
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/pthreads/sobel_pthreads.o: src/sobel_pthreads.cpp | $(BUILD)/pthreads
	$(CXX) $(CXXFLAGS) -pthread -DHAS_PTHREADS -c $< -o $@

$(BUILD)/pthreads/utils.o: src/utils.cpp           | $(BUILD)/pthreads
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ── OpenMP ───────────────────────────────────────────────────────────────────
$(BUILD)/sobel_openmp: $(BUILD)/openmp/main.o \
                       $(BUILD)/openmp/sobel.o \
                       $(BUILD)/openmp/sobel_openmp.o \
                       $(BUILD)/openmp/utils.o
	$(CXX) $(CXXFLAGS) -fopenmp -o $@ $^

$(BUILD)/openmp/main.o: src/main.cpp              | $(BUILD)/openmp
	$(CXX) $(CXXFLAGS) -fopenmp -DHAS_OPENMP -c $< -o $@

$(BUILD)/openmp/sobel.o: src/sobel.cpp            | $(BUILD)/openmp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/openmp/sobel_openmp.o: src/sobel_openmp.cpp | $(BUILD)/openmp
	$(CXX) $(CXXFLAGS) -fopenmp -DHAS_OPENMP -c $< -o $@

$(BUILD)/openmp/utils.o: src/utils.cpp            | $(BUILD)/openmp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ── Directory creation ────────────────────────────────────────────────────────
$(BUILD)/serial $(BUILD)/pthreads $(BUILD)/openmp:
	mkdir -p $@

clean:
	rm -rf $(BUILD)

.PHONY: all clean