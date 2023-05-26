#include"../include/fs.h"
#include"../include/Inode.h"
#include"../include/utility.h"
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

    
    // 关闭文件（后续还需要使用fstream流）
    // disk.close();
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

    cout << "alloc inode : " << ino << "\n";
    return ino;
}

//放回一个inode
int FileSystem::dealloc_inode(int ino)
{
    sblock.s_inode[sblock.s_ninode++] = ino;
    cout << "dealloc inode : " << ino << "\n";
    
    return 0;
}

//分配一个空闲块,返回块号
int FileSystem::alloc_block()
{
    /* 当前s_free的空闲块数不足（当前的100块用完） */
    //TODO:s_free[0]到底是什么，可能有点问题（init.cpp的部分可能也有点问题）
    int blkno;
    if(sblock.s_nfree <= 1){
        //换表 + sblock内容更新 （101个元素，第一个是长度，后续是内容）
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

    cout << "alloc block : " << blkno << "\n";
    return blkno;
}

//放回一个物理块
int FileSystem::dealloc_block(int blkno)
{
    //TODO:s_free[0]到底是什么，可能有点问题
    if(sblock.s_nfree >= 100){ // free list 满了
        //换表
        char buf[BLOCK_SIZE]="";
        int *table=(int *)buf;
        table[0]=sblock.s_nfree;
        for(int i=0;i<sblock.s_nfree;i++)
            table[i+1] = sblock.s_free[i];
        write_block(sblock.s_free[0],buf);//对应alloc_block的读取
        
        //sblock更新
        sblock.s_nfree=1;
    }
    //释放
    sblock.s_free[sblock.s_nfree++]=blkno;

    cout << "dealloc block : " << blkno << "\n";

    return 0;
}


//磁盘blkno块=>buf
bool FileSystem::read_block(int blkno, char* buf) 
{
    // std::cout<<OFFSET_DATA + blkno*BLOCK_SIZE<<"\n";
    // disk.seekg(0, ios::end);
    // std::cout<<disk.tellg()<<"\n";

    disk.seekg(OFFSET_DATA + blkno*BLOCK_SIZE, ios::beg);
    disk.read(buf, BLOCK_SIZE);
    return true;
}

//buf=>磁盘blkno块
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

//上级文件夹ino
int FileSystem::find_fa_ino(const string& path)
{
    if(path.rfind('/') == -1)
        return user->current_dir_ino;
    else
        return find_from_path(path.substr(0, path.rfind('/')));
}

//set 当前目录名
void FileSystem::set_current_dir_name(const string& dir_name)
{
    vector<string>paths=split_path(dir_name);

    for(auto &path:paths){
        if(path==".."){//考虑向上切换目录情况
            if(user->current_dir_name !="/") {
                size_t pos = user->current_dir_name.rfind('/');
                user->current_dir_name = user->current_dir_name.substr(0, pos);
                if(user->current_dir_name == "")
                    user->current_dir_name = "/";
            }
            else
                cerr<<"Can't set current dir_name : has reached the root!"<<"\n";        
        }
        else if(path=="."){;}
        else{
            if (user->current_dir_name.back() != '/')
                user->current_dir_name += '/';
            user->current_dir_name += path;
        }
    }


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
                    // cout << "make folder: " << Name << " success! inode:" << ino << "\n";
                    
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

                    // cout << "make file: " << Name << " success!"<<"\n";
                }
                else{
                    cout << "other file" << Name <<"\n";
                }
            }
        }
    }
    // 关闭目录
    closedir(pDIR);
    // 返回目录文件的inode号
    return true;
}


/* 命令实现 */
string FileSystem::pCommand(int uid, string& command)
{
    set_u(uid);

    // 解析命令
    istringstream iss(input);
    string s;
    vector<string> tokens;
    while(iss >> s)
        tokens.emplace_back(s);
    if (tokens.empty()) {
        continue;
    }

    if(tokens[0] == "init"){
        // if(!fs.initialize_filetree_from_externalFile(tokens[1],user.current_dir_ino)) {
        if(!fs.initialize_filetree_from_externalFile("my_test",user.current_dir_ino)) {
            cout << "Initialize failed!" << "\n";
            return -1;
        }
        else
            cout << "Initialize success!" << "\n";
    }
    else if(tokens[0] == "ls"){
        if(tokens.size() >= 2)
            result = ls(tokens[1]);
        else
            result = ls("");
    }
    else if(tokens[0] == "cd"){
        if(changeDir(tokens[1]))
            result = "cd : success!";
    }
    else if(tokens[0] == "mkdir"){
        if(createDir(user_[uid_].current_dir_,tokens[1]))
            result = "mkdir : success!";
    }
    else if(tokens[0] == "cat"){
        result = cat(tokens[1]);
    }
    else if(tokens[0] == "rm"){
        if(deleteFile(tokens[1]))
            result = "rm : success!";
    }
    else if(tokens[0] == "cp"){
        if(copyFile(tokens[1], tokens[2]))
            result = "cp : success!";
    }
    else if(tokens[0] == "save"){
        if(saveFile(tokens[1], tokens[2]))
            result = "save : success!";
    }
    else if(tokens[0] == "export"){
        if(exportFile(tokens[1], tokens[2]))
            result = "export : success!";
    }
    else if(tokens[0] == "mv"){
         if(moveFile(tokens[1], tokens[2]))
            result = "move : success!";
    }
    else if(tokens[0] == "rename")
        if(renameFile(tokens[1], tokens[2]))
            result = "rename : success!";
    else {
        result = "Invalid command!";
    }
    
    /* 
    else if(tokens[0] == "touch"){
        fs.createFile(tokens[1]);
    }
    else if(tokens[0] == "write"){
        fs.writeFile(tokens[1]);
    }
    else if(tokens[0] == "help"){
        fs.help();
    else if(tokens[0] == "exit")
        break;
    } */
    return result;
}


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

    // 检查是否是目录
    Inode inode=inodes[path_ino];
    if(!(inode.i_mode & Inode::FileType::Directory)) {
        cerr << "'" << path << "' is not a directory" << "\n";
        return FAIL;
    }

    auto entries=inode.get_entry();
    for(auto &entry : entries){
        if(entry.m_ino){
            Inode child_inode = inodes[entry.m_ino];
            string name(entry.m_name);
            string mtime=change_to_localTime(child_inode.i_mtime);

            cout<<setw(7)<<user->username;
            cout<<setw(5)<<child_inode.i_size  << "B ";
            cout<<mtime<<" ";
            cout<<name;
            if (child_inode.i_mode & Inode::FileType::Directory) {
                cout << "/";
            }
            cout << "\n";
        }
    }

    return true;
}

//cat 输出指定文件的内容
bool FileSystem::cat(const string& path)
{
    //ino
    int path_ino=find_from_path(path);
    if(path_ino==FAIL){
        cerr<<"cat: cannot access '" << path << "': No such file or directory" << "\n";
        return false;
    }

    //是否为普通文件
    Inode &inode=inodes[path_ino];
    if (inode.i_mode & Inode::Directory) {
        std::cerr << "cat: " << path << ": Is a directory" << "\n";
        return false;
    }

    string str;
    str.resize(inode.i_size+1);//否则会缓冲区溢出！
    inode.read_at(0,(char *)str.data(),inode.i_size);

    cout<<str<<"\n";
    return true;
}

//cd 切换目录
bool FileSystem::changeDir(const std::string& path)
{
    //inode
    int path_ino=find_from_path(path);
    if(path_ino==FAIL){
        cerr << "Failed to find directory: " << path << "\n";
        return false;
    }

    //是否为普通文件
    Inode inode=inodes[path_ino];
    if((inode.i_mode & Inode::FileType::Directory)==0){
        cerr << "'" << path << "' is not a directory" << "\n";
        return false;
    }

    //set inode & dirname
    this->set_current_dir(path_ino);
    this->set_current_dir_name(path);

    return true;
}

//mkdir 创建目录
bool FileSystem::createDir(const string& path)
{
    int path_no = inodes[user->current_dir_ino].create_file(path, true);
    cout << "make folder: " << path << " success! inode:" << path_no << "\n";
    
    return true;
    // return path_no;
}

//rm 删除文件
bool FileSystem::deleteFile(const string& path)
{
    int dir_ino=find_fa_ino(path);
    int ino=find_from_path(path);
    if(dir_ino==FAIL || ino==FAIL){
        cerr << "rm: '" << path << "' No such file or directory" << "\n";
        return false;
    }

    if(inodes[ino].i_mode & Inode::FileType::Directory) {
        cerr << "rm: cannot remove '" << path << "': Is a directory" << "\n";
        return false;
    }

    // 从父目录中删除entry项
    ino = inodes[dir_ino].delete_file_entry(path.substr(path.rfind('/')+1));
    if (ino == FAIL)
        return false;

    // 删除文件 释放inode
    inodes[ino].clear_();


    return true;
}
    
//cp 复制文件
bool FileSystem::copyFile(const string& src, const string& dst)
{
    int src_ino = find_from_path(src);
    if(src_ino == FAIL) {
        cerr << "cp: cannot stat '" << src << "': No such file or directory" << "\n";
        return false;
    }

    int dst_dir=find_fa_ino(dst);
    int dst_ino = find_from_path(dst);
    if(dst_ino != FAIL) {
        cerr << "cp: cannot stat '" << dst << "': File exists" << "\n";
        return false;
    }

    // 复制inode和数据
    int new_ino = inodes[dst_dir].create_file(dst.substr(dst.rfind('/')+1), false);
    inodes[new_ino].copy_inode(inodes[src_ino]);

    return true;    
}

//save 外部文件写入磁盘中
bool FileSystem::saveFile(const string& src, const string& filename) 
{
    // 确定目标目录inode号

    // 在目标目录下 inode层级 创建新文件

    //读取源文件

    //写入inode信息

    int dir_ino;
    dir_ino=find_fa_ino(filename);
    if (dir_ino == FAIL) {
        cerr << "Failed to find directory: " << filename.substr(0, filename.rfind('/')) << "\n";
        return false;
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

//export 磁盘中的文件导出到本地
bool FileSystem::exportFile(const string& src, const string& outsideFile)
{
    int ino = find_from_path(src);
    if (ino == FAIL) {
        cerr << "Failed to find file: " << src << "\n";
        return false;
    }

    Inode &inode = inodes[ino];
    
    // 判断是否为文件
    if(!(inode.i_mode & Inode::FileType::RegularFile)) {
        cerr << "Failed to export " << src << ". Is not a Regular File!" << "\n";
        return false;
    }

    // 打开外部文件
    ofstream fout(outsideFile, ios::binary);
    if (!fout) {
        cerr << "Failed to open file: " << outsideFile << "\n";
        return false;
    }

    // 读取文件内容
    int size = inode.i_size;
    string buf;
    buf.resize(size);
    inode.read_at(0, (char *)buf.data(), size);

    // 写入外部文件
    fout.write(buf.data(), size);
    fout.close();

    return true;    
}

//rename 修改文件名
bool FileSystem::renameFile(const string& filename, const string& newname)
{
    /* 父级 目录项修改 */
    int dir_ino,ino;
    string name,new_name;

    //exists
    dir_ino=find_fa_ino(filename);
    ino=find_from_path(filename);
    if (dir_ino == FAIL || ino == FAIL ) {
        cerr << "rm : Failed to find directory: " << filename.substr(0, filename.rfind('/')) << "\n";
        return false;
    }

    //是否在同级目录下
    name = filename.substr(filename.rfind('/') + 1);
    new_name=newname.substr(filename.rfind('/') + 1);

    if(dir_ino!=find_fa_ino(newname)){
        cerr << "rm : " << name << "and " << new_name << " are not under the same folder!" << "\n";
        return false;  
    }

    //是否为普通文件
    Inode &inode=inodes[ino];
    if (!(inode.i_mode & Inode::RegularFile)) {
        cerr << "rm : " << filename << ": Is not a Regular File!" << "\n";
        return false;
    }

    //修改
    bool found = false;
    Inode &dir_inode=inodes[dir_ino];
    auto entrys=dir_inode.get_entry();
    for (auto& entry : entrys) {
        if (entry.m_ino && strcmp(entry.m_name, name.c_str()) == 0) {
            ino = entry.m_ino;
            found = true;
            strcpy(entry.m_name,new_name.c_str());
            break;
        }
    }
    if (!found){
        cerr << "rm : Failed to find file: " << filename<< "\n";
        return false;
    } 
    if(dir_inode.set_entry(entrys)==FAIL){
        cerr << "rm : Failed to set entry for " << newname << "\n";
        return false;
    }

    
    return true;
}

//mv 将一个文件从一个目录移动到另一个目录
bool FileSystem::moveFile(const string& src, const string& dst)
{
    /* 两个父级目录修改目录项 */

    int src_ino=find_from_path(src);
    int src_dir_ino=find_fa_ino(src);
    int dst_ino;
    int dst_dir_ino=find_fa_ino(dst);

    if(src_ino==FAIL || src_dir_ino == FAIL){
        cerr << "mv : Failed to find file: " << src << "\n";
        return false;
    }
    if(dst_dir_ino ==FAIL){
        cerr << "mv : Failed to find directory: "<< dst.substr(0, dst.rfind('/')) << "\n";
        return false;
    }

    Inode & src_inode=inodes[src_ino];
    if(!(src_inode.i_mode & Inode::FileType::RegularFile)){
        cerr << "mv : "<< src <<" is not a Regular File" << "\n";
        return false;
    }

    //src
    Inode &src_dir_inode=inodes[src_dir_ino];
    src_dir_inode.delete_file_entry(src.substr(src.rfind('/')+1));
    //dst
    Inode &dst_dir_inode=inodes[dst_dir_ino];
    dst_dir_inode.add_file_entry(dst.substr(dst.rfind('/')+1),src_ino);

    return true;
}