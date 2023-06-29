//client——处理用户与fs的中介
#include"../include/user.h"
#include"../include/fs.h"
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

using namespace std;

int main() 
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[1024];

    // 创建套接字
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Failed to create socket" << endl;
        return 1;
    }

    // 设置服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 连接服务器
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Failed to connect to server" << endl;
        return 1;
    }

    cout << "Connected to server" << endl;
    // 进入交互循环
    string input, server_reply, head;
    while (true) {
        // 显示路径提示符
        head = buffer+server_reply.length()+1;
        if(head.length())
            cout << "[" << head << "] ";
        cout << ">> ";

        // 读取用户输入
        getline(cin, input);

        // 发送数据到服务器
        if (send(clientSocket, input.c_str(), input.length(), 0) == -1) {
            cerr << "Failed to send data to server" << endl;
            break;
        }

        // 接收数据从服务器
        memset(buffer, 0, sizeof(buffer));
        int numBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (numBytes == -1) {
            cerr << "Failed to receive data from server" << endl;
            break;
        } else if (numBytes == 0) {
            cout << "Server closed the connection" << endl;
            break;
        }

        // 显示服务器响应
        server_reply = buffer;
        cout << server_reply << endl;
    }

    // 关闭套接字
    close(clientSocket);

    return 0;
}