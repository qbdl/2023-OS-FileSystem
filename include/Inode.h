/* 磁盘的DiskInode结构 */
#ifndef DISKINODE
#define DISKINODE

#include "../include/directory.h"
#include<string>
#include<vector>

class DiskInode
{
public:
    DiskInode(){};
    ~DiskInode(){};

public:
    unsigned int d_mode;	//状态的标志位，定义见enum INODEFLAG
    int		d_nlink;	    // 硬链接计数（磁盘中的概念）:一个文件在文件系统中有多少个硬链接指向它。硬链接本质上是一个指向文件的目录条目，它允许在文件系统中为同一个文件创建多个名称。每个硬链接都指向同一个 Inode，因此它们共享相同的文件数据和元数据。
		                    // 该文件在目录树中不同路径名的数量
	short	d_uid;			//文件所有者的ID
	short	d_gid;			//文件所有者的group ID
	
	int		d_size;			//文件大小，字节为单位
	int		d_addr[10];		// 指针数组，用于存储文件数据块在磁盘上的索引 : 6个直接索引、2 个一级间接指针，2个二级间接索引
	                        // 文件逻辑块号=>物理块号转换
	int		d_atime;		//最后访问时间
	int		d_mtime;		//最后修改时间
};

#endif

#ifndef INODE
#define INODE

/* 内存的Inode结构 */
class Inode
{
public:
    enum INODEFLAG
    {
        INITSTATE = 0x0,	//inode初始态
        ILOCK = 0x1,		//索引节点上锁
		IUPD  = 0x2,		//内存inode被修改过，需要更新相应外存inode
		IACC  = 0x4,		//内存inode被访问过，需要修改最近一次访问时间
		IMOUNT = 0x8,		//内存inode用于挂载子文件系统
		IWANT = 0x10,		//有进程正在等待该内存inode被解锁，清ILOCK标志时，要唤醒这种进程
		ITEXT = 0x20		//内存inode对应进程图像的正文段
    };

    Inode(){};
    ~Inode(){};

public:
	/*Inode 特有*/
	int		i_ino;			//inode在内存inode数组中的下标
	int		i_lock;			//该内存inode的锁

	/*DiskInode 共有*/
    unsigned int i_mode;	//状态的标志位，定义见enum INODEFLAG
    int		i_nlink;	    // 硬链接计数（磁盘中的概念）:一个文件在文件系统中有多少个硬链接指向它。硬链接本质上是一个指向文件的目录条目，它允许在文件系统中为同一个文件创建多个名称。每个硬链接都指向同一个 Inode，因此它们共享相同的文件数据和元数据。
		                    // 该文件在目录树中不同路径名的数量
	short	i_uid;			//文件所有者的ID
	short	i_gid;			//文件所有者的group ID
	
	int		i_size;			//文件大小，字节为单位
	int		i_addr[10];		// 指针数组，用于存储文件数据块在磁盘上的索引 : 6个直接索引、2 个一级间接指针，2个二级间接索引
	                        // 文件逻辑块号=>物理块号转换
	int		i_atime;		//最后访问时间
	int		i_mtime;		//最后修改时间

public:
	/* 外部接口 */
	int create_file(const std::string&fileName,bool is_dir);

	/* 内部实现 */
	std::vector<DirectoryEntry> get_entry();//获取当前文件(目录文件)的所有目录项

	int read_at(int offset, char *buf, int size);//文件内offset~offset+size=>buf

	int push_back_block();//inode级 分配block(调用fs的分配block)

	int get_block_id(int inner_id);//文件内blockno=>磁盘blockno

	int init_as_dir(int ino, int fa_ino);//在目录文件中加.与..的目录项

};

#endif