CC = gcc
CFLAGS = -O2 -Wall -pthread
LDFLAGS = -pthread

# Target executables
TARGET_FALSE_SHARING = false_sharing
TARGET_NO_FALSE_SHARING = no_false_sharing

# Source files
SRC = false_sharing.c

# Number of threads (can be overridden: make NTHREADS=5)
NTHREADS ?= 2

.PHONY: all clean run compare test-threads

all: $(TARGET_FALSE_SHARING) $(TARGET_NO_FALSE_SHARING)

# Compile with false sharing (counters adjacent in memory)
$(TARGET_FALSE_SHARING): $(SRC)
	$(CC) $(CFLAGS) -DNTHREADS=$(NTHREADS) -o $@ $< $(LDFLAGS)

# Compile with padding to avoid false sharing
$(TARGET_NO_FALSE_SHARING): $(SRC)
	$(CC) $(CFLAGS) -DNTHREADS=$(NTHREADS) -DPADDED -o $@ $< $(LDFLAGS)

# Run both versions and measure execution time
run: all
	@echo "=== Running with FALSE SHARING ==="
	time ./$(TARGET_FALSE_SHARING)
	@echo ""
	@echo "=== Running WITHOUT FALSE SHARING (padded) ==="
	time ./$(TARGET_NO_FALSE_SHARING)

# Run each version padding and non-padding
run_pad: all
	@echo "=== Running WITHOUT FALSE SHARING (padded) ==="
	time ./$(TARGET_NO_FALSE_SHARING)
	
run_nonpad: all
	@echo "=== Running with FALSE SHARING ==="
	time ./$(TARGET_FALSE_SHARING)
	@echo ""
# Run comparison multiple times
compare: all
	@echo "Running false sharing comparison (3 runs each)..."
	@echo ""
	@echo "=== With FALSE SHARING ==="
	@for i in 1 2 3; do \
		echo "Run $$i:"; \
		time ./$(TARGET_FALSE_SHARING); \
		echo ""; \
	done
	@echo "=== WITHOUT FALSE SHARING (padded) ==="
	@for i in 1 2 3; do \
		echo "Run $$i:"; \
		time ./$(TARGET_NO_FALSE_SHARING); \
		echo ""; \
	done

# Test with different thread counts
test-threads:
	@echo "=== Testing with 2 threads ==="
	@$(MAKE) clean && $(MAKE) NTHREADS=2 run
	@echo ""
	@echo "=== Testing with 4 threads ==="
	@$(MAKE) clean && $(MAKE) NTHREADS=4 run
	@echo ""
	@echo "=== Testing with 8 threads ==="
	@$(MAKE) clean && $(MAKE) NTHREADS=8 run

perf_false_sharing: all
	perf stat ./$(TARGET_FALSE_SHARING)
perf_non_false_sharing: all
	perf stat ./$(TARGET_NO_FALSE_SHARING)

clean:
	rm -f $(TARGET_FALSE_SHARING) $(TARGET_NO_FALSE_SHARING)
