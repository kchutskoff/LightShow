// MessageHandler.h

#ifndef _MESSAGEHANDLER_h
#define _MESSAGEHANDLER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define MESSAGE_HANDLER_MESSAGE_BUFFER_SIZE 512
#define MESSAGE_HANDLER_READ_BUFFER_SIZE 256
#define MESSAGE_HANDLER_COUNT 8

extern "C"
{
	typedef void(*messageHandler) (void);
	typedef void(*debugHandler) (char[]);
}

class MessageHandlerClass
{
 protected:
	 bool lastWasSpecial;
	 bool lastWasHeader;
	 bool isSending;
	 bool isReading;
	 HardwareSerial* serial;
	 messageHandler handlers[MESSAGE_HANDLER_COUNT];
	 void debugMessage(char[]);

 public:
	void init();
	void readSerial();
	bool hasNextByte();
	uint8_t getNextByte();
	MessageHandlerClass(HardwareSerial&);
	void addHandler(uint8_t, messageHandler);
	bool beginSend(uint8_t);
	bool sendByte(uint8_t);
	bool endSend();
	bool send(uint8_t);
	bool send(uint8_t, uint8_t[], int, int);

	debugHandler onDebug;
	
};

extern MessageHandlerClass MessageHandler;

#endif

