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

	addr, err := net.ResolveUDPAddr("udp", ":8088")
	if err != nil {
		log.Errorf("Error resolving address: %v", err)
		return
	}

	conn, err := net.ListenUDP("udp", addr)
	if err != nil {
		log.Errorf("Error starting UDP server: %v", err)
		return
	}
	defer conn.Close()

	log.Infof("UDP server started on port 8088")

	//	If subscriber closes the connection, it will stop receiving from subscriber.
	//	To ensure receiving all messages, keep connection open
	//	Connection will be closed by publisher if no more data available

	id := 0

	manager := rpcmple.NewMessageManager(
		conn,
		rpcmple.NewDataSubscriber([]byte{'i', 's'}, func(success bool, values ...any) {
			if !success {
				log.Error("error getting data from publisher")
			} else {
				log.WithFields(log.Fields{"app": "rpcmple_go_example5", "msg_id": id}).Infoln(values[0], values[1])
				id++
			}
		}),
	)

	manager.StartDataFlowBlocking()
}
