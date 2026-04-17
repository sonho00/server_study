#include <iostream>
#include <thread>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")


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

    SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected." << std::endl;

    while (true) {
        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            break;
        }
        buffer[bytesReceived] = '\0';

        std::cout << "Message received from client: " << buffer << std::endl;

        int bytesSent = send(clientSocket, buffer, bytesReceived, 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            break;
        }

        std::cout << "Message echoed back to client." << std::endl;
    }

    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();

    std::cout << "Echo server stopped." << std::endl;

    return 0;
}
