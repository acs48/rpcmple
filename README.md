# rpcmple
Rpcmple intends to be a simple and easy to use protocol for remote procedure call and message transfer between processes
## Why?
Why do we need another library for remote procedure call, when there are already several others available, well tested and supported by big companies? 
Main need is to have a simple system to embed to c++ code, without going crazy in cmake errors, installing tons of dependencies and requiring hours of build time. 
As such this solution is essential. It is only partially developed for Go and c++.
## What is implemented so far
Rpcmple is still newborn and only fits few cases, with the idea of growing it when new case is needed. Currently supports:
- Languages: Go, c++
- Remote procedure call:
  - the c++ application is the RPC server
  - the Go application is the RPC client
- Publisher/Subscriber:
  - the c++ application is the publisher
  - the Go application is the subscriber
- Connection protocols:
  - the c++ application is the client, on Windows interfaces already available for named pipes and TCP connection
  - the Go application is the server, and supports whatever implements net.Conn interface
## How it works
Base block of rpcmple is the data signature, which is a character string representing the data to be transferred. It supports the following data types:
- 'i' for int64
- 'I' for array of int64
- 'u' for uint64
- 'U' for array of uint64
- 'd' for double precision floating point number (64bit)
- 'D' for array of double
- 's' for UTF-8 encoded string
- 'S' for array of UTF-8 encoded string
- 'w' (c++ only) for UTF-8 encoded string marshaled as std::wstring
- 'W' (c++ only) for array of UTF-8 encoded string marshaled as std::wstring
- 'u' for variant, which van be any of the above

Data is passed to the Call or Publish functions:
- on c++ application, in form of std::vector<std::variant<>> (requires c++17). Supported data is int64_t, uint64_t, double, std::string, std::wstring. Arrays are std::vector of supported data
- on Go application, in form of []any. Supported data is int64, uint46, float64, string. Arrays are slices of supported data

Limit for strings is 65536 bytes. Limit for arrays is 65536 elements. Each signature cannot exceed 16777216 bytes. The RPC can support up to 256 procedures.
## Notes on c++ application
Rpcmple for c++ is only tested o Windows environment using Microsoft Visual C++ Compiler (and the redists to be installed where application will run)
It requires c++17. It comes with no dependencies. Just copy the header files in your project, include what you need and build.

## Examples
See the example files in the language directories.
- Example1: the Go application listens on localhost:8080. The c++ application dials on localhost::8080 and starts an RPC server. On new connection, the Go application calls the RPC procedures and display the results.
- Example2: the Go application launches the c++ application as subprocess. The c++ application starts an RPC server waiting for calls on the standard input, and sending replies to the standard output. The Go application calls the RPC procedures and display the results on standard output.
- Example3: (for windows only) the Go applications listens on named pipe. The c++ application dials on named pipe and starts a publisher server, publishing 100000 int64, string pairs. The Go application prints the published data on standard output.

## Licensing
The rpcmple project is released under MIT LICENSE. A copy of the license is available in the LICENSE file

### Rpcmple for Go
rpcmple for Go depends on the following projects:
- github.com/natefinch/npipe released under MIT license https://github.com/natefinch/npipe/blob/v2/LICENSE.txt
- github.com/sirupsen/logrus released under MIT license https://github.com/sirupsen/logrus/blob/master/LICENSE

### Rpcmple for c++

rpcmple for c++ depend on the following projects:
- https://github.com/gabime/spdlog released under MIT license https://github.com/gabime/spdlog/blob/v1.x/LICENSE
  - spdlog depends on https://github.com/fmtlib/fmt released under license available here https://github.com/fmtlib/fmt/blob/master/LICENSE
