// ******  rpcmple for c++ v0.2  ******
// Copyright (C) 2025 Carlo Seghi. All rights reserved.
// Author Carlo Seghi github.com/acs48.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the MIT license
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Library General Public License for more details.
//
// Use of this source code is governed by the MIT license
// License that can be found in the LICENSE file.

#ifndef CONNECTIONMANAGERSOCKETSERVER_H
#define CONNECTIONMANAGERSOCKETSERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include "connectionManagerBase.h"

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

namespace rpcmple {
    inline SOCKET startTCPServer(int port) {
        SOCKET listenSocket = INVALID_SOCKET;
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            spdlog::error("SocketServer: WSAStartup failed with error: {}",result);
            return listenSocket;
        }

        // Create a socket for listening
        listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET)
        {
            spdlog::error("SocketServer: error creating socket: {}",WSAGetLastError());
            WSACleanup();
            return listenSocket;
        }

        // Bind the socket to the port
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        result = bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
        if (result == SOCKET_ERROR)
        {
            spdlog::error("SocketServer: bind failed with error: {}", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return listenSocket;
        }

        // Start listening for incoming connections
        result = listen(listenSocket, SOMAXCONN);
        if (result == SOCKET_ERROR)
        {
            spdlog::error("SocketServer: listen failed with error: {}", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return listenSocket;
        }

        spdlog::info("SocketServer: server listening on port {}", port);
        return listenSocket;
    }

    inline void stopTCPServer(SOCKET listenSocket) {
        if (listenSocket != INVALID_SOCKET)
        {
            closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
            WSACleanup();
        }
    }
}


/* connectionManagerSocketServer implements connectionManager and acts as a TCP server */
class connectionManagerSocketServer : public connectionManager {
private:
    SOCKET listenSocket;
    SOCKET clientSocket;
    bool readyToAccept;

public:
    explicit connectionManagerSocketServer(SOCKET socket)
        : listenSocket(socket), clientSocket(INVALID_SOCKET), readyToAccept(false)
    {
    }

    ~connectionManagerSocketServer() override {
        connectionManagerSocketServer::close();
    }

    // Accepts a new connection from a client
    bool create() override{
        clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            spdlog::error("SocketServer: accept failed with error: {}", WSAGetLastError());
            return false;
        }

        spdlog::info("Client connected");
        return true;
    }

    // Writes data to the connected client
    bool write(std::vector<uint8_t>& bytes) override {
        if (clientSocket == INVALID_SOCKET) {
            spdlog::error("SocketServer: no client is connected");
            return false;
        }

        int totalBytesSent = 0;
        unsigned int bytesLeft = bytes.size();

        while (totalBytesSent < bytes.size()) {
            int bytesSent = send(clientSocket, (const char*)bytes.data() + totalBytesSent, bytesLeft, 0);
            if (bytesSent == SOCKET_ERROR) {
                spdlog::error("SocketServer: send failed with error: {}", WSAGetLastError());
                return false;
            }
            totalBytesSent += bytesSent;
            bytesLeft -= bytesSent;
        }

        return true;
    }

    // Reads data from the connected client
    bool read(std::vector<uint8_t>& bytes, uint32_t* pBytesRead) override {
        if (clientSocket == INVALID_SOCKET) {
            spdlog::error("SocketServer: no client is connected");
            return false;
        }

        int bytesReceived = recv(clientSocket, (char*)bytes.data(), bytes.size(), 0);
        if (bytesReceived == SOCKET_ERROR) {
            spdlog::error("SocketServer: receive failed with error: {}", WSAGetLastError());
            return false;
        }
        if (bytesReceived == 0) {
            spdlog::info("SocketServer: client has disconnected");
            return false;
        }

        *pBytesRead = bytesReceived;
        return true;
    }

    // Closes the sockets and cleans up
    bool close() override {
        if (clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
        }


        return true;
    }
};

#endif // CONNECTIONMANAGERSOCKETSERVER_H