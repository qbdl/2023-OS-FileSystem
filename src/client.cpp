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
    fs.set_u(&user);
    fs.set_current_dir(1);//TODO:需要动态修改

    while (true) {
        // 读取用户输入的命令
        string input;
        cout << "$ "<< user.get_current_dir_name() << " >>";
        getline(cin, input);

        // 解析命令
        istringstream iss(input);
        string s;
        vector<string> tokens;
        while(iss >> s)
            tokens.emplace_back(s);
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
            if(tokens.size() >= 2)
                fs.ls(tokens[1]);
            else    
                fs.ls("");
        }
        else if(tokens[0] == "cd")
            fs.changeDir(tokens[1]);
        else if(tokens[0] == "mkdir")
            fs.createDir(tokens[1]);
        else if(tokens[0] == "cat")
            fs.cat(tokens[1]);
        else if(tokens[0] == "rm")
            fs.deleteFile(tokens[1]);
        else if(tokens[0] == "cp")
            fs.copyFile(tokens[1], tokens[2]);
        else if(tokens[0] == "save"){
            fs.saveFile(tokens[1], tokens[2]);
        }
        else if(tokens[0] == "export"){
            fs.exportFile(tokens[1], tokens[2]);
        }
        else if(tokens[0] == "exit")
            break;
        /* 
        else if(tokens[0] == "touch"){
            fs.createFile(tokens[1]);
        }
        else if(tokens[0] == "mv"){
            fs.moveFile(tokens[1], tokens[2]);
        }
        else if(tokens[0] == "write"){
            fs.writeFile(tokens[1]);
        }
        else if(tokens[0] == "rename"){
            fs.renameFile(tokens[1], tokens[2]);
        }
        else if(tokens[0] == "help"){
            fs.help();
        } */
    }

    cout<<"client over!"<<"\n";

    return 0;
}