#include"../include/fs.h"
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
FileSystem::FileSystem(string disk_path)
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
    this->disk=move(temp_disk);
}

//fstream的内容 写回磁盘文件
FileSystem::~FileSystem()
{
    // superblock
    disk.seekp(0, ios::beg);
    disk.write((char*)(&sblock), sizeof(SuperBlock));

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
    disk.write((char*)(&dinodes[0]), sizeof(DiskInode)*INODE_NUM);

    //data
    //真正的文件数据不应该全部写回，应在修改后考虑写回，这里不进行写回

    
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
        int *table=(int*)buf;
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
    // std::cout<<OFFSET_DATA + blkno*BLOCK_SIZE<<"\n";
    // disk.seekg(0, ios::end);
    // std::cout<<disk.tellg()<<"\n";

    disk.seekg(OFFSET_DATA + blkno*BLOCK_SIZE, ios::beg);
    disk.read(buf, BLOCK_SIZE);
    return true;
}

//buf=>blkno块
bool FileSystem::write_block(int blkno, char* buf) 
{
    disk.seekg(OFFSET_DATA + blkno*BLOCK_SIZE, ios::beg);
    disk.write(buf, BLOCK_SIZE);
    // std::cout<<"write_block buf content:\n"<<buf<<"\n";
    return true;
}

//文件树 path =>inode号
int FileSystem::find_from_path(const string& path)
{
    int ino;    // 起始查询的目录INode号
    if(path.empty()){
        cerr << "error path!" << "\n";
        return FAIL;
    }
    else {      // 判断是相对路径还是绝对路径
        if(path[0] == '/')
            ino = ROOT_INO;         
        else
            ino = user->current_dir_ino;
    }

    // 解析Path
    vector<string> tokens;
    istringstream iss(path);
    string token;
    while (getline(iss, token, '/')) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    // 依次查找每一级目录或文件
    for (const auto& token : tokens) {
        auto entrys = inodes[ino].get_entry();
        bool found = false;
        // 遍历所有目录项
        for (auto& entry : entrys) {
            if (entry.m_ino && strcmp(entry.m_name, token.c_str()) == 0) {
                ino = entry.m_ino;
                found = true;
                break;
            }
        }
        if (!found) return FAIL;
    }

    return ino;    
}

//外部文件写入磁盘中
bool FileSystem::saveFile(const string& src, const string& filename) 
{
    // 确定目标目录inode号

    // 在目标目录下 inode层级 创建新文件

    //读取源文件

    //写入inode信息

    int dir_ino;
    if(filename.rfind('/') == -1)
        dir_ino = user->current_dir_ino;
    else {
        dir_ino = find_from_path(filename.substr(0, filename.rfind('/')));
        if (dir_ino == FAIL) {
            cerr << "Failed to find directory: " << filename.substr(0, filename.rfind('/')) << "\n";
            return false;
        }
    }


    string name = filename.substr(filename.rfind('/') + 1);
    int ino = inodes[dir_ino].create_file(name, false);
    if (ino == FAIL) {
        cerr << "Failed to create file: " << filename << "\n";
        return false;
    }


    Inode& inode = inodes[ino];
    ifstream infile(src, ios::binary | ios::in);
    if (!infile) {
        cerr << "Failed to open file: " << src << "\n";
        return false;
    }
    infile.seekg(0, ios::end);
    size_t size = infile.tellg();
    infile.seekg(0, ios::beg);
    vector<char> data(size);
    infile.read(data.data(), size);
    // cout<<"saveFile data content:\n"<<data.data()<<"\n";


    if (!inode.write_at(0, data.data(), size)) {
        cerr << "Failed to write data to inode" << "\n";
        return false;
    }
    inode.i_size = size;
    inode.i_mtime = get_cur_time();
    inode.i_atime = get_cur_time();
    inode.i_nlink = 1;

    infile.close();

    return true;
}

//扫描源文件夹并在 文件系统中创建对应 普通文件与目录文件
bool FileSystem::initialize_filetree_from_externalFile(const string &path, const int root_no)
{
    //根目录初始化
    if(inodes[ROOT_INO].i_size == 0)
        inodes[ROOT_INO].init_as_dir(ROOT_INO, ROOT_INO);
    

    // 打开目录
    DIR *pDIR = opendir((path + '/').c_str());
    dirent *pdirent = NULL;
    if (pDIR != NULL)// 如果目录打开成功
    {
        cout << "under " << path << ":" << "\n";
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
                    cout << "make folder: " << Name << " success! inode:" << ino << "\n";
                    
                    //递归进入
                    user->current_dir_ino = ino;
                    if(initialize_filetree_from_externalFile(path + '/' + Name, ino) == false)
                        return false;
                    user->current_dir_ino = root_no;
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


/* 命令实现 */

//ls 列出目录的内容
bool FileSystem::ls(const string& path)
{
    //获取path目录的inode
    int path_ino;
    if(path.empty())
        path_ino=user->current_dir_ino;
    else
        path_ino=find_from_path(path);
    
    if (path_ino == FAIL) {
        cerr << "ls: cannot access '" << path << "': No such file or directory" << "\n";
        return false;
    }

    //通过get_entry获取所有目录项
    Inode inode=inodes[path_ino];

    auto entries=inode.get_entry();
    for(auto &entry : entries){
        if(entry.m_ino){
            string name(entry.m_name);
            cout<<name;
            if (entry.m_type == DirectoryEntry::FileType::Directory) {
                cout << "/";
            }
            cout << "\n";
        }
    }

    return true;
}