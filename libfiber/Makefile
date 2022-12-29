CC := g++
CPPFLAGS := -std=c++11 -g -Wall
C := gcc
CFLAGS := -std=gnu99 -g -Wall

INCLUDEPATH := -I./util
STATICLIBLIST := /usr/local/lib/libfiber.a

SHAREDLIBPATH := -L/usr/local/lib/
SHAREDLIBLIST := -lutils -llog -lpthread -ldl

FIBER_SERVER_SRC := fiber_server.cc ./util/util.c
FIBER_SERVER_OBJ := fiber_server.o ./util/util.o

all :
	make fiber_server

fiber_server : $(FIBER_SERVER_OBJ)
	$(CC) $^ -o $@ $(STATICLIBLIST) $(SHAREDLIBPATH) $(SHAREDLIBLIST)

%.o : %.cc
	$(CC) -c $^ -o $@ $(CPPFLAGS) $(INCLUDEPATH)
%.o : %.c
	$(C) -c $^ -o $@ $(CFLAGS) $(INCLUDEPATH)

.PHONY : clean all \
		fiber_server

clean :
	-rm -rf $(FIBER_SERVER_OBJ)
