//
// Created by carlo on 3/5/2025.
//

#ifndef DATASUBSCRIBER_H
#define DATASUBSCRIBER_H


#include "messageManager.h"
#include "dataSignature.h"
#include "rpcmple.h"

#include <functional>

class dataSubscriber : public messageManager {
private:
	dataSignature mSignature;
	std::function<void(rpcmpleVariantVector)> callbackFunction;


    uint32_t sectionLen;
    uint16_t sectionID;
public:
	dataSubscriber(connectionManager* pConn, std::vector<char> signature, std::function<void(rpcmpleVariantVector)> callback)
		: messageManager(pConn, true), mSignature(std::move(signature)), callbackFunction(std::move(callback)),
		  sectionLen(4),
		  sectionID(0)
	{
	}

	~dataSubscriber() override {
		stopDataFlow();
	};

	bool parseMessage(std::vector<uint8_t> message) override {
		spdlog::debug("dataSubscriber: parsing message");
        switch (sectionID) {
            case 0: {
                uint32_t mVal = bytesToUint32(message.data(), true);
                sectionLen = mVal % 16777216;
                if (sectionLen > 0) {
                    sectionID = 1;
                } else {
                  	if(mSignature.size() > 0) {
                          spdlog::error("dataSubscriber: error in RPC message parsing: invalid message");
                          return false;
                  	}
                    rpcmpleVariantVector noV;
                    if(callbackFunction) callbackFunction(noV);
                    sectionID = 0;
                    sectionLen = 4;
                }
                break;
            }
            case 1: {
              	rpcmpleVariantVector args;
                mSignature.fromBinary(message, args);
                if (callbackFunction) callbackFunction(args);
                if(mSignature.size() != args.size()) {
                    spdlog::error("dataSubscriber: error in RPC message parsing: invalid message");
                    return false;
                }
                sectionID = 0;
                sectionLen = 4;
                break;
            }
            default: {
                spdlog::error("dataSubscriber: error in RPC message parsing: invalid message section index");
                return false;
            }
        }
        return true;

	}

	int getMessageLen() override {return sectionLen;}

	bool writeMessage(std::vector<uint8_t>& message) override {
		message.resize(0);
		return true;
	}

	void stopParser() override {}
};

#endif //DATASUBSCRIBER_H
