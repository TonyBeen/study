/*************************************************************************
    > File Name: test_miniupnp_add.cc
    > Author: hsz
    > Brief: g++ test_miniupnp_add.cc -o test_miniupnp_add -lminiupnpc
    > Created Time: 2024年02月19日 星期一 13时51分41秒
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

/* Test function
 * 1 - get connection type
 * 2 - get extenal ip address
 * 3 - Add port mapping
 * 4 - get this port mapping from the IGD */
static int SetRedirectAndTest(struct UPNPUrls * urls,
                              struct IGDdatas * data,
                              const char * iaddr,
                              const char * iport,
                              const char * eport,
                              const char * proto,
                              const char * leaseDuration,
                              const char * remoteHost,
                              const char * description,
                              int addAny)
{
    char externalIPAddress[40];
    char intClient[40];
    char intPort[6];
    char reservedPort[6];
    char duration[16];
    int r;

    if(!iaddr || !iport || !eport || !proto)
    {
        fprintf(stderr, "Wrong arguments\n");
        return -1;
    }
    proto = protofix(proto);
    if(!proto)
    {
        fprintf(stderr, "invalid protocol\n");
        return -1;
    }

    r = UPNP_GetExternalIPAddress(urls->controlURL,
                        data->first.servicetype,
                        externalIPAddress);
    if(r!=UPNPCOMMAND_SUCCESS)
        printf("GetExternalIPAddress failed.\n");
    else
        printf("ExternalIPAddress = %s\n", externalIPAddress);

    if (addAny) {
        r = UPNP_AddAnyPortMapping(urls->controlURL, data->first.servicetype,
                        eport, iport, iaddr, description,
                        proto, remoteHost, leaseDuration, reservedPort);
        if(r==UPNPCOMMAND_SUCCESS)
            eport = reservedPort;
        else
            printf("AddAnyPortMapping(%s, %s, %s) failed with code %d (%s)\n",
                    eport, iport, iaddr, r, strupnperror(r));
    } else {
        r = UPNP_AddPortMapping(urls->controlURL, data->first.servicetype,
                    eport, iport, iaddr, description,
                    proto, remoteHost, leaseDuration);
        if(r!=UPNPCOMMAND_SUCCESS) {
            printf("AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
                    eport, iport, iaddr, r, strupnperror(r));
            return -2;
    }
    }

    r = UPNP_GetSpecificPortMappingEntry(urls->controlURL,
                            data->first.servicetype,
                            eport, proto, remoteHost,
                            intClient, intPort, NULL/*desc*/,
                            NULL/*enabled*/, duration);
    if(r!=UPNPCOMMAND_SUCCESS) {
        printf("GetSpecificPortMappingEntry() failed with code %d (%s)\n",
                r, strupnperror(r));
        return -2;
    } else {
        printf("InternalIP:Port = %s:%s\n", intClient, intPort);
        printf("external %s:%s %s is redirected to internal %s:%s (duration=%s)\n",
                externalIPAddress, eport, proto, intClient, intPort, duration);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("usage: %s localPort external_port protocol[udp/tcp]\n", argv[0]);
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
    SetRedirectAndTest(&urls, &data, lanaddr, argv[1], argv[2], argv[3], "0", NULL, "test miniupnp", 0);

    return 0;
}
