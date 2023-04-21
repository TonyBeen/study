
all :
	make write_protobuf
	make read_protobuf
	make write_protocol
	make read_protocol
	make write_log
	make read_log

write_protobuf: write_protobuf.cc msg.pb.cc
	g++ $^ -o $@ -lprotobuf

read_protobuf: read_protobuf.cc msg.pb.cc
	g++ $^ -o $@ -lprotobuf

msg.pb.cc : msg.proto
	protoc $^ -I=. --cpp_out=.

write_protocol : write_protocol.cc protocol.pb.cc
	g++ $^ -o $@ -lprotobuf

read_protocol : read_protocol.cc protocol.pb.cc
	g++ $^ -o $@ -lprotobuf

protocol.pb.cc : protocol.proto
	protoc $^ -I=. --cpp_out=.

write_log: write_log.cc log.pb.cc
	g++ $^ -o $@ -lprotobuf

read_log: read_log.cc log.pb.cc
	g++ $^ -o $@ -lprotobuf

log.pb.cc: log.proto
	protoc $^ -I=. --cpp_out=.

.PHONY:
	all write_protobuf read_protobuf write_protocol read_protocol write_log read_log clean

clean:
	rm -f write_protobuf read_protobuf write_protocol read_protocol \
	msg.pb.cc msg.pb.h protocol.pb.cc protocol.pb.h \
	write_log read_log log.pb.h log.pb.cc
