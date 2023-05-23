//初始化磁盘
#include"../include/fs.h"

#include "../include/superBlock.h"        // superblock相关
#include "../include/diskInode.h"    // inode相关
#include "../include/initDir.h"  // 初始化目录
#include "../include/parameter.h" // 所有全局const int变量
#include <string>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
using namespace std;

//Global Variables
SuperBlock sblock;//SuperBlock区
DiskInode dInode[INODE_NUM];//Inode区
char data_[DATA_SIZE];//数据区

const int PAGE_SIZE = 4 * 1024; // mmap 限定4KB的倍数，这里取4 KB

FileSystem fs;

void init_superblock()
{
    sblock.s_isize=INODE_NUM*INODE_SIZE/BLOCK_SIZE;//Inode区占用的盘块数  12.5~=12
    sblock.s_fsize=FILE_SIZE/BLOCK_SIZE;//盘块总数——Block总数

    //初始化 superblock管理的 Inode及栈//TODO:注释重新修改（把内容放进readme里）
    sblock.s_ninode=INODE_NUM;
    for(int i=0; i<INODE_NUM; i++){
        sblock.s_inode[i]=i;//固定后不变的，仅是移动指针（栈顶）
    }

    //初始化data表
    int data_i=0;
    //初始化 superblock的空闲表
    sblock.s_nfree=100;//Unix V6++设计只有100个数据块（剩余的数据块不是用统一的数据结构管理）
    for(int i=0;i<100&data_i<DATA_NUM;i++){
        sblock.s_free[i]=data_i++;//固定后不变的，仅是移动指针（栈顶）
    }

    //数据区的数据块都是以100个组织的

    //填写后续的空闲表——从后往前逆序分配剩余数据区（除了sblock管理的100个数据块以外的）
    //剩余数据块的组织方式 0：长度 1-101 具体内容
    //1-100 具体内容 101 指向下一块表
    int blkno=sblock.s_free[0];//superblock里管理的最后一块表（每块表索引从100到0），sblock.s_free[0]指向没有superblock管理的第一个数据区（第一个刚好是长度）
    while(data_i<DATA_NUM)
    {
        char *p=data_+blkno*BLOCK_SIZE;//定位到
        int *table=(int *)p;//table就为新的表（101块）:table[0]存表里有效块个数
        if (DATA_NUM-data_i>100)
            table[0]=100;
        else    
            table[0]=DATA_NUM-data_i;
        
        for(int i=1;i<=table[0];i++){
            table[i]=data_i++;
        }
        blkno=table[1];//重新指向下一个表
    }
}

void copy_data(char *addr, int size)
{
    static int data_p = 0;
    if (size >= DATA_SIZE - data_p)//超出数据区的容量
       size = DATA_SIZE - data_p;
    memcpy(addr, data_ + data_p, sizeof(char) * size);
    data_p += size;
}

void copy_first(char *addr)
{
    memcpy(addr, &sblock, sizeof(sblock));//superblock区空间分配
    memcpy(addr + OFFSET_INODE, &dInode[0], sizeof(dInode));//Inode区
    copy_data(addr + OFFSET_DATA, 2 * PAGE_SIZE - OFFSET_DATA);//Data数据区一部分（2页剩余的部分)
}

void copy_subseq(char *addr)
{
    copy_data(addr, PAGE_SIZE);//Data数据区剩下部分
}

//内存与文件系统建立映射
int map_img(int fd, int offset, int size, void (*fun)(char *))
{
    // 映射文件到内存中
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    if (addr == MAP_FAILED)
    {
        std::cerr << "Failed to mmap superblock " << std::endl;
        close(fd);
        return -1;
    }

    fun((char *)addr);

    // 解除内存映射
    if (munmap(addr, size) < 0)
    {
        std::cerr << "Failed to munmap superblock " << std::endl;
        close(fd);
        return -1;
    }
    return size;
}


int main(int argc, char *argv[])
{
    const char *file_path = "myDisk.img";
    int fd;
    
    if (argc > 1)
        fd = open(argv[1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);//读写权限
    else
        fd = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd < 0)
    {
        cerr << "Failed to open file " << file_path << endl;
        return 1;
    }
    else
        cout << "start map " << file_path << endl;

    // 调整文件大小
    if (ftruncate(fd, FILE_SIZE) < 0)//ftruncate函数可以改变一个已打开文件的大小，将文件截断到指定大小。
    {
        cerr << "Failed to truncate file " << file_path << endl;
        close(fd);
        return 1;
    }

    // 初始化superblock
    init_superblock();

    // //开始扫描文件 并创建对应普通文件与目录文件
    fs.initialize_filetree_from_externalFile("my_test");
    // scan_path("my_test");

    int size = 0;
    //初次拷贝，superblock, inode ,部分data
    size += map_img(fd, 0, 2 * PAGE_SIZE, copy_first);

    //从剩余data部分开始
    int data_off = 2 * PAGE_SIZE - OFFSET_DATA;
    for (; data_off + PAGE_SIZE < DATA_SIZE; data_off += PAGE_SIZE)
        size += map_img(fd, size, PAGE_SIZE, copy_subseq);
    if (DATA_SIZE - data_off > 0)
        size += map_img(fd, size, DATA_SIZE - data_off, copy_subseq);

    // 关闭文件
    close(fd);
    cout << "map disk done!"<<"\n";
    return 0;
}
