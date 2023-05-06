CC := g++
CPPFLAG := -std=c++11 -g

LIBLIST := -lutils -llog -lsqlutils -lpthread -ldl

SRCLIST := $(wildcard *.cc)
OBJLIST := $(patsubst %.cc, %.o, $(SRCLIST))

insert : $(OBJLIST)
	$(CC) $^ -o $@ $(LIBLIST)
%.o : %.cc
	$(CC) -c $^ -o $@ $(CPPFLAG)


.PHONY:
	clean insert
clean:
	-rm -rf $(OBJLIST)