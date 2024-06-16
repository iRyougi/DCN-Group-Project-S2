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

    cout << "Server started." << endl;

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
            cout << "Received from " << (isAdmin[activeUsers[clientSock]] ? "Admin" : "User") << " " << activeUsers[clientSock] << ": " << cmd << endl;

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
        if (userDatabase.find(username) == userDatabase.end()) {
            userDatabase[username] = { username, password}; // Default: not an admin
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
    else if (cmd.substr(0, 12) == "ADMIN_LOGIN:") {
        string adminName = cmd.substr(12);
        activeUsers[sock] = adminName;
        isAdmin[adminName] = true;  // Assume admin login always successful
        sendToClient(sock, "Admin login successful.");
        return true;
    }
    else if (cmd == "/list" && isAdmin[activeUsers[sock]]) {
        // 列出在线用户，只有管理员可以执行
        string userList = "Online Users:\n";
        for (const auto& user : activeUsers) {
            userList += user.second + " (" + (isAdmin[user.second] ? "Admin" : "User") + ")\n";
        }
        sendToClient(sock, userList);
        return true;
    }
    else if (cmd.substr(0, 5) == "/kick" && isAdmin[activeUsers[sock]]) {
        string userId = cmd.substr(6);
        for (auto it = activeUsers.begin(); it != activeUsers.end(); ++it) {
            if (it->second == userId) {
                sendToClient(it->first, "You have been kicked out by an admin.");
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
