CC_LINUX = gcc
CC_WIN32 = mingw32-gcc
CC_WIN64 = x86_64-w64-mingw32-gcc
CFLAGS = -static -std=c11 -O3
SRC = src/*.c
DEPS = -pthread
EXEC_LINUX = wyld
EXEC_WIN32 = binaries/wyld_win32.exe
EXEC_WIN64 = binaries/wyld_win64.exe

all:
	$(CC_LINUX) $(CFLAGS) $(SRC) -o $(EXEC_LINUX) $(DEPS)
win32:
	$(CC_WIN32) $(CFLAGS) $(SRC) -o $(EXEC_WIN32) $(DEPS)
win64:
	$(CC_WIN64) $(CFLAGS) $(SRC) -o $(EXEC_WIN64) $(DEPS)
stats:
	$(CC_LINUX) $(CFLAGS) $(SRC) -o $(EXEC_LINUX) $(DEPS) -DSTATS