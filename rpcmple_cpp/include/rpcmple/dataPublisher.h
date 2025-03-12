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


#ifndef DATAPUBLISHER_H
#define DATAPUBLISHER_H

#include "messageManager.h"
#include "dataSignature.h"
#include "rpcmple.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>

namespace rpcmple
{
	class dataPublisher : public messageManager
	{
	private:
		dataSignature mSignature;

		std::mutex stackMtx;
		std::condition_variable cv;
		std::queue<std::vector<uint8_t>> messageStack;
		bool stopWait;
		bool groupMessages;

	public:
		dataPublisher(rpcmple::connectionManager::base* pConn, std::vector<char> signature, bool groupMessages = false)
			: messageManager(pConn, true), mSignature(std::move(signature)), groupMessages(groupMessages)
		{
			stopWait = false;
		}

		~dataPublisher() override
		{
			stopDataFlow();
		};

		bool parseMessage(std::vector<uint8_t> message) override
		{
			return true;
		}

		int getMessageLen() override { return 0; }

		bool writeMessage(std::vector<uint8_t>& retMessage) override
		{
			{
				spdlog::debug("Publisher: locking connection resources and waiting for data");
				std::unique_lock<std::mutex> lock(stackMtx);
				if (messageStack.empty())
				{
					cv.wait(lock, [this] { return (!this->messageStack.empty() || this->stopWait); });
				}
			}
			std::thread::id tid = std::this_thread::get_id();
			auto hid = std::hash<std::thread::id>{}(tid);
			spdlog::debug("Publisher: new data to publish, running on thread {}", hid);

			if (stopWait)
			{
				spdlog::debug("Publisher: detected stop request");
				retMessage.resize(0);
				return false;
			}

			{
				std::lock_guard<std::mutex> lock(stackMtx);

				unsigned int offset = 0;

				bool grouping = true;

				while (grouping && !messageStack.empty() && retMessage.size() < 1024)
				{
					grouping = !groupMessages;
					std::vector<uint8_t> stackMessage;

					stackMessage = std::move(messageStack.front());
					messageStack.pop();

					if (stackMessage.size() > 16777216)
					{
						spdlog::error("message size {} exceeding max allowed size 16777216", stackMessage.size());
						return false;
					}

					retMessage.resize(offset + 4 + stackMessage.size());

					uint32_t callSuccessInt = 1;
					uint32_t headerVal = callSuccessInt * 16777216 + stackMessage.size();
					uint32ToBytes(headerVal, retMessage.data() + offset, true);
					offset += 4;

					std::copy(stackMessage.begin(), stackMessage.end(), retMessage.data() + offset);
					offset += stackMessage.size();
				}
			}
			cv.notify_all();

			return true;
		}

		void stopParser() override
		{
			{
				spdlog::debug("Publisher was requested to stop. Locking resources and notifying stop");
				std::lock_guard<std::mutex> lock(stackMtx);
				stopWait = true;
			}
			cv.notify_all();
		}

		bool publish(variantVector& data)
		{
			if (data.size() != mSignature.size())
			{
				spdlog::error("publisher: invalid number of arguments");
				return false;
			}

			std::vector<uint8_t> message(1024);
			if (!mSignature.toBinary(data, message))
			{
				spdlog::error("publisher: error translating variables to binary");
				return false;
			}

			{
				spdlog::debug("Publisher is locking resources and pushing new message");
				std::lock_guard<std::mutex> stackLock(stackMtx);
				messageStack.push(message);
			}

			cv.notify_all();
			return true;
		}

		void waitPublishComplete()
		{
			{
				spdlog::debug("publisher: waiting until all messages are published");
				std::unique_lock<std::mutex> lock(stackMtx);
				if (messageStack.empty()) return;
				cv.wait(lock, [this] { return this->messageStack.empty() || this->stopWait; });
			}
		}
	};
}
#endif //DATAPUBLISHER_H
