CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE
LDFLAGS = 

# 目标文件
SERVER = mtu_probe_server.out
CLIENT = mtu_probe_client.out
COMMON_OBJ = mtu_probe_common.o

# 默认目标
all: $(SERVER) $(CLIENT)

# 编译公共模块
mtu_probe_common.o: mtu_probe_common.c mtu_probe_common.h
	$(CC) $(CFLAGS) -c $< -o $@

# 编译服务端
$(SERVER): mtu_probe_server.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# 编译客户端
$(CLIENT): mtu_probe_client.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# 清理
clean:
	rm -f $(SERVER) $(CLIENT) $(COMMON_OBJ)

# 调试版本
debug:  CFLAGS += -g -DDEBUG
debug:  clean all

# 安装
install: all
	install -m 755 $(SERVER) /usr/local/bin/
	install -m 755 $(CLIENT) /usr/local/bin/

. PHONY: all clean debug install