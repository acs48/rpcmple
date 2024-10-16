package rpcmple_test

import (
	"fmt"
	rpcmple "github.com/acs48/rpcmple/rpcmple_go"
	"net"
)

func ExampleNewRPCClient() {

	type funcName int

	const (
		Greet funcName = iota
		Sum
	)

	mRProcedures := []rpcmple.RemoteProcedureSignature{
		{ProcedureName: "Greet", Arguments: []byte{'s'}, Returns: []byte{'s'}},
		{ProcedureName: "Sum", Arguments: []byte{'I'}, Returns: []byte{'i'}},
	}

	serv, err := net.Listen("tcp", ":8080")
	if err != nil {
		panic(err)
	}
	defer serv.Close()
	conn, err := serv.Accept()
	if err != nil {
		panic(err)
	}
	defer conn.Close()

	manager := rpcmple.NewMessageManager(
		1024,
		1024,
		conn,
		rpcmple.NewRPCClient(mRProcedures),
	)

	manager.StartDataFlowNonBlocking()

	syncReply := make(chan bool)
	mRProcedures[Greet].ReplyCallback = func(b bool, a ...any) {
		if b {
			fmt.Println("Greet: ", a[0].(string))
		}
		syncReply <- b
	}
	mRProcedures[Greet].Call("Hello World")
	callSuccess := <-syncReply
	if callSuccess {
		fmt.Println("Greet call success")
	} else {
		fmt.Println("Greet call failed")
	}

	mRProcedures[Sum].ReplyCallback = func(b bool, a ...any) {
		if b {
			fmt.Println("Sum: ", a[0].(int64))
		}
		syncReply <- b
	}
	mRProcedures[Sum].Call([]int64{1, 2, 3, 4, 5})
	callSuccess = <-syncReply
	if callSuccess {
		fmt.Println("Sum call success")
	} else {
		fmt.Println("Sum call failed")
	}
}

func ExampleNewDataSubscriber() {

	serv, err := net.Listen("tcp", ":8080")
	if err != nil {
		panic(err)
	}
	defer serv.Close()
	conn, err := serv.Accept()
	if err != nil {
		panic(err)
	}
	defer conn.Close()

	manager := rpcmple.NewMessageManager(
		1024,
		1024,
		conn,
		rpcmple.NewDataSubscriber([]byte{'i', 's', 's'}, func(b bool, a ...any) {
			if b {
				fmt.Println("ID: ", a[0].(int64))
				fmt.Println("Name: ", a[1].(string))
				fmt.Println("Address: ", a[2].(string))
			}
		}),
	)

	manager.StartDataFlowBlocking()
}
