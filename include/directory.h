//DirectEntry类：存储目录列表（Inode号与名称）
//这里的注释可能不一定对，后续再修改——————————参考Inode.h : 对于一个 INODEFLAG 为 IDIRECTORY 的 Inode,其i_addr[0]所指向的即为一个Directory结构体

#ifndef DIRECTORYENTRY
#define DIRECTORYENTRY

#include<cstring>
class DirectoryEntry
{
public:
    static const int DIR_SIZE=28;//文件名最大长度

public:
    DirectoryEntry(){};
    DirectoryEntry(int ino,const char * name){
        m_ino=ino;
        strcpy(m_name,name);
    }
    ~DirectoryEntry(){};
public:
    int m_ino;//文件INode号
    char m_name[DIR_SIZE];//文件名
};

#endif