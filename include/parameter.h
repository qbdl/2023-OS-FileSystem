#ifndef PARAMETER
#define PARAMETER

//通用参数设置
const long FILE_SIZE = 32*1024*2*512;//整个文件系统的大小
const long BLOCK_SIZE = 512;//每块(block)的大小为512字节
// const long BLOCK_NUM=FILE_SIZE/BLOCK_SIZE;//块的总个数
const long INODE_NUM = 100;//Inode个数
const long INODE_SIZE = 64;//每个Inode的大小为64字节
const long OFFSET_SUPERBLOCK = 0;//SuperBlock区的起始位置
const long OFFSET_INODE = 2*BLOCK_SIZE;//Inode区的起始位置（前两个Block是SuperBlock区的）
const long OFFSET_DATA = OFFSET_INODE+INODE_NUM*INODE_SIZE;//数据区的起始位置
const long DATA_SIZE = FILE_SIZE-OFFSET_DATA;//数据区长度
const long DATA_NUM = DATA_SIZE/BLOCK_SIZE;//数据区 文件的块个数

#define FAIL -1

#endif