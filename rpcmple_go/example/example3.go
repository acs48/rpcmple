// ******  rpcmple for go  ******
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
//
// Listen for incoming connection on localhost:8080
// On new connection, create an RPC client and call functions

package main

import (
	rpcmple "github.com/acs48/rpcmple/rpcmple_go"
	"github.com/natefinch/npipe"
	log "github.com/sirupsen/logrus"
	"os"
)

func main() {

	log.SetOutput(os.Stdout)
	log.SetFormatter(&log.TextFormatter{ForceColors: true, FullTimestamp: true})
	log.SetLevel(log.InfoLevel)

	id := 0

	serv, err := npipe.Listen(`\\.\pipe\rpcmple_example3`)
	if err != nil {
		log.Fatal(err)
	}
	defer serv.Close()
	conn, err := serv.Accept()
	if err != nil {
		log.Fatal(err)
	}
	// 	defer conn.Close()
	//	If subscriber closes the connection, it will stop receiving from subscriber.
	//	To ensure receiving all messages, keep connection open
	//	Connection will be closed by publisher if no more data available

	manager := rpcmple.NewMessageManager(
		conn,
		rpcmple.NewDataSubscriber([]byte{'i', 's'}, func(success bool, values ...any) {
			if !success {
				log.Error("error getting data from publisher")
			} else {
				log.WithFields(log.Fields{"app": "rpcmple_go_example3", "msg_id": id}).Infoln(values[0], values[1])
				id++
			}
		}),
	)

	manager.StartDataFlowBlocking()
}
