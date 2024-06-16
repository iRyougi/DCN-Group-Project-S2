#include <iostream>
#include <Winsock2.h>
#include <string>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int BUFFER_SIZE = 1024;

struct Admin {
    string name;
    bool loggedIn = false;
};

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
    cout << "Enter admin name to log in: ";
    getline(cin, admin.name);
    admin.loggedIn = true;
    string loginMessage = "ADMIN_LOGIN:" + admin.name;
    send(cliSock, loginMessage.c_str(), loginMessage.length(), 0);

    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];
    string input;

    // Receive login confirmation
    int bytesReceived = recv(cliSock, recvbuf, BUFFER_SIZE, 0);
    if (bytesReceived > 0) {
        recvbuf[bytesReceived] = '\0';
        cout << recvbuf << endl;
    }

    while (true) {
        getline(cin, input);

        if (input == "/quit") {
            break;
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

    closesocket(cliSock);
    WSACleanup();
    return 0;
}
