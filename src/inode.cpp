#include"../include/Inode.h"
#include"../include/parameter.h"
#include"../include/directory.h"
#include"../include/fs.h"
#include<iostream>

const int DIRECT_COUNT=6;
const int FIRST_LEVEL_COUNT=128;//一个块512字节，一个指针4字节，那么一个块可以包含128个指针
const int SECOND_LEVEL_COUNT=128*128;

//inode级 创建文件,返回inode号
int Inode::create_file(const string& fileName, bool is_dir) 
{
    //创建目录项
        //获取目录项get_entry()

        //检查文件是否已经存在

        //检查当前的目录项是否已满，如果已满，需要分配一个新的数据块用来存放目录项。
            //增加父文件的目录项

    //创建该文件的inode
        // 分配一个新的inode用来存放新创建的文件或者目录

        //（创建该文件的目录内容)
    
    auto entrys=get_entry();
    int entrynum=entrys.size();
    for(auto entry:entrys){
        if(entry.m_ino && strcmp(entry.m_name,fileName.c_str())==0){
            std::cerr << "createFile: File already exists." << "\n";
            return -1;
        }
    }

    if (entrynum % ENTRYS_PER_BLOCK == 0)
        push_back_block();

    int blknum=entrynum/ENTRYS_PER_BLOCK;
    char new_entry_block[BLOCK_SIZE];
    fs.read_block(get_block_id(blknum),new_entry_block);
    Directory new_entry(ino,fileName.c_str(),is_dir? DirectoryEntry::FileType::Directory : DirectoryEntry::FileType::RegularFile);
    new_entry_block[entrynum%ENTRYS_PER_BLOCK]=new_entry;
    d_size+=ENTRY_SIZE;
    


    int ino=fs.alloc_inode();
    if (ino == 0) {
        std::cerr << "createFile: No free inode" << "\n";
        return -1;
    }

    if(is_dir)
        fs.inodes[ino].init_as_dir(ino,i_ino);

    return ino;
}

//获取当前文件(目录文件)的所有目录项
vector<DirectoryEntry> Inode::get_entry()
{
    vector<DirectoryEntry>entrys;
    entrys.resize(d_size/ENTRY_SIZE);

    if(entry.size()==0) return entrys;
    if(read_at(0, (char *)entrys.data(), d_size)==FAIL) {
        cerr << "getEntry: read directory entries failed." << "\n";
        return entrys;
    }

    return entrys;
}

//在目录文件中加.与..的目录项
int Inode::init_as_dir(int ino, int fa_ino)
{
    /* 分配 block 来存 目录项*/
    int sub_dir_blk = push_back_block();
    if (sub_dir_blk == FAIL) {
        std::cerr << "createFile: No free block" << std::endl;
        return -1;
    }

    /* 读取block 加.与..的目录项*/
    char *inner_buf;
    fs.read_block(sub_dir_blk,inner_buf);
    memset(inner_buf,0,BLOCK_SIZE);
    DirectoryEntry *sub_entrys=(DirectoryEntry *)inner_buf;

    DirectoryEntry dot_entry(ino, 
                        ".", 
                        DirectoryEntry::FileType::Directory);
    DirectoryEntry dotdot_entry(fa_ino, 
                                "..", 
                                DirectoryEntry::FileType::Directory);
    sub_entrys[0] = dot_entry;
    sub_entrys[1] = dotdot_entry;
    d_size += ENTRY_SIZE*2;

    fs.write_block(sub_dir_blk, inner_buf);

    return 0;
}

//文件内offset~offset+size=>buf
int Inode::read_at(int offset, char *buf, int size)
{
    if(offset>=d_size)
        return FAIL;

    if(offset+size>d_size)
        size=d_size-offset;
    
    int read_size=0;

    for(int pos=offset;pos<offset+size;){
        int no=pos/BLOCK_SIZE; //文件内块编号
        int block_offset=pos%BLOCK_SIZE;//块内偏移
        int blkno=get_block_id(no);//全局 块编号

        if(blkno==FAIL)
            break;

        /* 从全局读块 进buf */
        char *inner_buf;
        fs.read_block(blkno,inner_buf);

        int block_read_size=std::min(BLOCK_SIZE - block_offset, size - read_size);//block剩余 or size剩余
        memcpy(buf+read_size,inner_buf+block_offset,block_read_size);
        read_size+=block_read_size;
        pos+=block_read_size;
    }

    return read_size;
}

//buf=>文件内offset~offset+size
int Inode::write_at(int offset, const char* buf, int size)
{
    
}

//inode级 分配block(调用fs的分配block)
int Inode::push_back_block()
{
    int blkno = fs.alloc_block();
    if(blkno == FAIL) 
        return FAIL;

    i_addr[d_size / BLOCK_SIZE] = blkno;//TODO:暂时还是直接索引
    return blkno;
}

//文件内 blockno => 磁盘全局 blockno
int Inode::get_block_id(int inner_id)
{
    if(inner_id > d_size/BLOCK_SIZE)//inner_id错误
        return FAIL;

    /* 索引 */
    if (inner_id < DIRECT_COUNT) {//直接索引
        return i_addr[inner_id];
    }
    else if(inner_id < FIRST_LEVEL_COUNT) {//TODO:二级索引
        cout<<"Unimplement Error!"<<endl;
        exit(-1);
    }
    return FAIL;
}
