CC := $(shell which gcc)
CXX := $(shell which g++)
CURRENT_PATH := $(shell pwd)

WARN_FLAGS := -m64 -Wall -Werror -pthread
C_FLAGS := -std=c11 $(WARN_FLAGS)
CXX_FLAGS := -std=c++11 $(WARN_FLAGS) -g
INC_PATH := -I$(CURRENT_PATH)

LIB_PATH :=
LIB_LIST := -lutils

C_SRC_LIST := $(wildcard *.c)
ASM_LIST := $(wildcard *.S)
CXX_SRC_LIST := $(wildcard *.cc)

OBJ_LIST := $(patsubst %.c, %.o, $(C_SRC_LIST))
OBJ_LIST += $(patsubst %.S, %.o, $(ASM_LIST))

all: test_libaco.out benchmark.out

test_libaco.out: test_libaco.o $(OBJ_LIST)
	$(CXX) $^ -o $@

benchmark.out: benchmark.o $(OBJ_LIST)
	$(CXX) $^ -o $@ $(LIB_LIST)

%.o : %.c
	$(CC) $^ -c -o $@ $(C_FLAGS) $(INC_PATH)

%.o : %.S
	$(CC) $^ -c -o $@ $(C_FLAGS) $(INC_PATH)

%.o : %.cc
	$(CXX) $^ -c -o $@ $(CXX_FLAGS) $(INC_PATH)

.PHONY: clean test_libaco.out benchmark.out

clean:
	-rm -rf $(OBJ_LIST) test_libaco.out bengch.out