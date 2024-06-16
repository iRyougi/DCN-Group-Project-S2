#include <iostream>
#include <Winsock2.h>
#include <cstring>
#include <string>
#include <thread>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int BUFFER_SIZE = 1024;

void receiveMessages(SOCKET cliSock, bool& loggedIn) {
    char recvbuf[BUFFER_SIZE];
    while (true) {
        int bytesReceived = recv(cliSock, recvbuf, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            recvbuf[bytesReceived] = '\0';
            string response = string(recvbuf);

            // 处理服务器的反馈
            if (response == "登录成功" || response == "注册成功") {
                loggedIn = true;
                cout << response << endl;
            }
            else if (response == "用户名或密码错误" || response == "该用户已存在") {
                loggedIn = false;
                cout << response << endl;
            }
            else {
                cout << response << endl;
            }
        }
        else if (bytesReceived == 0) {
            cout << "服务器连接已关闭。" << endl;
            break;
        }
        else {
            cerr << "接收错误，错误代码：" << WSAGetLastError() << endl;
            break;
        }
    }
}

int main() {
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Winsock initialization failed." << endl;
        return 1;
    }

    SOCKET cliSock = socket(AF_INET, SOCK_STREAM, 0);
    if (cliSock == INVALID_SOCKET) {
        cerr << "Socket creation failed with error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    SOCKADDR_IN servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(12345);

    if (connect(cliSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        cerr << "Connection failed with error: " << WSAGetLastError() << endl;
        closesocket(cliSock);
        WSACleanup();
        return 1;
    }

    bool loggedIn = false;
    thread recvThread(receiveMessages, cliSock, ref(loggedIn));

    cout << "已连接到服务器. 输入 '/reg' 来注册，或者输入 '/login' 来登录" << endl;
    cout << "登录后输入’/pri’进入私聊模式，输入’/list’列出所有在线用户\n输入 '/quit' 可以退出聊天程序" << endl;

    string input, username, password;
    char sendbuf[BUFFER_SIZE];

    while (true) {
        getline(cin, input);

        if (input == "/quit") {
            send(cliSock, "QUIT", 4, 0);  // 发送退出信号给服务器
            closesocket(cliSock);  // 关闭socket
            WSACleanup();  // 清理Winsock
            exit(0);  // 立即退出程序
        }
        else if (input == "/reg") {
            cout << "请输入用户名：";
            getline(cin, username);
            cout << "请输入密码：";
            getline(cin, password);
            sprintf(sendbuf, "REG:%s:%s", username.c_str(), password.c_str());
            send(cliSock, sendbuf, strlen(sendbuf), 0);
        }
        else if (input == "/login") {
            cout << "请输入用户名：";
            getline(cin, username);
            cout << "请输入密码：";
            getline(cin, password);
            sprintf(sendbuf, "LOGIN:%s:%s", username.c_str(), password.c_str());
            send(cliSock, sendbuf, strlen(sendbuf), 0);
        }
        else if (input == "/pri") {
            cout << "请输入私聊的用户名和消息，格式为 用户名:消息" << endl;
            getline(cin, input);
            size_t colonPos = input.find(':');
            if (colonPos != string::npos) {
                string targetUser = input.substr(0, colonPos);
                string message = input.substr(colonPos + 1);
                sprintf(sendbuf, "PRI:%s:%s", targetUser.c_str(), message.c_str());
                send(cliSock, sendbuf, strlen(sendbuf), 0);
            }
            else {
                cout << "无效的输入格式。正确格式为：用户名:消息" << endl;
            }
        }
        else if (input.substr(0, 5) == "/list") {
            strcpy(sendbuf, input.c_str());
            send(cliSock, sendbuf, strlen(sendbuf), 0);
        }
        else {
            if (!loggedIn) {
                cout << "请先登录或注册。" << endl;
                continue;
            }
            send(cliSock, input.c_str(), input.length(), 0);
        }
    }

    recvThread.join();
    closesocket(cliSock);
    WSACleanup();
    return 0;
}
