CC_LINUX = clang
CC_WIN32 = mingw32-gcc
CC_WIN64 = x86_64-w64-mingw32-gcc
CFLAGS = -static -std=c11 -O3 -march=native
CFLAGS_DIST = -static -std=c11 -O3
SRC = src/*.c
DEPS = -pthread
MAKE_DATE := $(shell date -Idate)
EXEC_BASE  = wyldchess
EXEC_LINUX = binaries/$(MAKE_DATE)/$(EXEC_BASE)_linux_$(MAKE_DATE)
EXEC_WIN32 = binaries/$(MAKE_DATE)/$(EXEC_BASE)_win32_$(MAKE_DATE).exe
EXEC_WIN64 = binaries/$(MAKE_DATE)/$(EXEC_BASE)_win64_$(MAKE_DATE).exe

all:
	$(CC_LINUX) $(CFLAGS) $(SRC) -o $(EXEC_BASE) $(DEPS)
stats:
	$(CC_LINUX) $(CFLAGS) $(SRC) -o $(EXEC_LINUX) $(DEPS) -DSTATS
linux:
	$(shell mkdir -p binaries/$(MAKE_DATE))
	$(CC_LINUX) $(CFLAGS) $(SRC) -o $(EXEC_LINUX) $(DEPS)
win32:
	$(shell mkdir -p binaries/$(MAKE_DATE))
	$(CC_WIN32) $(CFLAGS_DIST) $(SRC) -o $(EXEC_WIN32) $(DEPS)
win64:
	$(shell mkdir -p binaries/$(MAKE_DATE))
	$(CC_WIN64) $(CFLAGS_DIST) $(SRC) -o $(EXEC_WIN64) $(DEPS)
