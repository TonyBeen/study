
all:
	make test_udp_client
	make test_udp_server

test_udp_client : test_udp_client.c
	g++ $^ -o $@ -lhv -lpthread

test_udp_server : test_udp_server.c
	g++ $^ -o $@ -lhv -lpthread


.PHONY: all test_udp_client test_udp_server clean

clean:
	rm -rf test_udp_client test_udp_server