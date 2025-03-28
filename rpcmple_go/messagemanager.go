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
	"io"
)

// MessageParser provides an interface for parsing, sending, and managing messages. An implementation
// of MessageParser must be passed to NewMessageManager to implement your custom message structure
type MessageParser interface {

	// ParseMessage gets called when a byte sequence of len returned by GetMessageLen is read from
	// the Reader. User implementing this function shall decode the byte sequence and return
	// true if successful. Returning false will stop the communication loop.
	ParseMessage(message []byte) bool

	// GetMessageLen gets called prior to read from Reader, for the MessageManager to know the len of the message.
	// A full message can be of variable len and can be split in multiple sub-messages. GetMessageLen
	// must return the len of the sub-message
	GetMessageLen() int

	// SendMessage gets called after a message or sub-message is read from Reader, and after the call to ParseMessage.
	// It shall serialize to the Buffer argument the data to be sent to the other process.
	// It can leave the buffer empty if no reply must be sent. Return false if an encoding error occur.
	// Returning false will stop the communication loop.
	SendMessage(*bytes.Buffer) bool

	// IsRequester gets called at the beginning of MessageManager loop. It tells the MessageManager
	// if this process must send the first message
	IsRequester() bool

	// Stop gets called by the MessageManager when the StopDataFlow is called or the other
	// application is closing the connection. It can be used to clean up the parser.
	Stop()
}

// MessageManager manages the flow of messages between a connection and a message parser.
type MessageManager struct {
	messageLastIdx      int
	messageLength       int
	messageMissingBytes int
	requester           bool
	stopRequest         bool

	conn   io.ReadWriteCloser
	parser MessageParser
}

// NewMessageManager creates a new MessageManager with specified connection and message parser.
func NewMessageManager(conn io.ReadWriteCloser, parser MessageParser) *MessageManager {
	mm := &MessageManager{
		parser:      parser,
		conn:        conn,
		requester:   parser.IsRequester(),
		stopRequest: false,
	}

	mm.messageLastIdx = 0
	mm.messageLength = parser.GetMessageLen()
	mm.messageMissingBytes = mm.messageLength
	return mm
}

func (mm *MessageManager) dataFlow() {
	var readBuffer *bytes.Buffer
	readBufferRaw := make([]byte, 16777220)
	readMessage := new(bytes.Buffer)
	replyMessage := new(bytes.Buffer)

	for !mm.stopRequest {
		if mm.messageLength != 0 {
			n, err := mm.conn.Read(readBufferRaw)
			if err != nil {
				if err != io.EOF {
					log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("error reading bytes: %v", err)
				}
				break
			}
			if n == 0 {
				log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("error reading bytes: no bytes received")
				break
			}
			readBuffer = bytes.NewBuffer(readBufferRaw[:n])
		} else {
			readBuffer = bytes.NewBuffer(nil)
			replyMessage.Reset()
			if replyOk := mm.parser.SendMessage(replyMessage); !replyOk {
				log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("parser failed to send replyMessage. Stopping flow\n")
				mm.stopRequest = true
				break
			} else {
				if replyMessage.Len() > 0 {
					if uint32(replyMessage.Len()) > 16777220 {
						log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Error("parser generated message is too big: %d, max side %ul", replyMessage.Len(), 16777220)
						mm.stopRequest = true
						break
					}

					if err := binary.Write(mm.conn, binary.LittleEndian, replyMessage.Bytes()); err != nil {
						log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("error writing bytes: %v", err)
						mm.stopRequest = true
						break
					}
				}
			}
			mm.messageLastIdx = 0
			mm.messageLength = mm.parser.GetMessageLen()
			mm.messageMissingBytes = mm.messageLength
		}

		for readBuffer.Len() > 0 {
			transferredBytes, err := readMessage.Write(readBuffer.Next(mm.messageLength))
			if err != nil {
				log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("error reading bytes: %v", err)
				mm.stopRequest = true
				break
			}
			mm.messageLastIdx += transferredBytes
			mm.messageMissingBytes -= transferredBytes

			if mm.messageMissingBytes == 0 {
				if success := mm.parser.ParseMessage(readMessage.Bytes()); !success {
					log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("parser failed to parse message. Stopping flow\n")
					mm.stopRequest = true
					break
				}
				readMessage.Reset()

				replyMessage.Reset()
				if replyOk := mm.parser.SendMessage(replyMessage); !replyOk {
					log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("parser failed to send replyMessage. Stopping flow\n")
					mm.stopRequest = true
					break
				} else {
					if replyMessage.Len() > 0 {
						if uint32(replyMessage.Len()) > 16777220 {
							log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Error("parser generated message is too big: %d, max side %ul", replyMessage.Len(), 16777220)
							mm.stopRequest = true
							break
						}
						if err := binary.Write(mm.conn, binary.LittleEndian, replyMessage.Bytes()); err != nil {
							log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("parser failed to send replyMessage. Stopping flow\n")
							mm.stopRequest = true
							break
						}
					}
				}

				mm.messageLastIdx = 0
				mm.messageLength = mm.parser.GetMessageLen()
				mm.messageMissingBytes = mm.messageLength
			}
		}
	}
	err := mm.conn.Close()
	if err != nil {
		log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("error closing connection: %v\n", err)
	}
}

// StartDataFlowBlocking initiates the data flow process in a blocking manner by sending messages and processing responses.
func (mm *MessageManager) StartDataFlowBlocking() {
	message := new(bytes.Buffer)
	if mm.requester {
		if replyOk := mm.parser.SendMessage(message); !replyOk {
			mm.stopRequest = true
			return
		} else {
			if message.Len() == 0 {
				log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("parser is requester but message is empty")
				mm.stopRequest = true
				return
			}
			if uint32(message.Len()) > 16777220 {
				log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Error("parser generated message is too big: %d, max side %ul", message.Len(), 16777220)
				mm.stopRequest = true
				return
			}

			if err := binary.Write(mm.conn, binary.LittleEndian, message.Bytes()); err != nil {
				log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("parser failed to send replyMessage. Stopping flow\n")
				mm.stopRequest = true
				return
			}
		}
	}

	mm.dataFlow()
}

// StartDataFlowNonBlocking initiates the data flow process in a non-blocking manner by sending messages in a goroutine.
func (mm *MessageManager) StartDataFlowNonBlocking() {
	go func() {
		message := new(bytes.Buffer)
		if mm.requester {
			if replyOk := mm.parser.SendMessage(message); !replyOk {
				mm.stopRequest = true
				return
			} else {
				if message.Len() == 0 {
					log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Error("parser is requester but message is empty")
					mm.stopRequest = true
					return
				}
				if uint32(message.Len()) > 16777220 {
					log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Error("parser generated message is too big: %d, max side %ul", message.Len(), 16777220)
					mm.stopRequest = true
					return
				}

				if err := binary.Write(mm.conn, binary.LittleEndian, message.Bytes()); err != nil {
					log.WithFields(log.Fields{"app": "rpcmple_go", "func": "manager"}).Errorf("parser failed to send replyMessage. Stopping flow\n")
					mm.stopRequest = true
					return
				}
			}
		}

		mm.dataFlow()
	}()
}

// StopDataFlow stops the data flow process and signals the parser to stop.
func (mm *MessageManager) StopDataFlow() {
	mm.stopRequest = true
	mm.parser.Stop()
}
