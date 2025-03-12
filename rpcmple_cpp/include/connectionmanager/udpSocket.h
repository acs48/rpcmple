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

#ifndef CONNECTIONMANAGERUDPSOCKET_H
#define CONNECTIONMANAGERUDPSOCKET_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include "connectionmanager/base.h"

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

namespace rpcmple
{
	namespace connectionManager
	{

		/* udpSocket implements connectionManager::base and acts as a UDP server/client */
		class udpSocket : public base
		{
		private:
           	int listenPort;
			SOCKET mSocket;

            sockaddr_in clientAddr;
            int clientPort;

			std::vector<uint8_t> internalBuffer;
			int internalBufferStartPos;
			int internalBufferEndPos;

		public:
			explicit udpSocket(int listenPort = -1, int clientPort = -1, std::string clientAddress = "")
				:  listenPort(listenPort), mSocket(INVALID_SOCKET), clientPort(clientPort), clientAddr({}), internalBufferStartPos(0), internalBufferEndPos(0)
			{
				internalBuffer.resize(65507);

				if(clientPort != -1) {
					clientAddr.sin_family = AF_INET;
					clientAddr.sin_port = htons(clientPort);
					inet_pton(AF_INET, clientAddress.c_str(), &clientAddr.sin_addr);
				}
			}

			~udpSocket() override
			{
				if (mSocket != INVALID_SOCKET) close();
			}

			// Accepts a new connection from a client
			bool create() override
			{
				WSADATA wsaData;
				int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
				if (result != 0)
				{
					spdlog::error("SocketServer: WSAStartup failed with error: {}", result);
					return false;
				}

				// Create a socket for listening
				mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				if (mSocket == INVALID_SOCKET)
				{
					spdlog::error("SocketServer: error creating socket: {}", WSAGetLastError());
					WSACleanup();
					return false;
				}

				// Bind the socket to the port
                if(listenPort != -1) {
                	sockaddr_in serverAddr{};
                	serverAddr.sin_family = AF_INET;
                	serverAddr.sin_addr.s_addr = INADDR_ANY;
                	serverAddr.sin_port = htons(listenPort);

                	result = bind(mSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
                	if (result == SOCKET_ERROR)
                	{
                		spdlog::error("udpSocket: bind failed with error: {}", WSAGetLastError());
                		closesocket(mSocket);
                		WSACleanup();
                		return false;
                	}
                	spdlog::info("udpSocket: configured as server on port {}", listenPort);
                }

                spdlog::info("udpSocket: ready");

				return true;
			}

			// Writes data to the connected client
			bool write(std::vector<uint8_t>& bytes) override
			{
				if (mSocket == INVALID_SOCKET)
				{
					spdlog::error("udpSocket: no open connection");
					return false;
				}

                if(!clientAddr.sin_family)
                {
                	spdlog::error("udpSocket: unknown destination address (nor previous connected client, nor specified server, nor broadcast)");
					return false;
                }

				int totalBytesSent = 0;
				unsigned int bytesLeft = bytes.size();

				while (totalBytesSent < bytes.size())
				{
					int bytesSent = sendto(mSocket, (const char*)bytes.data() + totalBytesSent, bytesLeft, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
					if (bytesSent == SOCKET_ERROR)
					{
						spdlog::error("SocketServer: send failed with error: {}", WSAGetLastError());
						return false;
					}
					totalBytesSent += bytesSent;
					bytesLeft -= bytesSent;
				}

				return true;
			}

			// Reads data from the connected client
			bool read(std::vector<uint8_t>& bytes, uint32_t* pBytesRead) override
			{
				if(internalBufferStartPos == internalBufferEndPos) {
					internalBufferStartPos = 0;
					internalBufferEndPos = 0;

					if (mSocket == INVALID_SOCKET)
					{
						spdlog::error("udpSocket: socket is not open");
						return false;
					}

					int clientAddrSize = sizeof(clientAddr);
					int bytesReceived = recvfrom(mSocket,  (char*)internalBuffer.data(), internalBuffer.size(), 0, (sockaddr*)&clientAddr, &clientAddrSize);
					internalBufferEndPos = bytesReceived;

					if (bytesReceived == SOCKET_ERROR)
					{
						spdlog::error("udpSocket: receive failed with error: {}", WSAGetLastError());
						return false;
					}
					if (bytesReceived == 0)
					{
						spdlog::info("udpSocket: client has disconnected");
						return false;
					}
				}

				int bytesInInternalBuffer = internalBufferEndPos - internalBufferStartPos;

				if(bytes.size() <= bytesInInternalBuffer) {
					std::copy(internalBuffer.begin() + internalBufferStartPos, internalBuffer.begin() + internalBufferStartPos + bytes.size(), bytes.begin());
					internalBufferStartPos += bytes.size();
					*pBytesRead = bytes.size();
					return true;
				} else {
					std::copy(internalBuffer.begin() + internalBufferStartPos, internalBuffer.begin() + internalBufferEndPos, bytes.begin());
					internalBufferStartPos = 0;
					internalBufferEndPos = 0;
					*pBytesRead = bytesInInternalBuffer;
					return true;
				}

				return true;
			}

			// Closes the sockets and cleans up
			bool close() override
			{
				if (mSocket != INVALID_SOCKET)
				{
					closesocket(mSocket);
					mSocket = INVALID_SOCKET;
					WSACleanup();
				}

				return true;
			}

            bool enableBroadcast(std::string broadcastAddress, int broadcastPort)
            {
              	clientAddr.sin_family = AF_INET;
				clientAddr.sin_port = htons(broadcastPort);					// Target UDP port
                if(broadcastAddress == "") clientAddr.sin_addr.s_addr = INADDR_BROADCAST;
                else {
                    if (inet_pton(AF_INET, broadcastAddress.c_str(), &clientAddr.sin_addr) <= 0) {
   						spdlog::error("Invalid address / Address not supported!");
    					return false;
					}
                }
                //clientAddr.sin_addr.s_addr = inet_addr(broadcastAddress.c_str());	// Broadcast address
                return true;
			}

            bool enableMulticast(std::string broadcastAddress, int broadcastPort)
            {
                clientAddr = sockaddr_in{};
				spdlog::error("Multicast system: todo");
                return false;
			}
		};
	}
}


#endif // CONNECTIONMANAGERUDPSOCKET_H
