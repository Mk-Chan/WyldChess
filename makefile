CC = gcc
CC_WIN = x86_64-w64-mingw32-gcc
CFLAGS = -static -std=c11 -Wall -O3 -march=native
SRC = src/*.c
DEPS = -pthread
EXEC = wyld
EXEC_WIN64 = binaries/wyld_win64.exe

all:
	$(CC) $(CFLAGS) $(SRC) -o $(EXEC) $(DEPS)
win64:
	$(CC_WIN) $(CFLAGS) $(SRC) -o $(EXEC_WIN64) $(DEPS)
stats:
	$(CC) $(CFLAGS) $(SRC) -o $(EXEC) $(DEPS) -DSTATS
