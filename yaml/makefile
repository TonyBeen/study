CPPFLAGS 	:= -std=c++11
INCLUDE_DIR := -I.
LIB_DIR		:=
SHARED_LIB	:= -lyaml-cpp
STATIC_LIB  := /usr/local/lib/libyaml-cpp.a
CC			:= g++

cleanall	:= testyaml readyaml

all : testyaml readyaml

testyaml : testyaml.cc
	$(CC) $^ -o $@ $(CPPFLAGS) $(STATIC_LIB)

readyaml : readyaml.cc
	$(CC) $^ -o $@ $(CPPFLAGS) $(STATIC_LIB)

.PHONY : testyaml readyaml

clean :
	-rm -rf $(cleanall)