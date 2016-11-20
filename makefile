VERSION = 1.0

CC_BASE  = clang
CC_PERFT  = gcc
CFLAGS = -std=c11 -O3 -mpopcnt
CFLAGS_PERFT = -std=c11 -O3 -mpopcnt
SRC = src/*.c
DEPS = -pthread
DEPS_PERFT = -pthread -fopenmp
EXEC_BASE  = wyldchess

CC_WIN64 = x86_64-w64-mingw32-gcc
CC_LINUX = clang
CFLAGS_DIST = -static -std=c11 -O3
CFLAGS_DIST_LINUX = -std=c11 -O3
BIN_PATH = binaries/v$(VERSION)
EXEC = $(BIN_PATH)/$(EXEC_BASE)$(VERSION)
EXT_WIN64 = _win64.exe
EXT_LINUX = _linux
EXT_POPCNT = _popcnt
FAST = _fast_tc

all:
	$(CC_BASE) $(CFLAGS) $(SRC) -o $(EXEC_BASE) $(DEPS)
test:
	$(CC_BASE) $(CFLAGS) $(SRC) -o $(EXEC_BASE) $(DEPS) -DTEST
stats:
	$(CC_BASE) $(CFLAGS) $(SRC) -o $(EXEC_BASE) $(DEPS) -DSTATS
perft:
	$(CC_PERFT) $(CFLAGS_PERFT) $(SRC) -o $(EXEC_BASE) $(DEPS_PERFT) -DPERFT -DTHREADS=3
targets:
	$(shell mkdir -p binaries/v$(VERSION))
	$(CC_WIN64) $(CFLAGS_DIST) $(SRC) -o $(EXEC)$(EXT_WIN64) $(DEPS)
	$(CC_WIN64) $(CFLAGS_DIST) -mpopcnt $(SRC) -o $(EXEC)$(EXT_POPCNT)$(EXT_WIN64) $(DEPS)
	$(CC_WIN64) $(CFLAGS_DIST) $(SRC) -o $(EXEC)$(FAST)$(EXT_WIN64) $(DEPS) -DTEST
	$(CC_WIN64) $(CFLAGS_DIST) -mpopcnt $(SRC) -o $(EXEC)$(EXT_POPCNT)$(FAST)$(EXT_WIN64) $(DEPS) -DTEST
	$(CC_LINUX) $(CFLAGS_DIST_LINUX) $(SRC) -o $(EXEC)$(EXT_LINUX) $(DEPS)
	$(CC_LINUX) $(CFLAGS_DIST_LINUX) -mpopcnt $(SRC) -o $(EXEC)$(EXT_POPCNT)$(EXT_LINUX) $(DEPS)
	$(CC_LINUX) $(CFLAGS_DIST_LINUX) $(SRC) -o $(EXEC)$(FAST)$(EXT_LINUX) $(DEPS) -DTEST
	$(CC_LINUX) $(CFLAGS_DIST_LINUX) -mpopcnt $(SRC) -o $(EXEC)$(EXT_POPCNT)$(FAST)$(EXT_LINUX) $(DEPS) -DTEST
