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

#ifndef MESSAGEMANAGER_H
#define MESSAGEMANAGER_H

//#include "connectionmanager/base.h"
#include "rpcmple.h"

#include <cstdint>
#include <utility>
#include <vector>
#include <thread>
#include <functional>

/* messageManager is a pure virtual class manages the flow of data with another rpcmple on a different process.
 * An implementation of messageManager must override the following methods:
 *  - parseMessage: it gets called when a byte sequence of len returned by GetMessageLen is read from the Reader.
 *  User implementing this function shall decode the byte sequence and return true if successful. Returning false
 *  will stop the communication loop.
 *  - getMessageLen gets called prior to read, for the messageManager to know the len of the message. A full message
 *  can be of variable len and can be split in multiple sub-messages. getMessageLen must return the len of the sub-message
 *  - writeMessage: it gets called after a message or sub-message is read, and after the call to ParseMessage.
 *  It shall serialize to the Buffer argument the data to be sent to the other process.
 *  It can leave the buffer empty if no reply must be sent. Return false if an encoding error occur.
 *  Returning false will stop the communication loop.
 *  stopParser gets called by the MessageManager when the StopDataFlow is called or the other application
 *  is closing the connection. It can be used to clean up the parser.
 * Constructor requires size if local buffer, maximum size of a message, connection through which read and write,
 * indication if this process should send the first message
 */

namespace rpcmple
{
	class messageManager
	{
	private:
		bool isInitialized;

		bool isRequester;
		bool stopRequested;

		rpcmple::connectionManager::base* mConn;
		std::vector<uint8_t> readBuffer;
		std::vector<uint8_t> message;
		//uint32_t maxDatagramSize;
		uint32_t messageLastIdx;
		uint32_t messageLength;
		uint32_t messageMissingBytes;

		std::function<void()> onCloseCallback;

		void init()
		{
			readBuffer.resize(16777220);
			messageLastIdx = 0;
			messageLength = getMessageLen();
			message.resize(messageLength);
			messageMissingBytes = messageLength;
			isInitialized = true;
		}

		void dataFlow()
		{
			if (!isInitialized)
			{
				init();
			}

			if (isRequester)
			{
				spdlog::debug("messageManager: waiting to write first message");
				std::vector<uint8_t> message;
				if (!writeMessage(message))
				{
					spdlog::error("messageManager: error generating initial message; stopping flow");
					stopRequested = true;
				}
				else
				{
					if (!message.empty())
					{
						if (message.size() > 16777220)
						{
							spdlog::error("messageManager: initial message too large; stopping flow");
							stopRequested = true;
						}
						else
						{
							if (!mConn->write(message))
							{
								spdlog::error("messageManager: error sending initial message; stopping flow");
								stopRequested = true;
							}
						}
					}
				}
			}

			uint32_t bytesRead = 0;
			uint32_t bytesParsed = 0;

			while (!stopRequested)
			{
				spdlog::debug("messageManager: entering main data flow");
				bytesRead = 0;
				bytesParsed = 0;

				if (messageLength > 0)
				{
					//readBuffer.resize(messageMissingBytes);
					if (!mConn->read(readBuffer, &bytesRead))
					{
						spdlog::debug("messageManager: cannot read, stopping flow");
						break;
					}

					while (bytesParsed<bytesRead)
					{
						uint32_t transferredBytes = messageMissingBytes;
						if (bytesRead-bytesParsed < transferredBytes)
						{
							transferredBytes = bytesRead-bytesParsed;
						}
						std::copy(readBuffer.begin() + bytesParsed, readBuffer.begin() + bytesParsed + transferredBytes,
						          message.begin() + messageLastIdx);

						messageLastIdx += transferredBytes;
						messageMissingBytes -= transferredBytes;
						bytesParsed += transferredBytes;
						//fakeBuffer.erase(fakeBuffer.begin(), fakeBuffer.begin() + transferredBytes);

						if (messageMissingBytes == 0)
						{
							if (!parseMessage({message.begin(), message.begin() + messageLength}))
							{
								spdlog::error("messageManager: error parsing received message; stopping flow");
								stopRequested = true;
								break;
							}

							std::vector<uint8_t> replyMessage(1024);
							if (!writeMessage(replyMessage))
							{
								spdlog::error("messageManager: error generating reply message; stopping flow");
								stopRequested = true;
								break;
							}
							if (!replyMessage.empty())
							{
								if (message.size() > 16777220)
								{
									spdlog::error("messageManager: message too large; stopping flow");
									stopRequested = true;
									break;
								}
								if (!mConn->write(replyMessage))
								{
									spdlog::error("messageManager: error sending reply message; stopping flow");
									stopRequested = true;
									break;
								}
							}

							messageLastIdx = 0;
							messageLength = getMessageLen();
							message.resize(messageLength);
							messageMissingBytes = messageLength;
						}
					}
				}
				else
				{
					std::vector<uint8_t> message;
					if (!writeMessage(message))
					{
						spdlog::error("messageManager: error sending reply message; stopping flow");
						stopRequested = true;
						break;
					}
					if (!message.empty())
					{
						if (!mConn->write(message))
						{
							spdlog::error("messageManager: error sending reply message; stopping flow");
							stopRequested = true;
							break;
						}
					}

					messageLastIdx = 0;
					messageLength = getMessageLen();
					messageMissingBytes = messageLength;
				}
			}
			mConn->close();
			if (onCloseCallback) onCloseCallback();
			spdlog::warn("messageManager: flow stopped");
		}

	public:
		messageManager(rpcmple::connectionManager::base* pConn, bool requester)
		{
			isInitialized = false;
			stopRequested = false;

			isRequester = requester;
			mConn = pConn;
		}

		virtual ~messageManager() = default;

		virtual bool parseMessage(std::vector<uint8_t> message) =0;
		virtual int getMessageLen() =0;
		virtual bool writeMessage(std::vector<uint8_t>& message) =0;
		virtual void stopParser() =0;

		void startDataFlowNonBlocking(std::function<void()> onCloseCallback = nullptr)
		{
			this->onCloseCallback = std::move(onCloseCallback);

			std::thread t(&messageManager::dataFlow, this);
			t.detach();
		}

		void startDataFlowBlocking()
		{
			dataFlow();
		}

		void stopDataFlow()
		{
			stopRequested = true;
			stopParser();
		}
	};
}


#endif //MESSAGEMANAGER_H
