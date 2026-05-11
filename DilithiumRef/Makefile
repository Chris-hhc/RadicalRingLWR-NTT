CC ?= cc
MODE ?= 2
BUILD ?= build

COMMON_CFLAGS ?= -O3 -Wall -Wextra -Wpedantic -std=c11
AVX_CFLAGS ?= -mavx2 -mpopcnt -march=native -mtune=native

REF_PREFIX = pqcrystals_dilithium$(MODE)_ref
AVX_PREFIX = pqcrystals_dilithium$(MODE)_avx2

NTT_BENCH_OBJS = \
  $(BUILD)/bench_ntt.o \
  $(BUILD)/ref/ntt.o \
  $(BUILD)/ref/reduce.o \
  $(BUILD)/avx2/ntt.o \
  $(BUILD)/avx2/shuffle.o \
  $(BUILD)/avx2/consts.o

INTT_BENCH_OBJS = \
  $(BUILD)/bench_intt.o \
  $(BUILD)/ref/ntt.o \
  $(BUILD)/ref/reduce.o \
  $(BUILD)/avx2/ntt.o \
  $(BUILD)/avx2/invntt.o \
  $(BUILD)/avx2/shuffle.o \
  $(BUILD)/avx2/consts.o

.PHONY: all clean run

all: bench_ntt bench_intt

run: all
	./bench_ntt
	./bench_intt

$(BUILD):
	mkdir -p $@

$(BUILD)/ref:
	mkdir -p $@

$(BUILD)/avx2:
	mkdir -p $@

$(BUILD)/bench_ntt.o: bench_ntt.c | $(BUILD)
	$(CC) $(COMMON_CFLAGS) $(AVX_CFLAGS) -c $< -o $@

$(BUILD)/bench_intt.o: bench_intt.c | $(BUILD)
	$(CC) $(COMMON_CFLAGS) $(AVX_CFLAGS) -c $< -o $@

$(BUILD)/ref/ntt.o: ref/ntt.c | $(BUILD)/ref
	$(CC) $(COMMON_CFLAGS) -DDILITHIUM_MODE=$(MODE) -Iref -c $< -o $@

$(BUILD)/ref/reduce.o: ref/reduce.c | $(BUILD)/ref
	$(CC) $(COMMON_CFLAGS) -DDILITHIUM_MODE=$(MODE) -Iref -c $< -o $@

$(BUILD)/avx2/ntt.o: avx2/ntt.S | $(BUILD)/avx2
	$(CC) $(COMMON_CFLAGS) $(AVX_CFLAGS) -DDILITHIUM_MODE=$(MODE) -Iavx2 \
	  -c $< -o $@

$(BUILD)/avx2/invntt.o: avx2/invntt.S | $(BUILD)/avx2
	$(CC) $(COMMON_CFLAGS) $(AVX_CFLAGS) -DDILITHIUM_MODE=$(MODE) -Iavx2 \
	  -c $< -o $@

$(BUILD)/avx2/shuffle.o: avx2/shuffle.S | $(BUILD)/avx2
	$(CC) $(COMMON_CFLAGS) $(AVX_CFLAGS) -DDILITHIUM_MODE=$(MODE) -Iavx2 \
	  -c $< -o $@

$(BUILD)/avx2/consts.o: avx2/consts.c | $(BUILD)/avx2
	$(CC) $(COMMON_CFLAGS) $(AVX_CFLAGS) -DDILITHIUM_MODE=$(MODE) -Iavx2 \
	  -c $< -o $@

bench_ntt: $(NTT_BENCH_OBJS)
	$(CC) $(COMMON_CFLAGS) $(AVX_CFLAGS) -o $@ $(NTT_BENCH_OBJS)

bench_intt: $(INTT_BENCH_OBJS)
	$(CC) $(COMMON_CFLAGS) $(AVX_CFLAGS) -o $@ $(INTT_BENCH_OBJS)

clean:
	rm -rf $(BUILD) bench_ntt bench_intt
