CC ?= cc
CFLAGS += -O3
CFLAGS += -Wall -Wextra -Wpedantic -std=c11 -mavx2
RM = /bin/rm
PYTHON = python3

SOURCES = fprime.c poly.c poly_precalcu.c
ASMS = ntt.S
HEADERS = fprime.h poly.h ntt.h parameters.h ntt_param.h
NTT_PARAM_HEADER = ntt_param.h
NTT_PARAM_SCRIPT = scripts/gen_ntt_param.py

.PHONY: all clean test gen_ntt_param

all: test/ntt_test

test: test/ntt_test

gen_ntt_param: $(NTT_PARAM_HEADER)

$(NTT_PARAM_HEADER): $(NTT_PARAM_SCRIPT) parameters.h
	$(PYTHON) $(NTT_PARAM_SCRIPT) --input parameters.h --output $@

test/ntt_test: $(NTT_PARAM_HEADER) $(SOURCES) $(ASMS) $(HEADERS) test/cpucycles.c test/speed_print.c test/ntt_test.c
	$(CC) -I. -Itest $(CFLAGS) $(SOURCES) $(ASMS) test/cpucycles.c test/speed_print.c test/ntt_test.c -o $@

clean:
	-$(RM) -f test/ntt_test
	-$(RM) -f $(NTT_PARAM_HEADER)
