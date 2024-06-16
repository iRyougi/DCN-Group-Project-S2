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

    cout << "�����������ɹ���" << endl;

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
            cout << "�յ���Ϣ " << (isAdmin[activeUsers[clientSock]] ? "Admin" : "User") << " " << activeUsers[clientSock] << ": " << cmd << endl;

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

        // ����û����Ƿ��Ѵ������û����ݿ�����Ա������
        if (userDatabase.find(username) != userDatabase.end() || isAdmin.find(username) != isAdmin.end()) {
            sendToClient(sock, "ע��ʧ�ܣ��û����Ѵ��ڻ������Ա���Ƴ�ͻ");
        }
        else {
            userDatabase[username] = { username, password }; // Default: not an admin
            activeUsers[sock] = username;
            isAdmin[username] = false;
            sendToClient(sock, "ע��ɹ�");
        }
        return true;
    }
    else if (cmd.substr(0, 6) == "LOGIN:") {
        string username = cmd.substr(6, cmd.find(':', 6) - 6);
        string password = cmd.substr(cmd.find(':', 6) + 1);
        auto it = userDatabase.find(username);

        // ����û����Ƿ������Ա���Ƴ�ͻ
        if (isAdmin.find(username) != isAdmin.end()) {
            sendToClient(sock, "�û������������");
        }
        else if (it != userDatabase.end() && it->second.password == password) {
            activeUsers[sock] = username;
            sendToClient(sock, "��¼�ɹ�");
        }
        else {
            sendToClient(sock, "�û������������");
        }
        return true;
    }
    else if (cmd.substr(0, 12) == "ADMIN_LOGIN:") {
        string adminName = cmd.substr(12);

        // ����û����Ƿ��Ѵ������û����ݿ���ѵ�¼�û���
        if (userDatabase.find(adminName) != userDatabase.end() ||
            isAdmin.find(adminName) != isAdmin.end() ||
            find_if(activeUsers.begin(), activeUsers.end(), [&](const pair<SOCKET, string>& p) {
                return p.second == adminName;
                }) != activeUsers.end()) {
            sendToClient(sock, "����Ա��¼ʧ�ܣ��û���Ϊ�ջ��Ѵ���");
            return true;
        }

        activeUsers[sock] = adminName;
        isAdmin[adminName] = true;  // Assume admin login always successful
        sendToClient(sock, "����Ա��¼�ɹ�\nʹ��/listָ���г����������û���ʹ��/kick+idָ���߳�ָ���û���ʹ��/quit�˳�����");
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
            sendToClient(it->first, "˽������ " + activeUsers[sock] + ": " + message);
            sendToClient(sock, "˽�ķ��͵� " + targetUser + ": " + message);
        }
        else {
            sendToClient(sock, "�û� " + targetUser + " �����߻��û�" + targetUser + "�����ڡ�");
        }
        return true;
    }
    else if (cmd == "/list") {
        string userList = "��ǰ�����û�:\n";
        for (const auto& user : activeUsers) {
            userList += user.second + " (" + (isAdmin[user.second] ? "����Ա" : "�û�") + ")\n";
        }
        sendToClient(sock, userList);
        return true;
    }
    else if (cmd.substr(0, 5) == "/kick" && isAdmin[activeUsers[sock]]) {
        string userId = cmd.substr(6);
        for (auto it = activeUsers.begin(); it != activeUsers.end(); ++it) {
            if (it->second == userId) {
                sendToClient(it->first, "�㱻һλ����Ա�߳���");
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
