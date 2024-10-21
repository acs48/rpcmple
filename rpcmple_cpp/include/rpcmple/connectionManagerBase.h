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


#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include "rpcmpleutility.h"

#include <vector>
#include <cstdint>

/* connectionManager is pure virtual class defining operation for stream opening, closing, reaing and writing */
class connectionManager {
public:
	connectionManager(){}
    virtual ~connectionManager()=default;

    virtual bool create()=0;
    virtual bool write(std::vector<uint8_t>&)=0;
    virtual bool read(std::vector<uint8_t>&, uint32_t*)=0;
    virtual bool close()=0;
 };

#endif //CONNECTIONMANAGER_H
