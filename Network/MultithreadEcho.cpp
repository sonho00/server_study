#include <iostream>
#include <thread>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

void handleClient(SOCKET clientSocket)
{
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            std::cout << "Received message from client: " << std::string(buffer, bytesReceived) << std::endl;
            send(clientSocket, buffer, bytesReceived, 0);
            std::cout << "Echoed message back to client." << std::endl;
        } else if (bytesReceived == 0) {
            std::cout << "Client disconnected." << std::endl;
            break;
        } else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    closesocket(clientSocket);
}

int main()
{
    std::cout << "Starting echo server..." << std::endl;

    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    std::cout << "Echo server started on port 8080..." << std::endl;

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Socket created successfully." << std::endl;

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Socket bound to port 8080." << std::endl;

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Listening for incoming connections..." << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::cout << "Client connected." << std::endl;

        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    closesocket(listenSocket);
    WSACleanup();

    std::cout << "Echo server stopped." << std::endl;

    return 0;
}
