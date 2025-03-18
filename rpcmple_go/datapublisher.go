// ******  rpcmple for go  ******
// Copyright (C) 2025 Carlo Seghi. All rights reserved.
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

type dataPublisher struct {
	signature DataSignature

	messagePool  *sync.Pool
	messageStack chan *bytes.Buffer
	wg           sync.WaitGroup
}

// NewDataPublisher creates a new data publisher with the given DataSignature.
func NewDataPublisher(signature DataSignature) *dataPublisher {
	dp := &dataPublisher{
		signature: signature,
		messagePool: &sync.Pool{
			New: func() any {
				return new(bytes.Buffer)
			},
		},
		messageStack: make(chan *bytes.Buffer, 1024),
	}

	return dp
}

// ParseMessage returns always true as a publisher does not expect messages from subscriber
func (dp *dataPublisher) ParseMessage([]byte) bool {
	return true
}

// GetMessageLen returns the length of the current message section, as an integer. Always 0 for a publisher
func (dp *dataPublisher) GetMessageLen() int {
	return 0
}

// SendMessage sends a message contained in the given bytes.Buffer.
func (dp *dataPublisher) SendMessage(message *bytes.Buffer) bool {

	lastMessage := <-dp.messageStack
	defer dp.wg.Done()
	defer lastMessage.Reset()
	defer dp.messagePool.Put(lastMessage)

	if lastMessage.Len() == 0 {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("invalid serialized arguments")
		return false
	}

	err := binary.Write(message, binary.LittleEndian, lastMessage.Bytes())
	if err != nil {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "rpc"}).Error("publisher: error writing lastMessage:", err)
		return false
	}

	return true
}

// IsRequester checks whether the data publisher is the requester in a communication scenario.
// It always returns true for dataPublisher which means it does not initiate requests.
func (dp *dataPublisher) IsRequester() bool {
	return true
}

// Stop terminates the dataPublisher's activity and releases associated resources.
func (dp *dataPublisher) Stop() {
	close(dp.messageStack)
}

func (dp *dataPublisher) Publish(data ...any) bool {

	if dp.messagePool == nil {
		dp.messagePool = &sync.Pool{
			New: func() any {
				return new(bytes.Buffer)
			},
		}
	}

	body := dp.messagePool.Get().(*bytes.Buffer)
	defer body.Reset()
	defer dp.messagePool.Put(body)

	if !dp.signature.ToBinary(body, data...) {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "publisher"}).Errorf("error serializing arguments")
		return false
	}

	if body.Len() > 16777216 {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "publisher"}).Errorf("serialized data size %d is higher than maximum 16777216", body.Len())
		return false
	}

	bodyArr := body.Bytes()
	headerMessage := uint32(1)*16777216 + uint32(len(bodyArr))

	message := dp.messagePool.Get().(*bytes.Buffer)
	err := binary.Write(message, binary.LittleEndian, headerMessage)
	if err != nil {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "publisher"}).Errorf("error writing serialized arguments to buffer: %v", err)
		return false
	}
	err = binary.Write(message, binary.LittleEndian, bodyArr)
	if err != nil {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "publisher"}).Errorf("error writing serialized arguments to buffer: %v\n", err)
		return false
	}

	dp.wg.Add(1)
	dp.messageStack <- message
	return true
}

func (dp *dataPublisher) WaitForPublishComplete() {
	dp.wg.Wait()
}
