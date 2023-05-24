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
    inodes[ino].i_gid = user->group;
    inodes[ino].i_atime = inodes[ino].i_mtime = get_cur_time();

    return ino;
}

//分配 空闲块,返回块号（未实际分配空间）
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
    // 暂未实现缓存
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


//通过传入的文件数据fdata创建文件,在数据区分配空间并存储，返回 文件唯一的inode号
int FileSystem::create_file(string& fdata)
{
    int data_p=0;//数据指针
    int addr_n=0;//dInode正使用的d_addr序号

    // 创建一个新的inode，记录文件长度 
    int ino=alloc_inode(fdata.length());
    cout<<"filelen:"<<inodes[ino].i_size<<"\n";

    //分配文件大小
    while(data_p<inodes[ino].i_size){
        // 分配一个数据块
        int blkno=alloc_block();
        int copy_len=BLOCK_SIZE;

        // 复制长度为BLOCK_SIZE的数据到数据块中，不够则复制剩余长度
        if(inodes[ino].i_size-data_p<BLOCK_SIZE){//剩余长度不足一个Block
            copy_len=inodes[ino].i_size-data_p;
        }
        memcpy(data_+blkno*BLOCK_SIZE,fdata.c_str()+data_p,copy_len);
        cout<<"offset:"<<hex<<OFFSET_DATA+blkno*BLOCK_SIZE<<dec<<"\n";// 输出数据块的偏移量

        inodes[ino].i_addr[addr_n++]=blkno;//在inode中记录数据块地址//TODO:目前全都是直接索引，没有一级和二级

        data_p+=copy_len;// 更新数据指针
    }

    return ino;//返回inode号
}


//扫描源文件夹并在 文件系统中创建对应 普通文件与目录文件
int FileSystem::initialize_filetree_from_externalFile(const std::string &path)
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
                    ino = initialize_filetree_from_externalFile(path + '/' + Name);
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
            else
                cout<<"未成功获取"<<"\n";
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