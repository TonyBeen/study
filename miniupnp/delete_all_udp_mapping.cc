#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>

int main(void) {
    struct UPNPDev * devlist = 0;
    struct UPNPUrls urls;
    struct IGDdatas data;
    char lanaddr[64];

    // 1. 搜索UPnP设备
    devlist = upnpDiscover(2000, NULL, NULL, 0, 0, 2, NULL);
    if (devlist == NULL) {
        printf("没有发现UPnP设备。\n");
        return 1;
    }

    // 2. 获取IGD
    int igd_status = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
    if (igd_status == 0) {
        printf("没有找到有效的IGD。\n");
        freeUPNPDevlist(devlist);
        return 1;
    }
    printf("找到IGD, 局域网地址: %s\n", lanaddr);

    // 3. 枚举端口映射并删除UDP映射
    int index = 0;
    char str_index[6];
    char extPort[6], intClient[40], intPort[6], protocol[4], desc[80], enabled[6], rHost[64], duration[16];

    while(1) {
        snprintf(str_index, 6, "%hu", index);
        int ret = UPNP_GetGenericPortMappingEntry(urls.controlURL, data.first.servicetype, 
            str_index, extPort, intClient, intPort, protocol, desc, enabled, rHost, duration);

        if (ret != UPNPCOMMAND_SUCCESS) {
            break; // 没有更多映射
        }

        if (strcmp(protocol, "UDP") == 0) {
            printf("删除UDP映射: 外部端口 %s, 内部 %s:%s\n", extPort, intClient, intPort);

            int delret = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, extPort, protocol, NULL);
            if (delret == UPNPCOMMAND_SUCCESS) {
                printf("删除成功。\n");
            } else {
                printf("删除失败: %s\n", strupnperror(delret));
            }
        }
        index++;
    }

    // 释放资源
    freeUPNPDevlist(devlist);
    FreeUPNPUrls(&urls);

    printf("所有UDP端口映射已尝试删除。\n");
    return 0;
}