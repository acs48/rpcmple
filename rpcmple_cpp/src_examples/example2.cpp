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

#include "rpcmple/rpcmple.h"
#include "connectionmanager/stdInOutClient.h"
#include "rpcmple/rpcServer.h"

#include "spdlog/sinks/stdout_color_sinks.h"



int main(int argc, char** argv) {

    auto console = spdlog::stderr_color_mt("rpcmple_cpp_example2");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::info);

    auto* mConn = new rpcmple::connectionManager::stdInOutClient();
    if(!mConn->create()) return -1;

    auto* mServer = new rpcmple::rpcServer(mConn);
    mServer->appendSignature(new rpcmple::localProcedureSignature(L"Greet",{'s'},{'s'},[](rpcmple::variantVector &arguments, rpcmple::variantVector &returns) -> bool {
        spdlog::info("example2: rpc server received call to function greet");
        std::string strArg;

        int i = 0;
        if (!rpcmple::getVariantValue(arguments[i++], &strArg)) return false;

        std::string retStr;
        retStr.append("You said: '").append(strArg).append("'; Hello world to you too!");

        returns.emplace_back(retStr);

        return true;
    }));
    mServer->appendSignature(new rpcmple::localProcedureSignature(L"Sum",{'I'},{'i'},[](rpcmple::variantVector &arguments, rpcmple::variantVector &returns) -> bool {
        spdlog::info("example2: rpc server received call to function sum");
        std::vector<int64_t> intArrArg;

        int i = 0;
        if (!rpcmple::getVariantValue(arguments[i++], &intArrArg)) return false;

        int64_t retInt=0;
        for(auto j=0; j<intArrArg.size();j++) {
            retInt+=intArrArg[j];
        }

        returns.emplace_back(retInt);

        return true;
    }));


    mServer->startDataFlowBlocking();

    delete mServer;
    delete mConn;
}