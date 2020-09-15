#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <linux/types.h>
#include <limits.h> //this is for CHAR_BIT to read the inode bitmap to find a free inode.
#include <math.h>
#include "data_types.h"

#define GET_BLOCK_OFFSET_FROM_BYTE_OFFSET(Block, Position, ByteOffset) \
                            Block = (UINT4)(ByteOffset / BLOCK_SIZE); \
                            Position = (UINT2)(ByteOffset % BLOCK_SIZE);

#define DIR_ENTRY_NAME_OFFSET sizeof(UINT8)
const int BLOCK_SIZE;
const int SUPER_BLOCK_OFFSET;
const int BLOCKS_PER_GROUP;
const int DATA_BLOCKS_PER_GROUP; 
const int INODE_SIZE;
const char EOI[8];
const int BLOCK_GROUP_DESC_SIZE;
const int ROOT_DIR_INODE;

int findFirst(int fd, int totalBlocks);

//function to return the indirect blocks
int findIndirectPointerBlock(int fd, int firstDatablock);
//function to compare the hex bytes of a file
int compareHexValues(unsigned char string1[], unsigned char string2[], int n);

/*adjusted version of the original get block group descriptor table
//this version does not write anything out, just gets the GBD table and then returns the first entry in the table
//can be adjust to take in an index and loop through the table until you get that particular indexed GBD. 
*/
struct ext3_group_desc *bgdGetGrpDescTable(int fd, int blockGroupNo);

/*gets a free inode block number from the inode bitmap. Reads the block# of the bitmap from the group descriptor, reads byte by byte into a buffer.
//scans the bits of each bit (an inode) to find a free one. Once a free one is found, it sets the bit to in use and returns the block number of the inode.
//The inode number is calculated by the outer counter times 8 (each byte is equivalent to 8 inodes) plus the inner loop counter (this is the number of inodes checked.)
*/
int getFreeInodeNumber(int fd);


int getFileSize(int fd, int block);
void setInode(int, int, struct ext3_inode *);

void setInodeInfo(int fd, int inode, int blocks[], int fSize);

void addDirEntry(char fileName[], int inode, int fd, int rootBlockNum);
INT4 InodeUtilReadDataBlock(UINT4 u4BlockNo, UINT2 u2StartPos, void *pBuffer, UINT4 u4Size, int fd);
INT4 InodeUtilWriteDataBlock(UINT4 u4BlockNo, UINT2 u2StartPos, void *pBuffer, UINT4 u4Size, int fd);
INT4 InodeUtilReadInode(UINT4 u4InodeNo, struct ext3_inode *pNewInode, int fd);
INT4 InodeUtilGetInodeOffset(UINT4 u4InodeNo, UINT8 *pu8Offset, int fd);
INT4 InodeUtilReadDataOffset(UINT8 u8Offset, void *pBuffer, UINT4 u4Size, int fd);
INT4 InodeDirAddChildEntry(struct ext3_dir_entry_2 *pNewChild, UINT4 u4BlockNo, int fd);
//function to get the total # of blocks in the partition
int bgdGetTotalNumberOfBlocks(int fd);

char *decimalToHexStringInReverseOrder(int decimalNumber);
INT4 InodeDirReadRecord(char *pEntries, UINT4 u4StartPos, 
        struct ext3_dir_entry_2 *pDirEntry);
int hexToInt(char buffer[], int);


#endif // FUNCTIONS_H
