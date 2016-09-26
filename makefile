CC = gcc
FLAGS = -g -std=c99 -Wall -O3 -march=native -pthread
EXEC = wyld

all:
	$(CC) $(FLAGS) *.c -o $(EXEC)
hash_128_bit:
	$(CC) $(FLAGS) *.c -o $(EXEC) -DHASH_128_BIT
stats:
	$(CC) $(FLAGS) *.c -o $(EXEC) -DSTATS
stats_hash_128_bit:
	$(CC) $(FLAGS) *.c -o $(EXEC) -DSTATS -DHASH_128_BIT
