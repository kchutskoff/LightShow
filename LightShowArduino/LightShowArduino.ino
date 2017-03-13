
/*
This is an example of how simple driving a Neopixel can be
This code is optimized for understandability and changability rather than raw speed
More info at https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
*/

#include "MessageHandler.h"

// These values depend on which pin your string is connected to and what board you are using 
// More info on how to find these at http://www.arduino.cc/en/Reference/PortManipulation

// These values are for the pin that connects to the Data Input pin on the LED strip. They correspond to...

// Arduino Yun:     Digital Pin 8
// DueMilinove/UNO: Digital Pin 12
// Arduino MeagL    PWM Pin 4

// You'll need to look up the port/bit combination for other boards. 
// Note that you could also include the DigitalWriteFast header file to not need to to this lookup.

#define PIXEL_PORT  PORTD  // Port of the pin the pixels are connected to
#define PIXEL_DDR   DDRD   // Port of the pin the pixels are connected to
#define PIXEL_BIT   6      // Bit of the pin the pixels are connected to

void ledsetup() {
	// Set the specified pin up as digital out
	bitSet(PIXEL_DDR, PIXEL_BIT);
}

uint64_t lastFrameMillis = 0;
uint64_t pingDropFrameEvery = 1000; // if we don't get a frame after 1000ms, send message for a new one
uint8_t resetCountAfterDrops = 10; // if we don't get a frame after 10 pings, reset the data count
uint8_t droppedFrameCount = 0; // count the number of dropped frames
uint16_t currentDataCount = 0; // amount of data to request for
uint16_t maxDataCount = 1000; // our message buffer is only 1024
uint8_t currentDataCountIndex = 0; // number of times we sent the current data count
uint8_t maxDataCountIndex = 20; // send each amount of data 50 times in a row before moving up

MessageHandlerClass msg = MessageHandlerClass(Serial);
enum Commands : uint8_t {
	FRM,
	FRM_RESP
};


void requestFrame() {
	uint8_t high = (currentDataCount >> 8);
	uint8_t low = currentDataCount;
	msg.beginSend(Commands::FRM);
	msg.sendByte(high);
	msg.sendByte(low);
	msg.endSend();
}

void onFrameResponse() {
	droppedFrameCount = 0;
	uint8_t byteToRead;
	for (uint16_t i = 0; i < currentDataCount; ++i) {
		byteToRead = msg.getNextByte();
	}
	cli();
	for (uint16_t i = 0; i < currentDataCount; ++i) {
		// pretend we did some processing on LEDs
		for (uint8_t j = 0; j < 8; ++j) {
			_NOP();
			_NOP();
			_NOP();
			_NOP();
			_NOP();
			_NOP();
			_NOP();
		}
	}
	sei();
	// how many times have we sent this data size?
	currentDataCountIndex++;
	if (currentDataCountIndex >= maxDataCountIndex) {
		// enough, go up a data size and reset the counter
		currentDataCountIndex = 0;
		currentDataCount++;
		if (currentDataCount >= maxDataCount) {
			currentDataCount = 0; // hit the max, go back to zero
		}
	}
	lastFrameMillis = millis();
	requestFrame();
}


// the setup function runs once when you press reset or power the board
void setup() {
	ledsetup();
	Serial.begin(500000);
	msg.addHandler(Commands::FRM_RESP, onFrameResponse);
}

// the loop function runs over and over again until power down or reset
void loop() {
	msg.readSerial();
	if (millis() - lastFrameMillis >= pingDropFrameEvery) {
		// missed a frame, so ask for a new one
		requestFrame();
		lastFrameMillis = millis();
		droppedFrameCount++;
		if (droppedFrameCount >= resetCountAfterDrops) {
			// dropped twoo many frames, restart count at 1 again
			currentDataCount = 0;
			currentDataCountIndex = 0;
		}
	}
}
