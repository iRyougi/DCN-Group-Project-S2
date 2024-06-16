#include <iostream>
#include <Winsock2.h>
#include <cstring>
#include <string>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int BUFFER_SIZE = 1024;

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
    servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(12345);

    if (connect(cliSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        cerr << "Connection failed with error: " << WSAGetLastError() << endl;
        closesocket(cliSock);
        WSACleanup();
        return 1;
    }

    cout << "Admin connected to server. You can send messages or commands (/list, /kick <username>)." << endl;

    char sendbuf[BUFFER_SIZE];
    string input;

    while (true) {
        getline(cin, input);

        if (input == "/quit") {
            break;
        }

        strcpy(sendbuf, input.c_str());
        send(cliSock, sendbuf, strlen(sendbuf), 0);

        if (input.substr(0, 5) == "/list" || input.substr(0, 5) == "/kick") {
            char recvbuf[BUFFER_SIZE];
            int bytesReceived = recv(cliSock, recvbuf, BUFFER_SIZE, 0);
            if (bytesReceived > 0) {
                recvbuf[bytesReceived] = '\0';
                cout << "Server response: " << recvbuf << endl;
            }
        }
    }

    closesocket(cliSock);
    WSACleanup();
    return 0;
}
