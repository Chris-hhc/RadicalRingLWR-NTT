CC = gcc
CFLAGS += -O3
CFLAGS += -mavx2
CFLAGS += -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wredundant-decls \
  -Wshadow -Wpointer-arith -fomit-frame-pointer
PYTHON = python3
RM = /bin/rm

BASE_FOLDER = .
SOURCES = fprime.c poly.c
ASMS = ntt.S
HEADERS = fprime.h poly.h ntt.h ntt_param.h parameters.h
NTT_PARAM_HEADER = ntt_param.h
NTT_PARAM_SCRIPT = scripts/gen_ntt_param.py

.PHONY: all test clean gen_ntt_param

all: test

test: test/test_ntt32_final_avx_vs_original

gen_ntt_param: $(NTT_PARAM_HEADER)

$(NTT_PARAM_HEADER): $(NTT_PARAM_SCRIPT) scripts/gen_ntt1024_bot6_dilithium_zetas.py scripts/trace_bottom6_dilithium_layout.py parameters.h
	$(PYTHON) $(NTT_PARAM_SCRIPT) --input parameters.h --output $@

test/test_ntt32_final_avx_vs_original: $(NTT_PARAM_HEADER) $(SOURCES) $(ASMS) $(HEADERS) test/cpucycles.c test/speed_print.c test/test_ntt32_final_avx_vs_original.c
	$(CC) -I$(BASE_FOLDER) -Itest $(CFLAGS) $(SOURCES) $(ASMS) test/cpucycles.c test/speed_print.c test/test_ntt32_final_avx_vs_original.c -o $@

clean:
	-$(RM) -f test/test_ntt32_final_avx_vs_original
