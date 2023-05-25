#ifndef USER_H
#define USER_H

#include<string>

class User
{
public:
    std::string username;
    std::string password;

    int uid;//uesr id
    int gid;//group id

public:
    int current_dir_ino;//当前路径文件的inode号
    std::string current_dir_name = "/";
public:
    int get_current_dir() const { return current_dir_ino; }
    std::string get_current_dir_name() const { return current_dir_name; }


};

#endif