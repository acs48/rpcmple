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
#include "rpcmple/connectionManagersocketClient.h"
#include "rpcmple/rpcClient.h"

#include "spdlog/sinks/stdout_color_sinks.h"



int main(int argc, char** argv) {

    auto console = spdlog::stderr_color_mt("rpcmple_cpp_example4");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::info);

    auto* mConn = new connectionManagerSocketClient("127.0.0.1", 8088);
    if(!mConn->create()) return -1;

    auto* mClient = new rpcClient(mConn);
    mClient->appendSignature(new remoteProcedureSignature(L"Greet",{'s'},{'s'}));
    mClient->appendSignature(new remoteProcedureSignature(L"Sum",{'I'},{'i'}));
    mClient->appendSignature(new remoteProcedureSignature(L"Tell",{'i'},{}));

    mClient->startDataFlowNonBlocking();

    rpcmpleVariantVector arguments;
    rpcmpleVariantVector returns;
    arguments.emplace_back("Hello world from client!");
    mClient->call(L"Greet",arguments,returns);
    std::string strRet;
    getRpcmpleVariantValue(returns[0],&strRet);
    spdlog::info("Example4: client got greet: {}\n",strRet);

    arguments.clear();
    returns.clear();

    arguments.emplace_back(std::vector<int64_t>{1,2,3,4,5,6,7,8,9,10});
    mClient->call(L"Sum",arguments,returns);
    int64_t sum=0;
    getRpcmpleVariantValue(returns[0],&sum);
    spdlog::info("Example4: client got sum: {}\n",sum);

    arguments.clear();
    returns.clear();

    arguments.emplace_back(sum);
    mClient->call(L"Tell",arguments,returns);
    spdlog::info("Example4: client got tell\n");

    mClient->waitRPCComplete();

    delete mClient;
    delete mConn;
}