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

package rpcmple

import (
	"bytes"
	"encoding/binary"
	"io"
	"log"
	"net"
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
	readBuffer  []byte
	readMessage []byte

	maxMessageLen       int
	messageLastIdx      int
	messageLength       int
	messageMissingBytes int
	requester           bool
	stopRequest         bool

	conn   net.Conn
	parser MessageParser
}

// NewMessageManager creates a new MessageManager with specified buffer sizes, connection, and message parser.
func NewMessageManager(bufferSize, maxMessageSize int, conn net.Conn, parser MessageParser) *MessageManager {
	mm := &MessageManager{
		parser:      parser,
		conn:        conn,
		requester:   parser.IsRequester(),
		stopRequest: false,
		readBuffer:  make([]byte, bufferSize),
		readMessage: make([]byte, maxMessageSize),

		maxMessageLen: maxMessageSize,
	}

	mm.messageLastIdx = 0
	mm.messageLength = parser.GetMessageLen()
	mm.messageMissingBytes = mm.messageLength

	if mm.messageLength > mm.maxMessageLen {
		log.Printf("messageManager: parser requesting oversize readMessage\n")
		mm.StopDataFlow()
	}

	return mm
}

func (mm *MessageManager) dataFlow() {
	fakeByte := make([]byte, 1)
	var fakeBuffer []byte
	replyMessage := new(bytes.Buffer)

	for !mm.stopRequest {
		if mm.messageLength != 0 {
			n, err := mm.conn.Read(mm.readBuffer)
			if err != nil {
				if err != io.EOF {
					log.Printf("messageManager: error reading bytes: %s\n", err)
				}
				break
			}
			fakeBuffer = mm.readBuffer[:n]
		} else {
			fakeBuffer = fakeByte
		}

		for len(fakeBuffer) > 0 {
			transferredBytes := copy(mm.readMessage[mm.messageLastIdx:mm.messageLength], fakeBuffer[:])
			mm.messageLastIdx += transferredBytes
			mm.messageMissingBytes -= transferredBytes
			fakeBuffer = fakeBuffer[transferredBytes:]

			if mm.messageMissingBytes == 0 {
				if success := mm.parser.ParseMessage(mm.readMessage[:mm.messageLength]); !success {
					log.Printf("messageManager: parser failed to parse message. Stopping flow\n")
					mm.stopRequest = true
					break
				}

				replyMessage.Reset()
				if replyOk := mm.parser.SendMessage(replyMessage); !replyOk {
					log.Printf("messageManager: parser failed to send replyMessage. Stopping flow\n")
					mm.stopRequest = true
					break
				} else {
					if replyMessage.Len() > mm.maxMessageLen {
						log.Printf("messageManager: parser requested replyMessage to send too big\n")
						mm.stopRequest = true
						break
					}
					if replyMessage.Len() > 0 {
						err := binary.Write(mm.conn, binary.LittleEndian, replyMessage.Bytes())
						if err != nil {
							log.Printf("messageManager: error writing bytes: %v", err)
						}
					}
				}

				mm.messageLastIdx = 0
				mm.messageLength = mm.parser.GetMessageLen()
				mm.messageMissingBytes = mm.messageLength
				if mm.messageLength > mm.maxMessageLen {
					log.Printf("messageManager: parser requesting oversize replyMessage")
					mm.stopRequest = true
					break
				}
			}
		}
	}
	err := mm.conn.Close()
	if err != nil {
		log.Printf("messageManager: error closing connection: %v\n", err)
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
			if message.Len() > mm.maxMessageLen {
				log.Printf("messageManager: parser requested readMessage to send too big\n")
				mm.stopRequest = true
				return
			}
			if message.Len() == 0 {
				log.Printf("messageManager: parser is requester but message is empty\n")
				mm.stopRequest = true
				return
			}
			err := binary.Write(mm.conn, binary.LittleEndian, message.Bytes())
			if err != nil {
				log.Printf("messageManager: error writing bytes: %v", err)
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
				if message.Len() > mm.maxMessageLen {
					log.Printf("messageManager: parser requested replyMessage to send too big\n")
					mm.stopRequest = true
					return
				}
				if message.Len() == 0 {
					log.Printf("messageManager: parser is requester but message is empty\n")
					mm.stopRequest = true
					return
				}
				err := binary.Write(mm.conn, binary.LittleEndian, message.Bytes())
				if err != nil {
					log.Printf("messageManager: error writing bytes: %v", err)
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
