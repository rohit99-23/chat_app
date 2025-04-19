#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <mutex>
#include <fstream>
#include <ctime>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

vector<SOCKET> clients;
vector<string> clientNames;
mutex clientsMutex;

string currentTime() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char timeStr[10];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", ltm);
    return string(timeStr);
}

void handleClient(SOCKET clientSocket, string clientName) {
    char buffer[1024];

    // Send welcome message with client name
    string welcome = "Welcome " + clientName + "! Type /exit to quit.\n";
    send(clientSocket, welcome.c_str(), welcome.size(), 0);

    while (true) {
        int recvSize = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (recvSize <= 0) break;

        buffer[recvSize] = '\0';
        string msg = buffer;

        if (msg == "/exit") {
            break;
        }

        cout << clientName << " says: " << msg << "\n";

        string formattedMsg = "[" + currentTime() + "] " + clientName + ": " + msg;
        cout << formattedMsg << "\n";

        // Log to file
        ofstream logFile("chatlog.txt", ios::app);
        logFile << formattedMsg << "\n";
        logFile.close();

        // Broadcast to other clients
        {
            lock_guard<mutex> lock(clientsMutex);  // ✅ only here
            for (SOCKET s : clients) {
                if (s != clientSocket) {
                    send(s, formattedMsg.c_str(), formattedMsg.size(), 0);
                }
            }
        }
    }

    // Remove this client (after loop ends)
    closesocket(clientSocket);
    {
        lock_guard<mutex> lock(clientsMutex);  // ✅ new scope to reuse variable name
        clients.erase(remove(clients.begin(), clients.end(), clientSocket), clients.end());
        clientNames.erase(remove(clientNames.begin(), clientNames.end(), clientName), clientNames.end());
    }

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
    cout << "Multi-client server listening on port 8080...\n";

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
