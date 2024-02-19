/*************************************************************************
    > File Name: test_miniupnp_remove.cc
    > Author: hsz
    > Brief: g++ test_miniupnp_remove.cc -o test_miniupnp_remove -lminiupnpc
    > Created Time: 2024年02月19日 星期一 14时11分17秒
 ************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <cctype>

#include "miniupnpc/miniupnpc.h"
#include "miniupnpc/upnpcommands.h"
#include "miniupnpc/portlistingparse.h"
#include "miniupnpc/upnperrors.h"

/* protofix() checks if protocol is "UDP" or "TCP"
 * returns NULL if not */
const char *protofix(const char *proto)
{
	static const char proto_tcp[4] = { 'T', 'C', 'P', 0};
	static const char proto_udp[4] = { 'U', 'D', 'P', 0};
	bool match = true;
	for (int32_t i = 0; i < strlen(proto_tcp); i++)
    {
        char ch = toupper(proto[i]);
        match = match && (ch == proto_tcp[i]);
    }

    if (match)
        return proto_tcp;

    match = true;
    for (int32_t i = 0; i < strlen(proto_udp); i++)
    {
        char ch = toupper(proto[i]);
        match = match && (ch == proto_udp[i]);
    }

    if (match)
        return proto_udp;

    return 0;
}

static int RemoveRedirect(struct UPNPUrls * urls,
                          struct IGDdatas * data,
                          const char * eport,
                          const char * proto,
                          const char * remoteHost)
{
	int r;
	if (!proto || !eport)
	{
		fprintf(stderr, "invalid arguments\n");
		return -1;
	}

	proto = protofix(proto);
	if (!proto)
	{
		fprintf(stderr, "protocol invalid\n");
		return -1;
	}

	r = UPNP_DeletePortMapping(urls->controlURL, data->first.servicetype, eport, proto, remoteHost);
	if (r != UPNPCOMMAND_SUCCESS) {
		printf("UPNP_DeletePortMapping() failed with code : %d\n", r);
		return -2;
	} else {
		printf("UPNP_DeletePortMapping() returned : %d\n", r);
	}

	return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("usage: %s external_port protocol[udp/tcp]\n", argv[0]);
        return -1;
    }

    struct UPNPDev *devlist = nullptr;
    int32_t error = 0;
    devlist = upnpDiscover(100, nullptr, nullptr, 0, 0, 2, &error);
    if (nullptr == devlist)
    {
        printf("No IGD UPnP Device found on the network!\n");
        return -1;
    }

    struct UPNPDev *device = nullptr;
    struct UPNPUrls urls;
    struct IGDdatas data;
    char lanaddr[64] = "unset";

    printf("List of UPNP devices found on the network:\n");
    for (device = devlist; nullptr != device; device = device->pNext)
    {
        printf("\txml url: %s\n\tdevice type: %s\n\n", device->descURL, device->st);
    }

    int32_t index = 0;
    index = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
    switch (index) {
    case 1:
        printf("Found valid IGD : %s\n", urls.controlURL);
        break;
    case 2:
        printf("Found a (not connected?) IGD : %s\n", urls.controlURL);
        break;
    case 3:
        printf("UPnP device found. Is it an IGD ? : %s\n", urls.controlURL);
        break;
    default:
        printf("Found device (igd ?) : %s\n", urls.controlURL);
        break;
    }

    printf("Local LAN ip address : %s\n", lanaddr);
    RemoveRedirect(&urls, &data, argv[1], argv[2], NULL);

    return 0;
}
