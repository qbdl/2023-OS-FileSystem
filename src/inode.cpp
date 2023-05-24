#include"../include/Inode.h"


//inode级创建文件
int Inode::create_file(const string& filename, bool is_dir) 
{
    //创建目录项
        //获取目录项get_entry()

        //检查文件是否已经存在

        //检查当前的目录项是否已满，如果已满，需要分配一个新的数据块用来存放目录项。

    //创建该文件的inode
        // 分配一个新的inode用来存放新创建的文件或者目录

        //修改父文件与该文件的目录内容

}

//获取当前文件(目录文件)的所有目录项
vector<DirectoryEntry> Inode::get_entry()
{

}

//在目录文件中加.与..的目录项
int Inode::init_as_dir(int ino, int fa_ino)
{

}

//文件内offset~offset+size=>buf
int Inode::read_at(int offset, char *buf, int size)
{

}

//调用block分配
int Inode::push_back_block()
{

}

//文件内blockno=>磁盘blockno
int Inode::get_block_id(int inner_id)
{

}
