#include "functions.h"

int main(int argc, char* argv[]) {
    int fd = open(argv[1], O_RDWR);
    _test_DisplayInodeBitmap(fd);
    printf("\nXXXXX\n%d\n", getFreeInodeNumber(fd));
    _test_DisplayInodeBitmap(fd);
    close(fd);
    return 0;
}