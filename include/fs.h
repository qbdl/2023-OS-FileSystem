//FileSystem
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
    int initialize_filetree_from_externalFile(const std::string &path);
public:
    int create_inode(int size);
    int alloc_data_block();
    int create_file(std::string& fdata);
public:
    std::fstream disk;//整个磁盘映射到该变量
    std::string disk_path;


    SuperBlock sblock;
    Inode inodes[INODE_NUM];
    char data_[DATA_SIZE];

};
