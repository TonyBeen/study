/*************************************************************************
    > File Name: open_video1.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年11月26日 星期二 10时59分40秒
 ************************************************************************/

#include <string.h>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

using namespace std;

// Function to check if a control is supported by the device
bool isControlSupported(int fd, int control_id) {
    struct v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(queryctrl));
    queryctrl.id = control_id;

    // Query the control info from the device
    if (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == -1) {
        perror("VIDIOC_QUERYCTRL");
        return false; // Control is not supported or an error occurred
    }

    // If we get here, the control is supported
    return true;
}

int main() {
    const char *dev_name = "/dev/video0";  // Device file path
    int fd = open(dev_name, O_RDONLY);
    if (fd == -1) {
        perror("Opening video device");
        return -1;
    }

    // Control IDs for common controls (you can expand this list)
    int control_ids[] = {
        V4L2_CID_BRIGHTNESS,
        V4L2_CID_CONTRAST,
        V4L2_CID_SATURATION,
        V4L2_CID_HUE,
        V4L2_CID_WHITE_BALANCE_TEMPERATURE,
        V4L2_CID_EXPOSURE_AUTO
    };

    // Check if each control is supported by the device
    for (int control_id : control_ids) {
        if (isControlSupported(fd, control_id)) {
            cout << "Control ID " << control_id << " is supported." << endl;
        } else {
            cout << "Control ID " << control_id << " is not supported." << endl;
        }
    }

    close(fd);
    return 0;
}
