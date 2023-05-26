//client——处理用户与fs的中介
#include"../include/user.h"
#include"../include/fs.h"
#include<iostream>
#include<sstream>
#include<vector>
#include<string>

using namespace std;

int main()
{
    User user=login();
    fs.set_current_dir(1);//TODO:需要动态修改

    while (true) {
        // 读取用户输入的命令
        string input;
        cout << "$ "<< user.get_current_dir_name() << " >>";
        getline(cin, input);
    }

    cout<<"client over!"<<"\n";

    return 0;
}