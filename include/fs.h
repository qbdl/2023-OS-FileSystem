//FileSystem
#ifndef FILESYSTEM
#define FILESYSTEM

#include"../include/user.h"
#include"../include/superBlock.h"
#include"../include/Inode.h"
#include"../include/parameter.h"
#include<string>
#include<fstream>

class FileSystem
{
public:
    FileSystem(std::string disk_path);
    ~FileSystem();

public:
    /* 初始化 接口 */
    bool initialize_filetree_from_externalFile(const std::string &path, const int root_no);
public:
    /* 分配 接口*/
    int alloc_inode();//分配一个空闲inode，并初步初始化
    int alloc_block();//分配一个空闲block
    
    /* 内部 */
    bool read_block(int blkno, char* buf);//获取一个物理块的所有内容，返回指向这片缓存的buffer(char *)类型
    bool write_block(int blkno, char* buf);//写入一个物理块(全覆盖)

    bool saveFile(const std::string& src, const std::string& filename);//外部文件写入磁盘中

private:
    User *user;//每个fs对应一个user

    std::fstream disk;//整个磁盘映射到该变量
    std::string disk_path;

    SuperBlock sblock;
    Inode inodes[INODE_NUM];
    char data_[DATA_SIZE];

};

#endif