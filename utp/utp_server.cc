/*************************************************************************
    > File Name: utp_server.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 24 Oct 2022 10:39:14 AM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>
#include <netdb.h>
#include <signal.h>

#ifdef __linux__
#include <linux/errqueue.h>
#include <netinet/ip_icmp.h>
#endif

#include <utp/utp.h>
#include <log/log.h>

#define LOG_TAG "utp_server"

utp_context *ctx;
utp_socket *sock;
bool gExitFlag = false;
int fd;
const char *gLocalAddr = "127.0.0.1";
uint16_t gLocalPort = 8000;

void die(const char *fmt, ...)
{
    va_list ap;
    fflush(stdout);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}

void pdie(const char *err)
{
    fflush(stdout);
    perror(err);
    exit(1);
}

void hexdump(const void *p, size_t len)
{
    int count = 1;
    const uint8_t *ptr = static_cast<const uint8_t *>(p);

    while (len--)
    {
        if (count == 1)
            fprintf(stderr, "    %p: ", p);

        fprintf(stderr, " %02x", *ptr & 0xff);
        ++ptr;

        if (count++ == 16)
        {
            fprintf(stderr, "\n");
            count = 1;
        }
    }

    if (count != 1)
        fprintf(stderr, "\n");
}

void handler(int number)
{
    LOGD("caught signal\n");
    if (sock)
        utp_close(sock);
    gExitFlag = true;
}

uint64 callback_on_read(utp_callback_arguments *a)
{
    const unsigned char *p;
    ssize_t len, left;

    left = a->len;
    p = a->buf;

    while (left)
    {
        len = write(STDOUT_FILENO, p, left);
        left -= len;
        p += len;
        LOGD("Wrote %d bytes, %d left\n", len, left);
    }
    utp_read_drained(a->socket);
    return 0;
}

uint64 callback_on_firewall(utp_callback_arguments *a)
{
    if (sock) {
        LOGD("Firewalling unexpected second inbound connection\n");
        return 1;
    }

    LOGD("Firewall allowing inbound connection\n");
    return 0;
}

void write_data(void)
{
    if (!sock)
        return;

    char buf[] = "hello";
    utp_write(sock, buf, strlen(buf));
}

uint64 callback_on_accept(utp_callback_arguments *a)
{
    assert(!sock);
    sock = a->socket;
    LOGD("Accepted inbound socket %p. state = %d\n", sock);
    return 0;
}

uint64 callback_on_error(utp_callback_arguments *a)
{
    fprintf(stderr, "Error: %s\n", utp_error_code_names[a->error_code]);
    if (sock != NULL) {
        utp_close(sock);
        sock = NULL;
    }

    gExitFlag = true;
    return 0;
}

uint64 callback_on_state_change(utp_callback_arguments *a)
{
    LOGD("state %d: %s\n", a->state, utp_state_names[a->state]);
    utp_socket_stats *stats;

    switch (a->state)
    {
    case UTP_STATE_CONNECT:
    case UTP_STATE_WRITABLE:
        write_data();
        break;

    case UTP_STATE_EOF:
        LOGD("Received EOF from socket\n");
        if (a->socket == sock) {
            sock = NULL;
        }
        utp_close(a->socket);
        break;

    case UTP_STATE_DESTROYING:
        LOGD("UTP socket is being destroyed; exiting\n");

        stats = utp_get_stats(a->socket);
        if (stats)
        {
            LOGD("Socket Statistics:\n");
            LOGD("    Bytes sent:          %d\n", stats->nbytes_xmit);
            LOGD("    Bytes received:      %d\n", stats->nbytes_recv);
            LOGD("    Packets received:    %d\n", stats->nrecv);
            LOGD("    Packets sent:        %d\n", stats->nxmit);
            LOGD("    Duplicate receives:  %d\n", stats->nduprecv);
            LOGD("    Retransmits:         %d\n", stats->rexmit);
            LOGD("    Fast Retransmits:    %d\n", stats->fastrexmit);
            LOGD("    Best guess at MTU:   %d\n", stats->mtu_guess);
        }
        else
        {
            LOGD("No socket statistics available\n");
        }

        sock = NULL;
        gExitFlag = 1;
        break;
    }

    return 0;
}

uint64 callback_sendto(utp_callback_arguments *a)
{
    struct sockaddr_in *sin = (struct sockaddr_in *)a->address;

    LOGD("sendto: %zd byte packet to %s:%d%s\n", a->len, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port),
         (a->flags & UTP_UDP_DONTFRAG) ? "  (DF bit requested, but not yet implemented)" : "");

    // hexdump(a->buf, a->len);

    sendto(fd, a->buf, a->len, 0, a->address, a->address_len);
    return 0;
}

uint64 callback_log(utp_callback_arguments *a)
{
    fprintf(stderr, "log: %s\n", a->buf);
    return 0;
}

uint64 callback_get_ms(utp_callback_arguments *a)
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000 / 1000;
}

#ifdef __linux__
void handle_icmp()
{
    while (1)
    {
        unsigned char vec_buf[4096], ancillary_buf[4096];
        struct iovec iov = {vec_buf, sizeof(vec_buf)};
        struct sockaddr_in remote;
        struct msghdr msg;
        ssize_t len;
        struct cmsghdr *cmsg;
        struct sock_extended_err *e;
        struct sockaddr *icmp_addr;
        struct sockaddr_in *icmp_sin;

        memset(&msg, 0, sizeof(msg));

        msg.msg_name = &remote;
        msg.msg_namelen = sizeof(remote);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_flags = 0;
        msg.msg_control = ancillary_buf;
        msg.msg_controllen = sizeof(ancillary_buf);

        len = recvmsg(fd, &msg, MSG_ERRQUEUE | MSG_DONTWAIT);

        if (len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                pdie("recvmsg");
        }

        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
        {
            if (cmsg->cmsg_type != IP_RECVERR)
            {
                LOGD("Unhandled errqueue type: %d\n", cmsg->cmsg_type);
                continue;
            }

            if (cmsg->cmsg_level != SOL_IP)
            {
                LOGD("Unhandled errqueue level: %d\n", cmsg->cmsg_level);
                continue;
            }

            LOGD("errqueue: IP_RECVERR, SOL_IP, len %zd\n", cmsg->cmsg_len);

            if (remote.sin_family != AF_INET)
            {
                LOGD("Address family is %d, not AF_INET?  Ignoring\n", remote.sin_family);
                continue;
            }

            LOGD("Remote host: %s:%d\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));

            e = (struct sock_extended_err *)CMSG_DATA(cmsg);

            if (!e)
            {
                LOGD("errqueue: sock_extended_err is NULL?\n");
                continue;
            }

            if (e->ee_origin != SO_EE_ORIGIN_ICMP)
            {
                LOGD("errqueue: Unexpected origin: %d\n", e->ee_origin);
                continue;
            }

            LOGD("    ee_errno:  %d\n", e->ee_errno);
            LOGD("    ee_origin: %d\n", e->ee_origin);
            LOGD("    ee_type:   %d\n", e->ee_type);
            LOGD("    ee_code:   %d\n", e->ee_code);
            LOGD("    ee_info:   %d\n", e->ee_info); // discovered MTU for EMSGSIZE errors
            LOGD("    ee_data:   %d\n", e->ee_data);

            // "Node that caused the error"
            // "Node that generated the error"
            icmp_addr = (struct sockaddr *)SO_EE_OFFENDER(e);
            icmp_sin = (struct sockaddr_in *)icmp_addr;

            if (icmp_addr->sa_family != AF_INET)
            {
                LOGD("ICMP's address family is %d, not AF_INET?\n", icmp_addr->sa_family);
                continue;
            }

            if (icmp_sin->sin_port != 0)
            {
                LOGD("ICMP's 'port' is not 0?\n");
                continue;
            }

            LOGD("msg_flags: %d", msg.msg_flags);

            if (msg.msg_flags & MSG_TRUNC)
                fprintf(stderr, " MSG_TRUNC");
            if (msg.msg_flags & MSG_CTRUNC)
                fprintf(stderr, " MSG_CTRUNC");
            if (msg.msg_flags & MSG_EOR)
                fprintf(stderr, " MSG_EOR");
            if (msg.msg_flags & MSG_OOB)
                fprintf(stderr, " MSG_OOB");
            if (msg.msg_flags & MSG_ERRQUEUE)
                fprintf(stderr, " MSG_ERRQUEUE");
            fprintf(stderr, "\n");
            hexdump(vec_buf, len);

            if (e->ee_type == 3 && e->ee_code == 4)
            {
                LOGD("ICMP type 3, code 4: Fragmentation error, discovered MTU %d\n", e->ee_info);
                utp_process_icmp_fragmentation(ctx, vec_buf, len, (struct sockaddr *)&remote, sizeof(remote), e->ee_info);
            }
            else
            {
                LOGD("ICMP type %d, code %d\n", e->ee_type, e->ee_code);
                utp_process_icmp_error(ctx, vec_buf, len, (struct sockaddr *)&remote, sizeof(remote));
            }
        }
    }
}
#endif

void setup(void)
{
    struct sockaddr_in sin;
    int error;
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0)
        pdie("socket");

#ifdef __linux__
    int on = 1;
    // NOTE 此设置可以在sendto无响应后通过poll返回POLLERR，在处理ICMP报文即可知道未响应原因
    if (setsockopt(fd, SOL_IP, IP_RECVERR, &on, sizeof(on)) != 0)
        pdie("setsockopt");

    socklen_t length = sizeof(on);
    // if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &on, &length) != 0)
    //     pdie("setsockopt");
    // printf("********* %d\n", on);
#endif

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(gLocalAddr);
    sin.sin_port = htons(gLocalPort);

    if (bind(fd, (sockaddr *)&sin, sizeof(sin)) != 0)
        pdie("bind");

    socklen_t len = sizeof(sin);
    if (getsockname(fd, (struct sockaddr *)&sin, &len) != 0)
        pdie("getsockname");
    LOGD("Bound to local %s:%d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

    ctx = utp_init(2);
    assert(ctx);
    LOGD("UTP context %p\n", ctx);

    utp_set_callback(ctx, UTP_LOG, &callback_log);
    utp_set_callback(ctx, UTP_SENDTO, &callback_sendto);
    utp_set_callback(ctx, UTP_ON_ERROR, &callback_on_error);
    utp_set_callback(ctx, UTP_ON_STATE_CHANGE, &callback_on_state_change);
    utp_set_callback(ctx, UTP_ON_READ, &callback_on_read);
    utp_set_callback(ctx, UTP_ON_FIREWALL, &callback_on_firewall);
    utp_set_callback(ctx, UTP_ON_ACCEPT, &callback_on_accept);
    utp_set_callback(ctx, UTP_GET_MILLISECONDS, &callback_get_ms);
}

void network_loop(void)
{
    unsigned char socket_data[4096];
    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);
    ssize_t len;
    int ret;

    struct pollfd p[2];
    p[0].fd = fd;
    p[0].events = POLLIN | POLLOUT | POLLERR | POLLRDHUP;

    p[1].fd = -1;
    p[1].events = POLLIN;

    ret = poll(p, 2, 500);
    if (ret < 0)
    {
        if (errno == EINTR)
            LOGD("poll() returned EINTR\n");
        else
            pdie("poll");
    }
    else if (ret == 0)
    {
        // LOGD("poll() timeout\n");
    }
    else
    {
        for (int i = 0; i < 2; ++i) {
#ifdef __linux__
            if (p[i].revents & POLLERR)
                handle_icmp();
#endif
            if (p[i].revents & POLLNVAL) {
                printf("get POLLNVAL fd = %d\n", p[i].fd);
                continue;
            }
            if (p[i].revents & POLLIN)
            {
                while (1)
                {
                    len = recvfrom(fd, socket_data, sizeof(socket_data), MSG_DONTWAIT, (struct sockaddr *)&src_addr, &addrlen);
                    if (len < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            utp_issue_deferred_acks(ctx);
                            break;
                        }
                        else
                            pdie("recvfrom");
                    }

                    LOGD("Received %zd byte UDP packet from %s:%d\n", len, inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));
                    hexdump(socket_data, len);

                    if (!utp_process_udp(ctx, socket_data, len, (struct sockaddr *)&src_addr, addrlen))
                        LOGD("UDP packet not handled by UTP.  Ignoring.\n");
                }
            }
        }
    }

    utp_check_timeouts(ctx);
}

int main(int argc, char **argv)
{
    int option = 0;
    uint16_t localPort = 8000;
    const char *localAddress = "127.0.0.1";

    while ((option = getopt(argc, argv, "hl:")) > 0)
    {
        switch (option)
        {
        case 'h': // help
            break;
        case 'l': // local port
            localPort = atoi(optarg);
            break;
        default:
            die("Unhandled argument: %c\n", option);
        }
    }

    gLocalAddr = localAddress;
    gLocalPort = localPort;

    setup();
    while (!gExitFlag)
        network_loop();

    utp_context_stats *stats = utp_get_context_stats(ctx);
    if (stats) {
		LOGD("           Bucket size:    <23    <373    <723    <1400    >1400\n");
		LOGD("Number of packets sent:  %5d   %5d   %5d    %5d    %5d\n",
			stats->_nraw_send[0], stats->_nraw_send[1], stats->_nraw_send[2], stats->_nraw_send[3], stats->_nraw_send[4]);
		LOGD("Number of packets recv:  %5d   %5d   %5d    %5d    %5d\n",
			stats->_nraw_recv[0], stats->_nraw_recv[1], stats->_nraw_recv[2], stats->_nraw_recv[3], stats->_nraw_recv[4]);
	}
	else {
		LOGD("utp_get_context_stats() failed?\n");
	}

	LOGD("Destroying context\n");
	utp_destroy(ctx);

    return 0;
}
