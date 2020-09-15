#include "functions.h"
void _test_DisplayInodeBitmap(int fd);
INT4 InodeUtilValidateIfInodeAvailable(UINT4 u4InodeNo, struct ext3_inode *pInode, int fd);
INT4 _test_InodeUtilPrintInode(int fd, UINT4 u4InodeNo);
void InodeUtilDumpInode(struct ext3_inode *pInode);