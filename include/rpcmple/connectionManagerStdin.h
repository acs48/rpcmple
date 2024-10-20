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
#include "rpcmpleutility.h"

#include <iostream>

/* connectionManagerPipeClient implements connectionManager on Windows named pipes as dialer */
class connectionManagerPipeClient : public connectionManager {
private:

public:
    connectionManagerPipeClient() {}
    virtual ~connectionManagerPipeClient()=default;

    bool create() override {
        return true;
    }

    bool write(std::vector<uint8_t>& bytes) override {
        std::cout.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        return true;
    }

    bool read(std::vector<uint8_t>& bytes, uint32_t* pBytesRead) override {
        std::cin.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        *pBytesRead = std::cin.gcount();
        return true;
    }

    bool close() override {
        return true;
    }
};


#endif //CONNECTIONMANAGERSTDIN_H
