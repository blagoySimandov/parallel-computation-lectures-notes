CC     = mpicc
CFLAGS = -Wall -O2
NP    ?= 4
P     ?= main
ROOT  := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS  := $(wildcard *.c)
PROGS := $(SRCS:%.c=bin/%)

.PHONY: all run clean list remote run-% remote-%

all: $(PROGS)

bin/%: %.c | bin
	$(CC) $(CFLAGS) -o $@ $<

bin:
	mkdir -p bin

%: bin/% ;

run: run-$(P)

run-%: bin/%
	mpirun -n $(NP) ./bin/$*

remote: remote-$(P)

remote-%:
	$(ROOT)remote.sh run $(NP) $*

list:
	@echo $(SRCS:%.c=%)

clean:
	rm -rf bin
