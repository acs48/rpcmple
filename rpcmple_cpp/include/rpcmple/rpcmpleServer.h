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

#ifndef RPCMANAGER_H
#define RPCMANAGER_H

#include "connectionManagerBase.h"
#include "messageManager.h"
#include "dataSignature.h"
#include "rpcmple.h"

#include <string>
#include <utility>
#include <cstdint>
#include <codecvt>

/* Class localProcedureSignature is a pure virtual class defining a procedure on local RPC server which can ce called
 * from another process.
 * Constructor requires types for arguments and returns, as vector of char. The following data types are supported:
 *   - 'i' for int64
 *   - 'I' for array of int64
 *   - 'u' for uint64
 *   - 'U' for array of uint64
 *   - 'd' for double precision floating point number (64bit)
 *   - 'D' for array of double
 *   - 's' for UTF-8 encoded string
 *   - 'S' for array of UTF-8 encoded string
 *   - 'u' for variant, which can be any of the above
 * Implementation requires to override called method, where the custom implementation of the procedure resides
 */
class localProcedureSignature {
public:
    uint16_t id;

    std::wstring procedureName;
    dataSignature args;
    dataSignature rets;

    localProcedureSignature(std::wstring name, std::vector<char> arguments, std::vector<char> returns)
        : procedureName(std::move(name)), args(std::move(arguments)), rets(std::move(returns)) { id = 0; }

    virtual ~localProcedureSignature() = default;

    virtual bool called(rpcmpleVariantVector &arguments, rpcmpleVariantVector &returns) =0;
};

/* Class rpcServer implements messageManager for the rpc protocol.
 * Procedures must be added using the appendSignature method, in the same order as they are entered on the other process
 */
class rpcServer : public messageManager {
private:
    std::vector<localProcedureSignature *> localProcedures;
    uint16_t procedureID;

    bool callSuccess;
    std::vector<uint8_t> callReturnsSerialized;

    uint16_t sectionLen;
    uint16_t sectionID;

    bool call(std::vector<uint8_t> &message) {
        if (procedureID < localProcedures.size()) {
            spdlog::debug("rpcServer: requested call to procedure {} {}", procedureID, wstring_to_utf8(localProcedures[procedureID]->procedureName));
            localProcedureSignature *pProc = localProcedures[procedureID];

            rpcmpleVariantVector args(pProc->args.size());
            pProc->args.fromBinary(message, args);

            rpcmpleVariantVector rets;
            if (!pProc->called(args, rets)) {
                spdlog::error("Error calling RPC procedure {} {}", procedureID ,wstring_to_utf8(localProcedures[procedureID]->procedureName));
                return false;
            }

            if (rets.size() != pProc->rets.size()) {
                callReturnsSerialized.clear();
                callSuccess = false;
                spdlog::error("rpcServer: procedure returned wrong number of variables");
                return false;
            }

            callSuccess = true;
            //callReturnsSerialized.resize(maxMessageSize);

            pProc->rets.toBinary(rets, callReturnsSerialized);
        } else {
            spdlog::error("rpcServer: invalid requested RPC procedure ID {}" , procedureID);
            return false;
        }
        return true;
    }

public:
    explicit rpcServer(connectionManager *pConn)
        : messageManager(pConn, false) {
        localProcedures.clear();
        procedureID = -1;
        callSuccess = false;

        sectionID = 0;
        sectionLen = 4;

        //maxMessageSize = messageSize;
    }

    ~rpcServer() override = default;

    void appendSignature(localProcedureSignature *signature) {
        signature->id = localProcedures.size();;
        localProcedures.push_back(signature);
    }


    bool parseMessage(std::vector<uint8_t> message) override {
        spdlog::debug("rpcServer: parsing message");
        switch (sectionID) {
            case 0: {
                uint32_t mVal = bytesToUint32(message.data(), true);
                procedureID = mVal / 65536;
                sectionLen = mVal % 65536;
                if (sectionLen > 0) {
                    sectionID = 1;
                } else {
                    std::vector<uint8_t> noV(0);
                    if (!this->call(noV)) {
                        spdlog::error("rpcServer: error calling RPC procedure {} with 0 arguments", procedureID);
                        return false;
                    }
                    sectionID = 0;
                    sectionLen = 4;
                }
                break;
            }
            case 1: {
                if (!this->call(message)) {
                    spdlog::error("rpcServer: error calling RPC procedure {} with 0 arguments", procedureID);
                    return false;
                }
                sectionID = 0;
                sectionLen = 4;
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

    bool writeMessage(std::vector<uint8_t> &retMessage) override {
        if (sectionID != 0) {
            retMessage.resize(0);
            return true;
        }

        uint16_t callSuccessInt = 0;
        if (callSuccess) callSuccessInt = 1;

        retMessage.resize(2+callReturnsSerialized.size());
        unsigned int offset = 0;

        uint16_t headerVal = callSuccessInt * 32768 + callReturnsSerialized.size();
        uint16ToBytes(headerVal, retMessage.data()+offset, true);
        offset += 2;

        std::copy(callReturnsSerialized.begin(),callReturnsSerialized.end(),retMessage.data()+offset);
        offset += callReturnsSerialized.size();

        retMessage.resize(offset);

        return true;
    }

    void stopParser() override {
    };
};

#endif //RPCMANAGER_H
