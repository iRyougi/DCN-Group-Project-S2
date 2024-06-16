#include <iostream>
#include <Winsock2.h>
#include <map>
#include <string>
#include <thread>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int BUFFER_SIZE = 1024;

struct User {
    string username;
    string password;
    bool isAdmin = false;
};

map<string, User> userDatabase;
map<SOCKET, string> activeUsers; // Maps sockets to usernames
map<string, bool> isAdmin; // Tracks whether a user is an admin

void sendToClient(SOCKET sock, const string& msg);
void broadcastMessage(const string& msg, SOCKET senderSock);
DWORD WINAPI clientThread(LPVOID IpParameter);
bool handleAdminCommand(const string& cmd, SOCKET sock);

int main() {
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET servSock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(12345);

    bind(servSock, (SOCKADDR*)&servAddr, sizeof(servAddr));
    listen(servSock, 10);

    cout << "服务器启动成功！" << endl;

    while (true) {
        SOCKADDR_IN cliAddr;
        int addrSize = sizeof(cliAddr);
        SOCKET clientSock = accept(servSock, (SOCKADDR*)&cliAddr, &addrSize);

        if (clientSock != INVALID_SOCKET) {
            CreateThread(NULL, 0, clientThread, (LPVOID)clientSock, 0, NULL);
        }
    }

    closesocket(servSock);
    WSACleanup();
    return 0;
}

DWORD WINAPI clientThread(LPVOID IpParameter) {
    SOCKET clientSock = (SOCKET)IpParameter;
    char buffer[BUFFER_SIZE];

    while (true) {
        int bytesReceived = recv(clientSock, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            string cmd(buffer);

            // Print received messages to server console for debugging
            cout << "收到消息 " << (isAdmin[activeUsers[clientSock]] ? "Admin" : "User") << " " << activeUsers[clientSock] << ": " << cmd << endl;

            if (!handleAdminCommand(cmd, clientSock)) {
                broadcastMessage(activeUsers[clientSock] + ": " + cmd, clientSock);
            }
        }
        else {
            break;
        }
    }

    closesocket(clientSock);
    activeUsers.erase(clientSock);
    return 0;
}

// Implementation of handleAdminCommand and other functions remains the same as provided

bool handleAdminCommand(const string& cmd, SOCKET sock) {
    if (cmd.substr(0, 4) == "REG:") {
        string username = cmd.substr(4, cmd.find(':', 4) - 4);
        string password = cmd.substr(cmd.find(':', 4) + 1);

        // 检查用户名是否已存在于用户数据库或管理员名称中
        if (userDatabase.find(username) != userDatabase.end() || isAdmin.find(username) != isAdmin.end()) {
            sendToClient(sock, "注册失败：用户名已存在或与管理员名称冲突");
        }
        else {
            userDatabase[username] = { username, password }; // Default: not an admin
            activeUsers[sock] = username;
            isAdmin[username] = false;
            sendToClient(sock, "注册成功");
        }
        return true;
    }
    else if (cmd.substr(0, 6) == "LOGIN:") {
        string username = cmd.substr(6, cmd.find(':', 6) - 6);
        string password = cmd.substr(cmd.find(':', 6) + 1);
        auto it = userDatabase.find(username);

        // 检查用户名是否与管理员名称冲突
        if (isAdmin.find(username) != isAdmin.end()) {
            sendToClient(sock, "用户名或密码错误");
        }
        else if (it != userDatabase.end() && it->second.password == password) {
            activeUsers[sock] = username;
            sendToClient(sock, "登录成功");
        }
        else {
            sendToClient(sock, "用户名或密码错误");
        }
        return true;
    }
    else if (cmd.substr(0, 12) == "ADMIN_LOGIN:") {
        string adminName = cmd.substr(12);

        // 检查用户名是否已存在于用户数据库或已登录用户中
        if (userDatabase.find(adminName) != userDatabase.end() ||
            isAdmin.find(adminName) != isAdmin.end() ||
            find_if(activeUsers.begin(), activeUsers.end(), [&](const pair<SOCKET, string>& p) {
                return p.second == adminName;
                }) != activeUsers.end()) {
            sendToClient(sock, "管理员登录失败：用户名为空或已存在");
            return true;
        }

        activeUsers[sock] = adminName;
        isAdmin[adminName] = true;  // Assume admin login always successful
        sendToClient(sock, "管理员登录成功\n使用/list指令列出所有在线用户，使用/kick+id指令踢出指定用户，使用/quit退出程序");
        return true;
    }
    else if (cmd.substr(0, 4) == "PRI:") {
        size_t colonPos = cmd.find(':', 4);
        string targetUser = cmd.substr(4, colonPos - 4);
        string message = cmd.substr(colonPos + 1);

        auto it = find_if(activeUsers.begin(), activeUsers.end(), [&](const pair<SOCKET, string>& p) {
            return p.second == targetUser;
            });

        if (it != activeUsers.end()) {
            sendToClient(it->first, "私聊来自 " + activeUsers[sock] + ": " + message);
            sendToClient(sock, "私聊发送到 " + targetUser + ": " + message);
        }
        else {
            sendToClient(sock, "用户 " + targetUser + " 不在线或用户" + targetUser + "不存在。");
        }
        return true;
    }
    else if (cmd == "/list") {
        string userList = "当前在线用户:\n";
        for (const auto& user : activeUsers) {
            userList += user.second + " (" + (isAdmin[user.second] ? "管理员" : "用户") + ")\n";
        }
        sendToClient(sock, userList);
        return true;
    }
    else if (cmd.substr(0, 5) == "/kick" && isAdmin[activeUsers[sock]]) {
        string userId = cmd.substr(6);
        for (auto it = activeUsers.begin(); it != activeUsers.end(); ++it) {
            if (it->second == userId) {
                sendToClient(it->first, "你被一位管理员踢出了");
                closesocket(it->first);
                activeUsers.erase(it);
                break;
            }
        }
        return true;
    }
    else if (cmd.substr(0, 10) == "MSG_ADMIN:" && isAdmin[activeUsers[sock]]) {
        string message = cmd.substr(10);
        broadcastMessage(message, sock);
        return true;
    }

    return false;
}

void sendToClient(SOCKET sock, const string& msg) {
    send(sock, msg.c_str(), msg.length(), 0);
}

void broadcastMessage(const string& msg, SOCKET senderSock) {
    for (auto& user_pair : activeUsers) {
        if (user_pair.first != senderSock) {
            sendToClient(user_pair.first, msg);
        }
    }
}
