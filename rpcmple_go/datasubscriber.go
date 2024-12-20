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

type dataSubscriber struct {
	signature DataSignature

	sectionLen uint32
	sectionID  uint16

	publisherSuccess bool
	callbackValues   []any

	myLock sync.Mutex

	replyCallback func(bool, ...any)
}

// NewDataSubscriber creates a new data subscriber with the given DataSignature and reply callback function.
func NewDataSubscriber(signature DataSignature, callback func(bool, ...any)) *dataSubscriber {
	retV := &dataSubscriber{
		signature: signature,

		callbackValues: make([]any, 50),

		sectionLen: 4,
		sectionID:  0,

		replyCallback: callback,
	}

	return retV
}

// ParseMessage processes a binary message based on the current sectionID, updates the state, and calls a callback if necessary.
// Returns true if the message is successfully parsed, false otherwise.
func (ds *dataSubscriber) ParseMessage(message []byte) bool {
	var err error

	ds.myLock.Lock()
	defer ds.myLock.Unlock()

	switch ds.sectionID {
	case 0:
		var section1 uint32

		if len(message) != 4 {
			log.WithFields(log.Fields{"app": "rpcmple_go", "func": "subscriber"}).Errorf(" wrong message section length")
			return false
		}

		mb := bytes.NewReader(message)
		err = binary.Read(mb, binary.LittleEndian, &section1)
		if err != nil {
			log.WithFields(log.Fields{"app": "rpcmple_go", "func": "subscriber"}).Error("error reading section: ", err)
			return false
		}

		ds.publisherSuccess = false
		if section1/16777216 == 1 {
			ds.publisherSuccess = true
		}

		ds.sectionLen = section1 % 16777216
		ds.sectionID = 1

	case 1:
		mr := bytes.NewReader(message)

		if success := ds.signature.FromBinary(mr, &ds.callbackValues); success {
			if ds.replyCallback != nil {
				ds.replyCallback(ds.publisherSuccess, ds.callbackValues...)
			}

			ds.callbackValues = ds.callbackValues[:0]
		} else {
			log.WithFields(log.Fields{"app": "rpcmple_go", "func": "subscriber"}).Error("error deserializing data")
			return false
		}

		ds.sectionLen = 4
		ds.sectionID = 0
	default:
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "subscriber"}).Error("wrong message section ID")
		return false
	}
	return true
}

// GetMessageLen returns the length of the current message section, as an integer.
func (ds *dataSubscriber) GetMessageLen() int {
	ds.myLock.Lock()
	defer ds.myLock.Unlock()
	return int(ds.sectionLen)
}

// SendMessage sends a message contained in the given bytes.Buffer and always returns true as the subscriber doesn't send replies.
func (ds *dataSubscriber) SendMessage(*bytes.Buffer) bool {
	// Subscriber never sends reply
	return true
}

// IsRequester checks whether the data subscriber is the requester in a communication scenario.
// It always returns false for dataSubscriber which means this subscriber does not initiate requests.
func (ds *dataSubscriber) IsRequester() bool {
	return false
}

// Stop terminates the dataSubscriber's activity and releases associated resources.
func (ds *dataSubscriber) Stop() {}
