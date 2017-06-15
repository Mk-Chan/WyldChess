CC = gcc
CC_FLAGS = -std=c11 -Wall -O3 -flto -pipe
EXT_LIBS = -lm -pthread

SZG_PATH = syzygy
SZG_FILES = tbprobe.c

CC_FILES = bitboard.c eval.c genmoves.c magicmoves.c main.c \
	  move.c mt19937-64.c perft.c position.c search.c   \
	  uci.c xboard.c misc.c

SZG_OBJS = $(addprefix $(SZG_PATH)/, $(SZG_FILES:.c=.o))
OBJS = $(CC_FILES:.c=.o)

EXEC_PATH = .
EXEC = wyldchess

ifdef RELEASE

EXEC := $(EXEC)$(RELEASE)
ifeq ("$(TARGET)", "win64")
	CC_FLAGS += -static
	EXEC_PATH = ../binaries/WyldChess_v$(RELEASE)/$(TARGET)
else ifeq ("$(TARGET)", "linux")
	EXEC_PATH = ../binaries/WyldChess_v$(RELEASE)/$(TARGET)
else
$(error TARGET not defined)
endif

endif

BIN = $(EXEC_PATH)/$(EXEC)

all: $(OBJS) $(SZG_OBJS)
	$(CC) $(CC_FLAGS) $^ -o $(BIN) $(EXT_LIBS)

profiling:
	$(MAKE) CC_FLAGS="$(CC_FLAGS) -mbmi -mbmi2 -mpopcnt -g -fno-omit-frame-pointer"

popcnt:
	$(MAKE) CC_FLAGS="$(CC_FLAGS) -mpopcnt" EXEC="$(EXEC)_popcnt"

bmi:
	$(MAKE) CC_FLAGS="$(CC_FLAGS) -mbmi -mbmi2 -mpopcnt" EXEC="$(EXEC)_bmi"

stats:
	$(MAKE) CC_FLAGS="$(CC_FLAGS) -DSTATS_BUILD"

debug:
	$(MAKE) CC_FLAGS="$(CC_FLAGS) -g -fno-omit-frame-pointer"

%.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@

$(SZG_PATH)/%.o: $(SZG_PATH)/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@

clean:
	-rm -f $(SZG_OBJS)
	-rm -f $(OBJS)
	-rm -f wyldchess
	-rm -f wyldchess_bmi
	-rm -f wyldchess_popcnt
