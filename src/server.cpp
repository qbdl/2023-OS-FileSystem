#include"../include/user.h"
#include"../include/fs.h"
#include<iostream>
#include<string>
#include<vector>
#include<fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

using namespace std;

FileSystem fs("myDisk.img");
int login(int clientSocket);

//验证登录
User verify(string username,string password)
{
    vector<User> userList;

    // char cwd[PATH_MAX];
    // if (getcwd(cwd, sizeof(cwd)) != NULL) {
    //     printf("Current working dir: %s\n", cwd);
    // } else {
    //     perror("getcwd() error");
    // }

    //users.txt
    fstream file("users.txt",ios::in);
    if(!file){
        cerr<<"can't open file "<<"users.txt"<<"\n";
        return User();//Empty
    }

    //读取用户信息
    while(!file.eof()){
        User user;
        file >> user.uid >> user.username >> user.password >> user.gid;
        userList.push_back(user);
    }
    file.close();

    //逐个匹配
    for(auto user:userList){
        if(user.username==username){
            if(user.password==password){
                return user;
            }
            else
                return User();
        }
    }
    return User();//Empty
}

//外部接口
User login()
{
    return verify("alice","123");

    string username, password;

    while(true){
        // 提示用户输入用户名和密码
        cout << "Username: ";
        getline(cin, username);
        cout << "Password: ";
        getline(cin, password);

        // 验证登录信息
        User user = verify(username, password);
        if (user.username != "") {
            cout << "Login successful"<<"\n";
            cout << user.username<<", Welcome Back!"<<"\n";
            return user;
        }
        else {
            cout << "Incorrect username or password"<<"\n";
        }
    }
}

void handleClient(int clientSocket) 
{
    
}

int main() 
{

    return 0;
}