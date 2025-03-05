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

#ifndef RPCCLIENT_H
#define RPCCLIENT_H


#include "connectionManagerBase.h"
#include "messageManager.h"
#include "dataSignature.h"
#include "rpcmple.h"

#include  "spdlog/spdlog.h"

#include <string>

class remoteProcedureSignature {
private:
public:
    uint32_t id;

    std::wstring procedureName;
    dataSignature args;
    dataSignature rets;

    remoteProcedureSignature(std::wstring name, std::vector<char> arguments, std::vector<char> returns)
    : procedureName(std::move(name)), args(std::move(arguments)), rets(std::move(returns)), id(0) {};
    virtual ~remoteProcedureSignature() = default;

    bool call(rpcmpleVariantVector &arguments, rpcmpleVariantVector &returns) {
      return false;
    }
};

/* Class rpcClient implements messageManager for the rpc protocol.
 * Procedures must be added using the appendSignature method, in the same order as they are entered on the other process
 */
class rpcClient : public messageManager {
private:
    std::vector<remoteProcedureSignature *> remoteProcedures;
    std::map<std::wstring, uint32_t> remoteProceduresMap;

    std::mutex mtx;
    std::condition_variable cv;
	bool canCall;
    bool sendIsReady;
    bool replIsReady;

    uint32_t procedureID;
    std::vector<uint8_t> args;
    uint32_t callSuccessInt;
    std::vector<uint8_t> rets;

    uint32_t sectionLen;
    uint16_t sectionID;

    bool stopWait;
public:
    explicit rpcClient(connectionManager *pConn)
      : messageManager(pConn, true) {
        remoteProcedures.clear();
        procedureID = 0;
		canCall = true;
        replIsReady = false;
        sendIsReady = false;
        callSuccessInt = 0;

        sectionID = 0;
        sectionLen = 4;

        stopWait = false;

    }

    ~rpcClient() override {
        stopDataFlow();
        joinMe();
        for(auto &p : remoteProcedures) {
            delete p;
        }
        remoteProcedures.clear();
        remoteProceduresMap.clear();
    }

    void appendSignature(remoteProcedureSignature *signature) {
        signature->id = remoteProcedures.size();
        remoteProcedures.push_back(signature);
        remoteProceduresMap[signature->procedureName] = signature->id;
    }

    bool call(uint32_t rpId, rpcmpleVariantVector &arguments, rpcmpleVariantVector &returns) {
        auto *proc = remoteProcedures[rpId];

        if (arguments.size() != proc->args.size()) {
            spdlog::error("rpcClient: invalid number of arguments");
            return false;
        }

        {
            spdlog::debug("rpcClient: locking connection resources and waiting for rpc ready");
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return (this->canCall || this->stopWait); });
        }

		if (stopWait) {
            spdlog::info("rpcClient: processing stop request");
			return false;
		}

        {
            spdlog::debug("rpcClient is locking resources and pushing new message");
            std::lock_guard<std::mutex> stackLock(mtx);

            if (!proc->args.toBinary(arguments, args)) {
                spdlog::error("rpcClient: error translating variables to binary" );
                return false;
            }
            procedureID = rpId;
			canCall = false;
            sendIsReady = true;
        }

        cv.notify_all();

        {
            spdlog::debug("rpcClient: locking connection resources and waiting for reply");
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return (this->replIsReady || this->stopWait); });
        }

        {
            std::lock_guard<std::mutex> lock(mtx);

            replIsReady = false;
            canCall = true;

            if (!proc->rets.fromBinary(rets, returns)) {
                spdlog::error("rpcClient: error translating variables to binary" );
                return false;
            }

            if (returns.size() != proc->rets.size()) {
                spdlog::error("rpcClient: invalid number of arguments");
                return false;
            }
        }
        cv.notify_all();

        return true;
    }


    bool call(std::wstring name, rpcmpleVariantVector &arguments, rpcmpleVariantVector &returns) {
        if(remoteProceduresMap.count(name) == 0) {
            spdlog::error("rpcClient: remote procedure name {} not found", "name");
            return false;
        }
        uint32_t id = remoteProceduresMap[name];

        return call(id,arguments,returns);
    }


    bool writeMessage(std::vector<uint8_t> &message) override {
        if (sectionID != 0) {
            message.resize(0);
            return true;
        }

        {
            spdlog::debug("rpcClient: locking connection resources and waiting for call");
            std::unique_lock<std::mutex> lock(mtx);cv.wait(lock, [this] { return (this->sendIsReady || this->stopWait); });
        }

        std::thread::id tid = std::this_thread::get_id();
        auto hid = std::hash<std::thread::id>{}(tid);
        spdlog::debug("rpcClient: new data to publish, running on thread {}", hid);

        if (stopWait) {
            spdlog::debug("rpcClient: detected stop request");
            message.resize(0);
            return false;
        }

        unsigned int offset = 0;

        {
            std::lock_guard<std::mutex> lock(mtx);
            if(args.size()>16777216) {
                spdlog::error("rpcClient: message size {} exceeding max allowed size 16777216", message.size());
                return false;
            }
            message.resize(offset+4+args.size());

            uint32_t headerVal = procedureID * 16777216 + args.size();
            uint32ToBytes(headerVal,message.data()+offset,true);
            offset +=4;
            std::copy(args.begin(),args.end(),message.data()+offset);

            sendIsReady = false;
        }

        cv.notify_all();

        if (stopWait) {
            spdlog::debug("rpcClient: detected stop request");
            return false;
        }

        return true;
    }

    bool parseMessage(std::vector<uint8_t> message) override {
        spdlog::debug("rpcClient: parsing message");
        switch (sectionID) {
        case 0: {
                uint32_t mVal = bytesToUint32(message.data(), true);
                callSuccessInt = mVal / 16777216;
                sectionLen = mVal % 16777216;
                if (sectionLen > 0) {
                    sectionID = 1;
                } else {
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        rets.resize(0);
                        replIsReady = true;
                    }
                    sectionID = 0;
                    sectionLen = 4;
                    cv.notify_all();
                }
                break;
        }
        case 1: {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    rets.resize(message.size());
                    std::copy(message.begin(),message.end(),rets.data());
                    replIsReady = true;
                }
                sectionID = 0;
                sectionLen = 4;
                cv.notify_all();
                break;
        }
        default: {
                spdlog::error("rpcServer: error in RPC message parsing: invalid message section index");
                return false;
        }
        }
        return true;
    }

    int getMessageLen() override { return sectionLen; }

    void stopParser() override {
        {
            spdlog::debug("rpcClient was requested to stop. Locking resources and notifying stop");
            std::lock_guard<std::mutex> lock(mtx);
            stopWait = true;
        }
        cv.notify_all();
    }

    void waitRPCComplete() {
        {
            spdlog::debug("rpcClient: waiting until all calls are complete");
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] {return this->canCall || this->stopWait;});
        }
    }

};


#endif //RPCCLIENT_H
