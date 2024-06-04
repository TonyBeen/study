
all: rdtsc

rdtsc : rdtsc.cc
	g++ rdtsc.cc -o rdtsc -std=c++11 -lutils -lpthread

.PHONY: rdtsc clean

clean:
	rm -rf rdtsc