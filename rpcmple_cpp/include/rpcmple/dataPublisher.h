// ******  rpcmple for c++ v0.1  ******
// Copyright (C) 2024 Carlo Seghi. All rights reserved.
// Author Carlo Seghi github.com/acs48.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation v3.0
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Library General Public License for more details.
//
// Use of this source code is governed by a GNU General Public License v3.0
// License that can be found in the LICENSE file.


#ifndef DATAPUBLISHER_H
#define DATAPUBLISHER_H

#include "messageManager.h"
#include "dataSignature.h"
#include "rpcmpleutility.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

class dataPublisher : public messageManager {
private:
    dataSignature mSignature;

    int maxMessageSize;

    std::mutex stackMtx;
    std::condition_variable cv;
    std::queue<std::vector<uint8_t>> messageStack;
    bool stopWait;

public:
    dataPublisher(int bufferSize, int messageSize, connectionManager* pConn, std::vector<char> signature)
    : messageManager(bufferSize, messageSize, pConn, true), mSignature(std::move(signature)) {
        maxMessageSize = messageSize;
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
            VERBOSE_PRINT("Publisher: locking connection resources and waiting for data" << std::endl);
            std::unique_lock<std::mutex> lock(stackMtx);
            if(messageStack.empty()) {
                cv.wait(lock, [this] { return (!this->messageStack.empty() || this->stopWait); });
            }
        }
        VERBOSE_PRINT("Publisher: new data to publish, running on thread " << std::this_thread::get_id() << std::endl);


        if (stopWait) {
            VERBOSE_PRINT("Publisher: detected stop request" << std::endl);
            retMessage.resize(0);
            return false;
        }

        stackMtx.lock();
        if (!messageStack.empty()) {
            stackMtx.unlock();
            if (stopWait) {
                VERBOSE_PRINT("Publisher: detected stop request" << std::endl);
                retMessage.resize(0);
                return false;
            }

            std::vector<uint8_t> stackMessage;
            stackMtx.lock();
            stackMessage = std::move(messageStack.front());
            messageStack.pop();
            stackMtx.unlock();

            if(stackMessage.size()+2>maxMessageSize) {
                std::wcerr << L"Publisher: message to write exceed max size" << std::endl;
                retMessage.resize(0);
                return false;
            }

            retMessage.resize(2+stackMessage.size());
            unsigned int offset = 0;

            uint16_t callSuccessInt = 1;
            uint16_t headerVal = callSuccessInt * 32768 + stackMessage.size();
            uint16ToBytes(headerVal,retMessage.data()+offset,true);
            offset +=2;

            std::copy(stackMessage.begin(),stackMessage.end(),retMessage.data()+offset);
            offset += stackMessage.size();

            retMessage.resize(offset);

        }


        if (stopWait) {
            VERBOSE_PRINT(L"Publisher: detected stop request" << std::endl);
            return false;
        }
        return true;
    }

    void stopParser() override {
        {
            VERBOSE_PRINT("Publisher was requested to stop. Locking resources and notifying stop" << std::endl);
            std::lock_guard<std::mutex> lock(stackMtx);
            stopWait = true;
        }
        cv.notify_all();
    }

    bool publish(rpcmpleVariantVector &data) {
        if (data.size() != mSignature.size()) {
            std::wcerr << L"Publisher error invalid number of arguments" << std::endl;
            return false;
        }

        std::vector<uint8_t> message(maxMessageSize);
        if (!mSignature.toBinary(data, message)) {
            std::wcerr << L"Publisher error translating variables to binary" << std::endl;
            return false;
        }

        {
            VERBOSE_PRINT("Publisher is locking resources and pushing new message" << std::endl);
            std::lock_guard<std::mutex> stackLock(stackMtx);
            messageStack.push(message);
        }

        cv.notify_all();
        return true;
    }
};




#endif //DATAPUBLISHER_H
