CC = gcc
CC_FLAGS = -std=c11 -Wall -O3 -flto -pipe
EXT_LIBS = -lm -pthread

SZG_PATH = syzygy

SZG_FILES = tbprobe.c
CC_FILES = bitboard.c eval.c genmoves.c magicmoves.c main.c \
	  move.c mt19937-64.c perft.c position.c search.c   \
	  uci.c xboard.c misc.c options.c parallel_search.c

SZG_OBJS = $(addprefix $(SZG_PATH)/, $(SZG_FILES:.c=.o))
OBJS = $(CC_FILES:.c=.o)

BIN_DIR = /usr/local/bin

EXEC_PATH = .
EXEC = wyldchess

all: $(OBJS) $(SZG_OBJS)
	$(CC) $(CC_FLAGS) $(EXTRA_FLAGS) $^ -o $(EXEC_PATH)/$(EXEC) $(EXT_LIBS)

popcnt:
	$(MAKE) CC_FLAGS="$(CC_FLAGS) -mpopcnt"

bmi:
	$(MAKE) CC_FLAGS="$(CC_FLAGS) -mbmi -mbmi2 -mpopcnt"

stats:
	$(MAKE) CC_FLAGS="$(CC_FLAGS) -DSTATS_BUILD"

debug:
	$(MAKE) CC_FLAGS="-g -fno-omit-frame-pointer"

profiling:
	$(MAKE) CC_FLAGS="$(CC_FLAGS) -mbmi -mbmi2 -mpopcnt -g -fno-omit-frame-pointer"

%.o: %.c
	$(CC) $(CC_FLAGS) $(EXTRA_FLAGS) -c $< -o $@

$(SZG_PATH)/%.o: $(SZG_PATH)/%.c
	$(CC) $(CC_FLAGS) $(EXTRA_FLAGS) -c $< -o $@

install:
	-cp $(EXEC) $(BIN_DIR)
	-strip $(BIN_DIR)/$(EXEC)

uninstall:
	-rm -f $(BIN_DIR)/$(EXEC)

clean:
	-rm -f $(SZG_OBJS) $(OBJS) $(EXEC)
