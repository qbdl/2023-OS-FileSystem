//client——处理用户与fs的中介
#include"../include/user.h"
#include"../include/fs.h"
#include<iostream>

using namespace std;

extern User login();
FileSystem fs("myDisk.img");

int main()
{
    User user=login();
    user.set_current_dir(1);//TODO:需要动态修改
    fs.set_u(&user);

    //将初始化文件树的部分在这里完成而非init.cpp（读初始的磁盘文件到内存变量+读用户文件到磁盘文件myDisk.img&内存变量）
    // 开始扫描文件 并创建对应普通文件与目录文件
    if(!fs.initialize_filetree_from_externalFile("my_test",user.current_dir_ino)) {
        cout << "Initialize failed!" <<"\n";
        return -1;
    }

    cout<<"client over!"<<"\n";

    return 0;
}