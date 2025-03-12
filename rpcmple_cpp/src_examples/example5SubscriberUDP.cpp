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
#include "connectionmanager/udpSocket.h"
#include "rpcmple/dataSubscriber.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include <random>
#include <vector>
#include <string>
#include <cstdint>

int main(int argc, char** argv)
{
	auto console = spdlog::stdout_color_mt("rpcmple_cpp_example5");
	spdlog::set_default_logger(console);
	spdlog::set_level(spdlog::level::info);

	// Create a random number engine (Mersenne Twister engine in this case)
	std::random_device rd; // Seed for the random number engine
	std::mt19937 gen(rd());

	// Define a uniform integer distribution for integers between 1 and 100
	std::uniform_int_distribution<> disInt(2, 9);

	auto* mConn = new rpcmple::connectionManager::udpSocket(8088, -1);
	if (!mConn->create()) return -1;

	int cc = 0;
	auto* mServer = new rpcmple::dataSubscriber(mConn, {'i', 's'}, [&cc](rpcmple::variantVector vals) -> void
	{
		int64_t intArg;
		std::string strArg;
		int i = 0;
		if (!rpcmple::getVariantValue(vals[i++], &intArg)) return;
		if (!rpcmple::getVariantValue(vals[i++], &strArg)) return;
		spdlog::info("example5: subscriber received {}: {} {}", cc++, intArg, strArg);
		return;
	});

	mServer->startDataFlowBlocking();

	delete mServer;
	delete mConn;
}
