// Copyright 2024 Carlo Seghi github.com/acs48. All rights reserved.
// Use of this source code is governed by a GNU General Public License v3.0
// license that can be found in the LICENSE file.

package rpcmple_go

import (
	"encoding/binary"
	"io"
	"log"
	"reflect"
)

type dataSignature []byte

// ToBinary serializes the provided arguments to binary format and writes them to the given io.Writer based on the dataSignature.
// Returns true if serialization is successful, false otherwise.
func (ds dataSignature) ToBinary(body io.Writer, arguments ...any) bool {
	if len(ds) != len(arguments) {
		log.Printf("error enconding argument array to binary: unexpected number of arguments: %d != %d\n", len(arguments), len(ds))
		return false
	}

	for i, arg := range ds {
		v := reflect.Indirect(reflect.ValueOf(arguments[i]))
		t := v.Type()

		switch arg {
		case 'v':
			switch t {
			case reflect.TypeOf(float64(1.)):
				arg = 'd'
			case reflect.TypeOf([]float64{}):
				arg = 'D'
			case reflect.TypeOf(int64(1.)):
				arg = 'i'
			case reflect.TypeOf([]int64{}):
				arg = 'I'
			case reflect.TypeOf(uint64(1.)):
				arg = 'u'
			case reflect.TypeOf([]uint64{}):
				arg = 'U'
			case reflect.TypeOf(""):
				arg = 's'
			case reflect.TypeOf([]string{}):
				arg = 'S'
			default:
				log.Printf("invalid signature serializing variant data: %v\n", arg)
				return false
			}
			err := binary.Write(body, binary.LittleEndian, arg)
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", arg, err)
				return false
			}
		}

		switch arg {
		case 'd': // double
			if t != reflect.TypeOf(float64(1.)) {
				log.Printf("wrong argument passed to rpc call, expected %v, got %v\n", reflect.TypeOf(float64(1.)), t)
				return false
			}
			dblVal := v.Float()
			err := binary.Write(body, binary.LittleEndian, dblVal)
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", dblVal, err)
				return false
			}
		case 'D': // array of double
			if t != reflect.TypeOf([]float64{}) {
				log.Printf("wrong argument passed to rpc call, expected %v, got %v\n", reflect.TypeOf([]float64{}), t)
				return false
			}
			dblArrVal := v.Interface().([]float64)
			arrLen := uint16(len(dblArrVal))
			err := binary.Write(body, binary.LittleEndian, arrLen)
			if err != nil {
				return false
				//panic(fmt.Errorf("could not serialize argument: %d", arrLen))
				// TODO
			}
			err = binary.Write(body, binary.LittleEndian, dblArrVal)
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", dblArrVal, err)
				return false
			}
		case 'i': // int
			if t != reflect.TypeOf(int64(1)) {
				log.Printf("wrong argument passed to rpc call, expected %v, got %v\n", reflect.TypeOf(int64(1)), t)
				return false
			}
			intVal := v.Int()
			err := binary.Write(body, binary.LittleEndian, intVal)
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", intVal, err)
				return false
			}
		case 'I': // array of int
			if t != reflect.TypeOf([]int64{}) {
				log.Printf("wrong argument passed to rpc call, expected %v, got %v\n", reflect.TypeOf([]int64{}), t)
				return false
			}
			intArrVal := v.Interface().([]int64)
			arrLen := uint16(len(intArrVal))
			err := binary.Write(body, binary.LittleEndian, arrLen)
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", arrLen, err)
				return false
			}
			err = binary.Write(body, binary.LittleEndian, intArrVal)
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", intArrVal, err)
				return false
			}
		case 'u': // int
			if t != reflect.TypeOf(uint64(1)) {
				log.Printf("wrong argument passed to rpc call, expected %v, got %v\n", reflect.TypeOf(uint64(1)), t)
				return false
			}
			uintVal := v.Uint()
			err := binary.Write(body, binary.LittleEndian, uintVal)
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", uintVal, err)
				return false
			}
		case 'U': // array of int
			if t != reflect.TypeOf([]uint64{}) {
				log.Printf("wrong argument passed to rpc call, expected %v, got %v\n", reflect.TypeOf([]uint64{}), t)
				return false
			}
			uintArrVal := v.Interface().([]uint64)
			arrLen := uint16(len(uintArrVal))
			err := binary.Write(body, binary.LittleEndian, arrLen)
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", arrLen, err)
				return false
			}
			err = binary.Write(body, binary.LittleEndian, uintArrVal)
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", uintArrVal, err)
				return false
			}
		case 's': // string
			if t != reflect.TypeOf("") {
				log.Printf("wrong argument passed to rpc call, expected %v, got %v\n", reflect.TypeOf(""), t)
				return false
			}
			strVal := v.String()
			strLen := uint16(len(strVal))
			err := binary.Write(body, binary.LittleEndian, strLen)
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", strLen, err)
				return false
			}
			err = binary.Write(body, binary.LittleEndian, []byte(strVal))
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", strVal, err)
				return false
			}
		case 'S': // array of string
			if t != reflect.TypeOf([]string{}) {
				log.Printf("wrong argument passed to rpc call, expected %v, got %v\n", reflect.TypeOf([]string{}), t)
				return false
			}

			strArrVal := v.Interface().([]string)
			strArrLen := uint16(len(strArrVal))
			err := binary.Write(body, binary.LittleEndian, strArrLen)
			if err != nil {
				log.Printf("could not serialize argument: %v: %v\n", strArrLen, err)
				return false
			}

			for _, strVal := range strArrVal {
				strLen := uint16(len(strVal))
				err := binary.Write(body, binary.LittleEndian, strLen)
				if err != nil {
					log.Printf("could not serialize argument: %v: %v\n", strLen, err)
					return false
				}
				err = binary.Write(body, binary.LittleEndian, []byte(strVal))
				if err != nil {
					log.Printf("could not serialize argument: %v: %v\n", strVal, err)
					return false
				}
			}
		default:
			log.Printf("invalid signature serializing data: %v\n", arg)
			return false
		}
	}
	return true
}

// FromBinary deserializes data from the given io.Reader into the callbackValues based on the dataSignature.
// Returns true if deserialization is successful, false otherwise.
func (ds dataSignature) FromBinary(mr io.Reader, callbackValues *[]any) bool {
	var err error

	if cap(*callbackValues) < len(ds) {
		*callbackValues = (*callbackValues)[:cap(*callbackValues)]
		*callbackValues = append(*callbackValues, make([]any, len(ds)-len(*callbackValues))...)
	}
	*callbackValues = (*callbackValues)[:len(ds)]

	for i, ret := range ds {
		switch ret {
		case 'v':
			byteVal := byte('x')
			err = binary.Read(mr, binary.LittleEndian, &byteVal)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", byteVal, err)
				return false
			}
			if byteVal == 'd' || byteVal == 'D' || byteVal == 'i' || byteVal == 'I' || byteVal == 'u' || byteVal == 'U' || byteVal == 's' || byteVal == 'S' {
				ret = byteVal
			} else {
				log.Printf("invalid signature deserializing variant type: %v\n", ret)
				return false
			}
		}

		switch ret {
		case 'd':
			dblVal := float64(1.)
			err = binary.Read(mr, binary.LittleEndian, &dblVal)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", dblVal, err)
				return false
			}
			(*callbackValues)[i] = dblVal
		case 'D':
			var dblArrLen uint16
			err = binary.Read(mr, binary.LittleEndian, &dblArrLen)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", dblArrLen, err)
				return false
			}
			dblArr := make([]float64, dblArrLen)
			err = binary.Read(mr, binary.LittleEndian, dblArr)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", dblArr, err)
				return false
			}
			(*callbackValues)[i] = dblArr
		case 'i':
			intVal := int64(1.)
			err = binary.Read(mr, binary.LittleEndian, &intVal)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", intVal, err)
				return false
			}
			(*callbackValues)[i] = intVal
		case 'I':
			var intArrLen uint16
			err = binary.Read(mr, binary.LittleEndian, &intArrLen)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", intArrLen, err)
				return false
			}
			intArr := make([]int64, intArrLen)
			err = binary.Read(mr, binary.LittleEndian, intArr)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", intArr, err)
				return false
			}
			(*callbackValues)[i] = intArr
		case 'u':
			uintVal := uint64(1.)
			err = binary.Read(mr, binary.LittleEndian, &uintVal)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", uintVal, err)
				return false
			}
			(*callbackValues)[i] = uintVal
		case 'U':
			var uintArrLen uint16
			err = binary.Read(mr, binary.LittleEndian, &uintArrLen)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", uintArrLen, err)
				return false
			}
			uintArr := make([]uint64, uintArrLen)
			err = binary.Read(mr, binary.LittleEndian, uintArr)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", uintArr, err)
				return false
			}
			(*callbackValues)[i] = uintArr
		case 's':
			var strLen uint16
			err = binary.Read(mr, binary.LittleEndian, &strLen)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", strLen, err)
				return false
			}

			strRaw := make([]byte, strLen)
			err = binary.Read(mr, binary.LittleEndian, strRaw)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", strRaw, err)
				return false
			}
			str := string(strRaw)
			(*callbackValues)[i] = str
		case 'S':
			var strArrLen uint16
			err = binary.Read(mr, binary.LittleEndian, &strArrLen)
			if err != nil {
				log.Printf("could not deserialize argument: %v: %v\n", strArrLen, err)
				return false
			}

			strArr := make([]string, strArrLen)

			for j := range strArr {
				var strLen uint16
				err = binary.Read(mr, binary.LittleEndian, &strLen)
				if err != nil {
					log.Printf("could not deserialize argument: %v: %v\n", strLen, err)
					return false
				}

				strRaw := make([]byte, strLen)
				err = binary.Read(mr, binary.LittleEndian, strRaw)
				if err != nil {
					return false
					// todo
				}
				str := string(strRaw)
				strArr[j] = str
			}
			(*callbackValues)[i] = strArr
		default:
			log.Printf("invalid signature deserializing data: %v\n", ret)
			return false
		}
	}
	return true
}
