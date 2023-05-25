#ifndef UTILITY_H
#define UTILITY_H

#include<string>
#include<sstream>
#include <ctime>
using namespace std;

vector<string> split_path(const string &path)
{
    vector<string> paths;
    istringstream iss(path);
    string token;
    while (getline(iss, token, '/')) {
        if (!token.empty()) {
            paths.push_back(token);
        }
    }
    return paths;
}

string change_to_localTime(int time)
{
    std::time_t t_time = time;
    std::tm* local_time = std::localtime(&t_time);  // int时间转换为当地时间
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local_time);  // 格式化输出

    return string(buffer);
}





#endif