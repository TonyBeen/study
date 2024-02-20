/*************************************************************************
    > File Name: poco_watch_file.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年02月20日 星期二 10时58分25秒
 ************************************************************************/

#include <iostream>
#include "Poco/DirectoryWatcher.h"
#include "Poco/Delegate.h"

using Poco::DirectoryWatcher;

class MyDirectoryWatcher {
public:
    void onDirectoryChanged(const DirectoryWatcher::DirectoryEvent& event) {
        if (event.event == Poco::DirectoryWatcher::DirectoryEventType::DW_ITEM_ADDED) {
            std::cout << "File created: " << event.item.path() << std::endl;
        } else if (event.event == Poco::DirectoryWatcher::DirectoryEventType::DW_ITEM_REMOVED) {
            std::cout << "File deleted: " << event.item.path() << std::endl;
        } else if (event.event == Poco::DirectoryWatcher::DirectoryEventType::DW_ITEM_MODIFIED) {
            std::cout << "File modified: " << event.item.path() << std::endl;
        }
    }
};

int main()
{
    MyDirectoryWatcher watcher;
    DirectoryWatcher dw(std::string("dir"));

    dw.itemAdded += Poco::delegate(&watcher, &MyDirectoryWatcher::onDirectoryChanged);
    dw.itemRemoved += Poco::delegate(&watcher, &MyDirectoryWatcher::onDirectoryChanged);
    dw.itemModified += Poco::delegate(&watcher, &MyDirectoryWatcher::onDirectoryChanged);

    std::cout << "Monitoring directory changes. Press Enter to quit." << std::endl;
    std::cin.get();

    return 0;
}