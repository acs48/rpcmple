// ******  rpcmple for c++ v0.1  ******
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


#ifndef DATASIGNATURE_H
#define DATASIGNATURE_H

#include <variant>
#include <string>
#include <cstdint>
#include <vector>
#include <codecvt>
#include <iostream>

#include "messageManager.h"
#include "rpcmpleutility.h"

typedef std::variant<int64_t, uint64_t, double, std::wstring, std::vector<int64_t>, std::vector<uint64_t>, std::vector<double>, std::vector<std::wstring>> rpcmpleVariant;
typedef std::vector<rpcmpleVariant> rpcmpleVariantVector;

inline bool getRpcmpleVariantValue(const rpcmpleVariant &val, int64_t* pRetVal) {
    if(!std::holds_alternative<int64_t>(val)) {
        return false;
    }

    *pRetVal = std::get<int64_t>(val);
    return true;
}

inline bool getRpcmpleVariantValue(const rpcmpleVariant &val, uint64_t* pRetVal) {
    if(!std::holds_alternative<uint64_t>(val)) {
        return false;
    }

    *pRetVal = std::get<uint64_t>(val);
    return true;
}

inline bool getRpcmpleVariantValue(const rpcmpleVariant &val, double* pRetVal) {
    if(!std::holds_alternative<double>(val)) {
        return false;
    }

    *pRetVal = std::get<double>(val);
    return true;
}

inline bool getRpcmpleVariantValue(const rpcmpleVariant &val, std::wstring* pRetVal) {
    if(!std::holds_alternative<std::wstring>(val)) {
        return false;
    }

    *pRetVal = std::get<std::wstring>(val);
    return true;
}

inline bool getRpcmpleVariantValue(const rpcmpleVariant &val, std::vector<int64_t>* pRetVal) {
    if(!std::holds_alternative<std::vector<int64_t>>(val)) {
        return false;
    }

    *pRetVal = std::get<std::vector<int64_t>>(val);
    return true;
}

inline bool getRpcmpleVariantValue(const rpcmpleVariant &val, std::vector<uint64_t>* pRetVal) {
    if(!std::holds_alternative<std::vector<uint64_t>>(val)) {
        return false;
    }

    *pRetVal = std::get<std::vector<uint64_t>>(val);
    return true;
}

inline bool getRpcmpleVariantValue(const rpcmpleVariant &val, std::vector<double>* pRetVal) {
    if(!std::holds_alternative<std::vector<double>>(val)) {
        return false;
    }

    *pRetVal = std::get<std::vector<double>>(val);
    return true;
}

inline bool getRpcmpleVariantValue(const rpcmpleVariant &val, std::vector<std::wstring>* pRetVal) {
    if(!std::holds_alternative<std::vector<std::wstring>>(val)) {
        return false;
    }

    *pRetVal = std::get<std::vector<std::wstring>>(val);
    return true;
}

class dataSignature : public std::vector<char> {
private:
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
public:
    dataSignature() : std::vector<char>() {}
    explicit dataSignature(std::vector<char> vec) : std::vector<char>(vec) {}

    bool toBinary(rpcmpleVariantVector &rets, std::vector<uint8_t> &message) {
        int replyOffset=0;

        for(int i=0;i<this->size();i++) {
            char dataType = this->at(i);

            switch (dataType) {
                case 'v':
                    if(std::holds_alternative<double>(rets[i])) dataType = 'd';
                    else if(std::holds_alternative<vector<double>>(rets[i])) dataType = 'D';
                    else if(std::holds_alternative<int64_t>(rets[i])) dataType = 'i';
                    else if(std::holds_alternative<vector<int64_t>>(rets[i])) dataType = 'I';
                    else if(std::holds_alternative<uint64_t>(rets[i])) dataType = 'u';
                    else if(std::holds_alternative<vector<uint64_t>>(rets[i])) dataType = 'U';
                    else if(std::holds_alternative<std::wstring>(rets[i])) dataType = 's';
                    else if(std::holds_alternative<vector<std::wstring>>(rets[i])) dataType = 'S';
                    else return false;

                    std::memcpy(message.data()+replyOffset, &dataType,1);
                    replyOffset += 1;
                    break;
            }

            switch (dataType) {
                case 'd': {
                    double dblVal;
                    bool success = getRpcmpleVariantValue(rets[i],&dblVal);
                    if(!success) {
                        return false;
                        // todo
                    }
                    if(message.size() < replyOffset+8) {
                        return false;
                    }
                    doubleToBytes(dblVal,message.data()+replyOffset,true);
                    replyOffset += 8;
                    break;
                }
                case 'D': {
                    std::vector<double> dblArr;
                    bool success = getRpcmpleVariantValue(rets[i],&dblArr);
                    if(!success) {
                        return false;
                        // todo
                    }

                    if(message.size() < replyOffset+2) {
                        return false;
                    }
                    uint16_t dblArrSize = dblArr.size();
                    uint16ToBytes(dblArrSize,message.data()+replyOffset,true);
                    replyOffset+=2;

                    for(int j=0;j<dblArrSize; j++) {
                        if(message.size() < replyOffset+8) {
                            return false;
                        }
                        doubleToBytes(dblArr[j],message.data()+replyOffset,true);
                        replyOffset += 8;
                    }
                    break;
                }
                case 'i': {
                    int64_t intVal;
                    bool success = getRpcmpleVariantValue(rets[i],&intVal);
                    if(!success) {
                        return false;
                        // todo
                    }
                    if(message.size() < replyOffset+8) {
                        return false;
                    }
                    int64ToBytes(intVal,message.data()+replyOffset,true);
                    replyOffset += 8;
                    break;
                }
                case 'I': {
                    std::vector<int64_t> intArr;
                    bool success = getRpcmpleVariantValue(rets[i],&intArr);
                    if(!success) {
                        return false;
                        // todo
                    }
                    if(message.size() < replyOffset+2) {
                        return false;
                    }
                    uint16_t intArrSize = intArr.size();
                    uint16ToBytes(intArrSize,message.data()+replyOffset,true);
                    replyOffset+=2;

                    for(int j=0;j<intArrSize; j++) {
                        if(message.size() < replyOffset+8) {
                            return false;
                        }
                        int64ToBytes(intArr[j],message.data()+replyOffset,true);
                        replyOffset += 8;
                    }
                    break;
                }
                case 'u': {
                    uint64_t uintVal;
                    bool success = getRpcmpleVariantValue(rets[i],&uintVal);
                    if(!success) {
                        return false;
                        // todo
                    }
                    if(message.size() < replyOffset+8) {
                        return false;
                    }
                    uint64ToBytes(uintVal,message.data()+replyOffset,true);
                    replyOffset += 8;
                    break;
                }
                case 'U': {
                    std::vector<uint64_t> uintArr;
                    bool success = getRpcmpleVariantValue(rets[i],&uintArr);
                    if(!success) {
                        return false;
                        // todo
                    }
                    if(message.size() < replyOffset+2) {
                        return false;
                    }
                    uint16_t uintArrSize = uintArr.size();
                    uint16ToBytes(uintArrSize,message.data()+replyOffset,true);
                    replyOffset+=2;

                    for(int j=0;j<uintArrSize; j++) {
                        if(message.size() < replyOffset+8) {
                            return false;
                        }
                        uint64ToBytes(uintArr[j],message.data()+replyOffset,true);
                        replyOffset += 8;
                    }
                    break;
                }
                case 's': {
                    std::wstring wstrVal;
                    bool success = getRpcmpleVariantValue(rets[i],&wstrVal);
                    if(!success) {
                        return false;
                        // todo
                    }
                    std::string strVal = converter.to_bytes(wstrVal);

                    if(message.size() < replyOffset+2) {
                        return false;
                    }
                    uint16_t strSize = strVal.size();
                    uint16ToBytes(strSize,message.data()+replyOffset,true);
                    replyOffset+=2;

                    if(message.size() < replyOffset+strSize) {
                        return false;
                    }
                    std::copy(strVal.begin(),strVal.end(),message.data()+replyOffset);
                    replyOffset += strSize;
                    break;
                }
                case 'S': {
                    std::vector<std::wstring> wstrArrVal;
                    bool success = getRpcmpleVariantValue(rets[i],&wstrArrVal);
                    if(!success) {
                        return false;
                        // todo
                    }

                    if(message.size() < replyOffset+2) {
                        return false;
                    }
                    uint16_t arrSize = wstrArrVal.size();
                    uint16ToBytes(arrSize,message.data()+replyOffset,true);
                    replyOffset+=2;

                    for(int j=0;j<arrSize;j++) {
                        std::wstring wstrVal = wstrArrVal[j];
                        std::string strVal = converter.to_bytes(wstrVal);

                        if(message.size() < replyOffset+2) {
                            return false;
                        }
                        uint16_t strSize = strVal.size();
                        uint16ToBytes(strSize,message.data()+replyOffset,true);
                        replyOffset+=2;

                        if(message.size() < replyOffset+strSize) {
                            return false;
                        }
                        std::copy(strVal.begin(),strVal.end(),message.data()+replyOffset);
                        replyOffset += strSize;
                    }
                    break;
                }
                default: {
                    return false;
                    // TODO
                }
            }
        }
        message.resize(replyOffset);
        return true;
    }

    bool fromBinary(std::vector<uint8_t> &message, rpcmpleVariantVector &args) {
        int messageOffset = 0;
        args.resize(this->size());

        for (auto i=0;i<this->size();i++) {
            char dataType = this->at(i);

            switch (dataType) {
                case 'v':
                    if(message.size()-messageOffset<1) {
                        return false;
                    }
                    char byte = static_cast<char>(*(message.data() + messageOffset));
                    messageOffset += 1;

                    switch (byte) {
                        case 'd':
                        case 'D':
                        case 'i':
                        case 'I':
                        case 'u':
                        case 'U':
                        case 's':
                        case 'S':
                            dataType = byte;
                            break;
                        default:
                            return false;
                    }
                    break;
            }

            switch (dataType) {
                case 'd': {
                    if(message.size()-messageOffset<8) {
                        return false;
                    }
                    double dblVal = bytesToDouble(message.data()+messageOffset,true);
                    messageOffset += 8;

                    args[i]=dblVal;
                    break;
                }
                case 'D': {
                    if(message.size()-messageOffset<2) {
                        return false;
                    }
                    uint16_t arrSize = bytesToUint16(message.data()+messageOffset,true);
                    messageOffset += 2;

                    std::vector<double> dblArrVal(arrSize);
                    for(auto j=0;j<dblArrVal.size();j++) {
                        if(message.size()-messageOffset<8) {
                            return false;
                        }
                        dblArrVal[j] = bytesToDouble(message.data()+messageOffset,true);
                        messageOffset += 8;
                    }

                    args[i]=dblArrVal;
                    break;
                }
                case 'i': {
                    if(message.size()-messageOffset<8) {
                        return false;
                    }
                    int64_t intVal = bytesToInt64(message.data()+messageOffset,true);
                    messageOffset += 8;

                    args[i]=intVal;
                    break;
                }
                case 'I': {
                    if(message.size()-messageOffset<2) {
                        return false;
                    }
                    uint16_t arrSize = bytesToUint16(message.data()+messageOffset,true);
                    messageOffset += 2;

                    std::vector<int64_t> intArrVal(arrSize);
                    for (auto j=0;j<intArrVal.size();j++) {
                        if(message.size()-messageOffset<8) {
                            return false;
                        }
                        intArrVal[j] = bytesToInt64(message.data()+messageOffset,true);
                        messageOffset += 8;
                    }

                    args[i]=intArrVal;
                    break;
                }
                case 'u': {
                    if(message.size()-messageOffset<8) {
                        return false;
                    }
                    uint64_t uintVal = bytesToUint64(message.data()+messageOffset,true);
                    messageOffset += 8;

                    args[i]=uintVal;
                    break;
                }
                case 'U': {
                    if(message.size()-messageOffset<2) {
                        return false;
                    }
                    uint16_t arrSize = bytesToUint16(message.data()+messageOffset,true);
                    messageOffset += 2;

                    std::vector<uint64_t> uintArrVal(arrSize);
                    for (auto j=0;j<uintArrVal.size();j++) {
                        if(message.size()-messageOffset<8) {
                            return false;
                        }
                        uintArrVal[j] = bytesToUint64(message.data()+messageOffset,true);
                        messageOffset += 8;
                    }

                    args[i]=uintArrVal;
                    break;
                }
                case 's': {
                    if(message.size()-messageOffset<2) {
                        return false;
                    }
                    uint16_t strSize = bytesToUint16(message.data()+messageOffset,true);
                    messageOffset += 2;

                    if(message.size()-messageOffset<strSize) {
                        return false;
                    }
                    std::string byteVal(message.data()+messageOffset,message.data()+messageOffset+strSize);
                    std::wstring strVal(converter.from_bytes(byteVal.c_str()));
                    messageOffset += strSize;

                    args[i]=strVal;
                    break;
                }
                case 'S': {
                    if(message.size()-messageOffset<2) {
                        return false;
                    }
                    uint16_t strArrSize = bytesToUint16(message.data()+messageOffset,true);
                    messageOffset += 2;

                    std::vector<std::wstring> strArr(strArrSize);

                    for(int j=0;j<strArrSize;j++) {
                        if(message.size()-messageOffset<2) {
                            return false;
                        }
                        uint16_t strSize = bytesToUint16(message.data()+messageOffset,true);
                        messageOffset += 2;

                        if(message.size()-messageOffset<strSize) {
                            return false;
                        }
                        std::string byteVal(message.data()+messageOffset,message.data()+messageOffset+strSize);
                        strArr[j] = converter.from_bytes(byteVal.c_str());
                        messageOffset += strSize;
                    }

                    args[i]=strArr;
                    break;
                }
                default: {
                    std::cerr << "invalid data type" << std::endl;
                    return false;
                }
            }
        }

        return true;
    }
};


#endif //DATASIGNATURE_H
