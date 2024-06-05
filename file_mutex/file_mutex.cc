/*************************************************************************
    > File Name: file_mutex.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年06月04日 星期二 10时52分48秒
 ************************************************************************/

#include <iostream>
#include <string>

#include <unistd.h>
#include <fcntl.h>

std::string g_fileName;

bool acquire_lock(int file_descriptor)
{
    // Attempt to acquire an exclusive lock on the file
    struct flock fl;
    fl.l_type = F_WRLCK;    // Exclusive lock
    fl.l_whence = SEEK_SET; // Start of file
    fl.l_start = 0;         // Offset from start of file
    fl.l_len = 0;           // Lock the entire file

    if (fcntl(file_descriptor, F_SETLK, &fl) == -1) {
        perror("Failed to acquire lock on file");
        return false;
    }

    std::cout << "Lock acquired on " << g_fileName << std::endl;
    return true;
}

void release_lock(int file_descriptor) {
    // Release the lock
    struct flock fl;
    fl.l_type = F_UNLCK;    // Unlock
    fl.l_whence = SEEK_SET; // Start of file
    fl.l_start = 0;         // Offset from start of file
    fl.l_len = 0;           // Unlock the entire file

    if (fcntl(file_descriptor, F_SETLK, &fl) == -1) {
        perror("Failed to release lock");
    }
    
    std::cout << "Lock released" << std::endl;
}

int main() {
    const char* file_path = "example.lock";
    g_fileName = file_path;

    // Acquire lock
    int file_descriptor = open(file_path, O_RDWR | O_CREAT, 0664);
    if (file_descriptor == -1) {
        perror("Failed to open file");
        return 1;
    }

    std::cout << "wait for 10s " << file_descriptor << std::endl;
    sleep(10);

    if (acquire_lock(file_descriptor)) {
        // Perform some operations on the locked file
        // For demonstration purposes, let's just sleep for a while
        sleep(5);

        // Release lock
        release_lock(file_descriptor);
    }

    close(file_descriptor);
    return 0;
}
