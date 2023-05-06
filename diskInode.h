//磁盘的DiskInode结构
class DiskInode
{
public:
    enum INODEFLAG
    {
        ILOCK = 0x1,		//索引节点上锁
		IUPD  = 0x2,		//内存inode被修改过，需要更新相应外存inode
		IACC  = 0x4,		//内存inode被访问过，需要修改最近一次访问时间
		IMOUNT = 0x8,		//内存inode用于挂载子文件系统
		IWANT = 0x10,		//有进程正在等待该内存inode被解锁，清ILOCK标志时，要唤醒这种进程
		ITEXT = 0x20		//内存inode对应进程图像的正文段
    };

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