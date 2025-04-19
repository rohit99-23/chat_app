#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

vector<SOCKET> clients;
vector<string> clientNames;
mutex clientsMutex;

void handleClient(SOCKET clientSocket, string clientName) {
    char buffer[1024];
    // send welcome message with client name
    string welcome = "Welcome " + clientName + "! Type /exit to quit.\n";
    send(clientSocket, welcome.c_str(), welcome.size(), 0);

    while (true) {
        int recvSize = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (recvSize <= 0) break; // error or disconnect

        buffer[recvSize] = '\0';
        string msg = buffer;

        if (msg == "/exit") { // client wants to quit
            break;
        }

        cout << clientName << " says: " << msg << "\n";

        // broadcast to everyone else
        lock_guard<mutex> lock(clientsMutex);
        for (SOCKET s : clients) {
            if (s != clientSocket) {
                send(s, msg.c_str(), msg.size(), 0);
            }
        }
    }

    // remove this client
    closesocket(clientSocket);
    lock_guard<mutex> lock(clientsMutex);
    clients.erase(remove(clients.begin(), clients.end(), clientSocket), clients.end());
    clientNames.erase(remove(clientNames.begin(), clientNames.end(), clientName), clientNames.end());
    cout << clientName << " disconnected.\n";
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&addr, sizeof(addr));
    listen(serverSocket, 5);
    cout << "Multi client server listening on port 8080...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) continue;

        // Get client name
        char buffer[1024];
        int recvSize = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (recvSize <= 0) {
            cout << "Error receiving name from client.\n";
            continue;
        }
        buffer[recvSize] = '\0';
        string clientName = buffer;

        {
            lock_guard<mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
            clientNames.push_back(clientName);
        }

        cout << clientName << " connected.\n";

        thread t(handleClient, clientSocket, clientName);
        t.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
