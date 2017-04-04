CC = g++
CFLAGS = -std=c++11 -Wall -O3 -flto -pipe
EXT_LIBS = -lm -pthread

HEADERS = $(addprefix $(SRC_PATH)/, bitboard.h defs.h engine.h magicmoves.h position.h misc.h search.h tt.h)

C_FILES = bitboard.c eval.c genmoves.c magicmoves.c main.c move.c mt19937-64.c perft.c position.c search.c uci.c xboard.c misc.c
SRC_PATH = src
SRC = $(addprefix $(SRC_PATH)/, $(C_FILES))

OBJ_PATH = objs
OBJS = $(addprefix $(OBJ_PATH)/, $(C_FILES:.c=.o))

EXEC_PATH = .
EXEC = wyldchess

ifdef RELEASE

EXEC := $(EXEC)$(RELEASE)
ifeq ("$(TARGET)", "win64")
	CFLAGS += -static
	EXEC_PATH = binaries/WyldChess_v$(RELEASE)/$(TARGET)
else ifeq ("$(TARGET)", "linux")
	EXEC_PATH = binaries/WyldChess_v$(RELEASE)/$(TARGET)
else
$(error TARGET not defined)
endif

endif

BIN = $(EXEC_PATH)/$(EXEC)

all: $(OBJS)
	@-mkdir -p $(EXEC_PATH)
	$(CC) $(CFLAGS) $^ -o $(BIN) $(EXT_LIBS)

profiling:
	$(MAKE) CFLAGS="$(CFLAGS) -mbmi -mbmi2 -mpopcnt -g -fno-omit-frame-pointer"

popcnt:
	$(MAKE) CFLAGS="$(CFLAGS) -mpopcnt" EXEC="$(EXEC)_popcnt"

bmi:
	$(MAKE) CFLAGS="$(CFLAGS) -mbmi -mbmi2 -mpopcnt" EXEC="$(EXEC)_bmi"

stats:
	$(MAKE) CFLAGS="$(CFLAGS) -DSTATS -mbmi -mbmi2 -mpopcnt"

plain:
	$(MAKE) CFLAGS="$(CFLAGS) -DPLAIN_AB"

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c $(HEADERS)
	@-mkdir -p $(OBJ_PATH)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(OBJS)
	-rm -f wyldchess
	-rm -f wyldchess_bmi
	-rm -f wyldchess_popcnt
