CC ?= cc
CFLAGS += -O3
CFLAGS += -Wall -Wextra -Wpedantic -std=c11 -mavx2
RM = /bin/rm
PYTHON = python3

SOURCES = fprime.c poly.c poly_precalcu.c
ASMS = intt.S
HEADERS = fprime.h poly.h parameters.h intt.h intt_param.h
TEST_SOURCES = test/cpucycles.c test/speed_print.c test/test_invntt_precalcu.c
TEST_HEADERS = test/cpucycles.h test/speed_print.h
INTT_PRECALCU_HEADER = intt_precalcu_tables.h
INTT_PRECALCU_SCRIPT = scripts/gen_intt_precalcu_tables.py
INTT_FINAL_FUSED_HEADER = intt_final_fused_tables.h
INTT_FINAL_FUSED_SCRIPT = scripts/gen_intt_final_fused_tables.py
INTT_PARAM_HEADER = intt_param.h
INTT_PARAM_SCRIPT = scripts/gen_intt_avx_params.py

.PHONY: all clean test gen_intt_precalcu_tables gen_intt_final_fused_tables gen_intt_avx_params

all: test/test_invntt_precalcu

test: test/test_invntt_precalcu

gen_intt_precalcu_tables: $(INTT_PRECALCU_HEADER)

gen_intt_final_fused_tables: $(INTT_FINAL_FUSED_HEADER)

gen_intt_avx_params: $(INTT_PARAM_HEADER)

$(INTT_PRECALCU_HEADER): $(INTT_PRECALCU_SCRIPT) parameters.h
	$(PYTHON) $(INTT_PRECALCU_SCRIPT) --input parameters.h --output $@

$(INTT_FINAL_FUSED_HEADER): $(INTT_FINAL_FUSED_SCRIPT) $(INTT_PRECALCU_HEADER) parameters.h
	$(PYTHON) $(INTT_FINAL_FUSED_SCRIPT) --precalcu $(INTT_PRECALCU_HEADER) --parameters parameters.h --output $@

$(INTT_PARAM_HEADER): $(INTT_PARAM_SCRIPT) $(INTT_PRECALCU_HEADER) $(INTT_FINAL_FUSED_HEADER) parameters.h
	$(PYTHON) $(INTT_PARAM_SCRIPT) --precalcu $(INTT_PRECALCU_HEADER) --fused $(INTT_FINAL_FUSED_HEADER) --parameters parameters.h --output $@

test/test_invntt_precalcu: $(INTT_PRECALCU_HEADER) $(INTT_FINAL_FUSED_HEADER) $(INTT_PARAM_HEADER) $(SOURCES) $(ASMS) $(HEADERS) $(TEST_SOURCES) $(TEST_HEADERS)
	$(CC) -I. -Itest $(CFLAGS) $(SOURCES) $(ASMS) $(TEST_SOURCES) -o $@

clean:
	-$(RM) -f $(INTT_PRECALCU_HEADER)
	-$(RM) -f $(INTT_FINAL_FUSED_HEADER)
	-$(RM) -f $(INTT_PARAM_HEADER)
	-$(RM) -f test/test_invntt_precalcu
	-$(RM) -f test_invntt_precalcu128
	-$(RM) -f test_invntt_precalcu256
	-$(RM) -f test_invntt_precalcu512
