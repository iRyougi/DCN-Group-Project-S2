#include <iostream>
#include <Winsock2.h>
#include <map>
#include <vector>
#include <cstring>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int BUFFER_SIZE = 1024;
map<SOCKET, string> activeUsers; // Maps sockets to usernames
map<string, bool> isAdmin; // Tracks whether a user is an admin

struct User {
    string username;
    string password;
    bool isAdmin = false;
};

map<string, User> userDatabase;

void sendToClient(SOCKET sock, const string& msg);
void broadcastMessage(const string& msg);
DWORD WINAPI servEventThread(LPVOID IpParameter);
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

    cout << "Chat server started." << endl;

    while (true) {
        SOCKADDR_IN cliAddr;
        int addrSize = sizeof(cliAddr);
        SOCKET clientSock = accept(servSock, (SOCKADDR*)&cliAddr, &addrSize);

        if (clientSock != INVALID_SOCKET) {
            CreateThread(NULL, 0, servEventThread, (LPVOID)clientSock, 0, NULL);
        }
    }

    closesocket(servSock);
    WSACleanup();
    return 0;
}

DWORD WINAPI servEventThread(LPVOID IpParameter) {
    SOCKET clientSock = (SOCKET)IpParameter;
    char buffer[BUFFER_SIZE];

    while (true) {
        int bytesReceived = recv(clientSock, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            string cmd(buffer);
            if (!handleAdminCommand(cmd, clientSock)) {
                broadcastMessage(activeUsers[clientSock] + ": " + cmd);
            }
        }
        else {
            break;
        }
    }

    closesocket(clientSock);
    activeUsers.erase(clientSock); // Remove user from active list
    return 0;
}

bool handleAdminCommand(const string& cmd, SOCKET sock) {
    if (cmd.substr(0, 4) == "REG:") {
        string username = cmd.substr(4, cmd.find(':', 4) - 4);
        string password = cmd.substr(cmd.find(':', 4) + 1);
        if (userDatabase.find(username) == userDatabase.end()) {
            userDatabase[username] = { username, password, false }; // Default: not an admin
            activeUsers[sock] = username;
            isAdmin[username] = false;
            sendToClient(sock, "Registration successful.");
        }
        else {
            sendToClient(sock, "Username already exists.");
        }
        return true;
    }
    else if (cmd.substr(0, 6) == "LOGIN:") {
        string username = cmd.substr(6, cmd.find(':', 6) - 6);
        string password = cmd.substr(cmd.find(':', 6) + 1);
        auto it = userDatabase.find(username);
        if (it != userDatabase.end() && it->second.password == password) {
            activeUsers[sock] = username;
            sendToClient(sock, "Login successful.");
        }
        else {
            sendToClient(sock, "Invalid username or password.");
        }
        return true;
    }
    else if (cmd.substr(0, 6) == "ADMINLOGIN:") {
        size_t firstColon = cmd.find(':', 11);
        size_t secondColon = cmd.find(':', firstColon + 1);
        string username = cmd.substr(11, firstColon - 11);
        string password = cmd.substr(firstColon + 1, secondColon - firstColon - 1);
        string adminFlag = cmd.substr(secondColon + 1);
        bool isAdmin = adminFlag == "1";  // 假设管理员标志为"1"代表true

        auto it = userDatabase.find(username);
        if (it != userDatabase.end() && it->second.password == password) {
            it->second.isAdmin = isAdmin;  // 更新用户的管理员状态
            activeUsers[sock] = username;
            sendToClient(sock, "Login successful.");
        }
        else {
            sendToClient(sock, "Invalid username or password.");
        }
        return true;
    }
    else if (cmd == "LIST" && isAdmin[activeUsers[sock]]) {
        string userList = "Online Users:\n";
        for (const auto& user : activeUsers) {
            userList += user.second + "\n";
        }
        sendToClient(sock, userList);
        return true;
    }
    else if (cmd.substr(0, 5) == "KICK:" && isAdmin[activeUsers[sock]]) {
        string usernameToKick = cmd.substr(5);
        for (auto it = activeUsers.begin(); it != activeUsers.end(); ++it) {
            if (it->second == usernameToKick) {
                sendToClient(it->first, "You have been kicked out by an admin.");
                closesocket(it->first);
                activeUsers.erase(it);
                break;
            }
        }
        return true;
    }
    return false;
}

void sendToClient(SOCKET sock, const string& msg) {
    string modifiedMsg = msg;
    // Check if the user is an admin and prepend "Admin: " if true
    if (isAdmin[activeUsers[sock]]) {
        modifiedMsg = "Admin: " + modifiedMsg;
    }
    send(sock, modifiedMsg.c_str(), modifiedMsg.length(), 0);
}


void broadcastMessage(const string& msg) {
    cout << "收到信息: " << msg << endl;  // Adding logging to see messages being broadcasted
    for (auto& user_pair : activeUsers) {
        SOCKET sock = user_pair.first;
        sendToClient(sock, msg);
    }
}
