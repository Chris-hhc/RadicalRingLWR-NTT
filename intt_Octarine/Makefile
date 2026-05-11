CC = gcc
CFLAGS += -O3
CFLAGS += -mavx2
CFLAGS += -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wredundant-decls \
  -Wshadow -Wpointer-arith -fomit-frame-pointer
PYTHON ?= python3
RM = /bin/rm

BASE = .
GENERATED_HEADER = intt_precalcu_tables.h
GENERATED_SCRIPT = scripts/gen_intt_precalcu_zetas.py
FINAL_FUSED_HEADER = intt_final_fused_tables.h
FINAL_FUSED_SCRIPT = scripts/gen_intt_final_fused_zetas.py
INTT_PARAM_HEADER = intt_param.h
INTT_PARAM_SCRIPT = scripts/gen_intt_first6_levels0t5_params.py

SOURCES_CORE = fprime.c poly.c intt.S
TEST_CYCLES = test/cpucycles.c test/speed_print.c
HEADERS_CORE = fprime.h poly.h intt.h parameters.h $(INTT_PARAM_HEADER)

.PHONY: all test clean gen_intt_precalcu_zetas gen_intt_final_fused_zetas gen_intt_param

all: test

test: test/test_intt_avx_fused_vs_poly_invntt

gen_intt_precalcu_zetas: $(GENERATED_HEADER)

gen_intt_final_fused_zetas: $(FINAL_FUSED_HEADER)

gen_intt_param: $(INTT_PARAM_HEADER)

$(GENERATED_HEADER): parameters.h $(GENERATED_SCRIPT)
	$(PYTHON) $(GENERATED_SCRIPT) --input parameters.h --output $(GENERATED_HEADER)

$(FINAL_FUSED_HEADER): $(GENERATED_HEADER) parameters.h $(FINAL_FUSED_SCRIPT)
	$(PYTHON) $(FINAL_FUSED_SCRIPT) --precalcu $(GENERATED_HEADER) --parameters parameters.h --output $(FINAL_FUSED_HEADER)

$(INTT_PARAM_HEADER): $(GENERATED_HEADER) $(FINAL_FUSED_HEADER) parameters.h $(INTT_PARAM_SCRIPT)
	$(PYTHON) $(INTT_PARAM_SCRIPT) --input $(GENERATED_HEADER) --fused $(FINAL_FUSED_HEADER) --parameters parameters.h --output $(INTT_PARAM_HEADER)

TEST_COMMON = $(INTT_PARAM_HEADER) $(GENERATED_HEADER) $(FINAL_FUSED_HEADER) $(SOURCES_CORE) $(TEST_CYCLES) $(HEADERS_CORE)

test/test_intt_avx_fused_vs_poly_invntt: $(TEST_COMMON) test/test_intt_avx_fused_vs_poly_invntt.c
	$(CC) -I$(BASE) -Itest $(CFLAGS) $(SOURCES_CORE) $(TEST_CYCLES) test/test_intt_avx_fused_vs_poly_invntt.c -o $@

clean:
	-$(RM) -f test/test_intt_avx_fused_vs_poly_invntt

clean-generated:
	-$(RM) -f $(GENERATED_HEADER) $(FINAL_FUSED_HEADER) $(INTT_PARAM_HEADER)
