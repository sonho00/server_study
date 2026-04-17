#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    std::cout << "Starting echo client..." << std::endl;

    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    std::cout << "Echo client started." << std::endl;

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Socket created successfully." << std::endl;

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(8080);
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to echo server." << std::endl;

    const char* message = "Hello, Echo Server!";

    while (true) {
        int bytesSent = send(clientSocket, message, strlen(message), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "Message sent to server: " << message << std::endl;

        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
        buffer[bytesReceived] = '\0';

        std::cout << "Message received from server: " << buffer << std::endl;

        Sleep(1000);
    }

    std::cout << "Closing connection..." << std::endl;

    closesocket(clientSocket);
    WSACleanup();

    std::cout << "Echo client stopped." << std::endl;

    return 0;
}
