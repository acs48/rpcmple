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
#include "rpcmple/dataPublisher.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include <random>
#include <vector>
#include <string>
#include <cstdint>

int main(int argc, char** argv) {

    auto console = spdlog::stdout_color_mt("rpcmple_cpp_example5");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::info);

	// Create a random number engine (Mersenne Twister engine in this case)
	std::random_device rd; // Seed for the random number engine
	std::mt19937 gen(rd());

	// Define a uniform integer distribution for integers between 1 and 100
	std::uniform_int_distribution<> disInt(2, 9);

    auto* mConn = new rpcmple::connectionManager::udpSocket(-1, 8088, "127.0.0.1");
    if(!mConn->create()) return -1;

    auto* mServer = new rpcmple::dataPublisher(mConn,{'i','s'});

    mServer->startDataFlowNonBlocking();

	std::vector<std::string> randStringList = {"","apples","frogs","dinosaurs","stones","melons","pens","crocodiles","cars","lizards"};

	for (int i=0;i<100000;i++) {
		rpcmple::variantVector arguments;

		// Generate a random integer
		int64_t randomInt = disInt(gen);
		std::string randomString = randStringList[disInt(gen)-1];

		arguments.emplace_back(randomInt);
		arguments.emplace_back(randomString);

	    mServer->publish(arguments);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	mServer->waitPublishComplete();

    delete mServer;
    delete mConn;
}