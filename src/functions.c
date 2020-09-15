#include "functions.h"
#include "data_types.h"

const int BLOCK_SIZE = 4096;
const int SUPER_BLOCK_OFFSET = 1024;
const int BLOCKS_PER_GROUP = 32768;
const int DATA_BLOCKS_PER_GROUP = 18000; 
const int INODE_SIZE = 128;
const char EOI[8] = "1C53BB6B";
const int BLOCK_GROUP_DESC_SIZE = 32;
const int ROOT_DIR_INODE = 2;

int findFirst(int fd, int totalBlocks)
{	
	char header[4];
	int count = 0;
	
	printf("Searching for mkv files\n");
	//set the header char for MKV's magic number 1A 45 DF A3
	header[0] = 0x1A;
	header[1] = 0x45;
	header[2] = 0xDF;
	header[3] = 0xA3;

	unsigned char b[4];
    int headerLength = 4;
    int blockNumber = 0;
	
	//lseek to the start of the partition
    	lseek(fd, 0, SEEK_SET);
	//read the first 2 bytes of the block
    	read(fd, b, headerLength);
	
	//loop through all the blocks in the partition
	for (int i = 0; i < totalBlocks; i++)
    	{
		//compare the first 2 bytes of the block with the hex signature of MKV
		 if(compareHexValues(b, header, headerLength) == 1)
        	{
                blockNumber = i;
                return blockNumber;

        	}		

		//lseek to the next block from the current position with is 2 bytes into the current block
        	lseek(fd, BLOCK_SIZE - headerLength, SEEK_CUR);
		//read the 2 bytes of the next block
        	read(fd, b, headerLength);
    	}
	return 0;
}

//function to return the indirect blocks
int findIndirectPointerBlock(int fd, int lastDatablock)
{
    int addrLength = 4;
    unsigned char *addr = malloc(sizeof(char) * addrLength); //store 4 adjacent addresses of pointers
    int indBlock = lastDatablock+1;
    addr = decimalToHexStringInReverseOrder(indBlock);
    for (int i=0; i<3; i++)
    {
        addr[4*(i+1)] = addr[4*i]+1;
        addr[4*(i+1)+1] = addr[1];
        addr[4*(i+1)+2] = addr[2];
        addr[4*(i+1)+3] = addr[3];
    }

    unsigned char b[4];
    long long offset = 0;
    lseek64(fd, offset, SEEK_SET);
    read(fd, b, addrLength);
    int n = 0;
    printf("Searching for indirect pointer block\n");
    for (int i = 0; i < DATA_BLOCKS_PER_GROUP; i++)
    {
        if(compareHexValues(b, addr, addrLength) == 1)
        {
            printf("Indirect pointer block found at block %d, its first entry points to block %d\n", i, indBlock);
			return i;
            //need to return first indirect block. 
        }
        lseek64(fd, BLOCK_SIZE - addrLength, SEEK_CUR);
        read(fd, b, addrLength);
    }
	return -1;
}

//function to compare the hex bytes of a file
int compareHexValues(unsigned char string1[], unsigned char string2[], int n) {
	int i;
	for (i = 0; i < n; i++) {
		if ((int) string1[i] == (int) string2[i]) {
			if (i == n - 1) {
				return 1;
            		}
        	} else {
            		return 0;
        	}
    	}
	return 0;
}



/*adjusted version of the original get block group descriptor table
//this version does not write anything out, just gets the GBD table and then returns the first entry in the table
//can be adjust to take in an index and loop through the table until you get that particular indexed GBD. 
*/
struct ext3_group_desc *bgdGetGrpDescTable(int fd, int blockGroupNo) {
    long long offset = 0;
    char *buff = (char *) malloc(sizeof(struct ext3_group_desc));
    struct ext3_group_desc *gdesc = (struct ext3_group_desc *) malloc(sizeof(struct ext3_group_desc));

    /* structure to store all block group descriptors */
    struct ext3_group_desc *gDescTable = (struct ext3_group_desc *) malloc(
            BLOCKS_PER_GROUP * sizeof(struct ext3_group_desc));

    /* block group desc table size is one block group desc size(32) * total number of block groups */
    int blockGroupTblSize = BLOCKS_PER_GROUP * BLOCK_GROUP_DESC_SIZE;
    unsigned char buffer[blockGroupTblSize];
    offset = BLOCK_SIZE;

    /* Go to the first block group descriptor in the second block */
    lseek64(fd, offset, SEEK_SET);


    /* read block group desc table into buffer which will be used for hex dump */
    int retVal = read(fd, buffer, blockGroupTblSize);
    if (retVal <= 0) {
        fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
        return NULL;
    }

    lseek64(fd, offset, SEEK_SET);
    //iterate through all the group descriptors in the group descriptor table
    int bg_iterator = 0;
    while (bg_iterator < BLOCKS_PER_GROUP) {
        /* read each group descriptor and write it to output file. */
        read(fd, buff, sizeof(struct ext3_group_desc));
        memcpy((void *) &gDescTable[bg_iterator], (void *) buff, sizeof(struct ext3_group_desc));
        bg_iterator++;
    }
    free(gdesc);
    free(buff);
    return gDescTable;
}


/*gets a free inode block number from the inode bitmap. Reads the block# of the bitmap from the group descriptor, reads byte by byte into a buffer.
//scans the bits of each byte (an inode) to find a free one. Once a free one is found, it sets the bit to in use and returns inode number.
//The inode number is calculated by the outer counter times 8 (each byte is equivalent to 8 inodes) plus the inner loop counter (this is the number of inodes checked.)
*/
int getFreeInodeNumber(int fd) {
	char buffer[BLOCK_SIZE];

	// Gets group descriptor table containing inode bitmap location.
	struct ext3_group_desc *group_desc = bgdGetGrpDescTable(fd, 0);
    int offset = (*group_desc).bg_inode_bitmap * BLOCK_SIZE;

	// Seeks to inode bitmap table
	lseek(fd, offset, SEEK_SET);
	// Reads inode bitmap block.
	read(fd, buffer, sizeof(buffer));
	// Loops through all the bytes in the inode bitmap block until the end is reach or a valid inode location is found.
	for (int byte_index = 0; byte_index < BLOCK_SIZE; byte_index++) {
		char byte = buffer[byte_index];
		// Loops through all bits in a byte in search of a bit set to 0.
		for (int bit_index = 0; bit_index < CHAR_BIT; bit_index++) {
			if((byte & (1 << bit_index)) == 0) {
				// Updates the inode bitmap
				lseek(fd, offset + byte_index, SEEK_SET);
				int updated_byte = (1 << bit_index) | byte;
				buffer[byte_index] = updated_byte;
				write(fd, buffer, sizeof(buffer));
				// Returns the newly claimed inode number
				return (byte_index * 8) + bit_index + 1;
				
			}
		}
	}
	return -1;
}

void setInode(int fd, int inodeNumber, struct ext3_inode *inode){


    struct ext3_group_desc *group_desc = bgdGetGrpDescTable(fd, 0);
    char *buff = sizeof(struct ext3_inode);
    
    int offset = (*group_desc).bg_inode_table * BLOCK_SIZE + ((inodeNumber - 1) * 32);
    lseek(fd, offset, SEEK_SET);
    buff = (char *) inode;
    write(fd, buff, sizeof(buff - 1));
}




int getFileSize(int fd, int block){
	int offset = (block -1 * BLOCK_SIZE) + 45;
 	int size = 0;
 	char buffer[7];
	long myLongSize = 0;//added by me
 	lseek(fd, offset, SEEK_SET);
 	read(fd, buffer, 7);
    printf("Size in hex %X\n", buffer);
	myLongSize = strtol(buffer, NULL, 16); //hexadecimalToDecimal(buffer);//added by me
    printf("Size %ld\n", myLongSize);
	size = (int)myLongSize;//originally size converted to long, so convert to int and store in size
 	size += 52; //add 52 to the value to compensate for the ebml header size. 
 	return size;
	
}


void setInodeInfo(int fd, int inode, int blocks[], int fSize){
	//static values taken from inode example for the file
    printf("entered inode method\n");
    struct ext3_inode inodeToFill;
    printf("inode struct\n");
	//fSize = fSize;//temporary to test functionality in this function
	//blocks = blocks;//temporary to test functionality in this function

	inodeToFill.i_mode=33206;		/* File mode */
	printf("inode struct1\n");
    inodeToFill.i_uid=1000;		/* Low 16 bits of Owner Uid */
    printf("inode struct2\n");
	inodeToFill.i_size=fSize;		/* Size in bytes */
    printf("inode struct3\n");
	inodeToFill.i_atime=0;	/* Access time */
    printf("inode struct4\n");
	inodeToFill.i_ctime=0;	/* Creation time */
    printf("inode struct5\n");
	inodeToFill.i_mtime=0;	/* Modification time */
    printf("inode struct6\n");
	inodeToFill.i_dtime=0;	/* Deletion Time */
    printf("inode struct7\n");
	inodeToFill.i_gid=1000;		/* Low 16 bits of Group Id */
    printf("inode struct8\n");
	inodeToFill.i_links_count=1;	/* Links count */
    printf("inode struct9\n");
	//inodeToFill->i_blocks=blocks;	/* Blocks count */
	inodeToFill.i_flags=0;
    printf("starting to print\n");
    int i;
	for(i = 0; i < sizeof(blocks); i++){
        inodeToFill.i_block[i]=blocks[i];
    }/* Pointers to blocks */
    printf("Inode filled passing to set\n");
    setInode(fd, inode, &inodeToFill);
	//i_generation=1646222021;	/* File version (for NFS) */
	//i_file_acl=0;	/* File ACL */
	//i_dir_acl=0;	/* Directory ACL */
	//i_faddr=0;	/* Fragment address */
	
	//i_extra_isize=32; 
	//i_pad1=0;


}

void addDirEntry(char fileName[], int inode, int fd, int rootBlockNum){
	UINT4 file_entry_inode = (UINT4) inode;
	struct ext3_dir_entry_2 newDirEntry;
	
	memset(&newDirEntry, 0, sizeof(newDirEntry));
	
	int index = 0;
    int recLen = 8;
    int temp = 0;
    char fn_char_buff = fileName[0];
    while(fn_char_buff != '\0') {
        newDirEntry.name[index] = fn_char_buff;
        fn_char_buff = fileName[index+1];
        index++;
    }

	newDirEntry.inode = file_entry_inode;
	newDirEntry.name_len = strlen(fileName);
    printf("%d", newDirEntry.name_len);
    recLen += newDirEntry.name_len;
    temp = recLen % 4;

    if(temp != 0){
        recLen += temp;
    }
	newDirEntry.rec_len = recLen;
    newDirEntry.file_type = 1;
    

	InodeDirAddChildEntry(&newDirEntry, rootBlockNum, fd);

}

INT4 InodeUtilReadDataBlock(UINT4 u4BlockNo, UINT2 u2StartPos, void *pBuffer, UINT4 u4Size, int fd) {
    UINT8 u8Offset;
    INT4 i4RetVal;

    //??????why u8offset

    u8Offset = ((UINT8) BLOCK_SIZE * u4BlockNo) + u2StartPos;
    i4RetVal = 0;

    /* Read the contents of the data block */
    if (lseek64(fd, u8Offset, SEEK_SET) == SYS_CALL_FAILURE) {
        return INODE_FAILURE;
    }

    /* Read the block */
    i4RetVal = read(fd, pBuffer, u4Size);
    //TODO: can set the read size to avoid recovery mixed
    if (i4RetVal == SYS_CALL_FAILURE) {
        return INODE_FAILURE;
    }

    return INODE_SUCCESS;
}

INT4 InodeUtilWriteDataBlock(UINT4 u4BlockNo, UINT2 u2StartPos, void *pBuffer, UINT4 u4Size, int fd) {
    UINT8 u8Offset;
    INT4 i4RetVal;

    /* NOTE: validation not performed on u2StartPos and u4Size. If they are
     * larger than the block size, the write would span across multiple blocks
     * without any problem */
    u8Offset = ((UINT8) BLOCK_SIZE * u4BlockNo) + u2StartPos;
    i4RetVal = 0;

    /* seek to the offset */
    if (lseek64(fd, u8Offset, SEEK_SET) == SYS_CALL_FAILURE) {
        return INODE_FAILURE;
    }

    /* write to the block */
    i4RetVal = write(fd, pBuffer, u4Size);
    if (i4RetVal == SYS_CALL_FAILURE) {

        return INODE_FAILURE;
    }

    return INODE_SUCCESS;
}

INT4 InodeUtilReadInode(UINT4 u4InodeNo, struct ext3_inode *pNewInode, int fd) {
    struct ext3_inode Inode;
    struct ext3_group_desc GroupDes;
    UINT8 u8Offset;
    INT4 i4RetVal;

    memset(&Inode, 0, sizeof(Inode));
    memset(&GroupDes, 0, sizeof(GroupDes));
    i4RetVal = 0;

    /* Obtain the Inode offset in bytes */
    i4RetVal = InodeUtilGetInodeOffset(u4InodeNo, &u8Offset, fd);
    if (i4RetVal == INODE_FAILURE) {
        return INODE_FAILURE;
    }

    /* Read the inode */
    i4RetVal = InodeUtilReadDataOffset(u8Offset, &Inode, sizeof(struct ext3_inode), fd);
    if (i4RetVal == INODE_FAILURE) {
        return INODE_FAILURE;
    }

    /* Copy the inode to the output pointer */
    memcpy(pNewInode, &Inode, sizeof(Inode));

    return INODE_SUCCESS;
}

INT4 InodeUtilGetInodeOffset(UINT4 u4InodeNo, UINT8 *pu8Offset, int fd) {
    struct ext3_inode Inode;
    struct ext3_group_desc GroupDes;
    UINT4 u4GroupNo;
    UINT4 u4LocalInodeIndex;
    INT4 i4RetVal;
    UINT8 u8GbdOffset;

    memset(&Inode, 0, sizeof(Inode));
    memset(&GroupDes, 0, sizeof(GroupDes));
    i4RetVal = 0;
    u8GbdOffset = 0;

    /* Find the block group no. corresponding to this inode */
    u4GroupNo = (u4InodeNo - 1) / INODES_PER_GROUP;

    /* Find the local inode index */
    u4LocalInodeIndex = (u4InodeNo - 1) % INODES_PER_GROUP;

    /* 
       1. Seek and Read the block group descriptor for u4GroupNo
       2. Identify the inode table location from the group descriptor
    */
    u8GbdOffset = BLOCK_SIZE + u4GroupNo * sizeof(struct ext3_group_desc);
    i4RetVal = InodeUtilReadDataOffset(u8GbdOffset, &GroupDes, sizeof(struct ext3_group_desc), fd);
    if (i4RetVal == INODE_FAILURE) {
        return INODE_FAILURE;
    }

    /* Inode table block no. is identified for this group no.
       Seek to this block and get offset */
    *pu8Offset = ((UINT8) GroupDes.bg_inode_table * BLOCK_SIZE) + (u4LocalInodeIndex *
                                                                     INODE_SIZE);

    /* printf("Inode no %d has offset %u\n", u4InodeNo, u8Offset); */
    return INODE_SUCCESS;
}

INT4 InodeUtilReadDataOffset(UINT8 u8Offset, void *pBuffer, UINT4 u4Size, int fd) {
    UINT4 u4BlockNo;
    UINT2 u2Start;
    INT4 i4RetVal;

    GET_BLOCK_OFFSET_FROM_BYTE_OFFSET(u4BlockNo, u2Start, u8Offset);
    /* write to the block */
    i4RetVal = InodeUtilReadDataBlock(u4BlockNo, u2Start, pBuffer, u4Size, fd);
    if (i4RetVal == INODE_FAILURE) {
        return INODE_FAILURE;
    }

    return INODE_SUCCESS;
}

INT4 InodeDirAddChildEntry(struct ext3_dir_entry_2 *pNewChild, UINT4 u4BlockNo, int fd)
{
    struct ext3_dir_entry_2 DirEntry;
    char *pBuffer;
    char buffer[BLOCK_SIZE]; 
    INT4 i4BytesReadSoFar = 0;
    UINT2 u2Len= 0;
    INT4 i4RetVal = 0;
    UINT4 u4Index = 0;
    UINT4 u4NextPos = 0;

    memset(&DirEntry, 0, sizeof(DirEntry));
    memset(buffer, 0, sizeof(buffer));
    pBuffer = buffer;
    
    /*read datablock into buffer */
    i4RetVal = InodeUtilReadDataBlock(u4BlockNo, 0, pBuffer, BLOCK_SIZE, fd);
    if(i4RetVal == INODE_FAILURE)
    {
        return INODE_FAILURE;
    }

    /* iterate through all directory entries */
    for (u4Index = 0; u4Index < BLOCK_SIZE; u4Index++)
    {
        memset(&DirEntry, 0, sizeof(DirEntry));

        /* Read the next directory record entry in the data block */
        InodeDirReadRecord(pBuffer, u4NextPos, &DirEntry);

        /* Move the offset to the start of next record */
        u4NextPos += DirEntry.rec_len;

        /* If end of records, break the loop */
        if (u4NextPos == BLOCK_SIZE)
        {
            break;
        }
        else
        {
            i4BytesReadSoFar = i4BytesReadSoFar + DirEntry.rec_len;
        }
    }

    /* setting the actual record length of the last dir entry */
    DirEntry.rec_len = (*pNewChild).rec_len;
    memcpy(pBuffer + i4BytesReadSoFar, &DirEntry, DirEntry.rec_len);
    u2Len = DirEntry.rec_len;

    /* adding the new directory entry to the buffer*/
    pNewChild->rec_len = BLOCK_SIZE - i4BytesReadSoFar - u2Len; 
    memcpy(pBuffer + i4BytesReadSoFar + u2Len, pNewChild, pNewChild->rec_len);

    /* rewrite the block with new data */
    i4RetVal = InodeUtilWriteDataBlock(u4BlockNo, 0, pBuffer, BLOCK_SIZE, fd);
    if (i4RetVal == SYS_CALL_FAILURE)
    {
        return INODE_FAILURE;
    }
    return INODE_SUCCESS;
}

//function to get the total # of blocks in the partition
int bgdGetTotalNumberOfBlocks(int fd) {

	//set the offset for the lseek to the location of the total number of blocks
	int offset = SUPER_BLOCK_OFFSET + sizeof(int);
	
	int totalNumOfBlocks;
	unsigned char buffer[4];
	
	//lseek to the postion of the offset
	lseek(fd, offset, SEEK_CUR);

	// read the total number of blocks from the superblock and store.
	read(fd, buffer, sizeof(int));
	memcpy(&totalNumOfBlocks, buffer, sizeof(int));

	return totalNumOfBlocks;
}

INT4 InodeDirReadRecord(char *pEntries, UINT4 u4StartPos, 
        struct ext3_dir_entry_2 *pDirEntry)
{
    char *pPos = NULL;

    if (pEntries == NULL)
    {
        return INODE_FAILURE;
    }

    pPos = pEntries + u4StartPos;
    memcpy(pDirEntry, pPos, sizeof(UINT8));
    strncpy(pDirEntry->name, pPos + DIR_ENTRY_NAME_OFFSET,
            pDirEntry->name_len);
    return INODE_SUCCESS;
}


char *decimalToHexStringInReverseOrder(int decimalNumber) {
    char *signs = (char *) malloc(sizeof(char) * 4);
    signs[0] = decimalNumber & 0xff;
    signs[1] = (decimalNumber >> 8) & 0xff;
    signs[2] = (decimalNumber >> 16) & 0xff;
    signs[3] = (decimalNumber >> 24) & 0xff;

    return signs;
}

int hexToInt(char buffer[], int bytes){
	int value;
	long convertedBuffer;
	convertedBuffer = strtol(buffer, NULL, 16); //hexadecimalToDecimal(buffer);//added by me
	value = (int)convertedBuffer;//originally size converted to long, so convert to int and store in size
	return value;

}


// Test functions