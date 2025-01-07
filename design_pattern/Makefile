CC = $(shell which g++)
CURRENT_PATH := $(shell pwd)

TARGET_DIR := $(CURRENT_PATH)/bin

CXX_FLAGS := -std=c++11 -g -O0

SRC_LIST := $(wildcard *.cc)
EXE_LIST := $(patsubst %.cc, $(TARGET_DIR)/%.out, $(SRC_LIST))

all:check_path $(EXE_LIST)

check_path:
	@if [ ! -d $(TARGET_DIR) ]; then mkdir -p $(TARGET_DIR); fi

$(TARGET_DIR)/%.out: %.cc
	$(CC) $^ -o $@ $(CXX_FLAGS)

.PHONY:all clean

clean:
	-rm -rf $(TARGET_DIR)
