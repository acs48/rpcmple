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
	"fmt"
	rpcmple "github.com/acs48/rpcmple/rpcmple_go"
	log "github.com/sirupsen/logrus"
	"io"
	"os"
	"os/exec"
	"path/filepath"
)

type stdPipe struct {
	stdin  io.WriteCloser
	stdout io.ReadCloser
	closed bool
}

func (s *stdPipe) Read(p []byte) (n int, err error) {
	if !s.closed {
		return s.stdout.Read(p)
	}
	return 0, fmt.Errorf("read on closed buffer")
}

func (s *stdPipe) Write(p []byte) (n int, err error) {
	if !s.closed {
		return s.stdin.Write(p)
	}
	return 0, fmt.Errorf("write on closed buffer")
}

func (s *stdPipe) Close() error {
	s.closed = true
	return nil
}

func main() {
	log.SetOutput(os.Stderr)
	log.SetFormatter(&log.TextFormatter{ForceColors: true, FullTimestamp: true})
	log.SetLevel(log.InfoLevel)
	logger := log.WithField("app", "rpcmple_go_example2")

	type funcName int

	const (
		Greet funcName = iota
		Sum
	)

	mRProcedures := []rpcmple.RemoteProcedureSignature{
		{ProcedureName: "Greet", Arguments: []byte{'s'}, Returns: []byte{'s'}},
		{ProcedureName: "Sum", Arguments: []byte{'I'}, Returns: []byte{'i'}},
	}

	exePath, err := os.Executable()
	if err != nil {
		log.Fatal(err)
	}
	exeDir := filepath.Dir(exePath)
	executableName := "rpcmple_cpp_example2.exe"

	executablePath := filepath.Join(exeDir, executableName)

	cmd := exec.Command(executablePath)

	stdinReader, stdinWriter := io.Pipe()
	stdoutReader, stdoutWriter := io.Pipe()
	procBuffer := &stdPipe{stdinWriter, stdoutReader, false}

	cmd.Stdout = stdoutWriter
	cmd.Stdin = stdinReader
	cmd.Stderr = os.Stderr

	if err := cmd.Start(); err != nil {
		log.Fatalf("Failed to start client app: %v", err)
	}

	manager := rpcmple.NewMessageManager(
		procBuffer,
		rpcmple.NewRPCClient(mRProcedures),
	)

	manager.StartDataFlowNonBlocking()

	syncReply := make(chan bool)
	mRProcedures[Greet].ReplyCallback = func(b bool, a ...any) {
		if b {
			logger.Infoln("Greet: ", a[0].(string))
		}
		syncReply <- b
	}
	mRProcedures[Greet].Call("Hello World")
	callSuccess := <-syncReply
	if callSuccess {
		logger.Warn("Greet call success")
	} else {
		logger.Error("Greet call failed")
	}

	mRProcedures[Sum].ReplyCallback = func(b bool, a ...any) {
		if b {
			logger.Info("Sum: ", a[0].(int64))
		}
		syncReply <- b
	}
	mRProcedures[Sum].Call([]int64{1, 2, 3, 4, 5})
	callSuccess = <-syncReply
	if callSuccess {
		logger.Warn("Sum call success")
	} else {
		logger.Error("Sum call failed")
	}
}
