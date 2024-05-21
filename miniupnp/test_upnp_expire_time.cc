/*************************************************************************
    > File Name: test_upnp_expire_time.cc
    > Author: hsz
    > Brief: g++ test_upnp_expire_time.cc -o test_upnp_expire_time -lminiupnpc -llog
    > Created Time: 2024年05月20日 星期一 16时39分31秒
 ************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <cctype>
#include <sstream>

#include <unistd.h>

#include <log/log.h>

#include "miniupnpc/miniupnpc.h"
#include "miniupnpc/upnpcommands.h"
#include "miniupnpc/portlistingparse.h"
#include "miniupnpc/upnperrors.h"

#define LOG_TAG "test_upnp_expire_time"

struct UPNPDev *g_pUpnpList = nullptr;

const char *protofix(const char *proto)
{
    static const char proto_tcp[4] = {'T', 'C', 'P', 0};
    static const char proto_udp[4] = {'U', 'D', 'P', 0};
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

static int SetRedirectAndTest(struct UPNPUrls *urls,
                              struct IGDdatas *data,
                              const char *iaddr,
                              const char *iport,
                              const char *eport,
                              const char *proto,
                              const char *leaseDuration,
                              const char *remoteHost,
                              const char *description,
                              int addAny)
{
    char externalIPAddress[40];
    char intClient[40];
    char intPort[6];
    char reservedPort[6];
    char duration[16];
    int r;

    if (!iaddr || !iport || !eport || !proto)
    {
        fprintf(stderr, "Wrong arguments\n");
        return -1;
    }
    proto = protofix(proto);
    if (!proto)
    {
        fprintf(stderr, "invalid protocol\n");
        return -1;
    }

    r = UPNP_GetExternalIPAddress(urls->controlURL,
                                  data->first.servicetype,
                                  externalIPAddress);
    if (r != UPNPCOMMAND_SUCCESS)
        printf("GetExternalIPAddress failed.\n");
    else
        printf("ExternalIPAddress = %s\n", externalIPAddress);

    if (addAny)
    {
        r = UPNP_AddAnyPortMapping(urls->controlURL, data->first.servicetype,
                                   eport, iport, iaddr, description,
                                   proto, remoteHost, leaseDuration, reservedPort);
        if (r == UPNPCOMMAND_SUCCESS)
            eport = reservedPort;
        else
            printf("AddAnyPortMapping(%s, %s, %s) failed with code %d (%s)\n",
                   eport, iport, iaddr, r, strupnperror(r));
    }
    else
    {
        r = UPNP_AddPortMapping(urls->controlURL, data->first.servicetype,
                                eport, iport, iaddr, description,
                                proto, remoteHost, leaseDuration);
        if (r != UPNPCOMMAND_SUCCESS)
        {
            printf("AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
                   eport, iport, iaddr, r, strupnperror(r));
            return -2;
        }
    }

    r = UPNP_GetSpecificPortMappingEntry(urls->controlURL,
                                         data->first.servicetype,
                                         eport, proto, remoteHost,
                                         intClient, intPort, NULL /*desc*/,
                                         NULL /*enabled*/, duration);
    if (r != UPNPCOMMAND_SUCCESS)
    {
        printf("GetSpecificPortMappingEntry() failed with code %d (%s)\n",
               r, strupnperror(r));
        return -2;
    }
    else
    {
        printf("InternalIP:Port = %s:%s\n", intClient, intPort);
        printf("external %s:%s %s is redirected to internal %s:%s (duration=%s)\n",
               externalIPAddress, eport, proto, intClient, intPort, duration);
    }
    return 0;
}

static bool ListRedirections(struct UPNPUrls *urls, struct IGDdatas *data,
                             uint16_t exPortWanted, uint16_t localPortWanted,
                             std::string ip, std::string proto)
{
    int r;
    unsigned short i = 0;
    char index[6];
    char intClient[40];
    char intPort[6];
    char extPort[6];
    char protocol[4];
    char desc[80];
    char enabled[6];
    char rHost[64];
    char duration[16];

    do
    {
        snprintf(index, 6, "%hu", i);
        rHost[0] = '\0';
        enabled[0] = '\0';
        duration[0] = '\0';
        desc[0] = '\0';
        extPort[0] = '\0';
        intPort[0] = '\0';
        intClient[0] = '\0';
        r = UPNP_GetGenericPortMappingEntry(urls->controlURL,
                                            data->first.servicetype,
                                            index,
                                            extPort, intClient, intPort,
                                            protocol, desc, enabled,
                                            rHost, duration);
        if (r == 0) {
            uint16_t exPort = atoi(extPort);
            uint16_t localPort = atoi(intPort);
            std::string localIP = intClient;
            std::string strProto = protocol;
            for (auto& c : strProto) {
                c = std::tolower(c);
            }

            if (exPort == exPortWanted && localPort == localPortWanted && localIP == ip && strProto == proto)
            {
                return true;
            }
        } else if (r != 713) {
            printf("GetGenericPortMappingEntry() returned %d (%s)\n", r, strupnperror(r));
        }
    } while (r == 0 && i++ < 65535);

    return false;
}

int main(int argc, char **argv)
{
    int32_t error = 0;
    g_pUpnpList = upnpDiscover(100, nullptr, nullptr, 0, 0, 2, &error);
    if (nullptr == g_pUpnpList)
    {
        printf("No IGD UPnP Device found on the network!\n");
        return -1;
    }

    struct UPNPUrls urls;
    struct IGDdatas data;
    char lanaddr[64] = "unset";
    int32_t index = 0;
    index = UPNP_GetValidIGD(g_pUpnpList, &urls, &data, lanaddr, sizeof(lanaddr));
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
    SetRedirectAndTest(&urls, &data, lanaddr, "11000", "11000", "tcp", "10000", NULL, "test miniupnp", 0);

    LOGI("start");
    uint32_t count = 1;
    while (1) {
        bool found = ListRedirections(&urls, &data, 11000, 11000, lanaddr, "tcp");
        if (!found) {
            LOGI("\nTCP 11000 -> %s:11000 Not Found", lanaddr);
            break;
        } else {
            printf("\rfound (count = %u)", count);
            fflush(stdout);
        }

        ++count;
        sleep(1);
    }

    return 0;
}
