// ******  rpcmple for c++ v0.2  ******
// Copyright (C) 2024 Carlo Seghi. All rights reserved.
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

#ifndef CONNECTIONMANAGERTCPSOCKETCLIENT_H
#define CONNECTIONMANAGERTCPSOCKETCLIENT_H

#include "connectionmanager/base.h"
#include "rpcmple/rpcmple.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <utility>
#include <vector>

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

namespace rpcmple
{
	namespace connectionManager
	{
		/* tcpSocketClient implements connectionManager::base on tcp as dialer using Microsoft WinSock */
		class tcpSocketClient : public base
		{
		private:
			SOCKET connectSocket;
			std::string serverAddress;
			int serverPort;

		public:
			tcpSocketClient(std::string address, int port) : connectSocket(INVALID_SOCKET),
			                                                               serverAddress(std::move(address)),
			                                                               serverPort(port)
			{
			}

			~tcpSocketClient() override
			{
				tcpSocketClient::close();
			}

			bool create() override
			{
				WSADATA wsaData;
				int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
				if (result != 0)
				{
					spdlog::error("SocketClient: WSAStartup failed with error: {}", result);
					return false;
				}

				connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (connectSocket == INVALID_SOCKET)
				{
					spdlog::error("SocketClient: error creating socket: {}", WSAGetLastError());
					WSACleanup();
					return false;
				}

				sockaddr_in serverAddr{};
				serverAddr.sin_family = AF_INET;
				serverAddr.sin_port = htons(serverPort);

				if (inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr) <= 0)
				{
					spdlog::error("SocketServer: inet_pton failed with error: {}", WSAGetLastError());
					closesocket(connectSocket);
					WSACleanup();
					return false;
				}

				result = connect(connectSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
				if (result == SOCKET_ERROR)
				{
					spdlog::error("SocketClient: connect failed with error: {}", WSAGetLastError());
					closesocket(connectSocket);
					WSACleanup();
					return false;
				}

				return true;
			}

			bool write(std::vector<uint8_t>& bytes) override
			{
				int totalBytesSent = 0;
				unsigned int bytesLeft = bytes.size();

				while (totalBytesSent < bytes.size())
				{
					int bytesSent = send(connectSocket, (const char*)bytes.data() + totalBytesSent, bytesLeft, 0);
					if (bytesSent == SOCKET_ERROR)
					{
						spdlog::error("SocketClient: send failed with error: {}", WSAGetLastError());
						return false;
					}
					totalBytesSent += bytesSent;
					bytesLeft -= bytesSent;
				}

				return true;
			}

			bool read(std::vector<uint8_t>& bytes, uint32_t* pBytesRead) override
			{
				int bytesReceived = recv(connectSocket, (char*)bytes.data(), bytes.size(), 0);
				if (bytesReceived == SOCKET_ERROR)
				{
					spdlog::error("SocketClient: recv failed with error: {}", WSAGetLastError());
					return false;
				}
				if (bytesReceived == 0)
				{
					return false;
				}
				*pBytesRead = bytesReceived;
				return true;
			}

			bool close() override
			{
				if (connectSocket != INVALID_SOCKET)
				{
					closesocket(connectSocket);
					connectSocket = INVALID_SOCKET;
					WSACleanup();
				}
				return true;
			}
		};
	}
}

#endif // CONNECTIONMANAGERTCPSOCKETCLIENT_H
