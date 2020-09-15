#include "recovery.h"


int main (int argc, char *argv[]){
    
    //must have 4 arguments the program, device name, chosen filename for the deleted file, and root directory block number. 
    if(argc < 4) {
        printf("Usage: ./recover /dev/sd# 'filename'\n");
        exit(1);
    }

    char* disk = argv[1];
    char* fileName= argv[2];
    int rootBlockNum = strtol(argv[3], (char **) NULL, 10);
    int fd = open(disk, O_RDWR); 
    int totalNumberOfBlocks, inode, fSize/*, file*/;
    int blocks[15]; //array for the first 12 blocks that should be in order.
    long long IND = 0, DIND = 0, TIND = 0, secondIndirectBlockOffset = 0;
    long long lastBlockAddressInIND = 0, lastBlockAddressInDIND = 0;
    unsigned char dataBlock[4];
    unsigned char b[BLOCK_SIZE];
    char *pch;


    if(fd < 0) {
        perror("Error ");
        exit(1);

    }

    totalNumberOfBlocks = bgdGetTotalNumberOfBlocks(fd);

    //get the first 12 blocks
    blocks[0] = findFirst(fd, totalNumberOfBlocks);
    fSize = getFileSize(fd, blocks[0]);
	int i = 1;
    while(i < 12) {
        blocks[i] = blocks[i - 1] + 1;
        i++;
    }

    //get the indirect blocks
    IND = findIndirectPointerBlock(fd, blocks[11]);
    if (IND > 0) {
        blocks[12] = IND;
        lseek64(fd, (IND * 4096), SEEK_SET);
        read(fd, b, BLOCK_SIZE);
        strncpy(dataBlock, &b[BLOCK_SIZE - 4], 4);
        if (compareHexValues(dataBlock, "\x0\x0\x0\x0", 4) == 1) {
            printf("Reached the end of the file 1\n");
            
            //break;//not within a loop
        }
        lastBlockAddressInIND = hexToInt(dataBlock, 4);
        lseek64(fd, (lastBlockAddressInIND * 4096), SEEK_SET);
        read(fd, b, BLOCK_SIZE);
        pch = strstr(b, EOI);
        if (pch) {
            printf("End of file found 2");
           
        }
        //TODO: Check whether that data block is part of the actual file
        DIND = findIndirectPointerBlock(fd,findIndirectPointerBlock(fd, lastBlockAddressInIND));

        if (DIND > 0) {
            /*file->*/secondIndirectBlockOffset = DIND;
            lseek64(fd, (1024 + DIND * 4096), SEEK_SET);
            read(fd, b, BLOCK_SIZE);
            strncpy(dataBlock, &b[BLOCK_SIZE - 4], 4);
            if (compareHexValues(dataBlock, "\x0\x0\x0\x0", 4) == 1) {
                printf("Reached the end of the file 3\n");
                
                //break;
            }
            lastBlockAddressInDIND = /*hex2intInReverseOrder*/hexToInt(dataBlock, 4);
            lseek64(fd, (1024 + lastBlockAddressInDIND * 4096), SEEK_SET);
            read(fd, b, BLOCK_SIZE);
            strncpy(dataBlock, &b[BLOCK_SIZE - 4], 4);
            if (compareHexValues(dataBlock, "\x0\x0\x0\x0", 4) == 1) {
                printf("Reached the end of the file 4\n");
               
                //break;
            }
            lastBlockAddressInIND = hexToInt(dataBlock, 4);
            lseek64(fd, (1024 + lastBlockAddressInIND * 4096), SEEK_SET);
            read(fd, b, BLOCK_SIZE);

            pch = strstr(b, EOI);
            if (pch) {
                printf("End of file found");
               
                //break:
            }

            //TODO: Check whether that data block is part of the actual file
            TIND = findIndirectPointerBlock(fd, lastBlockAddressInDIND);
            if (TIND > 0) {
                blocks[14] = TIND;
                
                }

        }
    }

    i = 0; 
    while (i < 14){
        printf("block: %d\n", blocks[i]);
        i++;
    }

    //locate the inode bitmap and read 
    inode = getFreeInodeNumber(fd);
    printf("Free inode found: %d\n", inode);

    //get the file size from first block
 
    printf("File size is: %d\n", fSize);

    //write the inode information. Takes in the inode received from getInodeNumber, the array of blocks, and filesize
    setInodeInfo(fd, inode, blocks, fSize);
    printf("Inode set\n");
	//commented out until function is complete and tested in test.c

    addDirEntry(fileName, inode, fd, rootBlockNum);
    printf("Completed recovery\n");

    return 0;
}
