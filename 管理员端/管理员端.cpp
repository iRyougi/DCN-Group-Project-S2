#include <iostream>
#include <Winsock2.h>
#include <string>
#include <thread>  // 引入线程库

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int BUFFER_SIZE = 1024;

struct Admin {
    string name;
    bool loggedIn = false;
};

void receiveMessages(SOCKET cliSock) {
    char recvbuf[BUFFER_SIZE];
    while (true) {
        int bytesReceived = recv(cliSock, recvbuf, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            recvbuf[bytesReceived] = '\0';
            cout << recvbuf << endl;  // 输出接收到的消息
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
    SOCKET cliSock;
    SOCKADDR_IN servAddr;

    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cerr << "WSAStartup failed: " << result << endl;
        return 1;
    }

    cliSock = socket(AF_INET, SOCK_STREAM, 0);
    if (cliSock == INVALID_SOCKET) {
        cerr << "Socket creation failed with error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(12345);
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(cliSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        cerr << "Connect failed with error: " << WSAGetLastError() << endl;
        closesocket(cliSock);
        WSACleanup();
        return 1;
    }

    Admin admin;
    string loginMessage;
    char recvbuf[BUFFER_SIZE];
    char sendbuf[BUFFER_SIZE];

    while (true) {
        cout << "输入一个管理员名称: ";
        getline(cin, admin.name);

        loginMessage = "ADMIN_LOGIN:" + admin.name;
        send(cliSock, loginMessage.c_str(), loginMessage.length(), 0);

        // 接收登录确认或失败消息
        int bytesReceived = recv(cliSock, recvbuf, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            recvbuf[bytesReceived] = '\0';
            string response(recvbuf);
            cout << response << endl;
            if (response.find("管理员登录成功") != string::npos) {
                admin.loggedIn = true;
                break;  // 登录成功，退出循环
            } else {
                cout << "管理员登录失败，请重新输入。" << endl;
            }
        }
    }

    thread recvThread(receiveMessages, cliSock);  // 启动接收消息的线程

    string input;
    while (true) {
        getline(cin, input);

        if (input == "/quit") {
            send(cliSock, "QUIT", 4, 0);  // 发送退出信号给服务器
            closesocket(cliSock);  // 关闭socket
            WSACleanup();  // 清理Winsock
            exit(0);  // 立即退出程序
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
        else if (input.substr(0, 5) == "/list" || input.substr(0, 5) == "/kick") {
            strcpy(sendbuf, input.c_str());
            send(cliSock, sendbuf, strlen(sendbuf), 0);
        }
        else {
            string formattedMessage = "MSG_ADMIN:" + admin.name + "（管理员）: " + input;
            strcpy(sendbuf, formattedMessage.c_str());
            send(cliSock, sendbuf, strlen(sendbuf), 0);
        }
    }

    recvThread.join();  // 等待接收线程结束
    closesocket(cliSock);
    WSACleanup();
    return 0;
}
