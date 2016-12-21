// 
// 
// 

#include "MessageHandler.h"

void MessageHandlerClass::init()
{


}

MessageHandlerClass::MessageHandlerClass(HardwareSerial& serial) : 
	serial(&serial), 
	msgBufferIndex(0),
	readBufferIndex(0),
	lastWasSpecial(false),
	isSending(false),
	msgEndIndex(-1),
	onDebug(NULL),
	msgStartIndex(-1){
	for (uint8_t i = 0; i < MESSAGE_HANDLER_COUNT; ++i) {
		handlers[i] = NULL;
	}
}

void MessageHandlerClass::readSerial() {
	int available = 0;
	while ((available = serial->available())) {
		int read = serial->readBytes(readBuffer, min(available, MESSAGE_HANDLER_READ_BUFFER_SIZE - readBufferIndex));
		for (int i = 0; i < read; ++i) {
			if (lastWasSpecial) {
				if (readBuffer[i] == 0xFE) {
					// get rid of 0xFE after 0x55
					lastWasSpecial = false;
					continue;
				}
				else if (readBuffer[i] == 0xFF) {
					// start of packet
					msgStartIndex = msgBufferIndex - 1;
					lastWasSpecial = false;
				}
				else if (readBuffer[i] == 0x00) {
					if (msgStartIndex != -1) {
						msgEndIndex = msgBufferIndex - 1;
					}
					lastWasSpecial = false;
				}else{
					lastWasSpecial = false;
				}
				
			}
			else if (readBuffer[i] == 0x55) {
				lastWasSpecial = true;
			}
			msgBuffer[msgBufferIndex++] = readBuffer[i];
			if (msgEndIndex != -1) {
				this->handleMessage();
				// restart buffer and message indexes
				msgBufferIndex = 0;
				msgEndIndex = -1;
				msgStartIndex = -1;
			}
		}
	}

}

void MessageHandlerClass::handleMessage() {
	// get past header and msg type byte
	msgStartIndex += 3;
	uint8_t msgType = msgBuffer[msgStartIndex - 1];
	if (handlers[msgType] != NULL) {
		handlers[msgType]();
	}
}

bool MessageHandlerClass::hasNextByte() {
	return msgStartIndex < msgEndIndex;
}

uint8_t MessageHandlerClass::getNextByte() {
	if (msgStartIndex < msgEndIndex) {
		return msgBuffer[msgStartIndex++];
	}
	return 0x00;
}

void MessageHandlerClass::addHandler(uint8_t msgType, messageHandler handler) {
	if (msgType < MESSAGE_HANDLER_COUNT) {
		handlers[msgType] = handler;
	}
}

bool MessageHandlerClass::beginSend(uint8_t messageType) {
	if (!isSending) {
		isSending = true;
		serial->write((uint8_t)0x55);
		serial->write((uint8_t)0xFF);
		this->sendByte(messageType);
		return true;
	}
	return false;
}
bool MessageHandlerClass::sendByte(uint8_t data) {
	if (isSending) {
		serial->write(data);
		if (data == 0x55) {
			serial->write((byte)0xFE);
		}
		return true;
	}
	return false;
}
bool MessageHandlerClass::endSend() {
	if (isSending) {
		serial->write((uint8_t)0x55);
		serial->write((uint8_t)0x00);
		isSending = false;
		return true;
	}
	return false;
}

bool MessageHandlerClass::send(uint8_t messageType) {
	if (!isSending) {
		this->beginSend(messageType);
		return this->endSend();
	}
	return false;
}

bool MessageHandlerClass::send(uint8_t messageType, uint8_t buffer[], int offset, int length) {
	if (!isSending) {
		this->beginSend(messageType);
		int end = offset + length;
		for (int i = offset; i < end; ++i) {
			this->sendByte(buffer[offset]);
		}
		return this->endSend();
	}
	return false;
}

void MessageHandlerClass::debugMessage(char msg[]) {
	if (onDebug != NULL) {
		onDebug(msg);
	}
}