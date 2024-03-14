/*************************************************************************
    > File Name: test_miniupnp.cc
    > Author: hsz
    > Brief: g++ test_miniupnp_list.cc -o test_miniupnp_list -lminiupnpc
    > Created Time: 2024年02月19日 星期一 12时57分06秒
 ************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "miniupnpc/miniupnpc.h"
#include "miniupnpc/upnpcommands.h"
#include "miniupnpc/portlistingparse.h"
#include "miniupnpc/upnperrors.h"

static void DisplayInfos(struct UPNPUrls *urls, struct IGDdatas *data)
{
    char externalIPAddress[40];
    char connectionType[64];
    char status[64];
    char lastconnerr[64];
    uint32_t uptime = 0;
    uint32_t brUp, brDown;
    time_t timenow, timestarted;
    int32_t r;
    if (UPNP_GetConnectionTypeInfo(urls->controlURL,
                                   data->first.servicetype,
                                   connectionType) != UPNPCOMMAND_SUCCESS)
        printf("GetConnectionTypeInfo failed.\n");
    else
        printf("Connection Type : %s\n", connectionType);
    if (UPNP_GetStatusInfo(urls->controlURL, data->first.servicetype,
                           status, &uptime, lastconnerr) != UPNPCOMMAND_SUCCESS)
        printf("GetStatusInfo failed.\n");
    else
        printf("Status : %s, uptime=%us, LastConnectionError : %s\n", status, uptime, lastconnerr);
    if (uptime > 0)
    {
        timenow = time(NULL);
        timestarted = timenow - uptime;
        printf("  Time started : %s", ctime(&timestarted));
    }
    if (UPNP_GetLinkLayerMaxBitRates(urls->controlURL_CIF, data->CIF.servicetype,
                                     &brDown, &brUp) != UPNPCOMMAND_SUCCESS)
    {
        printf("GetLinkLayerMaxBitRates failed.\n");
    }
    else
    {
        printf("MaxBitRateDown : %u bps", brDown);
        if (brDown >= 1000000)
        {
            printf(" (%u.%u Mbps)", brDown / 1000000, (brDown / 100000) % 10);
        }
        else if (brDown >= 1000)
        {
            printf(" (%u Kbps)", brDown / 1000);
        }
        printf("   MaxBitRateUp %u bps", brUp);
        if (brUp >= 1000000)
        {
            printf(" (%u.%u Mbps)", brUp / 1000000, (brUp / 100000) % 10);
        }
        else if (brUp >= 1000)
        {
            printf(" (%u Kbps)", brUp / 1000);
        }
        printf("\n");
    }
    r = UPNP_GetExternalIPAddress(urls->controlURL,
                                  data->first.servicetype,
                                  externalIPAddress);
    if (r != UPNPCOMMAND_SUCCESS)
    {
        printf("GetExternalIPAddress failed. (errorcode=%d)\n", r);
    }
    else if (!externalIPAddress[0])
    {
        printf("GetExternalIPAddress failed. (empty string)\n");
    }
    else
    {
        printf("ExternalIPAddress = %s\n", externalIPAddress);
    }
}

void format(uint16_t index, const char *protocol, const char *exPort, const char *inAddr, const char *inPort,
            const char *description, const char *remoteHost, const char *leaseTime)
{
    printf("| %hu | %-8s | %5s->%s:%-5s | '%s' | '%s' | %s |\n",
           index, protocol, exPort, inAddr, inPort, description, remoteHost, leaseTime);
}

static void ListRedirections(struct UPNPUrls *urls,
                             struct IGDdatas *data)
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

    printf("| i | protocol | exPort->inAddr:inPort | description | remoteHost | leaseTime |\n");
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
        if (r == 0)
            format(i, protocol, extPort, intClient, intPort, desc, rHost, duration);
        else
            printf("GetGenericPortMappingEntry() returned %d (%s)\n",
                   r, strupnperror(r));
    } while (r == 0 && i++ < 65535);
}

int main(int argc, char **argv)
{
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
    switch (index)
    {
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
    DisplayInfos(&urls, &data);
    ListRedirections(&urls, &data);

    return 0;
}
