sorting: sorting.o
	mpicc -O2 -o sorting sorting.o

sorting.o: sorting.c
	mpicc -O2 -c sorting.c

clean:
	rm -f sorting sorting.o
