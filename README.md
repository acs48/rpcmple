# rpcmple
Rpcmple intends to be a simple and easy to use protocol for remote procedure call and message transfer between processes
## Why?
You may be asking yourself, why do we need another framework for remote procedure call, when there are alreasy several others available, well tested and supported by big companies? When I had the same need, for prototyping or simple applications, every solution I have found was over-complicated because of dependencies, libraries to install, complexity building code, excessive overhead of features not necessary for my use case.
So I tried to build my taylored solution. It ended up general enough to be pheraps of use for others too.
## What is implemented so far
Rpcmple is still newborn and only fits few cases, with the idea of growing it when new case is needed. Currently supports:
- Languages: Go, c++
- Remote procedure call:
  - the c++ application is the RPC server
  - the Go application is the RPC client
- Publisher/Subscriber:
  - the c++ applicatin is the publisher
  - the Go application is the subscriber
- Connection protocols:
  - the c++ application is the client, on Windows interfaces already available for named pipes and TCP connection
  - the Go application is the server, and supports whatever implements net.Conn interface
## How it works
Base block of rpcmple is the data signature, which is a charachter string representing the data to be transferred. It supports the following data types:
- 'i' for int64
- 'I' for array of int64
- 'u' for uint64
- 'U' for array of uint64
- 'd' for double precision floating point number (64bit)
- 'D' for array of double
- 's' for UTF-8 encoded string
- 'S' for array of UTF-8 encoded string
- 'u' for variant, which van be any of the above

Data is passed to the Call or Publish functions in form of:
- on c++ application, in form of std::vector<std::variant<>> (requires c++17). Supporteed data is int64_t, uint64_t, double, std::wstring. Arrays are std::vector of supported data
- on Go application, in form of []any. Supported data is int64, uint46, float64, string. Arrays are slices of supported data
The subscriber will publish a data block as described by its signature, and does not expect a reply.
The RPC client will send data described by an argument signature and receive return value described by a return signature.
When client, subscriber, server or publisher Stop a connection, the other side will also stop looping and exit. Any connection error will stop loop on both ends.


## Examples

### RPC

Go application:

c== application:
