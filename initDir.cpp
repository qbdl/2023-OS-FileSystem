#include"superBlock.h" //sblock
#include"diskInode.h" //DiskInode
#include"initDir.h"//初始化目录
#include"parameter.h"//文件系统通用参数
#include"directory.h"
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

// using namespace std;

//extern Global Variables
extern SuperBlock sblock;
extern DiskInode dInode[INODE_NUM];
extern char data_[DATA_SIZE];

//分配一个inode,返回inode号（未实际分配空间）
int create_inode(int size)
{
    int ino=sblock.s_free[--sblock.s_nfree];//分配inode号
    dInode[ino].d_nlink=1;//硬链接置为1
    dInode[ino].d_size=size;//初始化大小
    return ino;
}

//分配 空闲块,返回块号（未实际分配空间）
int alloc_data_block()
{
    int blkno=sblock.s_inode[--sblock.s_ninode];
    if(sblock.s_ninode<0){
        cout<<"ERROR: sblock is empty!"<<"\n";
        return -1;
    }
    return blkno;
}

//通过传入的文件数据fdata创建文件,在数据区分配空间并存储，返回 文件唯一的inode号
int create_file(string& fdata)
{
    int data_p=0;//数据指针
    int addr_n=0;//dInode正使用的d_addr序号

    // 创建一个新的inode，记录文件长度 
    int ino=create_inode(fdata.length());
    cout<<"filelen:"<<dInode[ino].d_size<<"\n";

    //分配文件大小
    while(data_p<dInode[ino].d_size){
        // 分配一个数据块
        int blkno=alloc_data_block();
        int copy_len=BLOCK_SIZE;

        // 复制长度为BLOCK_SIZE的数据到数据块中，不够则复制剩余长度
        if(dInode[ino].d_size-data_p<BLOCK_SIZE){//剩余长度不足一个Block
            copy_len=dInode[ino].d_size-data_p;
        }
        memcpy(data_+blkno*BLOCK_SIZE,fdata.c_str()+data_p,copy_len);
        cout<<"offset:"<<hex<<OFFSET_DATA+blkno*BLOCK_SIZE<<dec<<"\n";// 输出数据块的偏移量

        dInode[ino].d_addr[addr_n++]=blkno;//在inode中记录数据块地址//TODO:目前全都是直接索引，没有一级和二级

        data_p+=copy_len;// 更新数据指针
    }

    return ino;//返回inode号
}


//扫描源文件夹并在 文件系统中创建对应 普通文件与目录文件
int scan_path(string path)
{
    // 打开目录
    int dir_file_ino = 0;
    DIR *pDIR = opendir((path + '/').c_str());
    dirent *pdirent = NULL;
    if (pDIR != NULL)// 如果目录打开成功
    {
        ostringstream dir_data;
        cout << "在目录" << path << "下：" << endl;

        // 循环读取目录
        while ((pdirent = readdir(pDIR)) != NULL)
        {
            string Name = pdirent->d_name;

            // 跳过"."和".."
            if (Name == "." || Name == "..")
                continue;

            struct stat buf;

            // 获取文件信息
            if (!lstat((path + '/' + Name).c_str(), &buf))
            {
                int ino; // 获取文件的inode号

                // 如果是目录文件
                if (S_ISDIR(buf.st_mode))
                {
                    cout << "目录文件：" << Name << endl;
                    ino = scan_path(path + '/' + Name);
                }
                // 如果是普通文件
                else if (S_ISREG(buf.st_mode))
                {
                    // 读取文件内容
                    ifstream tmp;
                    ostringstream tmpdata;
                    tmp.open(path + '/' + Name);
                    tmpdata << tmp.rdbuf(); // 数据流导入进file_data
                    string tmp_data_string = tmpdata.str();

                    // 创建文件
                    ino = create_file(tmp_data_string);
                    tmp.close();
                }

                // 输出文件名和inode号，并将其加入到目录数据段中
                cout << "目录项内容: "<<"name:" << Name << " ino:" << ino << "加入到目录数据段中" << "\n";
                dir_data << ino << setfill('\0') << setw(DirectoryEntry::DIR_SIZE) << Name << endl;
            }
        }

        cout << "构造目录文件:" << path <<"\n";
        string dir_data_string = dir_data.str();
        dir_file_ino = create_file(dir_data_string);
        cout << "构造目录inode成功,inode:" << dir_file_ino << "\n";
    }

    // 关闭目录
    closedir(pDIR);
    // 返回目录文件的inode号
    return dir_file_ino;
}



