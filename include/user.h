#ifndef USER
#define USER

#include<string>

class User
{
public:
    std::string username;
    std::string password;

    int uid;//uesr id
    int gid;//group id

    int current_dir_ino;//当前路径文件的inode号
public:
    void set_current_dir(int inum) { current_dir_ino = inum; }
};

#endif