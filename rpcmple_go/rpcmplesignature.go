// Copyright 2024 Carlo Seghi github.com/acs48. All rights reserved.
// Use of this source code is governed by a GNU General Public License v3.0
// license that can be found in the LICENSE file.

package rpcmple

import (
	"bytes"
	"encoding/binary"
	"log"
	"sync"
)

// RemoteProcedureSignature represents the signature of a remote procedure call.
type RemoteProcedureSignature struct {
	id uint16
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
		log.Printf("RPC: error serializing arguments from %v\n", rps.ProcedureName)
		return false
	}

	bodyArr := body.Bytes()
	headerMessage := uint32(rps.id)*65536 + uint32(len(bodyArr))

	err := binary.Write(rps.rc.command, binary.LittleEndian, headerMessage)
	if err != nil {
		log.Printf("error writing serialized arguments to buffer: %v\n", err)
		return false
	}
	err = binary.Write(rps.rc.command, binary.LittleEndian, bodyArr)
	if err != nil {
		log.Printf("error writing serialized arguments to buffer: %v\n", err)
		return false
	}

	rps.rc.lastRemoteProc = rps.ProcedureName

	rps.rc.myLock.Unlock()
	rps.rc.commandReady <- true
	return true
}
