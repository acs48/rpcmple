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
#include "rpcmple/connectionManagerSocketClient.h"
#include "rpcmple/rpcServer.h"


#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


class greetFunctionSignature : public localProcedureSignature {
private:
public:
	explicit greetFunctionSignature()
		: localProcedureSignature(L"Greet", {'s'}, {'s'}) {
	}

	~greetFunctionSignature() override = default;

	bool called(std::vector<rpcmpleVariant> &arguments, std::vector<rpcmpleVariant> &returns) override {
		spdlog::info("example1: rpc server received call to function greet");
		std::string strArg;

		int i = 0;
		if (!getRpcmpleVariantValue(arguments[i++], &strArg)) return false;

		std::string retStr;
		retStr.append("You said: '").append(strArg).append("'; Hello world to you too!");

		returns.emplace_back(retStr);

		return true;
	}
};

class sumFunctionSignature : public localProcedureSignature {
private:
public:
	explicit sumFunctionSignature()
		: localProcedureSignature(L"Sum", {'I'}, {'i'}) {
	}

	~sumFunctionSignature() override = default;

	bool called(std::vector<rpcmpleVariant> &arguments, std::vector<rpcmpleVariant> &returns) override {
		spdlog::info("example1: rpc server received call to function sum");
		std::vector<int64_t> intArrArg;

		int i = 0;
		if (!getRpcmpleVariantValue(arguments[i++], &intArrArg)) return false;

		int64_t retInt = 0;
		for (auto j = 0; j < intArrArg.size(); j++) {
			retInt += intArrArg[j];
		}

		returns.emplace_back(retInt);

		return true;
	}
};


int main(int argc, char **argv) {

	auto console = spdlog::stdout_color_mt("rpcmple_cpp_example1");
	spdlog::set_default_logger(console);
	spdlog::set_level(spdlog::level::debug);

	spdlog::info("example1: rpc server on cpp process connecting to a tcp server");

	auto *mConn = new connectionManagerSocketClient("127.0.0.1", 8080);
	if (!mConn->create()) return -1;

	auto *mServer = new rpcServer(mConn);
	mServer->appendSignature(new greetFunctionSignature);
	mServer->appendSignature(new sumFunctionSignature);

	mServer->startDataFlowBlocking();

	delete mServer;
	delete mConn;
}
