#include "testing_functions.h"
INT4 _test_InodeUtilPrintInode(int fd, UINT4 u4InodeNo) {
    struct ext3_inode Inode;
    INT4 i4RetVal;

    memset(&Inode, 0, sizeof(Inode));
    i4RetVal = 0;

    if (InodeUtilValidateIfInodeAvailable(u4InodeNo, &Inode, fd) == INODE_FAILURE) {
        return INODE_FAILURE;
    };

    InodeUtilDumpInode(&Inode);
    return INODE_SUCCESS;
}

INT4 InodeUtilValidateIfInodeAvailable(UINT4 u4InodeNo, struct ext3_inode *pInode, int fd) {
    if (InodeUtilReadInode(u4InodeNo, pInode, fd) == INODE_FAILURE) {
        return INODE_FAILURE;
    }
    return INODE_SUCCESS;
}

void InodeUtilDumpInode(struct ext3_inode *pInode) {
    UINT4 u4Index = 0;
    printf(" i_mode = %hu\n", pInode->i_mode);
    printf(" i_uid = %hu\n", pInode->i_uid);
    printf(" i_size = %u\n", pInode->i_size);
    printf(" i_atime = %u\n", pInode->i_atime);
    printf(" i_ctime = %u\n", pInode->i_ctime);
    printf(" i_mtime = %u\n", pInode->i_mtime);
    printf(" i_dtime = %u\n", pInode->i_dtime);
    printf(" i_gid = %hu\n", pInode->i_gid);
    printf(" i_links_count = %hu\n", pInode->i_links_count);
    printf(" i_blocks = %u\n", pInode->i_blocks);

    for (u4Index = 0; u4Index < EXT3_N_BLOCKS; u4Index++) {
        printf("    i_block[%d] = %u\n", u4Index, pInode->i_block[u4Index]);
    }
    printf(" i_flags = %u\n", pInode->i_flags);
    printf(" i_generation = %u\n", pInode->i_generation);
    printf(" i_file_acl = %u\n", pInode->i_file_acl);
    printf(" i_dir_acl = %u\n", pInode->i_dir_acl);
    printf(" i_faddr = %u\n", pInode->i_faddr);
    printf(" i_extra_isize = %hu\n", pInode->i_extra_isize);
    printf(" i_pad1 = %hu\n", pInode->i_pad1);
}

void _test_DisplayInodeBitmap(int fd) {
	struct ext3_group_desc *group_desc = bgdGetGrpDescTable(fd, 0);
    int offset = (*group_desc).bg_inode_bitmap * BLOCK_SIZE;
	
	printf("bg_block_bitmap %d\n", (*group_desc).bg_block_bitmap);
	printf("bg_inode_bitmap %d\n", (*group_desc).bg_inode_bitmap);
	printf("offset %d\n", offset);
	lseek(fd, offset, SEEK_SET);
	char buffer[4096];
	read(fd, buffer, sizeof(buffer));
	int counter = 1;
	for (int i = 0; i < 4096; i++) {
		int byte = buffer[i];
		for (int j = 0; j < 8; j++) {
			printf("inode no. %d:", counter++);
			int value = byte & (1 << j);
			int out = 1;
			if(value == 0) {
				out = 0;
			}
			printf("%d\n ", out);
		}
	}
}