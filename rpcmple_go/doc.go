// Copyright 2024 Carlo Seghi github.com/acs48. All rights reserved.
// Use of this source code is governed by a GNU General Public License v3.0
// license that can be found in the LICENSE file.

// Package rpcmple is the Go implementation of rpcmple protocol. It is a simple message
// encoding / decoding system for inter-process data transfer and remote procedure call.
// Clearly unnecessary for inter-process communication between Go applications (use net/rpc
// in this case), may be an option to connect processes built in different languages with
// less effort compared with other frameworks.
//
// # Base usage for your own message structure is to implement the MessageParser interface
//
// Encoding and decoding can be done using the DataSignature and related methods FromBinary
// and ToBinary.
//
// The package comes with the following parsers:
//   - rpcClient to handle calls to procedures hosted on the other process (see NewRPCClient)
//   - dataSubscriber to handle a flow coming from the publisher coming from the other process (see NewDataSubscriber)
package rpcmple
