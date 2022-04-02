all: trace LC4.o loader.o
all: trace LC4.o loader.o

trace: loader.o trace.c LC4.o
	clang -g loader.o LC4.o trace.c -o trace

LC4.o: LC4.c LC4.h
	clang -c LC4.c 

loader.o: LC4.o
	clang -c loader.c

clean:
	rm -rf *.o

clobber: clean
	rm -rf trace LC4.o loader.o
