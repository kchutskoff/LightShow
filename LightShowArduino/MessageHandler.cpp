// 
// 
// 

#include "MessageHandler.h"

void MessageHandlerClass::init()
{


}

MessageHandlerClass::MessageHandlerClass(HardwareSerial& serial) :
	serial(&serial),
	lastWasHeader(false),
	lastWasSpecial(false),
	isSending(false),
	isReading(false),
	onDebug(NULL){
	for (uint8_t i = 0; i < MESSAGE_HANDLER_COUNT; ++i) {
		handlers[i] = NULL;
	}
}

void MessageHandlerClass::readSerial() {
	int available = 0;
	while (serial->available() > 0) {
		uint8_t read = serial->read();
		if (lastWasHeader) {
			lastWasHeader = false;
			// this is the type, send off a message arrival
			if (read < MESSAGE_HANDLER_COUNT && handlers[read] != NULL) {
				isReading = true;
				handlers[read]();
			}
			else {
				isReading = false;
			}
		}else if (lastWasSpecial) {
			lastWasSpecial = false;
			if (read == 0xFF) {
				// start of packet
				lastWasHeader = true;
			}
			else if (read == 0x00) {
				isReading = false;
			}
		}
		else if (read == 0x81) {
			lastWasSpecial = true;
		}
	}
}

bool MessageHandlerClass::hasNextByte() {
	return isReading && serial->available() > 0;
}

uint8_t MessageHandlerClass::getNextByte() {
	if (isReading) {
		int attempts = 100;
		while (--attempts > 0) {
			if (serial->available() > 0) {
				attempts = 100;
				uint8_t read = serial->read();
				if (lastWasSpecial) {
					if (read == 0xFE) {
						// escaped
						return 0x81;
					}
					else if (read == 0x00) {
						// end of packet
						isReading = false;
						return 0x00;
					}
					else {
						// ignore
						continue;
					}
				}
				if (read == 0x81) {
					// escape character
					lastWasSpecial = true;
					continue;
				}
				else {
					return read;
				}
			}
		}
		return 0x00;
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
		serial->write((uint8_t)0x81);
		serial->write((uint8_t)0xFF);
		this->sendByte(messageType);
		return true;
	}
	return false;
}
bool MessageHandlerClass::sendByte(uint8_t data) {
	if (isSending) {
		serial->write(data);
		if (data == 0x81) {
			serial->write((byte)0xFE);
		}
		return true;
	}
	return false;
}
bool MessageHandlerClass::endSend() {
	if (isSending) {
		serial->write((uint8_t)0x81);
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
			this->sendByte(buffer[i]);
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