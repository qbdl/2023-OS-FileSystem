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
    /* 分配与释放 接口*/
    int alloc_inode();//分配一个空闲inode，并初步初始化
    int alloc_block();//分配一个空闲block
    int dealloc_inode(int ino);//放回一个inode
    int dealloc_block(int blkno);//放回一个物理块
    
    /* 内部 */
    bool read_block(int blkno, char* buf);//磁盘blkno块=>buf
    bool write_block(int blkno, char* buf);//buf=>磁盘blkno块
    int find_from_path(const std::string& path);//文件树 path =>inode号
    int find_fa_ino(const std::string& path);//上级文件夹ino

public:
    /* 外部命令 */
    bool ls(const std::string& path);//ls 列出目录
    bool cat(const std::string& path);//cat 输出指定文件的内容
    bool changeDir(const std::string& path);//cd 切换目录
    bool saveFile(const std::string& outsideFile, const std::string& dst);//save 外部文件写入磁盘中
    bool createDir(const std::string& path);//mkdir 创建目录
    bool deleteFile(const std::string& path);//rm 删除文件
    bool copyFile(const std::string& src, const std::string& dst);//cp 复制文件
    bool exportFile(const std::string& src, const std::string& outsideFile);//export 磁盘中的文件导出到本地
    bool renameFile(const std::string& filename, const std::string& newname);//rename 修改文件名
    

    std::fstream disk;//整个磁盘映射到该变量

private:
    User *user;//每个fs对应一个user

    std::string disk_path;

    SuperBlock sblock;
    Inode inodes[INODE_NUM];
    char data_[DATA_SIZE];

    BlockCacheMgr block_cache_mgr_;

};

#endif