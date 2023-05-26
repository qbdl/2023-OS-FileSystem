#include"../include/Inode.h"
#include"../include/BlockCache.h"
#include"../include/fs.h"
#include<iostream>

using namespace std;


extern FileSystem fs;
extern time_t get_cur_time();

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
            

    //创建该文件的inode
        // 分配一个新的inode用来存放新创建的文件或者目录
        
        //（创建该文件的目录内容)
        
        // 设置文件类型
        
        //增加父文件的目录项
    
    auto entrys=get_entry();
    int entrynum=entrys.size();
    for(auto entry:entrys){
        if(entry.m_ino && strcmp(entry.m_name,fileName.c_str())==0){
            cerr << "createFile: File already exists." << "\n";
            return -1;
        }
    }

    if (entrynum % ENTRYS_PER_BLOCK == 0)
        push_back_block();


    int ino=fs.alloc_inode();
    if (ino == 0) {
        cerr << "createFile: No free inode" << "\n";
        return -1;
    }

    if(is_dir)
        fs.inodes[ino].init_as_dir(ino,i_ino);
    else
        fs.inodes[ino].i_mode |= FileType::RegularFile;

    int blknum = entrynum / ENTRYS_PER_BLOCK;
    
    //使用缓存
    auto cache_blk = fs.block_cache_mgr_.get_block_cache(get_block_id(blknum));
    auto new_entry_block_ = (DirectoryEntry *)cache_blk->data();
    cache_blk->modified_ = true;
    
    //不使用缓存
    // char new_entry_block[BLOCK_SIZE];
    // fs.read_block(get_block_id(blknum),(char*)new_entry_block);
    // DirectoryEntry* new_entry_block_=(DirectoryEntry*)new_entry_block;

    DirectoryEntry new_entry(ino,fileName.c_str());
    new_entry_block_[entrynum % ENTRYS_PER_BLOCK]=new_entry;

    // fs.write_block(get_block_id(blknum), (char*)new_entry_block_);


    i_size+=ENTRY_SIZE;

    return ino;
}

//获取当前文件(目录文件)的所有目录项
vector<DirectoryEntry> Inode::get_entry()
{
    vector<DirectoryEntry>entrys;
    entrys.resize(i_size/ENTRY_SIZE);

    if(entrys.size()==0) return entrys;
    if(read_at(0, (char *)entrys.data(), i_size)==FAIL) {
        cerr << "getEntry: read directory entries failed." << "\n";
        return entrys;
    }

    return entrys;
}

//设置 所有目录项
int Inode::set_entry(vector<DirectoryEntry>& entrys)
{
    if(!write_at(0, (char *)entrys.data(), i_size)) {
        cerr << "setEntry: write directory entries failed." << "\n";
        return FAIL;
    }
    return 0;
}

// 删除目录项，返回Inode号，但是没有删除Inode(调用者保证删除目录时子文件的处理)
int Inode::delete_file_entry(const string &fileName)
{
    auto entrys = get_entry();
    int ino;

    bool found = false;
    for (int i=0; i<entrys.size(); i++) {
        if (found)                              // 删除节点之后，后续的目录项前移
            entrys[i-1] = entrys[i];
        else{
           if(entrys[i].m_ino && strcmp(entrys[i].m_name, fileName.c_str()) == 0) {
                ino = entrys[i].m_ino;
                found = true;
           }
        }
    }
    if(!found) {
        cerr << "Delete_File_Entry: File not found." << "\n";
        return FAIL;
    }

    // 更新目录文件内容
    if(set_entry(entrys) == FAIL)
        return FAIL;


    i_size -= ENTRY_SIZE;                       // 目录文件收缩
    if (i_size % BLOCK_SIZE == 0)               // 需要弹出最后一块物理块
        pop_back_block();

    return ino;
}

//利用Inode号 增加目录项，但是没有新建Inode(调用者保证新增目录时子文件的处理)
int Inode::add_file_entry(const string &fileName,int src_ino)
{
    auto entrys = get_entry();

    bool found = false;
    for (auto entry:entrys) {
        if(entry.m_ino==src_ino) {
            found = true;
            break;
        }
    }
    if(found) {
        cerr << "Add_File_Entry: File has existed." << "\n";
        return FAIL;
    }
    entrys.emplace_back(DirectoryEntry(src_ino,fileName.c_str()));

    i_size += ENTRY_SIZE;                       // 目录文件扩大
    if (i_size % BLOCK_SIZE == 0)               // 需要新增一块物理块
        push_back_block();


    // 更新目录文件内容
    if(set_entry(entrys) == FAIL)
        return FAIL;

    return 0;   
}


//在目录文件中加.与..的目录项
int Inode::init_as_dir(int ino, int fa_ino)
{
    i_mode |= FileType::Directory;

    /* 分配 block 来存 目录项*/
    int sub_dir_blk = push_back_block();
    if (sub_dir_blk == FAIL) {
        cerr << "createFile: No free block" << "\n";
        return -1;
    }

    /* 不使用缓存 */

    /* 读取block 加.与..的目录项*/
    // char inner_buf[BLOCK_SIZE];//需要分配指定空间，否则会报段错误！！
    // fs.read_block(sub_dir_blk,(char*)inner_buf);

    // /* 修改 ,写回 */
    // DirectoryEntry *sub_entrys=(DirectoryEntry *)inner_buf;

    // DirectoryEntry dot_entry(ino, 
    //                     ".", 
    //                     Inode::FileType::Directory);
    // DirectoryEntry dotdot_entry(fa_ino, 
    //                             "..", 
    //                             Inode::FileType::Directory);
    // sub_entrys[0] = dot_entry;
    // sub_entrys[1] = dotdot_entry;
    // i_size += ENTRY_SIZE*2;

    // fs.write_block(sub_dir_blk, inner_buf);

    /* 使用缓存 */
    auto cache_blk = fs.block_cache_mgr_.get_block_cache(sub_dir_blk);
    auto sub_entrys = (DirectoryEntry *)cache_blk->data();
    cache_blk->modified_ = true;
    DirectoryEntry dot_entry(ino,".");
    DirectoryEntry dotdot_entry(fa_ino,"..");
    sub_entrys[0] = dot_entry;
    sub_entrys[1] = dotdot_entry;
    i_size += ENTRY_SIZE*2;    

    return 0;
}

//文件内offset~offset+size=>buf
int Inode::read_at(int offset, char *buf, int size)
{
    if(offset>=i_size)
        return FAIL;

    if(offset+size>i_size)
        size=i_size-offset;
    
    int read_size=0;

    for(int pos=offset;pos<offset+size;){
        int no=pos/BLOCK_SIZE; //文件内块编号
        int block_offset=pos%BLOCK_SIZE;//块内偏移
        int blkno=get_block_id(no);//全局 块编号

        if(blkno==FAIL)
            break;

        /* 从全局读块 进buf */

        /* 不使用缓存 */
        // char inner_buf[BLOCK_SIZE];
        // fs.read_block(blkno,(char*)inner_buf);

        /* 使用缓存 */
        char *inner_buf = fs.block_cache_mgr_
                      .get_block_cache(blkno)
                      ->data();
        //

        int block_read_size=min<int>(BLOCK_SIZE - block_offset, size - read_size);//block剩余 or size剩余
        memcpy(buf+read_size,inner_buf+block_offset,block_read_size);
        read_size+=block_read_size;
        pos+=block_read_size;
    }

    i_atime = get_cur_time();

    return read_size;
}

//resize file大小
int Inode::resize_file(int size)
{
    if(size<=i_size){//TODO:收缩
        return 0;
    }

    int append_block_num=(size + BLOCK_SIZE - 1) / BLOCK_SIZE - (i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;//利用+ BLOCK_SIZE - 1进行向上取整
    while(append_block_num--) {
        if (push_back_block() == FAIL) {
            return FAIL;
        }
    }

    return 0;
}


//buf=>文件内offset~offset+size
int Inode::write_at(int offset, const char* buf, int size)
{
    if (offset + size > i_size)//TODO:缩小
        resize_file(offset + size);
    
    int written_size = 0;

    for (int pos = offset; pos < offset + size;) {
        int no = pos / BLOCK_SIZE;             //计算inode中的块编号
        int block_offset = pos % BLOCK_SIZE;   //块内偏移
        int blkno = get_block_id(no);

        if (blkno==FAIL){
            cerr<<"block 不足"<<"\n";
            return written_size;
        }

        int block_write_size = min<int>(BLOCK_SIZE - block_offset, size - written_size);
        
        /* 不使用缓存 */
        // char inner_buf[BLOCK_SIZE];
        // //不是全写的——需要读，修改部分后写回
        // if(block_offset!=0 || block_write_size < BLOCK_SIZE)
        //     fs.read_block(blkno, (char*)inner_buf);

        // //写
        // memcpy(inner_buf + block_offset, buf + written_size, block_write_size);
        // fs.write_block(blkno, inner_buf);

        /* 使用缓存 */
        char *inner_buf = fs.block_cache_mgr_
                .get_block_cache(blkno)
                ->data();
        fs.block_cache_mgr_.get_block_cache(blkno)->modified_ = true;
        memcpy(inner_buf + block_offset, buf + written_size, block_write_size);
        //

        written_size += block_write_size;
        pos += block_write_size;
    }

    // 更新inode的文件大小和最后修改时间
    if (offset + written_size > i_size) {
        i_size = offset + written_size;
    }
    i_mtime = i_atime = get_cur_time();

    return written_size;
}

//inode级 分配block(调用fs的分配block)
int Inode::push_back_block()
{
    int blkno = fs.alloc_block();
    if(blkno == FAIL) 
        return FAIL;

    i_addr[i_size / BLOCK_SIZE] = blkno;//TODO:暂时还是直接索引
    return blkno;
}

//inode级 释放block(调用fs的释放block)
int Inode::pop_back_block()
{
    //TODO:暂时还是直接索引
    int blkno = i_addr[i_size / BLOCK_SIZE +1];//到释放前-> +1
    if(blkno == 0) 
        return FAIL;

    fs.dealloc_block(blkno);
    i_addr[i_size / BLOCK_SIZE +1] = 0;

    return 0;
}



//文件内 blockno => 磁盘全局 blockno
int Inode::get_block_id(int inner_id)
{
    if(inner_id > i_size/BLOCK_SIZE)//inner_id错误
        return FAIL;

    /* 索引 */
    if (inner_id < DIRECT_COUNT) {//直接索引
        return i_addr[inner_id];
    }
    else if(inner_id < FIRST_LEVEL_COUNT) {//TODO:二级索引
        cout<<"Unimplement Error!"<<"\n";
        exit(-1);
    }
    return FAIL;
}

//copy inode
int Inode::copy_inode(Inode &src)
{
    i_mode = src.i_mode;
    i_size = src.i_size;
    i_mtime = i_atime = get_cur_time();

    /* 复制物理块内容（逐块复制） */
    int blknum = (src.i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for(int i=0; i<blknum; i++) {
        int srcno = src.get_block_id(i);
        int new_blkno = fs.alloc_block();
        if(new_blkno == FAIL) {
            cerr << "copyFrom: No free block" << "\n";
            return FAIL;
        }

        char *src_buf = fs.block_cache_mgr_.get_block_cache(srcno)->data();
        char *new_buf = fs.block_cache_mgr_.get_block_cache(new_blkno)->data();
        memcpy(new_buf, src_buf, BLOCK_SIZE);

        this->i_addr[i++] = new_blkno;//TODO:暂时还是直接索引
    }
    return 0; 
}

//删除 该inode
int Inode::clear_()
{
    //block块释放
    for (int i=0; i<10; i++) {
        if (i_addr[i]) {
            fs.dealloc_block(i_addr[i]);
            i_addr[i] = 0;
        }
    }
    //inode释放
    fs.dealloc_inode(i_ino);

    return 0;
}