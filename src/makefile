CXX = g++
CXX_FLAGS = -std=c++11 -Wall -O3 -flto -pipe
EXT_LIBS = -lm -pthread

SZG_PATH = syzygy
SZG_FILES = tbprobe.c

CXX_FILES = bitboard.cpp eval.cpp genmoves.cpp magicmoves.cpp main.cpp \
	  move.cpp mt19937-64.cpp perft.cpp position.cpp search.cpp    \
	  uci.cpp xboard.cpp misc.cpp

SZG_OBJS = $(addprefix $(SZG_PATH)/, $(SZG_FILES:.c=.o))
OBJS = $(CXX_FILES:.cpp=.o)

EXEC_PATH = .
EXEC = wyldchess

ifdef RELEASE

EXEC := $(EXEC)$(RELEASE)
ifeq ("$(TARGET)", "win64")
	CXX_FLAGS += -static
	EXEC_PATH = ../binaries/WyldChess_v$(RELEASE)/$(TARGET)
else ifeq ("$(TARGET)", "linux")
	EXEC_PATH = ../binaries/WyldChess_v$(RELEASE)/$(TARGET)
else
$(error TARGET not defined)
endif

endif

BIN = $(EXEC_PATH)/$(EXEC)

all: $(OBJS) $(SZG_OBJS)
	$(CXX) $(CXX_FLAGS) $^ -o $(BIN) $(EXT_LIBS)

profiling:
	$(MAKE) CXXFLAGS="$(CXX_FLAGS) -mbmi -mbmi2 -mpopcnt -g -fno-omit-frame-pointer"

popcnt:
	$(MAKE) CXXFLAGS="$(CXX_FLAGS) -mpopcnt" EXEC="$(EXEC)_popcnt"

bmi:
	$(MAKE) CXXFLAGS="$(CXX_FLAGS) -mbmi -mbmi2 -mpopcnt" EXEC="$(EXEC)_bmi"

stats:
	$(MAKE) CXXFLAGS="$(CXX_FLAGS) -DSTATS_BUILD -mbmi -mbmi2 -mpopcnt"

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c $< -o $@

$(SZG_PATH)/%.o: $(SZG_PATH)/%.c
	$(CXX) $(CXX_FLAGS) -c $< -o $@

clean:
	-rm -f $(SZG_OBJS)
	-rm -f $(OBJS)
	-rm -f wyldchess
	-rm -f wyldchess_bmi
	-rm -f wyldchess_popcnt
