CC = gcc
FLAGS = -g -std=c11 -Wall -O3 -march=native -pthread
EXEC = wyld

all:
	$(CC) $(FLAGS) *.c -o $(EXEC)
stats:
	$(CC) $(FLAGS) *.c -o $(EXEC) -DSTATS
