CC = $(shell which g++)

CXX_FLAGS := -std=c++11

INC_PATH = -I/usr/local/include/eigen3

LIB_PATH = -L/usr/local/lib/

LIB_LIST = -lRaptorQ -pthread

SRC_LIST := test_raptorq.cc
OBJ_LIST := test_raptorq.o

test_raptorq.out : $(OBJ_LIST)
	$(CC) $^ -o $@ $(LIB_PATH) $(LIB_LIST)

%.o : %.cc
	$(CC) $^ -c -o $@ $(CXX_FLAGS) $(INC_PATH)

.PHONY: test_raptorq.out clean

clean:
	-rm -rf $(OBJ_LIST) test_raptorq.out