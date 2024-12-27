CC := $(shell which gcc)
CXX := $(shell which g++)
CURRENT_PATH := $(shell pwd)

WARN_FLAGS := -m64 -Wall -Werror
C_FLAGS := -std=c11 $(WARN_FLAGS)
CXX_FLAGS := -std=c++11 $(WARN_FLAGS) -g
INC_PATH := -I$(CURRENT_PATH)

C_SRC_LIST := $(wildcard *.c)
CXX_SRC_LIST := $(wildcard *.cc)
ASM_LIST := $(wildcard *.S)

OBJ_LIST := $(patsubst %.c, %.o, $(C_SRC_LIST))
OBJ_LIST += $(patsubst %.cc, %.o, $(CXX_SRC_LIST))
OBJ_LIST += $(patsubst %.S, %.o, $(ASM_LIST))

test_libaco.out: $(OBJ_LIST)
	$(CXX) $^ -o $@

%.o : %.c
	$(CC) $^ -c -o $@ $(C_FLAGS)

%.o : %.S
	$(CC) $^ -c -o $@ $(C_FLAGS)

%.o : %.cc
	$(CXX) $^ -c -o $@ $(CXX_FLAGS)

.PHONY: clean

clean:
	-rm -rf $(OBJ_LIST) test_libaco.out