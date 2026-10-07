CC     = mpicc
CFLAGS = -Wall -O2
NP     = 4

.PHONY: all run clean remote

all: main

main: main.c
	$(CC) $(CFLAGS) -o $@ $<

run: main
	mpirun -n $(NP) ./main

clean:
	rm -f main

remote:
	./remote.sh run $(NP)
