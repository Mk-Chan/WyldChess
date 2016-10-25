CC_BASE  = clang
CC_PERFT  = gcc
CC_WIN32 = mingw32-gcc
CC_WIN64 = x86_64-w64-mingw32-gcc
CFLAGS = -static -std=c11 -O3 -mpopcnt
CFLAGS_PERFT = -std=c11 -O3 -mpopcnt
CFLAGS_DIST = -static -std=c11 -O3
SRC = src/*.c
DEPS = -pthread
DEPS_PERFT = -pthread -fopenmp
MAKE_DATE := $(shell date -Idate)
EXEC_BASE  = wyldchess
EXEC_WIN32 = binaries/$(MAKE_DATE)/$(EXEC_BASE)_win32_$(MAKE_DATE).exe
EXEC_WIN64 = binaries/$(MAKE_DATE)/$(EXEC_BASE)_win64_$(MAKE_DATE).exe

all:
	$(CC_BASE) $(CFLAGS) $(SRC) -o $(EXEC_BASE) $(DEPS)
test:
	$(CC_BASE) $(CFLAGS) $(SRC) -o $(EXEC_BASE) $(DEPS) -DTEST
stats:
	$(CC_BASE) $(CFLAGS) $(SRC) -o $(EXEC_BASE) $(DEPS) -DSTATS
perft:
	$(CC_PERFT) $(CFLAGS_PERFT) $(SRC) -o $(EXEC_BASE) $(DEPS_PERFT) -DPERFT -DTHREADS=3
targets:
	$(shell rm -rf binaries)
	$(shell mkdir -p binaries/$(MAKE_DATE))
	$(CC_WIN32) $(CFLAGS_DIST) $(SRC) -o $(EXEC_WIN32) $(DEPS)
	$(CC_WIN64) $(CFLAGS_DIST) $(SRC) -o $(EXEC_WIN64) $(DEPS)
