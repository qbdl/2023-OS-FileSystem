#include"../include/fs.h"
#include"../include/superBlock.h" //sblock
#include"../include/Inode.h" //DiskInode&Inode
#include"../include/parameter.h"//文件系统通用参数
#include"../include/directory.h"
#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

using namespace std;

//初始化 读磁盘进内存变量
//用fstream映射整个磁盘文件 进行操作
FileSystem::FileSystem(std::string disk_path)
{
    /*init.cpp怎么写进去，这里就怎么读出来*/
    fstream temp_disk(disk_path,ios::in|ios::out|ios::binary);
    if(!temp_disk) {
        cerr << "Failed to open disk file " << disk_path << "\n";
        exit(EXIT_FAILURE);
    }

    //读取SuperBlock
    temp_disk.seekg(0,ios::beg);
    temp_disk.read(reinterpret_cast<char*>(&sblock), sizeof(SuperBlock));

    //读取Inode
    DiskInode dinodes[INODE_NUM];
    temp_disk.seekg(OFFSET_INODE, ios::beg);
    temp_disk.read(reinterpret_cast<char*>(&dinodes[0]), sizeof(DiskInode)*INODE_NUM);
    for (int i=0; i<INODE_NUM;i++) {
        inodes[i].i_ino = i;
        inodes[i].i_lock=Inode::INITSTATE;
        inodes[i].i_mode = dinodes[i].d_mode;
        inodes[i].i_nlink = dinodes[i].d_nlink;
        inodes[i].i_uid = dinodes[i].d_uid;
        inodes[i].i_gid = dinodes[i].d_gid;
        inodes[i].i_size = dinodes[i].d_size;
        inodes[i].i_atime = dinodes[i].d_atime;
        inodes[i].i_mtime = dinodes[i].d_mtime;
        for (int j = 0; j < 10; j++)
            inodes[i].i_addr[j] = dinodes[i].d_addr[j];
    }

    //读取数据区(empty)——后续修改(不读取全部数据区)
    temp_disk.seekg(OFFSET_DATA,ios::beg);
    temp_disk.read(reinterpret_cast<char*>(&data_[0]),sizeof(data_[0])*DATA_NUM);


    //返回disk与disk_path
    this->disk_path=disk_path;
    this->disk=std::move(temp_disk);
}

//fstream的内容 写回磁盘文件
FileSystem::~FileSystem()
{
    // superblock
    disk.seekp(0, ios::beg);
    disk.write(reinterpret_cast<char*>(&sblock), sizeof(SuperBlock));

    //Inode
    disk.seekp(OFFSET_INODE, ios::beg);
    DiskInode dinodes[INODE_NUM];
    for (int i=0; i<INODE_NUM;i++) {
        dinodes[i].d_mode = inodes[i].i_mode;
        dinodes[i].d_nlink = inodes[i].i_nlink;
        dinodes[i].d_uid = inodes[i].i_uid;
        dinodes[i].d_gid = inodes[i].i_gid;
        dinodes[i].d_size = inodes[i].i_size;
        dinodes[i].d_atime = inodes[i].i_atime;
        dinodes[i].d_mtime = inodes[i].i_mtime;
        for (int j = 0; j < 10; j++)
            dinodes[i].d_addr[j] = inodes[i].i_addr[j];
    }
    disk.write(reinterpret_cast<char*>(&dinodes[0]), sizeof(DiskInode)*INODE_NUM);

    //data
    //真正的文件数据不应该全部写回，应在修改后考虑写回，这里暂时全部写回
    disk.seekp(OFFSET_DATA,ios::beg);
    disk.write(reinterpret_cast<char*>(&data_[0]), sizeof(data_[0])*DATA_NUM);
    
    // 关闭文件
    disk.close();
}

time_t get_cur_time() 
{
    return chrono::system_clock::to_time_t(chrono::system_clock::now());
}

//分配一个空闲inode，并初步初始化
int FileSystem::alloc_inode() 
{
    if(sblock.s_ninode <= 1)
        return 0;
    int ino = sblock.s_inode[--sblock.s_ninode];//栈 逆序分配
    inodes[ino].i_nlink = 1;
    inodes[ino].i_uid = user->uid;
    inodes[ino].i_gid = user->gid;
    inodes[ino].i_atime = inodes[ino].i_mtime = get_cur_time();

    return ino;
}

//分配一个空闲块,返回块号
int FileSystem::alloc_block()
{
    /* 当前s_free的空闲块数不足（当前的100块用完） */
    int blkno;
    if(sblock.s_nfree <= 1){
        //换下一个新表（101个元素，第一个是长度，后续是内容）
        char buf[BLOCK_SIZE]="";//这里其实不用取这么大空间
        read_block(sblock.s_free[0],buf);//s_free[0]指向下一块表的位置（init时候决定）
        int *table=(int *)buf;
        sblock.s_nfree=table[0];
        for(int i=0;i<sblock.s_nfree;i++)
            sblock.s_free[i] = table[i+1];
    }

    /* 取一个块 */
    blkno=sblock.s_free[--sblock.s_nfree];
    if (blkno == 0) {
        cerr << "error : block list empty" << "\n";
        return FAIL;
    }
    return blkno;
}

//blkno块=>buf
bool FileSystem::read_block(int blkno, char* buf) 
{
    disk.seekg(OFFSET_DATA + blkno*BLOCK_SIZE, std::ios::beg);
    disk.read(buf, BLOCK_SIZE);
    return true;
}

//buf=>blkno块
bool FileSystem::write_block(int blkno, char* buf) 
{
    disk.seekg(OFFSET_DATA + blkno*BLOCK_SIZE, std::ios::beg);
    disk.write(buf, BLOCK_SIZE);
    return true;
}

//外部文件写入磁盘中
bool FileSystem::saveFile(const std::string& src, const std::string& filename) 
{
    //创建新文件

    //读取源文件

    //写入inode信息

}

//扫描源文件夹并在 文件系统中创建对应 普通文件与目录文件
bool FileSystem::initialize_filetree_from_externalFile(const std::string &path, const int root_no)
{
    //根目录初始化
    if(inodes[ROOT_INO].i_size == 0)
        inodes[ROOT_INO].init_as_dir(ROOT_INO, ROOT_INO);
    

    // 打开目录
    DIR *pDIR = opendir((path + '/').c_str());
    dirent *pdirent = NULL;
    if (pDIR != NULL)// 如果目录打开成功
    {
        cout << "under" << path << ":" << endl;
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
                    ino = inodes[root_no].create_file(Name, true);
                    cout << "make folder: " << Name << " success! inode:" << ino << endl;
                    
                    //递归进入
                    user_->current_dir_ino = ino;
                    if(initialize_from_external_directory(path + '/' + Name, ino) == false)
                        return false;
                    user_->current_dir_ino = root_no;
                }                
                // 如果是普通文件
                else if (S_ISREG(buf.st_mode))
                {
                    cout << "normal file:" << Name << "\n";
                    if(saveFile(path + '/' + Name, Name) == false)
                        return false;

                    cout << "make file: " << Name << " success!"<<"\n";
                }
            }
            else{
                cout << "other file" << Name <<"\n";
            }
        }
    }
    // 关闭目录
    closedir(pDIR);
    // 返回目录文件的inode号
    return true;
}