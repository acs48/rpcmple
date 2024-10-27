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


#ifndef CONNECTIONMANAGERSTDIN_H
#define CONNECTIONMANAGERSTDIN_H


#include "connectionManagerBase.h"
#include "rpcmple.h"

#include <iostream>

/* connectionManagerPipeClient implements connectionManager on Windows named pipes as dialer */
class connectionManagerStdInOutClient : public connectionManager {
private:

public:
    connectionManagerStdInOutClient() = default;

    ~connectionManagerStdInOutClient() override =default;

    bool create() override {
        return true;
    }

    bool write(std::vector<uint8_t>& bytes) override {
        std::cout.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        if (!std::cout) {  // Stream state check
            if (std::cout.fail()) {
                spdlog::error("connectionManagerStdInOut: stream error occurred during write");
            } else if (std::cout.bad()) {
                spdlog::error("connectionManagerStdInOut: irrecoverable stream error occurred during write");
            }
            return false;
        }
        return true;
    }

    bool read(std::vector<uint8_t>& bytes, uint32_t* pBytesRead) override {
        spdlog::debug("connectionManagerStdInOut: reading data from stdin expecting {} bytes", bytes.size());
        std::cin.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        *pBytesRead = std::cin.gcount();
        if (std::cin.eof()) {
            spdlog::debug("connectionManagerStdInOut: EOF reached");
            return false;
        }
        if (std::cin.fail()) {
            spdlog::error("connectionManagerStdInOut: stream error occurred");
            return false;
        }
        spdlog::debug("connectionManagerStdInOut: red from stdin {} bytes", *pBytesRead);
        return true;
    }

    bool close() override {
        return true;
    }
};


#endif //CONNECTIONMANAGERSTDIN_H