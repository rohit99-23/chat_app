#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void receiveMessages(SOCKET clientSocket) {
    char buffer[1024];
    while (true) {
        int recvSize = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (recvSize == SOCKET_ERROR) {
            cout << "Error receiving data from server." << endl;
            break;
        }
        if (recvSize == 0) {
            cout << "Server disconnected.\n";
            break;
        }
        buffer[recvSize] = '\0';
        cout << buffer << endl;
    }
}

void sendMessages(SOCKET clientSocket) {
    string msg;
    while (true) {
        getline(cin, msg);
        send(clientSocket, msg.c_str(), msg.size(), 0);
        if (msg == "/exit") break;
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    cout << "Connected to the server." << endl;

    // Send the client name to the server
    cout << "Enter your name: ";
    string clientName;
    getline(cin, clientName);
    send(clientSocket, clientName.c_str(), clientName.size(), 0); // Send name to server

    // Start receiving and sending messages
    thread recvThread(receiveMessages, clientSocket);
    thread sendThread(sendMessages, clientSocket);

    sendThread.join();
    recvThread.join();

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
