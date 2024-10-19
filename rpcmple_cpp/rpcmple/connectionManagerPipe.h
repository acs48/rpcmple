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


#ifndef CONNECTIONMANAGERPIPE_H
#define CONNECTIONMANAGERPIPE_H

#include "connectionManagerBase.h"
#include "rpcmpleutility.h"

#include <atlbase.h>

#include <string>

/* connectionManagerPipeClient implements connectionManager on Windows named pipes as dialer */
class connectionManagerPipeClient : public connectionManager {
private:
    HANDLE hPipe;
    std::string fullPipeName;
public:
    connectionManagerPipeClient(std::string name) {
        fullPipeName = R"(\\.\pipe\)";
        fullPipeName.append(name);
    }
    virtual ~connectionManagerPipeClient()=default;

    bool create() override {
       hPipe = CreateFile(
       fullPipeName.c_str(),
       GENERIC_READ | GENERIC_WRITE,
       0,
       nullptr,
       OPEN_EXISTING,
       0,
       nullptr);

      if (hPipe == INVALID_HANDLE_VALUE) {
          return false;
      }
    return true;
    }

    bool write(std::vector<uint8_t>& bytes) override {
        DWORD tBytesWritten = 0;

        while(tBytesWritten < bytes.size()) {
            DWORD bytesWritten;
            BOOL res = WriteFile(
               hPipe,
               (LPCVOID)bytes.data(),
               bytes.size(),
               &bytesWritten,
               nullptr);
            if(res == FALSE) {
                return false;
            }
            tBytesWritten+=bytesWritten;
        }
        return true;
    }

    bool read(std::vector<uint8_t>& bytes, uint32_t* pBytesRead) override {
        BOOL success = ReadFile(
                   hPipe,
                   bytes.data(),
                   bytes.size(),
                   (DWORD*)pBytesRead,
                   nullptr);
        if (!success) {
            DWORD error = GetLastError();
            if(error != ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED) {
                // Pipe was closed by the other side
            }
            return false;
        }
        return true;
    }

    bool close() {
        BOOL res = CloseHandle(hPipe);
        if(!res) return false;
        return true;
    }
};

#endif //CONNECTIONMANAGERPIPE_H
