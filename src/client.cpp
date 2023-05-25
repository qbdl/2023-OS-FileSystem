//client——处理用户与fs的中介
#include"../include/user.h"
#include"../include/fs.h"
#include<iostream>
#include<sstream>
#include<vector>
#include<string>

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
    while (true) {
        // 读取用户输入的命令
        string input;
        cout << ">> ";
        getline(cin, input);

        // 解析命令
        istringstream iss(input);
        string s;
        vector<string> tokens;
        while(iss >> s){
            tokens.emplace_back(s);
        }
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
            if(tokens.size() >= 2){
                fs.ls(tokens[1]);
            }
            else    
                fs.ls("");
        }
    }

    cout<<"client over!"<<"\n";

    return 0;
}