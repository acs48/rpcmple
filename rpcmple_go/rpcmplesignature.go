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

// RemoteProcedureSignature represents the signature of a remote procedure call.
type RemoteProcedureSignature struct {
	id uint8
	rc *rpcClient

	bufferPool *sync.Pool

	ProcedureName string
	Arguments     DataSignature
	Returns       DataSignature
	ReplyCallback func(bool, ...any)
}

// Call invokes the remote procedure using the provided arguments.
// Serializes the arguments, writes them to a buffer, and sends the command to the remote system.
// Returns true if the call was successful, false otherwise.
func (rps RemoteProcedureSignature) Call(arguments ...any) bool {
	rps.rc.myLock.Lock()

	if rps.bufferPool == nil {
		rps.bufferPool = &sync.Pool{
			New: func() any {
				return new(bytes.Buffer)
			},
		}
	}

	rps.rc.command.Reset()
	body := rps.bufferPool.Get().(*bytes.Buffer)
	defer body.Reset()
	defer rps.bufferPool.Put(body)

	if !rps.Arguments.ToBinary(body, arguments...) {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Errorf("error serializing arguments from %v", rps.ProcedureName)
		return false
	}

	if body.Len() > 16777216 {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Errorf("serialized data size %d from %v is higher than maximum 16777216", body.Len(), rps.ProcedureName)
		return false
	}

	bodyArr := body.Bytes()
	headerMessage := uint32(rps.id)*16777216 + uint32(len(bodyArr))

	err := binary.Write(rps.rc.command, binary.LittleEndian, headerMessage)
	if err != nil {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Errorf("error writing serialized arguments to buffer: %v", err)
		return false
	}
	err = binary.Write(rps.rc.command, binary.LittleEndian, bodyArr)
	if err != nil {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Errorf("error writing serialized arguments to buffer: %v\n", err)
		return false
	}

	rps.rc.lastRemoteProc = rps.ProcedureName

	rps.rc.myLock.Unlock()
	rps.rc.commandReady <- true
	return true
}
