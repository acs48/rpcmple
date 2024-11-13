// ******  rpcmple for go  ******
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

package main

import (
	rpcmple "github.com/acs48/rpcmple/rpcmple_go"
	log "github.com/sirupsen/logrus"
	"net"
	"os"
)

func main() {

	log.SetOutput(os.Stdout)
	log.SetFormatter(&log.TextFormatter{ForceColors: true, FullTimestamp: true})
	log.SetLevel(log.DebugLevel)
	logger := log.WithField("app", "rpcmple_go_example1")

	type funcName int

	const (
		Greet funcName = iota
		Sum
	)

	mRProcedures := []rpcmple.RemoteProcedureSignature{
		{ProcedureName: "Greet", Arguments: []byte{'s'}, Returns: []byte{'s'}},
		{ProcedureName: "Sum", Arguments: []byte{'I'}, Returns: []byte{'i'}},
	}

	serv, err := net.Listen("tcp", ":8080")
	if err != nil {
		log.Fatal(err)
	}
	defer serv.Close()
	conn, err := serv.Accept()
	if err != nil {
		log.Fatal(err)
	}
	defer conn.Close()

	manager := rpcmple.NewMessageManager(
		conn,
		rpcmple.NewRPCClient(mRProcedures),
	)

	manager.StartDataFlowNonBlocking()

	syncReply := make(chan bool)
	mRProcedures[Greet].ReplyCallback = func(b bool, a ...any) {
		if b {
			logger.Println("Greet:", a[0].(string))
		}
		syncReply <- b
	}
	mRProcedures[Greet].Call("Hello World")
	callSuccess := <-syncReply
	if callSuccess {
		logger.Warnf("Greet call success")
	} else {
		logger.Errorf("Greet call failed")
	}

	mRProcedures[Sum].ReplyCallback = func(b bool, a ...any) {
		if b {
			log.Println("Sum:", a[0].(int64))
		}
		syncReply <- b
	}
	mRProcedures[Sum].Call([]int64{1, 2, 3, 4, 5})
	callSuccess = <-syncReply
	if callSuccess {
		logger.Warnf("Sum call success")
	} else {
		logger.Errorf("Sum call failed")
	}
}
