CC     = mpicc
CFLAGS = -Wall -O2
NP     = 2

.PHONY: all run clean remote

all: main

main: main.c
	$(CC) $(CFLAGS) -o $@ $<

sort: sort.c
	$(CC) $(CFLAGS) -o $@ $<

run: main
	mpirun -n $(NP) ./main

runsort: sort
	mpirun -n $(NP) ./sort

clean:
	rm -f main

remote:
	./remote.sh run $(NP)
