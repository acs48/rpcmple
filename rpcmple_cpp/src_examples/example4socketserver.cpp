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

#include "rpcmple/rpcmple.h"
#include "rpcmple/connectionManagersocketServer.h"
#include "rpcmple/rpcServer.h"

#include "spdlog/sinks/stdout_color_sinks.h"

int main(int argc, char** argv) {

    auto console = spdlog::stderr_color_mt("rpcmple_cpp_example4");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::info);

    auto sServer = rpcmple::startTCPServer(8088);

    while(sServer) {
        auto* mConn = new connectionManagerSocketServer(sServer);

        auto* mServer = new rpcServer(mConn);
        mServer->appendSignature(new localProcedureSignature(L"Greet",{'s'},{'s'},[](rpcmpleVariantVector &arguments, rpcmpleVariantVector &returns) -> bool {
            spdlog::info("example4: rpc server received call to function greet");
            std::string strArg;

            int i = 0;
            if (!getRpcmpleVariantValue(arguments[i++], &strArg)) return false;

            std::string retStr;
            retStr.append("You said: '").append(strArg).append("'; Hello world to you too!");

            returns.emplace_back(retStr);

            return true;
        }));
        mServer->appendSignature(new localProcedureSignature(L"Sum",{'I'},{'i'},[](rpcmpleVariantVector &arguments, rpcmpleVariantVector &returns) -> bool {
            spdlog::info("example4: rpc server received call to function sum");
            std::vector<int64_t> intArrArg;

            int i = 0;
            if (!getRpcmpleVariantValue(arguments[i++], &intArrArg)) return false;

            int64_t retInt=0;
            for(auto j=0; j<intArrArg.size();j++) {
                retInt+=intArrArg[j];
            }

            returns.emplace_back(retInt);

            return true;
        }));
        mServer->appendSignature(new localProcedureSignature(L"Tell",{'i'},{},[](rpcmpleVariantVector &arguments, rpcmpleVariantVector &returns) -> bool {
            spdlog::info("example4: rpc server received call to function Tell");
            int64_t intArg;

            int i = 0;
            if (!getRpcmpleVariantValue(arguments[i++], &intArg)) return false;

            return true;
        }));

        if(mConn->create()) mServer->startDataFlowNonBlocking([mConn, mServer]() -> void
            {
                delete mConn;
                delete mServer;
            }
        );
    }
}