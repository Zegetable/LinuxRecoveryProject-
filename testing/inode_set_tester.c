#include "testing_functions.h"
int main (int argc, char* argv[]) {
    int fd = open(argv[1], O_RDWR);
    _test_DisplayInodeBitmap(fd);
    int inode = getFreeInodeNumber(fd);
    printf("\nXXXXX\n%d\n", inode);
    _test_DisplayInodeBitmap(fd);
    int blocks[] = {0, 1, 2, 3, 4, 5, 6, 7};
    setInodeInfo(fd, inode, blocks, 32768);
    _test_InodeUtilPrintInode(fd, inode);
    close(fd);
    return 0;
}