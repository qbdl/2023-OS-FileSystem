//FileSystem
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include"../include/user.h"
#include"../include/superBlock.h"
#include"../include/Inode.h"
#include"../include/parameter.h"
#include"../include/BlockCache.h"
#include<string>
#include<fstream>

class FileSystem
{
public:
    friend class Inode;

    FileSystem(std::string disk_path);
    ~FileSystem();

public:
    /* 初始化 接口 */
    bool initialize_filetree_from_externalFile(const std::string &path, const int root_no);
    
    /* 外部 接口*/
    void set_u(User *u) {user = u;};
    void set_current_dir(int inum) { user->current_dir_ino = inum; }//set 当前目录inode号
    void set_current_dir_name(const std::string& dir_name);//set 当前目录名

public:
    /* 分配 接口*/
    int alloc_inode();//分配一个空闲inode，并初步初始化
    int alloc_block();//分配一个空闲block
    
    /* 内部 */
    bool read_block(int blkno, char* buf);//磁盘blkno块=>buf
    bool write_block(int blkno, char* buf);//buf=>磁盘blkno块

    bool saveFile(const std::string& src, const std::string& filename);//外部文件写入磁盘中
    int find_from_path(const std::string& path);//文件树 path =>inode号

public:
    /* 外部命令 */
    bool ls(const std::string& path);//列出目录
    bool cat(const std::string& path);//输出指定文件的内容
    bool changeDir(const std::string& path);//切换目录
    
    


private:
    User *user;//每个fs对应一个user

    std::fstream disk;//整个磁盘映射到该变量
    std::string disk_path;

    SuperBlock sblock;
    Inode inodes[INODE_NUM];
    char data_[DATA_SIZE];

    BlockCacheMgr block_cache_mgr_;

};

#endif