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

package rpcmple

import (
	"bytes"
	"encoding/binary"
	log "github.com/sirupsen/logrus"
	"sync"
)

// rpcClient handles remote procedure calls and manages the state of communications.
type rpcClient struct {
	remoteProcedures map[string]*RemoteProcedureSignature

	sectionLen     uint32
	sectionID      uint16
	lastRemoteProc string

	callbackValues []any
	replySuccess   bool

	myLock       sync.Mutex
	commandReady chan bool
	command      *bytes.Buffer
}

// NewRPCClient initializes a new rpcmple remote procedure call client instance.
// User must provide a slice of RemoteProcedureSignature to be reported in the same order as they are
// indicated in the server code, on the other process (functions are identified by index, the name is only for user reference).
// Returns a pointer to the created rpcClient.
func NewRPCClient(remoteProcedures []RemoteProcedureSignature) *rpcClient {
	retV := &rpcClient{
		remoteProcedures: make(map[string]*RemoteProcedureSignature),
		commandReady:     make(chan bool),
		command:          new(bytes.Buffer),

		callbackValues: make([]any, 50),

		sectionLen: 4,
		sectionID:  0,
	}

	for i := range remoteProcedures {
		remoteProcedures[i].id = uint8(i)
		remoteProcedures[i].rc = retV
		retV.remoteProcedures[remoteProcedures[i].ProcedureName] = &remoteProcedures[i]
	}

	return retV
}

// ParseMessage processes the given byte slice message according to the current sectionID and updates the rpcClient state.
// Returns true if the message is successfully parsed, false otherwise.
func (rc *rpcClient) ParseMessage(message []byte) bool {
	var err error

	rc.myLock.Lock()
	defer rc.myLock.Unlock()

	switch rc.sectionID {
	case 0: // header section: uint16 storing success flag and length of body message
		var section1 uint32

		if len(message) != 4 {
			log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("error parsing message: invalid length")
			return false
		}

		mb := bytes.NewReader(message)
		err = binary.Read(mb, binary.LittleEndian, &section1)
		if err != nil {
			log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("error reading message:", err)
			return false
		}

		rc.replySuccess = false
		if section1/16777216 == 1 {
			rc.replySuccess = true
		}

		rc.sectionLen = section1 % 16777216

		if rc.sectionLen > 0 {
			rc.sectionID = 1
		} else {
			if mProc, ok := rc.remoteProcedures[rc.lastRemoteProc]; ok {
				mr := bytes.NewReader(make([]byte, 0))

				if success := mProc.Returns.FromBinary(mr, &rc.callbackValues); !success {
					log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("error deserializing message")
					return false
				}
				if mProc.ReplyCallback != nil {
					mProc.ReplyCallback(rc.replySuccess, rc.callbackValues...)
				}

				rc.callbackValues = rc.callbackValues[0:cap(rc.callbackValues)]
				for i := range rc.callbackValues {
					rc.callbackValues[i] = nil
				}
				rc.callbackValues = rc.callbackValues[:0]
			} else {
				log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("error during message deserialization: procedure not found")
				return false
			}

			rc.sectionLen = 4
			rc.sectionID = 0
		}
	case 1:
		if mProc, ok := rc.remoteProcedures[rc.lastRemoteProc]; ok {
			mr := bytes.NewReader(message)

			if success := mProc.Returns.FromBinary(mr, &rc.callbackValues); !success {
				log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("error deserializing message")
				return false
			}

			if mProc.ReplyCallback != nil {
				mProc.ReplyCallback(rc.replySuccess, rc.callbackValues...)
			}
		} else {
			log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("error during message deserialization: procedure not found")
			return false
		}
		rc.sectionLen = 4
		rc.sectionID = 0
	default:
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("error parsing message: invalid section id")
		return false
	}
	return true
}

// GetMessageLen returns the length of the current message section by converting the sectionLen from uint16 to int.
func (rc *rpcClient) GetMessageLen() int {
	rc.myLock.Lock()
	defer rc.myLock.Unlock()
	return int(rc.sectionLen)
}

// SendMessage writes the command buffer to the provided message buffer and returns whether the process is successful.
func (rc *rpcClient) SendMessage(message *bytes.Buffer) bool {
	if rc.sectionID != 0 {
		return true
	}

	<-rc.commandReady

	rc.myLock.Lock()
	defer rc.myLock.Unlock()
	if rc.command.Len() == 0 {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("invalid serialized arguments")
		return false
	}

	err := binary.Write(message, binary.LittleEndian, rc.command.Bytes())
	if err != nil {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("RPC: error writing message:", err)
		return false
	}

	return true
}

// IsRequester checks if the current rpcClient instance is designated as a requester.
func (rc *rpcClient) IsRequester() bool { return true }

// Stop gracefully stops the rpcClient by closing the commandReady channel.
func (rc *rpcClient) Stop() {
	close(rc.commandReady)
}

// Call invokes a remote procedure identified by the given name with the provided arguments.
// Returns true if the procedure is found and invoked successfully, false otherwise.
func (rc *rpcClient) Call(remoteProcedure string, arguments ...any) bool {
	if mProc, ok := rc.remoteProcedures[remoteProcedure]; ok {
		return mProc.Call(arguments...)
	}
	log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("error during Call: procedure not found")
	return false
}
