CC     = mpicc
CFLAGS = -Wall -O2
NP    ?= 4
P     ?= sorting
ALGO  ?= direct
PRINT ?=

ALGOS := direct bucket all-to-all odd-even
ARGS   = $(ALGO) $(if $(PRINT),print)

SRCS  := $(wildcard *.c)
PROGS := $(SRCS:%.c=bin/%)

.PHONY: all run clean list run-% $(ALGOS) bench

all: $(PROGS)

bin/%: %.c | bin
	$(CC) $(CFLAGS) -o $@ $<

bin:
	mkdir -p bin

%: bin/% ;

run: run-$(P)

run-%: bin/%
	mpirun -n $(NP) ./bin/$* $(ARGS)

$(ALGOS):
	$(MAKE) --no-print-directory run ALGO=$@

bench: bin/$(P)
	@for a in $(ALGOS); do \
	  echo "== $$a NP=$(NP)"; \
	  mpirun -n $(NP) ./bin/$(P) $$a; \
	done

list:
	@echo $(SRCS:%.c=%)

clean:
	rm -rf bin
