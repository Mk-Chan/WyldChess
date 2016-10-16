CC_BASE  = clang
CC_WIN32 = mingw32-gcc
CC_WIN64 = x86_64-w64-mingw32-gcc
CFLAGS = -static -std=c11 -O3 -march=native
CFLAGS_DIST = -static -std=c11 -O3
SRC = src/*.c
DEPS = -pthread
MAKE_DATE := $(shell date -Idate)
EXEC_BASE  = wyldchess
EXEC_WIN32 = binaries/$(MAKE_DATE)/$(EXEC_BASE)_win32_$(MAKE_DATE).exe
EXEC_WIN64 = binaries/$(MAKE_DATE)/$(EXEC_BASE)_win64_$(MAKE_DATE).exe

all:
	$(CC_BASE) $(CFLAGS) $(SRC) -o $(EXEC_BASE) $(DEPS)
stats:
	$(CC_BASE) $(CFLAGS) $(SRC) -o $(EXEC_BASE) $(DEPS) -DSTATS
targets:
	$(shell rm -rf binaries)
	$(shell mkdir -p binaries/$(MAKE_DATE))
	$(CC_WIN32) $(CFLAGS_DIST) $(SRC) -o $(EXEC_WIN32) $(DEPS)
	$(CC_WIN64) $(CFLAGS_DIST) $(SRC) -o $(EXEC_WIN64) $(DEPS)
