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

class dataPublisher : public messageManager {
private:
    dataSignature mSignature;

    std::mutex stackMtx;
    std::condition_variable cv;
    std::queue<std::vector<uint8_t>> messageStack;
    bool stopWait;

public:
    dataPublisher(connectionManager* pConn, std::vector<char> signature)
    : messageManager(pConn, true), mSignature(std::move(signature)) {
        stopWait = false;
    }
    ~dataPublisher() override {
        stopDataFlow();
    };

    bool parseMessage(std::vector<uint8_t> message) override {
        return true;
    }

    int getMessageLen() override {return 0;}

    bool writeMessage(std::vector<uint8_t> &retMessage) override {
        {
            spdlog::debug("Publisher: locking connection resources and waiting for data");
            std::unique_lock<std::mutex> lock(stackMtx);
            if(messageStack.empty()) {
                cv.wait(lock, [this] { return (!this->messageStack.empty() || this->stopWait); });
            }
        }
        std::thread::id tid = std::this_thread::get_id();
        auto hid = std::hash<std::thread::id>{}(tid);
        spdlog::debug("Publisher: new data to publish, running on thread {}", hid);

        if (stopWait) {
            spdlog::debug("Publisher: detected stop request");
            retMessage.resize(0);
            return false;
        }

        unsigned int offset = 0;
        stackMtx.lock();
        while (!messageStack.empty() && retMessage.size()<1024) {
            stackMtx.unlock();
            if (stopWait) {
                spdlog::debug("Publisher: detected stop request");
                retMessage.resize(0);
                return false;
            }

            std::vector<uint8_t> stackMessage;
            stackMtx.lock();
            stackMessage = std::move(messageStack.front());
            messageStack.pop();
            stackMtx.unlock();

            retMessage.resize(offset+2+stackMessage.size());

            uint16_t callSuccessInt = 1;
            uint16_t headerVal = callSuccessInt * 32768 + stackMessage.size();
            uint16ToBytes(headerVal,retMessage.data()+offset,true);
            offset +=2;

            std::copy(stackMessage.begin(),stackMessage.end(),retMessage.data()+offset);
            offset += stackMessage.size();

            stackMtx.lock();
        }
        stackMtx.unlock();
        cv.notify_all();

        if (stopWait) {
            spdlog::debug("Publisher: detected stop request");
            return false;
        }
        return true;
    }

    void stopParser() override {
        {
            spdlog::debug("Publisher was requested to stop. Locking resources and notifying stop");
            std::lock_guard<std::mutex> lock(stackMtx);
            stopWait = true;
        }
        cv.notify_all();
    }

    bool publish(rpcmpleVariantVector &data) {
        if (data.size() != mSignature.size()) {
            spdlog::error("publisher: invalid number of arguments");
            return false;
        }

        std::vector<uint8_t> message(1024);
        if (!mSignature.toBinary(data, message)) {
            spdlog::error("publisher: error translating variables to binary" );
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

    void waitPublishComplete() {
        {
            spdlog::debug("Publisher: waiting until all messages are published");
            std::unique_lock<std::mutex> lock(stackMtx);
            if(messageStack.empty()) return;
            cv.wait(lock, [this] {return this->messageStack.empty();});
        }
    }
};




#endif //DATAPUBLISHER_H
