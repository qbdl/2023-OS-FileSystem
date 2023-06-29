#include"../include/user.h"
#include"../include/fs.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>
#include <map>
#include <iterator>

using namespace std;

FileSystem fs("myDisk.img");

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
User login(int clientSocket)
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
    auto user = login(clientSocket);
    fs.set_u(&user);
    fs.set_current_dir(1);//TODO:

    char buffer[1024];

    // 读取客户端发送的命令行字符串
    while(true){
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            string command = buffer;
            if(command == "exit"){
                write(clientSocket, "exit!", 6);
                break;
            }

            // fs 处理
            string result = fs.pCommand(user, command);

            // 发送结果给客户端
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, result.c_str());
            strcpy(buffer+result.length() + 1, user.get_current_dir_name().c_str());
            int len = result.length() + 1 + user.get_current_dir_name().length() + 1;
            write(clientSocket, buffer, len); 
        }
    }
    close(clientSocket);  // 关闭客户端套接字
}

int main() 
{
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen;

    // 创建套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Failed to create socket" << endl;
        return 1;
    }

    // 设置服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);  // 指定服务器端口号

    // 绑定套接字到指定端口
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Failed to bind socket" << endl;
        return 1;
    }

    // 监听传入的连接
    if (listen(serverSocket, 5) < 0) {
        cerr << "Failed to listen on socket" << endl;
        return 1;
    }

    cout << "Server started. Listening on port 8888" << endl;

    while (true) {
        // 接受新的连接
        clientAddrLen = sizeof(clientAddr);
        newSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (newSocket < 0) {
            cerr << "Failed to accept connection" << endl;
            continue;
        }

        // 创建新线程处理客户端连接
        thread clientThread(handleClient, newSocket);
        clientThread.detach();  // 在后台运行线程，不阻塞主循环
    }

    close(serverSocket);  // 关闭服务器端的套接字

    return 0;
}