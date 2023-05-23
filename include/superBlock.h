//SuperBlock类（磁盘中的数据结构）:记录Inode个数，数据块个数等，用于资源分配与回收
#ifndef SUPERBLOCK
#define SUPERBLOCK

class SuperBlock
{
public:
    SuperBlock(){};
    ~SuperBlock(){};

public:
	int		s_isize;		//外存Inode区占用的盘块数
	int		s_fsize;		//盘块总数——Block总数
	
	int		s_nfree;		//Free 数据块总数
	int		s_free[100];	//Free 数据块栈——直接管理的空闲盘块索引表
	
	int		s_ninode;		//栈中空闲 Inode数————栈顶指针
	int		s_inode[100];	//空闲 inode 栈————直接管理的空闲外存Inode索引表
							// 栈顶 s_inode[s_ninode-1] 是 待分配的下个空闲 DiskInode（的号码）

	int		s_flock;		//封锁空闲盘块索引表标志
	int		s_ilock;		//封锁空闲Inode表标志
	
	int		s_fmod;			//内存中super block副本被修改标志，意味着需要更新外存对应的Super Block */
	int		s_ronly;		//本文件系统只能读出
	int		s_time;			//最近一次更新时间
	int		padding[47];	//填充使SuperBlock块大小等于1024字节，占据2个扇区

};

#endif