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
            if (response == "Login successful." || response == "Registration successful.") {
                loggedIn = true;
                cout << response << endl;
            }
            else if (response == "Invalid username or password." || response == "Username already exists.") {
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

    cout << "Connected to server. Type '/reg' to register or '/login' to login." << endl;
    cout << "Type '/quit' to exit the chat room." << endl;

    string input, username, password;
    char sendbuf[BUFFER_SIZE];

    while (true) {
        getline(cin, input);

        if (input == "/quit") {
            break;
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
