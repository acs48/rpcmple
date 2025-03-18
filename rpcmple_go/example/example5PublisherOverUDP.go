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
	"math/rand"
	"net"
	"os"
	"time"
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

	conn, err := net.DialUDP("udp", nil, addr)
	if err != nil {
		log.Errorf("Error connecting to udp server: %v", err)
		return
	}
	defer conn.Close()

	log.Infof("UDP client connected to publisher on port 8088")

	publisher := rpcmple.NewDataPublisher([]byte{'i', 's'})

	manager := rpcmple.NewMessageManager(
		conn,
		publisher,
	)

	manager.StartDataFlowNonBlocking()

	source := rand.NewSource(time.Now().UnixNano())
	r := rand.New(source)

	randStringList := []string{"apples", "frogs", "dinosaurs", "stones", "melons", "pens", "crocodiles", "cars", "lizards"}
	for i := 0; i < 100000; i++ {
		randomIndex := r.Intn(len(randStringList))
		randomQty := int64(r.Intn(7) + 2)

		publisher.Publish(randomQty, randStringList[randomIndex])
		time.Sleep(time.Millisecond * 1)
	}

	publisher.WaitForPublishComplete()

}
