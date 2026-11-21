# purehdc-agent — C11 build (zig cc / clang / gcc compatible)
CC      ?= zig cc
CFLAGS  ?= -O2 -std=c11 -Wall -Iinclude
LDLIBS  ?= -lm
BIN     := bin
HVCORE  := src/algebra/hv_binary.c

EXPS := bench_store qa_kg gridworld hard_qa adversarial_qa subst_qa cap_curve
BINS := $(addprefix $(BIN)/,test_algebra test_ghrr test_sdr purehdc-agent $(EXPS))

all: $(BINS) | $(BIN)

$(BIN)/test_algebra: tests/test_algebra.c $(HVCORE) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
$(BIN)/test_ghrr: tests/test_ghrr.c src/algebra/ghrr.c $(HVCORE) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
$(BIN)/test_sdr: tests/test_sdr.c src/encoding/sdr.c $(HVCORE) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
$(BIN)/purehdc-agent: src/agent/main.c src/tam/store.c $(HVCORE) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
$(BIN)/%: experiments/%.c src/tam/store.c $(HVCORE) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BIN):
	mkdir -p $(BIN)

test: $(BIN)/test_algebra $(BIN)/test_ghrr $(BIN)/test_sdr
	./$(BIN)/test_algebra && ./$(BIN)/test_sdr && ./$(BIN)/test_ghrr

clean:
	rm -rf $(BIN)

.PHONY: all test clean
