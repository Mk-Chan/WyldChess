CC_BASE  = clang
CC_PERFT  = gcc
CFLAGS = -std=c11 -O3 -mpopcnt
CFLAGS_PERFT = -std=c11 -O3 -mpopcnt
SRC = src/*.c
DEPS = -pthread
DEPS_PERFT = -pthread -fopenmp
EXEC_BASE  = wyldchess

CC_WIN64 = x86_64-w64-mingw32-gcc
CC_LINUX = gcc
MAKE_DATE := $(shell date -Idate)
CFLAGS_DIST = -static -std=c11 -O3
EXEC_WIN64 = binaries/$(MAKE_DATE)/$(EXEC_BASE)_win64_$(MAKE_DATE).exe
EXEC_LINUX = binaries/$(MAKE_DATE)/$(EXEC_BASE)_linux_$(MAKE_DATE)

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
	$(CC_WIN64) $(CFLAGS_DIST) $(SRC) -o $(EXEC_WIN64) $(DEPS)
	$(CC_LINUX) $(CFLAGS_DIST) $(SRC) -o $(EXEC_LINUX) $(DEPS)
