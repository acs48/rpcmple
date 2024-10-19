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

#include "connectionManagerSocket.h"
#include "rpcmpleServer.h"


class greetFunctionSignature : public localProcedureSignature {
private:
public:
    explicit greetFunctionSignature()
        : localProcedureSignature(L"Greet",{'s'},{'s'}) {}
    ~greetFunctionSignature() override = default;

    bool called(std::vector<rpcmpleVariant> &arguments, std::vector<rpcmpleVariant> &returns) override {
        std::wstring strArg;

        int i = 0;
        if (!getRpcmpleVariantValue(arguments[i++], &strArg)) return false;

        std::wstring retStr;
        retStr.append(L"You said: ").append(strArg).append(L" Hello world to you too!");

        returns.emplace_back(retStr);

        return true;
    }
};

class sumFunctionSignature : public localProcedureSignature {
private:
public:
    explicit sumFunctionSignature()
        : localProcedureSignature(L"Sum",{'I'},{'i'}) {}
    ~sumFunctionSignature() override = default;

    bool called(std::vector<rpcmpleVariant> &arguments, std::vector<rpcmpleVariant> &returns) override {
        std::vector<int64_t> intArrArg;

        int i = 0;
        if (!getRpcmpleVariantValue(arguments[i++], &intArrArg)) return false;

        int64_t retInt=0;
        for(auto j=0; j<intArrArg.size();j++) {
            retInt+=intArrArg[j];
        }

        returns.emplace_back(retInt);

        return true;
    }
};



int main(int argc, char** argv) {
    auto* mConn = new connectionManagerSocketClient("127.0.0.1",8080);
    if(!mConn->create()) return -1;

    auto* mServer = new rpcServer(1024,1024,mConn);
    mServer->appendSignature(new greetFunctionSignature);
    mServer->appendSignature(new sumFunctionSignature);


    mServer->startDataFlowBlocking();

    delete mServer;
    delete mConn;
}